#pragma once

#include <string>

#include "../common/Protocol.h"

class AccountService;
class AuthService;
class TransactionService;

class RequestHandler {
public:
    RequestHandler(AuthService* authService,
                   AccountService* accountService,
                   TransactionService* transactionService);

    std::string HandleRequest(const std::string& requestText, const std::string& clientIp);

private:
    Response Dispatch(const Request& request, const std::string& clientIp);

private:
    AuthService* authService_;
    AccountService* accountService_;
    TransactionService* transactionService_;
};
