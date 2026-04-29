#include "Database.h"

#if __has_include(<mysql.h>)
#include <mysql.h>
#elif __has_include("C:/Program Files/MySQL/MySQL Server 8.0/include/mysql.h")
#include "C:/Program Files/MySQL/MySQL Server 8.0/include/mysql.h"
#else
#error MySQL header not found. Configure include path for mysql.h.
#endif

#include <vector>

Database::Database() : conn_(nullptr), result_(nullptr) {}

Database::~Database() {
    Close();
}

bool Database::Connect(const std::string& host,
                       int port,
                       const std::string& user,
                       const std::string& password,
                       const std::string& schema) {
    Close();

    conn_ = mysql_init(nullptr);
    if (conn_ == nullptr) {
        return false;
    }

    if (mysql_real_connect(conn_, host.c_str(), user.c_str(), password.c_str(), schema.c_str(),
                           static_cast<unsigned int>(port), nullptr, 0) == nullptr) {
        Close();
        return false;
    }

    mysql_set_character_set(conn_, "utf8mb4");
    return true;
}

void Database::Close() {
    FreeResult();
    if (conn_ != nullptr) {
        mysql_close(conn_);
        conn_ = nullptr;
    }
}

bool Database::IsConnected() const {
    return conn_ != nullptr;
}

bool Database::Execute(const std::string& sql) {
    if (conn_ == nullptr) {
        return false;
    }

    FreeResult();
    if (mysql_query(conn_, sql.c_str()) != 0) {
        return false;
    }

    if (mysql_field_count(conn_) > 0) {
        result_ = mysql_store_result(conn_);
    }
    return true;
}

bool Database::Query(const std::string& sql) {
    if (conn_ == nullptr) {
        return false;
    }

    FreeResult();
    if (mysql_query(conn_, sql.c_str()) != 0) {
        return false;
    }

    result_ = mysql_store_result(conn_);
    return result_ != nullptr || mysql_field_count(conn_) == 0;
}

bool Database::FetchRow(std::vector<std::string>& row) {
    row.clear();
    if (result_ == nullptr) {
        return false;
    }

    MYSQL_ROW mysqlRow = mysql_fetch_row(result_);
    if (mysqlRow == nullptr) {
        return false;
    }

    unsigned long* lengths = mysql_fetch_lengths(result_);
    const unsigned int count = mysql_num_fields(result_);
    row.reserve(count);
    for (unsigned int i = 0; i < count; ++i) {
        if (mysqlRow[i] == nullptr) {
            row.emplace_back();
        } else {
            row.emplace_back(mysqlRow[i], lengths[i]);
        }
    }
    return true;
}

void Database::FreeResult() {
    if (result_ != nullptr) {
        mysql_free_result(result_);
        result_ = nullptr;
    }
}

bool Database::BeginTransaction() {
    return Execute("START TRANSACTION");
}

bool Database::Commit() {
    return Execute("COMMIT");
}

bool Database::Rollback() {
    return Execute("ROLLBACK");
}

std::string Database::Escape(const std::string& value) const {
    if (conn_ == nullptr) {
        return value;
    }

    std::vector<char> buffer(value.size() * 2 + 1, '\0');
    const unsigned long length = mysql_real_escape_string(conn_, buffer.data(), value.c_str(),
                                                           static_cast<unsigned long>(value.size()));
    return std::string(buffer.data(), length);
}

long long Database::GetLastInsertId() const {
    return conn_ == nullptr ? 0 : static_cast<long long>(mysql_insert_id(conn_));
}

long long Database::GetAffectedRows() const {
    return conn_ == nullptr ? -1 : static_cast<long long>(mysql_affected_rows(conn_));
}

std::string Database::GetLastError() const {
    return conn_ == nullptr ? "mysql not connected" : std::string(mysql_error(conn_));
}
