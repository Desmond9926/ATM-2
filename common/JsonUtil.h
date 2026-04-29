#pragma once

#include <map>
#include <string>

class JsonUtil {
public:
    static std::string Escape(const std::string& value);
    static std::string Quote(const std::string& value);
    static std::string SerializeObject(const std::map<std::string, std::string>& values);
    static bool ExtractStringField(const std::string& json, const std::string& key, std::string& value);
    static bool ExtractBoolField(const std::string& json, const std::string& key, bool& value);
    static bool ExtractIntField(const std::string& json, const std::string& key, int& value);
    static bool ExtractObjectFields(const std::string& json, const std::string& key, std::map<std::string, std::string>& values);
};
