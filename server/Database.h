#pragma once

#include <string>
#include <vector>

struct MYSQL;
struct MYSQL_RES;

class Database {
public:
    Database();
    ~Database();

    bool Connect(const std::string& host,
                 int port,
                 const std::string& user,
                 const std::string& password,
                 const std::string& schema);
    void Close();
    bool IsConnected() const;

    bool Execute(const std::string& sql);
    bool Query(const std::string& sql);
    bool FetchRow(std::vector<std::string>& row);
    void FreeResult();

    bool BeginTransaction();
    bool Commit();
    bool Rollback();

    std::string Escape(const std::string& value) const;
    long long GetLastInsertId() const;
    long long GetAffectedRows() const;
    std::string GetLastError() const;

private:
    MYSQL* conn_;
    MYSQL_RES* result_;
};
