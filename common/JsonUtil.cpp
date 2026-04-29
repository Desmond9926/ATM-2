#include "JsonUtil.h"

#include <cctype>
#include <regex>
#include <sstream>

namespace {
std::string Trim(const std::string& input) {
    size_t begin = 0;
    while (begin < input.size() && std::isspace(static_cast<unsigned char>(input[begin])) != 0) {
        ++begin;
    }

    size_t end = input.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(input[end - 1])) != 0) {
        --end;
    }

    return input.substr(begin, end - begin);
}
}

std::string JsonUtil::Escape(const std::string& value) {
    std::string escaped;
    escaped.reserve(value.size());

    for (char ch : value) {
        switch (ch) {
        case '\\':
            escaped += "\\\\";
            break;
        case '"':
            escaped += "\\\"";
            break;
        case '\n':
            escaped += "\\n";
            break;
        case '\r':
            escaped += "\\r";
            break;
        case '\t':
            escaped += "\\t";
            break;
        default:
            escaped += ch;
            break;
        }
    }

    return escaped;
}

std::string JsonUtil::Quote(const std::string& value) {
    return std::string("\"") + Escape(value) + "\"";
}

std::string JsonUtil::SerializeObject(const std::map<std::string, std::string>& values) {
    std::ostringstream oss;
    oss << "{";

    bool first = true;
    for (const auto& [key, value] : values) {
        if (!first) {
            oss << ",";
        }
        first = false;
        oss << Quote(key) << ":" << Quote(value);
    }

    oss << "}";
    return oss.str();
}

bool JsonUtil::ExtractStringField(const std::string& json, const std::string& key, std::string& value) {
    const std::regex pattern("\"" + key + "\"\\s*:\\s*\"((?:\\\\.|[^\"])*)\"");
    std::smatch match;
    if (!std::regex_search(json, match, pattern)) {
        return false;
    }

    value = match[1].str();
    return true;
}

bool JsonUtil::ExtractBoolField(const std::string& json, const std::string& key, bool& value) {
    const std::regex pattern("\"" + key + "\"\\s*:\\s*(true|false)");
    std::smatch match;
    if (!std::regex_search(json, match, pattern)) {
        return false;
    }

    value = match[1].str() == "true";
    return true;
}

bool JsonUtil::ExtractIntField(const std::string& json, const std::string& key, int& value) {
    const std::regex pattern("\"" + key + "\"\\s*:\\s*(-?[0-9]+)");
    std::smatch match;
    if (!std::regex_search(json, match, pattern)) {
        return false;
    }

    value = std::stoi(match[1].str());
    return true;
}

bool JsonUtil::ExtractObjectFields(const std::string& json, const std::string& key, std::map<std::string, std::string>& values) {
    values.clear();

    const std::regex objectPattern("\"" + key + "\"\\s*:\\s*\\{([^}]*)\\}");
    std::smatch objectMatch;
    if (!std::regex_search(json, objectMatch, objectPattern)) {
        return false;
    }

    const std::string objectBody = objectMatch[1].str();
    const std::regex fieldPattern("\"([^\"]+)\"\\s*:\\s*(\"((?:\\\\.|[^\"])*)\"|true|false|-?[0-9]+(?:\\.[0-9]+)?)");

    auto begin = std::sregex_iterator(objectBody.begin(), objectBody.end(), fieldPattern);
    auto end = std::sregex_iterator();
    for (auto it = begin; it != end; ++it) {
        const std::smatch& match = *it;
        const std::string fieldKey = match[1].str();
        std::string fieldValue = match[2].str();
        if (fieldValue.size() >= 2 && fieldValue.front() == '"' && fieldValue.back() == '"') {
            fieldValue = match[3].str();
        }
        values[fieldKey] = Trim(fieldValue);
    }

    return true;
}
