#include "TransactionService.h"

#include "AuthService.h"
#include "Database.h"
#include "PasswordUtil.h"

#include <algorithm>
#include <vector>

namespace {
Response MakeResponse(bool success, int code, const std::string& message) {
    Response response;
    response.success = success;
    response.code = code;
    response.message = message;
    return response;
}

std::string SqlQuote(Database* db, const std::string& value) {
    return "'" + db->Escape(value) + "'";
}

struct AccountRow {
    int id = 0;
    int userId = 0;
    std::string accountNo;
    double balance = 0.0;
    std::string status;
};

bool TryParsePositiveInt(const Request& request,
                         const std::string& key,
                         int defaultValue,
                         int minimumValue,
                         int maximumValue,
                         int& value) {
    value = defaultValue;
    const auto it = request.data.find(key);
    if (it == request.data.end() || it->second.empty()) {
        return true;
    }

    try {
        value = std::stoi(it->second);
    } catch (...) {
        return false;
    }

    value = std::max(minimumValue, std::min(value, maximumValue));
    return true;
}
}

TransactionService::TransactionService(Database* db, AuthService* authService)
    : db_(db), authService_(authService) {}

Response TransactionService::Transfer(const Request& request) {
    SessionInfo session;
    if (!authService_->CheckSession(request.sessionId, session)) {
        return MakeResponse(false, 1004, "会话无效");
    }

    Response errorResponse;
    if (!VerifySessionPassword(session, request, errorResponse)) {
        return errorResponse;
    }

    const auto targetIt = request.data.find("targetAccountNo");
    const auto amountIt = request.data.find("amount");
    if (targetIt == request.data.end() || amountIt == request.data.end()) {
        return MakeResponse(false, 1006, "参数不完整");
    }

    double amount = 0.0;
    try {
        amount = std::stod(amountIt->second);
    } catch (...) {
        return MakeResponse(false, 1006, "金额格式错误");
    }
    if (amount <= 0.0) {
        return MakeResponse(false, 1006, "金额必须大于0");
    }

    std::vector<std::string> row;
    const std::string sourceSql =
        "SELECT id, user_id, account_no, balance, status FROM accounts WHERE user_id = " +
        std::to_string(session.userId) + " LIMIT 1";
    if (!db_->Query(sourceSql) || !db_->FetchRow(row)) {
        db_->FreeResult();
        return MakeResponse(false, 1010, "源账户不存在");
    }
    AccountRow source{std::stoi(row[0]), std::stoi(row[1]), row[2], std::stod(row[3]), row[4]};
    db_->FreeResult();

    if (source.status != "active") {
        return MakeResponse(false, 1010, "源账户状态异常");
    }
    if (source.accountNo == targetIt->second) {
        return MakeResponse(false, 1009, "不能给自己转账");
    }
    if (source.balance < amount) {
        return MakeResponse(false, 1007, "余额不足");
    }

    const std::string targetSql =
        "SELECT id, user_id, account_no, balance, status FROM accounts WHERE account_no = " +
        SqlQuote(db_, targetIt->second) + " LIMIT 1";
    if (!db_->Query(targetSql) || !db_->FetchRow(row)) {
        db_->FreeResult();
        return MakeResponse(false, 1008, "目标账户不存在");
    }
    AccountRow target{std::stoi(row[0]), std::stoi(row[1]), row[2], std::stod(row[3]), row[4]};
    db_->FreeResult();

    if (target.status != "active") {
        return MakeResponse(false, 1010, "目标账户状态异常");
    }

    if (!db_->BeginTransaction()) {
        return MakeResponse(false, 1011, db_->GetLastError());
    }

    const double sourceAfter = source.balance - amount;
    const double targetAfter = target.balance + amount;
    const std::string remark = request.data.count("remark") ? request.data.at("remark") : "";

    if (!db_->Execute("UPDATE accounts SET balance = " + std::to_string(sourceAfter) + " WHERE id = " + std::to_string(source.id)) ||
        !db_->Execute("UPDATE accounts SET balance = " + std::to_string(targetAfter) + " WHERE id = " + std::to_string(target.id)) ||
        !InsertTransaction(source.id, "transfer_out", amount, source.balance, sourceAfter, target.accountNo, remark) ||
        !InsertTransaction(target.id, "transfer_in", amount, target.balance, targetAfter, source.accountNo, remark)) {
        db_->Rollback();
        return MakeResponse(false, 1011, db_->GetLastError());
    }

    if (!db_->Commit()) {
        db_->Rollback();
        return MakeResponse(false, 1011, db_->GetLastError());
    }

    Response response = MakeResponse(true, 0, "转账成功");
    response.data["balance"] = std::to_string(sourceAfter);
    return response;
}

bool TransactionService::VerifySessionPassword(const SessionInfo& session, const Request& request, Response& errorResponse) {
    const auto passwordIt = request.data.find("password");
    if (passwordIt == request.data.end() || passwordIt->second.empty()) {
        errorResponse = MakeResponse(false, 1006, "缺少密码");
        return false;
    }

    std::vector<std::string> row;
    const std::string sql =
        "SELECT password_hash, password_salt FROM users WHERE id = " + std::to_string(session.userId) + " LIMIT 1";
    if (!db_->Query(sql) || !db_->FetchRow(row)) {
        db_->FreeResult();
        errorResponse = MakeResponse(false, 1001, "用户不存在");
        return false;
    }
    db_->FreeResult();

    if (!PasswordUtil::VerifyPassword(passwordIt->second, row[1], row[0])) {
        errorResponse = MakeResponse(false, 1012, "密码验证失败");
        return false;
    }

    return true;
}

Response TransactionService::QueryMyTransactions(const Request& request) {
    SessionInfo session;
    if (!authService_->CheckSession(request.sessionId, session)) {
        return MakeResponse(false, 1004, "会话无效");
    }

    int page = 1;
    int pageSize = 10;
    if (!TryParsePositiveInt(request, "page", 1, 1, 1000000, page) ||
        !TryParsePositiveInt(request, "pageSize", 10, 1, 50, pageSize)) {
        return MakeResponse(false, 1006, "分页参数错误");
    }

    std::vector<std::string> row;
    const std::string countSql =
        "SELECT COUNT(*) FROM transactions t INNER JOIN accounts a ON a.id = t.account_id "
        "WHERE a.user_id = " + std::to_string(session.userId);
    if (!db_->Query(countSql) || !db_->FetchRow(row)) {
        db_->FreeResult();
        return MakeResponse(false, 1011, db_->GetLastError());
    }
    const int total = std::stoi(row[0]);
    db_->FreeResult();

    const int offset = (page - 1) * pageSize;

    const std::string sql =
        "SELECT t.id, t.tx_type, t.amount, t.balance_before, t.balance_after, IFNULL(t.related_account_no, ''), "
        "IFNULL(t.remark, ''), DATE_FORMAT(t.created_at, '%Y-%m-%d %H:%i:%s') "
        "FROM transactions t INNER JOIN accounts a ON a.id = t.account_id "
        "WHERE a.user_id = " + std::to_string(session.userId) + " ORDER BY t.created_at DESC, t.id DESC LIMIT " +
        std::to_string(pageSize) + " OFFSET " + std::to_string(offset);
    if (!db_->Query(sql)) {
        return MakeResponse(false, 1011, db_->GetLastError());
    }

    Response response = MakeResponse(true, 0, "查询成功");
    int index = 0;
    while (db_->FetchRow(row)) {
        const std::string prefix = "record" + std::to_string(index++) + ".";
        response.data[prefix + "txId"] = row[0];
        response.data[prefix + "txType"] = row[1];
        response.data[prefix + "amount"] = row[2];
        response.data[prefix + "balanceBefore"] = row[3];
        response.data[prefix + "balanceAfter"] = row[4];
        response.data[prefix + "relatedAccountNo"] = row[5];
        response.data[prefix + "remark"] = row[6];
        response.data[prefix + "createdAt"] = row[7];
    }
    db_->FreeResult();
    response.data["page"] = std::to_string(page);
    response.data["pageSize"] = std::to_string(pageSize);
    response.data["total"] = std::to_string(total);
    response.data["count"] = std::to_string(index);
    return response;
}

Response TransactionService::AdminQueryTransactions(const Request& request) {
    if (!authService_->IsAdmin(request.sessionId)) {
        return MakeResponse(false, 1005, "权限不足");
    }

    const auto accountIt = request.data.find("accountNo");
    if (accountIt == request.data.end()) {
        return MakeResponse(false, 1006, "缺少账户号");
    }

    int page = 1;
    int pageSize = 10;
    if (!TryParsePositiveInt(request, "page", 1, 1, 1000000, page) ||
        !TryParsePositiveInt(request, "pageSize", 10, 1, 50, pageSize)) {
        return MakeResponse(false, 1006, "分页参数错误");
    }

    std::vector<std::string> row;
    const std::string countSql =
        "SELECT COUNT(*) FROM transactions t INNER JOIN accounts a ON a.id = t.account_id "
        "WHERE a.account_no = " + SqlQuote(db_, accountIt->second);
    if (!db_->Query(countSql) || !db_->FetchRow(row)) {
        db_->FreeResult();
        return MakeResponse(false, 1011, db_->GetLastError());
    }
    const int total = std::stoi(row[0]);
    db_->FreeResult();

    const int offset = (page - 1) * pageSize;

    const std::string sql =
        "SELECT t.id, t.tx_type, t.amount, t.balance_before, t.balance_after, IFNULL(t.related_account_no, ''), "
        "IFNULL(t.remark, ''), DATE_FORMAT(t.created_at, '%Y-%m-%d %H:%i:%s') "
        "FROM transactions t INNER JOIN accounts a ON a.id = t.account_id "
        "WHERE a.account_no = " + SqlQuote(db_, accountIt->second) + " ORDER BY t.created_at DESC, t.id DESC LIMIT " +
        std::to_string(pageSize) + " OFFSET " + std::to_string(offset);
    if (!db_->Query(sql)) {
        return MakeResponse(false, 1011, db_->GetLastError());
    }

    Response response = MakeResponse(true, 0, "查询成功");
    int index = 0;
    while (db_->FetchRow(row)) {
        const std::string prefix = "record" + std::to_string(index++) + ".";
        response.data[prefix + "txId"] = row[0];
        response.data[prefix + "txType"] = row[1];
        response.data[prefix + "amount"] = row[2];
        response.data[prefix + "balanceBefore"] = row[3];
        response.data[prefix + "balanceAfter"] = row[4];
        response.data[prefix + "relatedAccountNo"] = row[5];
        response.data[prefix + "remark"] = row[6];
        response.data[prefix + "createdAt"] = row[7];
    }
    db_->FreeResult();
    response.data["page"] = std::to_string(page);
    response.data["pageSize"] = std::to_string(pageSize);
    response.data["total"] = std::to_string(total);
    response.data["count"] = std::to_string(index);
    return response;
}

bool TransactionService::InsertTransaction(int accountId,
                                           const std::string& txType,
                                           double amount,
                                           double balanceBefore,
                                           double balanceAfter,
                                           const std::string& relatedAccountNo,
                                           const std::string& remark) {
    const std::string relatedValue = relatedAccountNo.empty() ? "NULL" : SqlQuote(db_, relatedAccountNo);
    const std::string remarkValue = remark.empty() ? "NULL" : SqlQuote(db_, remark);
    const std::string sql =
        "INSERT INTO transactions(account_id, tx_type, amount, balance_before, balance_after, related_account_no, remark) VALUES(" +
        std::to_string(accountId) + "," + SqlQuote(db_, txType) + "," + std::to_string(amount) + "," +
        std::to_string(balanceBefore) + "," + std::to_string(balanceAfter) + "," + relatedValue + "," + remarkValue + ")";
    return db_->Execute(sql);
}
