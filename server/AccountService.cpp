#include "AccountService.h"

#include "AuthService.h"
#include "Database.h"
#include "PasswordUtil.h"

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
}

AccountService::AccountService(Database* db, AuthService* authService)
    : db_(db), authService_(authService) {}

Response AccountService::QueryBalance(const Request& request) {
    Account account;
    SessionInfo session;
    if (!GetCurrentAccountBySession(request.sessionId, account, session)) {
        return MakeResponse(false, 1004, "会话无效或账户不存在");
    }
    Response errorResponse;
    if (!VerifySessionPassword(session, request, errorResponse)) {
        return errorResponse;
    }

    Response response = MakeResponse(true, 0, "查询成功");
    response.data["accountNo"] = account.accountNo;
    response.data["balance"] = std::to_string(account.balance);
    return response;
}

Response AccountService::Deposit(const Request& request) {
    Account account;
    SessionInfo session;
    if (!GetCurrentAccountBySession(request.sessionId, account, session)) {
        return MakeResponse(false, 1004, "会话无效或账户不存在");
    }
    Response errorResponse;
    if (!VerifySessionPassword(session, request, errorResponse)) {
        return errorResponse;
    }
    if (account.status != "active") {
        return MakeResponse(false, 1010, "账户状态异常");
    }

    const auto amountIt = request.data.find("amount");
    if (amountIt == request.data.end()) {
        return MakeResponse(false, 1006, "缺少金额");
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

    const double before = account.balance;
    const double after = before + amount;
    if (!db_->BeginTransaction()) {
        return MakeResponse(false, 1011, db_->GetLastError());
    }

    const std::string updateSql =
        "UPDATE accounts SET balance = " + std::to_string(after) + " WHERE id = " + std::to_string(account.id);
    if (!db_->Execute(updateSql) || !InsertTransaction(account.id, "deposit", amount, before, after, "",
                                                       request.data.count("remark") ? request.data.at("remark") : "")) {
        db_->Rollback();
        return MakeResponse(false, 1011, db_->GetLastError());
    }

    if (!db_->Commit()) {
        db_->Rollback();
        return MakeResponse(false, 1011, db_->GetLastError());
    }

    Response response = MakeResponse(true, 0, "存款成功");
    response.data["balance"] = std::to_string(after);
    return response;
}

Response AccountService::Withdraw(const Request& request) {
    Account account;
    SessionInfo session;
    if (!GetCurrentAccountBySession(request.sessionId, account, session)) {
        return MakeResponse(false, 1004, "会话无效或账户不存在");
    }
    Response errorResponse;
    if (!VerifySessionPassword(session, request, errorResponse)) {
        return errorResponse;
    }
    if (account.status != "active") {
        return MakeResponse(false, 1010, "账户状态异常");
    }

    const auto amountIt = request.data.find("amount");
    if (amountIt == request.data.end()) {
        return MakeResponse(false, 1006, "缺少金额");
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
    if (account.balance < amount) {
        return MakeResponse(false, 1007, "余额不足");
    }

    const double before = account.balance;
    const double after = before - amount;
    if (!db_->BeginTransaction()) {
        return MakeResponse(false, 1011, db_->GetLastError());
    }

    const std::string updateSql =
        "UPDATE accounts SET balance = " + std::to_string(after) + " WHERE id = " + std::to_string(account.id);
    if (!db_->Execute(updateSql) || !InsertTransaction(account.id, "withdraw", amount, before, after, "",
                                                       request.data.count("remark") ? request.data.at("remark") : "")) {
        db_->Rollback();
        return MakeResponse(false, 1011, db_->GetLastError());
    }

    if (!db_->Commit()) {
        db_->Rollback();
        return MakeResponse(false, 1011, db_->GetLastError());
    }

    Response response = MakeResponse(true, 0, "取款成功");
    response.data["balance"] = std::to_string(after);
    return response;
}

bool AccountService::VerifySessionPassword(const SessionInfo& session, const Request& request, Response& errorResponse) {
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

bool AccountService::GetCurrentAccountBySession(const std::string& sessionId, Account& account, SessionInfo& session) {
    if (!authService_ || !authService_->CheckSession(sessionId, session)) {
        return false;
    }

    std::vector<std::string> row;
    const std::string sql =
        "SELECT id, user_id, account_no, balance, status FROM accounts WHERE user_id = " +
        std::to_string(session.userId) + " LIMIT 1";
    if (!db_->Query(sql) || !db_->FetchRow(row)) {
        db_->FreeResult();
        return false;
    }
    db_->FreeResult();

    account.id = std::stoi(row[0]);
    account.userId = std::stoi(row[1]);
    account.accountNo = row[2];
    account.balance = std::stod(row[3]);
    account.status = row[4];
    return true;
}

bool AccountService::InsertTransaction(int accountId,
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
