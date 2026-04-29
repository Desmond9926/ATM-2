#include "AccountService.h"
#include "AuthService.h"
#include "Database.h"
#include "RequestHandler.h"
#include "TcpServer.h"
#include "TransactionService.h"

#include <Windows.h>

#include <cstdlib>
#include <iostream>
#include <string>

namespace {
void ConfigureConsoleEncoding() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
}

std::string GetEnvOrDefault(const char* name, const char* defaultValue) {
    const char* value = std::getenv(name);
    return (value != nullptr && value[0] != '\0') ? value : defaultValue;
}

int GetEnvIntOrDefault(const char* name, int defaultValue) {
    const char* value = std::getenv(name);
    if (value == nullptr || value[0] == '\0') {
        return defaultValue;
    }

    try {
        return std::stoi(value);
    } catch (...) {
        return defaultValue;
    }
}
}

int main() {
    ConfigureConsoleEncoding();

    const std::string dbHost = GetEnvOrDefault("ATM_DB_HOST", "127.0.0.1");
    const int dbPort = GetEnvIntOrDefault("ATM_DB_PORT", 3306);
    const std::string dbUser = GetEnvOrDefault("ATM_DB_USER", "root");
    const std::string dbPassword = GetEnvOrDefault("ATM_DB_PASSWORD", "@13921161526zhDQ");
    const std::string dbSchema = GetEnvOrDefault("ATM_DB_SCHEMA", "atm_system");
    const std::string serverHost = GetEnvOrDefault("ATM_SERVER_HOST", "127.0.0.1");
    const int serverPort = GetEnvIntOrDefault("ATM_SERVER_PORT", 8888);

    Database database;
    if (!database.Connect(dbHost, dbPort, dbUser, dbPassword, dbSchema)) {
        std::cerr << "Database connection failed." << std::endl;
        return 1;
    }

    AuthService authService(&database);
    AccountService accountService(&database, &authService);
    TransactionService transactionService(&database, &authService);
    RequestHandler handler(&authService, &accountService, &transactionService);

    TcpServer server;
    std::cout << "ATM server started at " << serverHost << ":" << serverPort << std::endl;
    if (!server.Start(serverHost, serverPort, &handler)) {
        std::cerr << "Server start failed." << std::endl;
        return 1;
    }

    return 0;
}
