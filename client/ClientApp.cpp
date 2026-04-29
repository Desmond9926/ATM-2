#include "ClientApp.h"

#include "ClientSocket.h"
#include "../common/Protocol.h"

#include <cstdlib>
#include <iostream>

ClientApp::ClientApp(ClientSocket* clientSocket)
    : clientSocket_(clientSocket), loggedIn_(false) {}

void ClientApp::Run() {
    while (true) {
        if (!loggedIn_) {
            ShowWelcomeMenu();
            continue;
        }

        if (role_ == "admin") {
            ShowAdminMenu();
        } else {
            ShowUserMenu();
        }
    }
}

void ClientApp::ShowWelcomeMenu() {
    std::cout << "\n===== ATM System =====\n";
    std::cout << "1. Register\n";
    std::cout << "2. Login\n";
    std::cout << "3. Exit\n";

    const int choice = PromptMenuChoice("Select");

    switch (choice) {
    case 1:
        Register();
        break;
    case 2:
        Login();
        break;
    case 3:
        std::exit(0);
    default:
        std::cout << "Invalid choice.\n";
        break;
    }
}

void ClientApp::ShowUserMenu() {
    std::cout << "\n===== User Menu =====\n";
    std::cout << "1. Query Balance\n";
    std::cout << "2. Deposit\n";
    std::cout << "3. Withdraw\n";
    std::cout << "4. Transfer\n";
    std::cout << "5. Change Password\n";
    std::cout << "6. Query Transactions\n";
    std::cout << "7. Logout\n";

    const int choice = PromptMenuChoice("Select");

    switch (choice) {
    case 1:
        QueryBalance();
        break;
    case 2:
        Deposit();
        break;
    case 3:
        Withdraw();
        break;
    case 4:
        Transfer();
        break;
    case 5:
        ChangePassword();
        break;
    case 6:
        QueryTransactions();
        break;
    case 7:
        Logout();
        break;
    default:
        std::cout << "Invalid choice.\n";
        break;
    }
}

void ClientApp::ShowAdminMenu() {
    std::cout << "\n===== Admin Menu =====\n";
    std::cout << "1. List Users\n";
    std::cout << "2. Unlock User\n";
    std::cout << "3. Reset User Password\n";
    std::cout << "4. Query Transactions\n";
    std::cout << "5. Logout\n";

    const int choice = PromptMenuChoice("Select");

    switch (choice) {
    case 1:
        AdminListUsers();
        break;
    case 2:
        AdminUnlockUser();
        break;
    case 3:
        AdminResetPassword();
        break;
    case 4:
        AdminQueryTransactions();
        break;
    case 5:
        Logout();
        break;
    default:
        std::cout << "Invalid choice.\n";
        break;
    }
}

void ClientApp::Register() {
    const std::string requestText = BuildRequest("register",
                                                 { {"cardNo", Prompt("Card No")},
                                                   {"userName", Prompt("User Name")},
                                                   {"password", Prompt("Password")},
                                                   {"accountNo", Prompt("Account No")},
                                                   {"initialBalance", Prompt("Initial Balance")} });
    SendAndPrint("register", requestText);
}

bool ClientApp::Login() {
    const std::string requestText = BuildRequest("login",
                                                 { {"cardNo", Prompt("Card No")},
                                                   {"password", Prompt("Password")} });

    std::string responseText;
    if (!clientSocket_->SendRequest(requestText, responseText)) {
        std::cout << "Request failed.\n";
        return false;
    }

    Response response;
    if (!ProtocolUtil::ParseResponse(responseText, response)) {
        std::cout << "Invalid response.\n";
        return false;
    }

    std::cout << response.message << "\n";
    if (!response.success) {
        return false;
    }

    sessionId_ = response.data["sessionId"];
    role_ = response.data["role"];
    loggedIn_ = true;
    return true;
}

void ClientApp::Logout() {
    SendAndPrint("logout", BuildRequest("logout", {}));
    sessionId_.clear();
    role_.clear();
    loggedIn_ = false;
}

void ClientApp::QueryBalance() {
    SendAndPrint("query_balance", BuildRequest("query_balance", { {"password", Prompt("Password")} }));
}

void ClientApp::Deposit() {
    SendAndPrint("deposit", BuildRequest("deposit", { {"password", Prompt("Password")},
                                                         {"amount", Prompt("Amount")},
                                                         {"remark", Prompt("Remark")} }));
}

void ClientApp::Withdraw() {
    SendAndPrint("withdraw", BuildRequest("withdraw", { {"password", Prompt("Password")},
                                                           {"amount", Prompt("Amount")},
                                                           {"remark", Prompt("Remark")} }));
}

void ClientApp::Transfer() {
    SendAndPrint("transfer", BuildRequest("transfer", { {"password", Prompt("Password")},
                                                           {"targetAccountNo", Prompt("Target Account")},
                                                           {"amount", Prompt("Amount")},
                                                           {"remark", Prompt("Remark")} }));
}

void ClientApp::ChangePassword() {
    SendAndPrint("change_password", BuildRequest("change_password", { {"oldPassword", Prompt("Old Password")},
                                                                        {"newPassword", Prompt("New Password")} }));
}

void ClientApp::QueryTransactions() {
    SendAndPrint("query_transactions", BuildRequest("query_transactions",
                                                     { {"page", PromptPositiveInteger("Page", 1)},
                                                       {"pageSize", PromptPositiveInteger("Page Size", 10)} }));
}

void ClientApp::AdminListUsers() {
    SendAndPrint("admin_list_users", BuildRequest("admin_list_users",
                                                   { {"page", PromptPositiveInteger("Page", 1)},
                                                     {"pageSize", PromptPositiveInteger("Page Size", 10)} }));
}

void ClientApp::AdminUnlockUser() {
    SendAndPrint("admin_unlock_user", BuildRequest("admin_unlock_user", { {"cardNo", Prompt("User Card No")} }));
}

void ClientApp::AdminResetPassword() {
    SendAndPrint("admin_reset_password", BuildRequest("admin_reset_password", { {"cardNo", Prompt("User Card No")},
                                                                                   {"newPassword", Prompt("New Password")} }));
}

void ClientApp::AdminQueryTransactions() {
    SendAndPrint("admin_query_transactions", BuildRequest("admin_query_transactions",
                                                           { {"accountNo", Prompt("Account No")},
                                                             {"page", PromptPositiveInteger("Page", 1)},
                                                             {"pageSize", PromptPositiveInteger("Page Size", 10)} }));
}

std::string ClientApp::BuildRequest(const std::string& action,
                                    const std::initializer_list<std::pair<std::string, std::string>>& fields) const {
    Request request;
    request.action = action;
    request.sessionId = sessionId_;
    for (const auto& field : fields) {
        request.data[field.first] = field.second;
    }
    return ProtocolUtil::BuildRequest(request);
}

bool ClientApp::SendAndPrint(const std::string& action, const std::string& requestText) {
    std::string responseText;
    if (!clientSocket_->SendRequest(requestText, responseText)) {
        std::cout << "Request failed.\n";
        return false;
    }

    Response response;
    if (!ProtocolUtil::ParseResponse(responseText, response)) {
        std::cout << "Invalid response.\n";
        return false;
    }

    std::cout << response.message << "\n";
    PrintResponseData(action, response.data);
    return response.success;
}

void ClientApp::PrintResponseData(const std::string& action, const std::map<std::string, std::string>& data) const {
    if (action == "query_transactions" || action == "admin_query_transactions") {
        PrintTransactionRecords(data);
        return;
    }

    if (action == "admin_list_users") {
        PrintUserRecords(data);
        return;
    }

    for (const auto& [key, value] : data) {
        std::cout << key << ": " << value << "\n";
    }
}

void ClientApp::PrintTransactionRecords(const std::map<std::string, std::string>& data) const {
    const auto pageIt = data.find("page");
    const auto pageSizeIt = data.find("pageSize");
    const auto totalIt = data.find("total");
    const auto countIt = data.find("count");
    if (pageIt != data.end() || pageSizeIt != data.end() || totalIt != data.end() || countIt != data.end()) {
        std::cout << "Page: " << (pageIt != data.end() ? pageIt->second : "1")
                  << ", Page Size: " << (pageSizeIt != data.end() ? pageSizeIt->second : "10")
                  << ", Total: " << (totalIt != data.end() ? totalIt->second : "0")
                  << ", Current Count: " << (countIt != data.end() ? countIt->second : "0") << "\n";
    }

    int index = 0;
    while (true) {
        const std::string prefix = "record" + std::to_string(index) + ".";
        const auto txIdIt = data.find(prefix + "txId");
        if (txIdIt == data.end()) {
            break;
        }

        std::cout << "[" << index + 1 << "] "
                  << "txId=" << txIdIt->second
                  << ", type=" << data.at(prefix + "txType")
                  << ", amount=" << data.at(prefix + "amount")
                  << ", before=" << data.at(prefix + "balanceBefore")
                  << ", after=" << data.at(prefix + "balanceAfter");

        const auto relatedIt = data.find(prefix + "relatedAccountNo");
        if (relatedIt != data.end() && !relatedIt->second.empty()) {
            std::cout << ", related=" << relatedIt->second;
        }
        const auto remarkIt = data.find(prefix + "remark");
        if (remarkIt != data.end() && !remarkIt->second.empty()) {
            std::cout << ", remark=" << remarkIt->second;
        }
        const auto timeIt = data.find(prefix + "createdAt");
        if (timeIt != data.end()) {
            std::cout << ", at=" << timeIt->second;
        }
        std::cout << "\n";
        ++index;
    }

    if (index == 0) {
        std::cout << "No records.\n";
    }
}

void ClientApp::PrintUserRecords(const std::map<std::string, std::string>& data) const {
    const auto pageIt = data.find("page");
    const auto pageSizeIt = data.find("pageSize");
    const auto totalIt = data.find("total");
    const auto countIt = data.find("count");
    if (pageIt != data.end() || pageSizeIt != data.end() || totalIt != data.end() || countIt != data.end()) {
        std::cout << "Page: " << (pageIt != data.end() ? pageIt->second : "1")
                  << ", Page Size: " << (pageSizeIt != data.end() ? pageSizeIt->second : "10")
                  << ", Total: " << (totalIt != data.end() ? totalIt->second : "0")
                  << ", Current Count: " << (countIt != data.end() ? countIt->second : "0") << "\n";
    }

    int index = 0;
    while (true) {
        const std::string prefix = "user" + std::to_string(index) + ".";
        const auto userIdIt = data.find(prefix + "userId");
        if (userIdIt == data.end()) {
            break;
        }

        std::cout << "[" << index + 1 << "] "
                  << "userId=" << userIdIt->second
                  << ", cardNo=" << data.at(prefix + "cardNo")
                  << ", userName=" << data.at(prefix + "userName")
                  << ", role=" << data.at(prefix + "role")
                  << ", locked=" << data.at(prefix + "isLocked")
                  << ", failedAttempts=" << data.at(prefix + "failedAttempts")
                  << ", accountNo=" << data.at(prefix + "accountNo")
                  << ", balance=" << data.at(prefix + "balance") << "\n";
        ++index;
    }

    if (index == 0) {
        std::cout << "No users.\n";
    }
}

std::string ClientApp::Prompt(const std::string& label) {
    std::cout << label << ": ";
    std::string value;
    if (!std::getline(std::cin, value)) {
        return {};
    }
    return value;
}

int ClientApp::PromptMenuChoice(const std::string& label) {
    while (true) {
        const std::string value = Prompt(label);
        if (!std::cin) {
            std::exit(0);
        }
        try {
            size_t consumed = 0;
            const int choice = std::stoi(value, &consumed);
            if (consumed == value.size()) {
                return choice;
            }
        } catch (...) {
        }
        std::cout << "Invalid choice.\n";
    }
}

std::string ClientApp::PromptPositiveInteger(const std::string& label, int defaultValue) {
    while (true) {
        std::cout << label << " [default " << defaultValue << "]: ";
        std::string value;
        if (!std::getline(std::cin, value)) {
            std::exit(0);
        }
        if (value.empty()) {
            return std::to_string(defaultValue);
        }

        try {
            size_t consumed = 0;
            const int parsed = std::stoi(value, &consumed);
            if (consumed == value.size() && parsed > 0) {
                return std::to_string(parsed);
            }
        } catch (...) {
        }

        std::cout << "Please enter a positive integer.\n";
    }
}
