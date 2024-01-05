#include "SocketServer.h"

//Might need to uncomment to run on Visual Studio
//#pragma comment(lib,"ws2_32.lib")

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include <string>
#include <cstring>

SocketServer::SocketServer() {
    Mood = 10;
    HasClient = false;
    _ready = false;
}

SocketServer::~SocketServer() {
    _ready = false;
    _clientThread.join();
    WSACleanup();
}

void SocketServer::StartServer() {
    SOCKET server;
    SOCKADDR_IN serverAddr;
    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2, 0), &wsaData) != NO_ERROR) {
        std::cout << "WSAStartup FAILED!";
        return;
    }

    if ((server = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        std::cout << "socket() FAILED!\n";
        WSACleanup();
        return;
    }

    // optional but could set socket server options
    ZeroMemory(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    //serverAddr.sin_addr.s_addr = /*INADDR_ANY; */ htonl(INADDR_ANY);
    serverAddr.sin_port = htons(PORT);

    // could try this instead for setting up the server address with the right IP
    InetPton(AF_INET, reinterpret_cast<LPCSTR>(L"10.72.95.19"), &serverAddr.sin_addr.s_addr);

    if (bind(server, (sockaddr *) &serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cout << "bind() FAILED!\n";
        WSACleanup();
        return;
    }

    if (listen(server, 2) == SOCKET_ERROR) {
        std::cout << "listen() FAILED!\n";
        WSACleanup();
        return;
    }

    std::cout << "Server Started!" << std::endl;

    //BuildMessage("#4|#0|#2|#3|#4|#0|#2|#3|#4|#0|#2|#3|");
    _ready = true;
    _clientThread = std::thread(&SocketServer::HandleClientMessages, this, server);
}

void SocketServer::HandleClientMessages(int server) {
    while (_ready) {
        SOCKET client;
        SOCKADDR_IN clientAddr;
        int clientlength = sizeof(clientAddr);

        std::cout << "Looking for clients\n";

        if ((client = accept(server, (struct sockaddr *) &clientAddr, &clientlength)) == INVALID_SOCKET)
            std::cout << "accept() FAILED!\n";

        char ipaddclient[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(clientAddr.sin_addr), ipaddclient, INET_ADDRSTRLEN);
        std::cout << "Connection from " << ipaddclient << std::endl;

        HasClient = true;
        char message[BUFFER_SIZE] = {0};
        while (recv(client, message, BUFFER_SIZE, 0) > 0) {
            //std::cout << "Raw message: " << message << std::endl;
            BuildMessage(message);
        }

        if (shutdown(client, SD_SEND) == SOCKET_ERROR)
            std::cout << "shutdown() FAILED!";

        if (closesocket(client) == SOCKET_ERROR)
            std::cout << "closesocket() FAILED!";

        HasClient = false;
        std::cout << "Client disconnected!" << std::endl;
    }
}

// Connection protocol, '#' signifies the beginning of a message and '|' signifies the end of a message
// Example message: #5|#0|#2|#3|
void SocketServer::BuildMessage(const char *message) {
    std::string formatedMessage;
    for (unsigned int i = 0; i < std::strlen(message); i++) {
        if (message[i] == '|' && (i + 1 < std::strlen(message) && message[i + 1] != '#'))
            break;

        if (message[i] == '#')
            continue;

        formatedMessage += message[i];

        if (message[i] == '|') {
            //std::cout << "sensor mood: " << std::stoi(formatedMessage) << std::endl;
            Mood += ProcessMessage(std::stoi(formatedMessage));
            formatedMessage.clear();
        }
    }
}

// used to make a sort of gradient where values go from 10 to 90
// the message value is within the current "mood" threshold, it does not do anything  
int SocketServer::ProcessMessage(const int &messageValue) const {
    int baseline = 20 * (messageValue + 1) - 10;
    int multiplier = 0;

    if (baseline < Mood)
        multiplier = -1;
    else if (baseline > Mood)
        multiplier = 1;

    return 5 * multiplier;
}
