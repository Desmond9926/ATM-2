#include "Protocol.h"

#include "JsonUtil.h"

#include <sstream>

bool ProtocolUtil::ParseRequest(const std::string& jsonText, Request& request) {
    request = Request{};
    if (!JsonUtil::ExtractStringField(jsonText, "action", request.action)) {
        return false;
    }

    JsonUtil::ExtractStringField(jsonText, "sessionId", request.sessionId);
    JsonUtil::ExtractObjectFields(jsonText, "data", request.data);
    return true;
}

bool ProtocolUtil::ParseResponse(const std::string& jsonText, Response& response) {
    response = Response{};
    if (!JsonUtil::ExtractBoolField(jsonText, "success", response.success)) {
        return false;
    }

    JsonUtil::ExtractIntField(jsonText, "code", response.code);
    JsonUtil::ExtractStringField(jsonText, "message", response.message);
    JsonUtil::ExtractObjectFields(jsonText, "data", response.data);
    return true;
}

std::string ProtocolUtil::BuildRequest(const Request& request) {
    std::ostringstream oss;
    oss << "{";
    oss << "\"action\":" << JsonUtil::Quote(request.action) << ",";
    oss << "\"sessionId\":" << JsonUtil::Quote(request.sessionId) << ",";
    oss << "\"data\":" << JsonUtil::SerializeObject(request.data);
    oss << "}";
    return oss.str();
}

std::string ProtocolUtil::BuildResponse(const Response& response) {
    std::ostringstream oss;
    oss << "{";
    oss << "\"success\":" << (response.success ? "true" : "false") << ",";
    oss << "\"code\":" << response.code << ",";
    oss << "\"message\":" << JsonUtil::Quote(response.message) << ",";
    oss << "\"data\":" << JsonUtil::SerializeObject(response.data);
    oss << "}";
    return oss.str();
}
