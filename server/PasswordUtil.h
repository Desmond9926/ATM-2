#pragma once

#include <string>

class PasswordUtil {
public:
    static std::string GenerateSalt(size_t length = 16);
    static std::string Sha256(const std::string& input);
    static std::string HashPassword(const std::string& password, const std::string& salt);
    static bool VerifyPassword(const std::string& inputPassword,
                               const std::string& salt,
                               const std::string& storedHash);
};
