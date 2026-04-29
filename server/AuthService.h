#pragma once

#include <mutex>
#include <string>
#include <unordered_map>

#include "../common/Models.h"
#include "../common/Protocol.h"

class Database;

class AuthService {
public:
    explicit AuthService(Database* db);

    Response RegisterUser(const Request& request);
    Response Login(const Request& request, const std::string& clientIp);
    Response Logout(const Request& request);
    Response ChangePassword(const Request& request);

    Response AdminUnlockUser(const Request& request);
    Response AdminResetPassword(const Request& request);
    Response AdminListUsers(const Request& request);

    bool CheckSession(const std::string& sessionId, SessionInfo& session) const;
    bool IsAdmin(const std::string& sessionId) const;

private:
    std::string GenerateSessionId() const;
    void RecordLoginLog(int userId,
                        const std::string& cardNo,
                        bool success,
                        const std::string& failReason,
                        const std::string& clientIp);

private:
    Database* db_;
    mutable std::mutex sessionMutex_;
    std::unordered_map<std::string, SessionInfo> sessions_;
};
