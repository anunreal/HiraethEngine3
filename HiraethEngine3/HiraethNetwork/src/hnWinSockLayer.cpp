#pragma once

#ifdef HN_USE_WINSOCK

#include "hnConnection.h"
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")


void hnNetworkCreate() {
    
    if(hn == nullptr) {
        hn = new HiraethNetwork();
        hn->status = HN_STATUS_ERROR; // assume that something will go wrong
        
        // set up wsa
        WSAData wsData;
        WORD ver = MAKEWORD(2, 2);
        
        if(WSAStartup(ver, &wsData) != 0) {
            HN_ERROR("Could not start wsa, exiting...");
            return;
        }
        
        hn->status = HN_STATUS_CONNECTED; // successful start
        //startTime = std::chrono::high_resolution_clock::now();
    }
    
};

void hnNetworkDestroy() {
    
    WSACleanup();
    if(hn != nullptr)
        hn->status = HN_STATUS_DISCONNECTED;
    
};


void hnSocketCreateUdp(HnSocket* hnSocket) {
    
    hnSocket->status = HN_STATUS_ERROR;
    hnSocket->id     = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    hnSocket->type   = HN_PROTOCOL_UDP;
    if(hnSocket->id == INVALID_SOCKET)
        HN_ERROR("Could not create socket");
    else
        hnSocket->status = HN_STATUS_CONNECTED;
    
};

void hnSocketCreateTcp(HnSocket* hnSocket) {
    
    hnSocket->status = HN_STATUS_ERROR;
    hnSocket->id     = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    hnSocket->type   = HN_PROTOCOL_TCP;
    if(hnSocket->id == INVALID_SOCKET)
        HN_ERROR("Could not create socket");
    else
        hnSocket->status = HN_STATUS_CONNECTED;
    
};

void hnSocketCreateClient(HnSocket* socket, const std::string& host, const uint32_t port) {
    
    hnSocketCreate(socket);
    if(socket->status != HN_STATUS_ERROR) {
        struct addrinfo* ip = NULL, *ptr = NULL, hints;
        ZeroMemory(&hints, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        
        int connResult = getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &ip);
        if(connResult != 0) {
            HN_ERROR("Could not resolve server domain!");
            hnSocketDestroy(socket);
        } else {
            ptr = ip;
            connResult = connect(socket->id, ptr->ai_addr, (int)ptr->ai_addrlen);
            if(connResult == SOCKET_ERROR) {
                HN_ERROR("Could not connect to server [" + host + ":" + std::to_string(port) + "]");
                hnSocketDestroy(socket);
            } else {
                // successful connection
                socket->destination.sa_family = ptr->ai_addr->sa_family;
                memcpy(socket->destination.sa_data, ptr->ai_addr->sa_data, 14);
                HN_LOG("Successfully connected to server [" + host + "][" + std::to_string(port) + "]");
            }
        }
        
    }
    
};

void hnSocketCreateServer(HnSocket* socket, const uint32_t port) {
    
    hnSocketCreate(socket);
    if(socket->status != HN_STATUS_ERROR) {
        sockaddr_in hint;
        hint.sin_family = AF_INET;
        hint.sin_port = htons(port);
        hint.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
        
        if(bind(socket->id, (sockaddr*) &hint, sizeof(hint)) == SOCKET_ERROR) {
            HN_ERROR("Could not bind server socket: [" + std::to_string(WSAGetLastError()) + "]");
            hnSocketDestroy(socket);
            socket->status = HN_STATUS_ERROR;
        } else {
            if(socket->type == HN_PROTOCOL_TCP) {
                if(listen(socket->id, SOMAXCONN) != 0) {
                    socket->status = HN_STATUS_ERROR;
                    //HN_ERROR("Port already in use"); ERROR CODE: 10048
                    HN_ERROR("Could not set up server socket: [" + std::to_string(WSAGetLastError()) + "]");
                }
            }
        }
    }
    
};

void hnSocketDestroy(HnSocket* socket) {
    
    closesocket(socket->id);
    socket->id = 0;
    socket->status = HN_STATUS_DISCONNECTED;
    
};


void hnSocketSendDataTcp(HnSocket* socket, const std::string& data) {
    
    int res = send(socket->id, data.c_str(), (int) data.size() + 1, 0);
    if(res <= 0) {
        HN_ERROR("Lost connection to socket");
        socket->status = HN_STATUS_ERROR;
    }
    
};

void hnSocketSendDataUdp(HnSocket* socket, const std::string& data) {
    
    SOCKADDR addr;
    addr.sa_family = socket->destination.sa_family;
    //addr.sa_data = to->sa_data;
    memcpy(addr.sa_data, socket->destination.sa_data, 14);
    int res = sendto(socket->id, data.c_str(), (int) data.size() + 1, 0, &addr, sizeof(addr));
    if(res <= 0) {
        HN_ERROR("Lost connection to socket");
        socket->status = HN_STATUS_ERROR;
    }
    
};

std::string hnSocketReadTcp(HnSocket* socket, char* buffer, const uint32_t maxSize) {
    
    bool customBuffer = (buffer == nullptr);
    if(customBuffer)
        buffer = (char*) malloc(maxSize);
    
    ZeroMemory(buffer, maxSize);
    int bytes = recv(socket->id, buffer, maxSize, 0);
    std::string msg;
    if(bytes > 0)
        msg = std::string(buffer, bytes - 1);
    else {
        socket->status = HN_STATUS_ERROR;
        HN_ERROR("Lost connection to socket");
    }
    
    if(customBuffer)
        free(buffer);
    
    return msg;
    
};

std::string hnSocketReadUdp(HnSocket* socket, char* buffer, const uint32_t maxSize, HnSocketAddress* from) {
    
    bool customBuffer = (buffer == nullptr);
    if(customBuffer)
        buffer = (char*) malloc(maxSize);
    
    std::string msg;
    SOCKADDR fromWs;
    int fromSize = sizeof(fromWs);
    ZeroMemory(buffer, maxSize);
    int bytes = recvfrom(socket->id, buffer, maxSize, 0, &fromWs, &fromSize); 
    if(bytes > 0) {
        msg = std::string(buffer, bytes - 1);
        
        if(from != nullptr) {
            from->sa_family = fromWs.sa_family;
            memcpy(from->sa_data, fromWs.sa_data, 14);
        }
    } else {
        socket->status = HN_STATUS_ERROR;
        HN_ERROR("Lost connection to socket");
    }
    
    if(customBuffer)
        free(buffer);
    
    return msg;
    
};

HnSocketId hnServerAcceptClientTcp(HnSocket* serverSocket) {
    
    HnSocketId socket = accept(serverSocket->id, nullptr, nullptr);
    if (socket == INVALID_SOCKET) {
        HN_ERROR("Connected to invalid socket");
        serverSocket->status = HN_STATUS_ERROR;
        socket = 0;
    }
    
    return socket;
    
};

#endif