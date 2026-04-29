#pragma once

#include <map>
#include <string>

struct Request {
    std::string action;
    std::string sessionId;
    std::map<std::string, std::string> data;
};

struct Response {
    bool success = false;
    int code = -1;
    std::string message;
    std::map<std::string, std::string> data;
};

class ProtocolUtil {
public:
    static bool ParseRequest(const std::string& jsonText, Request& request);
    static bool ParseResponse(const std::string& jsonText, Response& response);
    static std::string BuildRequest(const Request& request);
    static std::string BuildResponse(const Response& response);
};
