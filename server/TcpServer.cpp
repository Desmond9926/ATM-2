#include "TcpServer.h"

#include "RequestHandler.h"

#include <ws2tcpip.h>

#include <iostream>
#include <thread>

#pragma comment(lib, "ws2_32.lib")

TcpServer::TcpServer() : listenSocket_(INVALID_SOCKET), handler_(nullptr), running_(false) {}

TcpServer::~TcpServer() {
    Stop();
}

bool TcpServer::Start(const std::string& ip, int port, RequestHandler* handler) {
    handler_ = handler;

    WSADATA wsaData{};
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return false;
    }

    listenSocket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket_ == INVALID_SOCKET) {
        WSACleanup();
        return false;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(static_cast<u_short>(port));
    inet_pton(AF_INET, ip.c_str(), &serverAddr.sin_addr);

    if (bind(listenSocket_, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
        closesocket(listenSocket_);
        listenSocket_ = INVALID_SOCKET;
        WSACleanup();
        return false;
    }

    if (listen(listenSocket_, SOMAXCONN) == SOCKET_ERROR) {
        closesocket(listenSocket_);
        listenSocket_ = INVALID_SOCKET;
        WSACleanup();
        return false;
    }

    running_ = true;
    AcceptLoop();
    return true;
}

void TcpServer::Stop() {
    running_ = false;
    if (listenSocket_ != INVALID_SOCKET) {
        closesocket(listenSocket_);
        listenSocket_ = INVALID_SOCKET;
    }
    WSACleanup();
}

void TcpServer::AcceptLoop() {
    while (running_) {
        sockaddr_in clientAddr{};
        int length = sizeof(clientAddr);
        SOCKET clientSocket = accept(listenSocket_, reinterpret_cast<sockaddr*>(&clientAddr), &length);
        if (clientSocket == INVALID_SOCKET) {
            continue;
        }

        char ipBuffer[INET_ADDRSTRLEN] = {};
        inet_ntop(AF_INET, &clientAddr.sin_addr, ipBuffer, sizeof(ipBuffer));
        std::thread(&TcpServer::HandleClient, this, clientSocket, std::string(ipBuffer)).detach();
    }
}

void TcpServer::HandleClient(SOCKET clientSocket, const std::string& clientIp) {
    char buffer[4096] = {};
    while (true) {
        const int received = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (received <= 0) {
            break;
        }

        buffer[received] = '\0';
        std::string requestText(buffer);
        const std::string responseText = handler_->HandleRequest(requestText, clientIp);
        send(clientSocket, responseText.c_str(), static_cast<int>(responseText.size()), 0);
    }

    closesocket(clientSocket);
}
