#include "AuthService.h"

#include "Database.h"
#include "PasswordUtil.h"

#include <algorithm>
#include <chrono>
#include <sstream>
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

int ToInt(const std::string& value) {
    return value.empty() ? 0 : std::stoi(value);
}

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

AuthService::AuthService(Database* db) : db_(db) {}

Response AuthService::RegisterUser(const Request& request) {
    if (!db_ || !db_->IsConnected()) {
        return MakeResponse(false, 1011, "数据库未连接");
    }

    const auto cardIt = request.data.find("cardNo");
    const auto nameIt = request.data.find("userName");
    const auto passwordIt = request.data.find("password");
    const auto accountIt = request.data.find("accountNo");
    if (cardIt == request.data.end() || nameIt == request.data.end() || passwordIt == request.data.end() || accountIt == request.data.end()) {
        return MakeResponse(false, 1006, "参数不完整");
    }

    double initialBalance = 0.0;
    try {
        if (request.data.count("initialBalance") > 0 && !request.data.at("initialBalance").empty()) {
            initialBalance = std::stod(request.data.at("initialBalance"));
        }
    } catch (...) {
        return MakeResponse(false, 1006, "初始金额格式错误");
    }

    if (initialBalance < 0.0 || passwordIt->second.size() < 6) {
        return MakeResponse(false, 1006, "初始金额或密码不合法");
    }

    const std::string cardNo = db_->Escape(cardIt->second);
    const std::string userName = db_->Escape(nameIt->second);
    const std::string accountNo = db_->Escape(accountIt->second);

    std::vector<std::string> row;
    if (!db_->Query("SELECT id FROM users WHERE card_no = '" + cardNo + "' LIMIT 1")) {
        return MakeResponse(false, 1011, db_->GetLastError());
    }
    if (db_->FetchRow(row)) {
        db_->FreeResult();
        return MakeResponse(false, 1013, "卡号已存在");
    }
    db_->FreeResult();

    if (!db_->Query("SELECT id FROM accounts WHERE account_no = '" + accountNo + "' LIMIT 1")) {
        return MakeResponse(false, 1011, db_->GetLastError());
    }
    if (db_->FetchRow(row)) {
        db_->FreeResult();
        return MakeResponse(false, 1014, "账户号已存在");
    }
    db_->FreeResult();

    const std::string salt = PasswordUtil::GenerateSalt();
    const std::string passwordHash = PasswordUtil::HashPassword(passwordIt->second, salt);

    if (!db_->BeginTransaction()) {
        return MakeResponse(false, 1011, db_->GetLastError());
    }

    const std::string insertUserSql =
        "INSERT INTO users(card_no, user_name, password_hash, password_salt, failed_attempts, is_locked, role) VALUES(" +
        SqlQuote(db_, cardIt->second) + "," + SqlQuote(db_, nameIt->second) + "," +
        SqlQuote(db_, passwordHash) + "," + SqlQuote(db_, salt) + ",0,0,'user')";
    if (!db_->Execute(insertUserSql)) {
        db_->Rollback();
        return MakeResponse(false, 1011, db_->GetLastError());
    }

    const long long userId = db_->GetLastInsertId();
    const std::string insertAccountSql =
        "INSERT INTO accounts(user_id, account_no, balance, status) VALUES(" + std::to_string(userId) + "," +
        SqlQuote(db_, accountIt->second) + "," + std::to_string(initialBalance) + ",'active')";
    if (!db_->Execute(insertAccountSql)) {
        db_->Rollback();
        return MakeResponse(false, 1011, db_->GetLastError());
    }

    if (initialBalance > 0.0) {
        const long long accountId = db_->GetLastInsertId();
        const std::string txSql =
            "INSERT INTO transactions(account_id, tx_type, amount, balance_before, balance_after, related_account_no, remark) VALUES(" +
            std::to_string(accountId) + ",'deposit'," + std::to_string(initialBalance) + ",0," +
            std::to_string(initialBalance) + ",NULL,'initial deposit')";
        if (!db_->Execute(txSql)) {
            db_->Rollback();
            return MakeResponse(false, 1011, db_->GetLastError());
        }
    }

    if (!db_->Commit()) {
        db_->Rollback();
        return MakeResponse(false, 1011, db_->GetLastError());
    }

    Response response = MakeResponse(true, 0, "开户成功");
    response.data["userId"] = std::to_string(userId);
    return response;
}

Response AuthService::Login(const Request& request, const std::string& clientIp) {
    if (!db_ || !db_->IsConnected()) {
        return MakeResponse(false, 1011, "数据库未连接");
    }

    const auto cardIt = request.data.find("cardNo");
    const auto passwordIt = request.data.find("password");
    if (cardIt == request.data.end() || passwordIt == request.data.end()) {
        return MakeResponse(false, 1006, "参数不完整");
    }

    std::vector<std::string> row;
    const std::string sql =
        "SELECT id, user_name, password_hash, password_salt, failed_attempts, is_locked, role FROM users WHERE card_no = " +
        SqlQuote(db_, cardIt->second) + " LIMIT 1";
    if (!db_->Query(sql)) {
        return MakeResponse(false, 1011, db_->GetLastError());
    }

    if (!db_->FetchRow(row)) {
        db_->FreeResult();
        RecordLoginLog(0, cardIt->second, false, "用户不存在", clientIp);
        return MakeResponse(false, 1001, "用户不存在");
    }
    db_->FreeResult();

    const int userId = ToInt(row[0]);
    const std::string userName = row[1];
    const std::string passwordHash = row[2];
    const std::string salt = row[3];
    const int failedAttempts = ToInt(row[4]);
    const bool isLocked = ToInt(row[5]) != 0;
    const std::string role = row[6];

    if (isLocked) {
        RecordLoginLog(userId, cardIt->second, false, "账户已锁定", clientIp);
        Response response = MakeResponse(false, 1003, "账户已锁定");
        response.data["isLocked"] = "true";
        return response;
    }

    if (!PasswordUtil::VerifyPassword(passwordIt->second, salt, passwordHash)) {
        const int nextAttempts = failedAttempts + 1;
        const int lockFlag = nextAttempts >= 3 ? 1 : 0;
        const std::string updateSql =
            "UPDATE users SET failed_attempts = " + std::to_string(nextAttempts) + ", is_locked = " +
            std::to_string(lockFlag) + " WHERE id = " + std::to_string(userId);
        db_->Execute(updateSql);

        RecordLoginLog(userId, cardIt->second, false, lockFlag == 1 ? "密码错误，账户锁定" : "密码错误", clientIp);
        Response response = MakeResponse(false, lockFlag == 1 ? 1003 : 1002,
                                         lockFlag == 1 ? "密码错误次数过多，账户已锁定" : "密码错误");
        response.data["remainingAttempts"] = std::to_string(lockFlag == 1 ? 0 : 3 - nextAttempts);
        return response;
    }

    db_->Execute("UPDATE users SET failed_attempts = 0 WHERE id = " + std::to_string(userId));

    const std::string sessionId = GenerateSessionId();
    {
        std::lock_guard<std::mutex> lock(sessionMutex_);
        sessions_[sessionId] = SessionInfo{sessionId, userId, cardIt->second, role, true};
    }

    RecordLoginLog(userId, cardIt->second, true, "", clientIp);
    Response response = MakeResponse(true, 0, "登录成功");
    response.data["sessionId"] = sessionId;
    response.data["userId"] = std::to_string(userId);
    response.data["userName"] = userName;
    response.data["role"] = role;
    return response;
}

Response AuthService::Logout(const Request& request) {
    std::lock_guard<std::mutex> lock(sessionMutex_);
    sessions_.erase(request.sessionId);
    return MakeResponse(true, 0, "退出成功");
}

Response AuthService::ChangePassword(const Request& request) {
    SessionInfo session;
    if (!CheckSession(request.sessionId, session)) {
        return MakeResponse(false, 1004, "会话无效");
    }

    const auto oldIt = request.data.find("oldPassword");
    const auto newIt = request.data.find("newPassword");
    if (oldIt == request.data.end() || newIt == request.data.end() || newIt->second.size() < 6) {
        return MakeResponse(false, 1006, "参数不完整或新密码过短");
    }

    std::vector<std::string> row;
    const std::string querySql =
        "SELECT password_hash, password_salt FROM users WHERE id = " + std::to_string(session.userId) + " LIMIT 1";
    if (!db_->Query(querySql) || !db_->FetchRow(row)) {
        db_->FreeResult();
        return MakeResponse(false, 1001, "用户不存在");
    }
    db_->FreeResult();

    if (!PasswordUtil::VerifyPassword(oldIt->second, row[1], row[0])) {
        return MakeResponse(false, 1012, "旧密码错误");
    }

    const std::string salt = PasswordUtil::GenerateSalt();
    const std::string hash = PasswordUtil::HashPassword(newIt->second, salt);
    const std::string updateSql =
        "UPDATE users SET password_hash = " + SqlQuote(db_, hash) + ", password_salt = " + SqlQuote(db_, salt) +
        " WHERE id = " + std::to_string(session.userId);
    if (!db_->Execute(updateSql)) {
        return MakeResponse(false, 1011, db_->GetLastError());
    }

    return MakeResponse(true, 0, "密码修改成功");
}

Response AuthService::AdminUnlockUser(const Request& request) {
    SessionInfo adminSession;
    if (!CheckSession(request.sessionId, adminSession) || adminSession.role != "admin") {
        return MakeResponse(false, 1005, "权限不足");
    }

    const auto cardIt = request.data.find("cardNo");
    if (cardIt == request.data.end()) {
        return MakeResponse(false, 1006, "缺少卡号");
    }

    std::vector<std::string> row;
    const std::string querySql = "SELECT id FROM users WHERE card_no = " + SqlQuote(db_, cardIt->second) + " LIMIT 1";
    if (!db_->Query(querySql) || !db_->FetchRow(row)) {
        db_->FreeResult();
        return MakeResponse(false, 1001, "用户不存在");
    }
    db_->FreeResult();

    const int targetUserId = ToInt(row[0]);
    const std::string updateSql =
        "UPDATE users SET failed_attempts = 0, is_locked = 0 WHERE id = " + std::to_string(targetUserId);
    if (!db_->Execute(updateSql)) {
        return MakeResponse(false, 1011, db_->GetLastError());
    }

    db_->Execute("INSERT INTO admin_logs(admin_user_id, action, target_user_id, detail) VALUES(" +
                 std::to_string(adminSession.userId) + ",'unlock_user'," + std::to_string(targetUserId) + "," +
                 SqlQuote(db_, "unlock cardNo=" + cardIt->second) + ")");
    return MakeResponse(true, 0, "账户已解锁");
}

Response AuthService::AdminResetPassword(const Request& request) {
    SessionInfo adminSession;
    if (!CheckSession(request.sessionId, adminSession) || adminSession.role != "admin") {
        return MakeResponse(false, 1005, "权限不足");
    }

    const auto cardIt = request.data.find("cardNo");
    const auto passwordIt = request.data.find("newPassword");
    if (cardIt == request.data.end() || passwordIt == request.data.end() || passwordIt->second.size() < 6) {
        return MakeResponse(false, 1006, "参数不完整或新密码过短");
    }

    std::vector<std::string> row;
    const std::string querySql = "SELECT id FROM users WHERE card_no = " + SqlQuote(db_, cardIt->second) + " LIMIT 1";
    if (!db_->Query(querySql) || !db_->FetchRow(row)) {
        db_->FreeResult();
        return MakeResponse(false, 1001, "用户不存在");
    }
    db_->FreeResult();

    const int targetUserId = ToInt(row[0]);
    const std::string salt = PasswordUtil::GenerateSalt();
    const std::string hash = PasswordUtil::HashPassword(passwordIt->second, salt);
    const std::string updateSql =
        "UPDATE users SET password_hash = " + SqlQuote(db_, hash) + ", password_salt = " + SqlQuote(db_, salt) +
        ", failed_attempts = 0, is_locked = 0 WHERE id = " + std::to_string(targetUserId);
    if (!db_->Execute(updateSql)) {
        return MakeResponse(false, 1011, db_->GetLastError());
    }

    db_->Execute("INSERT INTO admin_logs(admin_user_id, action, target_user_id, detail) VALUES(" +
                 std::to_string(adminSession.userId) + ",'reset_password'," + std::to_string(targetUserId) + "," +
                 SqlQuote(db_, "reset cardNo=" + cardIt->second) + ")");
    return MakeResponse(true, 0, "密码重置成功");
}

Response AuthService::AdminListUsers(const Request& request) {
    if (!IsAdmin(request.sessionId)) {
        return MakeResponse(false, 1005, "权限不足");
    }

    int page = 1;
    int pageSize = 10;
    if (!TryParsePositiveInt(request, "page", 1, 1, 1000000, page) ||
        !TryParsePositiveInt(request, "pageSize", 10, 1, 50, pageSize)) {
        return MakeResponse(false, 1006, "分页参数错误");
    }

    std::vector<std::string> row;
    if (!db_->Query("SELECT COUNT(*) FROM users") || !db_->FetchRow(row)) {
        db_->FreeResult();
        return MakeResponse(false, 1011, db_->GetLastError());
    }
    const int total = ToInt(row[0]);
    db_->FreeResult();

    const int offset = (page - 1) * pageSize;

    const std::string sql =
        "SELECT u.id, u.card_no, u.user_name, u.role, u.is_locked, u.failed_attempts, "
        "IFNULL(a.account_no, ''), IFNULL(a.balance, 0) "
        "FROM users u LEFT JOIN accounts a ON a.user_id = u.id ORDER BY u.id ASC LIMIT " +
        std::to_string(pageSize) + " OFFSET " + std::to_string(offset);
    if (!db_->Query(sql)) {
        return MakeResponse(false, 1011, db_->GetLastError());
    }

    Response response = MakeResponse(true, 0, "查询成功");
    int index = 0;
    while (db_->FetchRow(row)) {
        const std::string prefix = "user" + std::to_string(index++) + ".";
        response.data[prefix + "userId"] = row[0];
        response.data[prefix + "cardNo"] = row[1];
        response.data[prefix + "userName"] = row[2];
        response.data[prefix + "role"] = row[3];
        response.data[prefix + "isLocked"] = row[4] == "1" ? "true" : "false";
        response.data[prefix + "failedAttempts"] = row[5];
        response.data[prefix + "accountNo"] = row[6];
        response.data[prefix + "balance"] = row[7];
    }
    db_->FreeResult();
    response.data["page"] = std::to_string(page);
    response.data["pageSize"] = std::to_string(pageSize);
    response.data["total"] = std::to_string(total);
    response.data["count"] = std::to_string(index);
    return response;
}

bool AuthService::CheckSession(const std::string& sessionId, SessionInfo& session) const {
    std::lock_guard<std::mutex> lock(sessionMutex_);
    const auto it = sessions_.find(sessionId);
    if (it == sessions_.end()) {
        return false;
    }
    session = it->second;
    return session.isValid;
}

bool AuthService::IsAdmin(const std::string& sessionId) const {
    SessionInfo session;
    return CheckSession(sessionId, session) && session.role == "admin";
}

std::string AuthService::GenerateSessionId() const {
    const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    std::ostringstream oss;
    oss << "S_" << now;
    return oss.str();
}

void AuthService::RecordLoginLog(int userId,
                                 const std::string& cardNo,
                                 bool success,
                                 const std::string& failReason,
                                 const std::string& clientIp) {
    if (!db_ || !db_->IsConnected()) {
        return;
    }

    const std::string userIdValue = userId > 0 ? std::to_string(userId) : "NULL";
    const std::string sql =
        "INSERT INTO login_logs(user_id, card_no, is_success, fail_reason, client_ip) VALUES(" +
        userIdValue + "," + SqlQuote(db_, cardNo) + "," + (success ? "1" : "0") + "," +
        (failReason.empty() ? "NULL" : SqlQuote(db_, failReason)) + "," +
        (clientIp.empty() ? "NULL" : SqlQuote(db_, clientIp)) + ")";
    db_->Execute(sql);
}
