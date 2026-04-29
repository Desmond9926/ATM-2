#pragma once

#include <string>

struct User {
    int id = 0;
    std::string cardNo;
    std::string userName;
    std::string passwordHash;
    std::string passwordSalt;
    int failedAttempts = 0;
    bool isLocked = false;
    std::string role;
};

struct Account {
    int id = 0;
    int userId = 0;
    std::string accountNo;
    double balance = 0.0;
    std::string status;
};

struct TransactionRecord {
    long long id = 0;
    int accountId = 0;
    std::string txType;
    double amount = 0.0;
    double balanceBefore = 0.0;
    double balanceAfter = 0.0;
    std::string relatedAccountNo;
    std::string remark;
    std::string createdAt;
};

struct SessionInfo {
    std::string sessionId;
    int userId = 0;
    std::string cardNo;
    std::string role;
    bool isValid = false;
};
