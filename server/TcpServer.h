#pragma once

#include <atomic>
#include <string>

#include <winsock2.h>

class RequestHandler;

class TcpServer {
public:
    TcpServer();
    ~TcpServer();

    bool Start(const std::string& ip, int port, RequestHandler* handler);
    void Stop();

private:
    void AcceptLoop();
    void HandleClient(SOCKET clientSocket, const std::string& clientIp);

private:
    SOCKET listenSocket_;
    RequestHandler* handler_;
    std::atomic<bool> running_;
};
