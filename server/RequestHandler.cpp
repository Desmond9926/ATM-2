#include "RequestHandler.h"

#include "AccountService.h"
#include "AuthService.h"
#include "TransactionService.h"
#include "../common/Protocol.h"

RequestHandler::RequestHandler(AuthService* authService,
                               AccountService* accountService,
                               TransactionService* transactionService)
    : authService_(authService), accountService_(accountService), transactionService_(transactionService) {}

std::string RequestHandler::HandleRequest(const std::string& requestText, const std::string& clientIp) {
    Request request;
    if (!ProtocolUtil::ParseRequest(requestText, request)) {
        Response response;
        response.success = false;
        response.code = 1006;
        response.message = "请求格式错误";
        return ProtocolUtil::BuildResponse(response);
    }

    return ProtocolUtil::BuildResponse(Dispatch(request, clientIp));
}

Response RequestHandler::Dispatch(const Request& request, const std::string& clientIp) {
    if (request.action == "register") {
        return authService_->RegisterUser(request);
    }
    if (request.action == "login") {
        return authService_->Login(request, clientIp);
    }
    if (request.action == "logout") {
        return authService_->Logout(request);
    }
    if (request.action == "change_password") {
        return authService_->ChangePassword(request);
    }
    if (request.action == "query_balance") {
        return accountService_->QueryBalance(request);
    }
    if (request.action == "deposit") {
        return accountService_->Deposit(request);
    }
    if (request.action == "withdraw") {
        return accountService_->Withdraw(request);
    }
    if (request.action == "transfer") {
        return transactionService_->Transfer(request);
    }
    if (request.action == "query_transactions") {
        return transactionService_->QueryMyTransactions(request);
    }
    if (request.action == "admin_list_users") {
        return authService_->AdminListUsers(request);
    }
    if (request.action == "admin_unlock_user") {
        return authService_->AdminUnlockUser(request);
    }
    if (request.action == "admin_reset_password") {
        return authService_->AdminResetPassword(request);
    }
    if (request.action == "admin_query_transactions") {
        return transactionService_->AdminQueryTransactions(request);
    }

    Response response;
    response.success = false;
    response.code = 1006;
    response.message = "未知操作";
    return response;
}
