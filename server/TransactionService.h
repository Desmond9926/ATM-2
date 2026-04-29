#pragma once

#include "../common/Models.h"
#include "../common/Protocol.h"

class AuthService;
class Database;

class TransactionService {
public:
    TransactionService(Database* db, AuthService* authService);

    Response Transfer(const Request& request);
    Response QueryMyTransactions(const Request& request);
    Response AdminQueryTransactions(const Request& request);

private:
    bool VerifySessionPassword(const SessionInfo& session, const Request& request, Response& errorResponse);
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
