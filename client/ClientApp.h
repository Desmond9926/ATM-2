#pragma once

#include <initializer_list>
#include <map>
#include <string>

class ClientSocket;

class ClientApp {
public:
    explicit ClientApp(ClientSocket* clientSocket);

    void Run();

private:
    void ShowWelcomeMenu();
    void ShowUserMenu();
    void ShowAdminMenu();

    void Register();
    bool Login();
    void Logout();

    void QueryBalance();
    void Deposit();
    void Withdraw();
    void Transfer();
    void ChangePassword();
    void QueryTransactions();

    void AdminListUsers();
    void AdminUnlockUser();
    void AdminResetPassword();
    void AdminQueryTransactions();

    std::string BuildRequest(const std::string& action,
                             const std::initializer_list<std::pair<std::string, std::string>>& fields) const;
    bool SendAndPrint(const std::string& action, const std::string& requestText);
    void PrintResponseData(const std::string& action, const std::map<std::string, std::string>& data) const;
    void PrintTransactionRecords(const std::map<std::string, std::string>& data) const;
    void PrintUserRecords(const std::map<std::string, std::string>& data) const;
    static std::string Prompt(const std::string& label);
    static int PromptMenuChoice(const std::string& label);
    static std::string PromptPositiveInteger(const std::string& label, int defaultValue);

private:
    ClientSocket* clientSocket_;
    std::string sessionId_;
    std::string role_;
    bool loggedIn_;
};
