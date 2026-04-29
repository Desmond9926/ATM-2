#pragma once

#include <string>

#include <winsock2.h>

class ClientSocket {
public:
    ClientSocket();
    ~ClientSocket();

    bool Connect(const std::string& ip, int port);
    void Close();

    bool Send(const std::string& text);
    bool Receive(std::string& text);
    bool SendRequest(const std::string& requestText, std::string& responseText);

private:
    SOCKET socket_;
};
