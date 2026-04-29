#pragma once

#include "../common/Models.h"
#include "../common/Protocol.h"

class AuthService;
class Database;

class AccountService {
public:
    AccountService(Database* db, AuthService* authService);

    Response QueryBalance(const Request& request);
    Response Deposit(const Request& request);
    Response Withdraw(const Request& request);

private:
    bool VerifySessionPassword(const SessionInfo& session, const Request& request, Response& errorResponse);
    bool GetCurrentAccountBySession(const std::string& sessionId, Account& account, SessionInfo& session);
    bool InsertTransaction(int accountId,
                           const std::string& txType,
                           double amount,
                           double balanceBefore,
                           double balanceAfter,
                           const std::string& relatedAccountNo,
                           const std::string& remark);

private:
    Database* db_;
    AuthService* authService_;
};
