#include "ClientApp.h"
#include "ClientSocket.h"

#include <Windows.h>

#include <iostream>

namespace {
void ConfigureConsoleEncoding() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
}
}

int main() {
    ConfigureConsoleEncoding();

    ClientSocket clientSocket;
    if (!clientSocket.Connect("127.0.0.1", 8888)) {
        std::cerr << "Failed to connect to server." << std::endl;
        std::cerr << "Press Enter to exit." << std::endl;
        std::cin.get();
        return 1;
    }

    ClientApp app(&clientSocket);
    app.Run();
    return 0;
}
