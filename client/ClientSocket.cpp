#include "ClientSocket.h"

#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

ClientSocket::ClientSocket() : socket_(INVALID_SOCKET) {
    WSADATA wsaData{};
    WSAStartup(MAKEWORD(2, 2), &wsaData);
}

ClientSocket::~ClientSocket() {
    Close();
    WSACleanup();
}

bool ClientSocket::Connect(const std::string& ip, int port) {
    socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket_ == INVALID_SOCKET) {
        return false;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(static_cast<u_short>(port));
    inet_pton(AF_INET, ip.c_str(), &serverAddr.sin_addr);

    return connect(socket_, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) != SOCKET_ERROR;
}

void ClientSocket::Close() {
    if (socket_ != INVALID_SOCKET) {
        closesocket(socket_);
        socket_ = INVALID_SOCKET;
    }
}

bool ClientSocket::Send(const std::string& text) {
    return send(socket_, text.c_str(), static_cast<int>(text.size()), 0) != SOCKET_ERROR;
}

bool ClientSocket::Receive(std::string& text) {
    char buffer[4096] = {};
    const int received = recv(socket_, buffer, sizeof(buffer) - 1, 0);
    if (received <= 0) {
        return false;
    }
    buffer[received] = '\0';
    text.assign(buffer, received);
    return true;
}

bool ClientSocket::SendRequest(const std::string& requestText, std::string& responseText) {
    return Send(requestText) && Receive(responseText);
}
