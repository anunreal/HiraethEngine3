#pragma once

#ifdef HN_USE_WINSOCK

#include "hnConnection.h"
#include <ws2tcpip.h>
#include <cassert>
#include <chrono>
#include <thread>
#pragma comment(lib, "Ws2_32.lib")


// -- file scope

bool wsaSetup = false;
std::chrono::high_resolution_clock::time_point startTime;

void hnWsaSetup() {    
    if(!wsaSetup) {
        // set up wsa
        WSAData wsData;
        WORD ver = MAKEWORD(2, 2);
        
        if(WSAStartup(ver, &wsData) != 0) {
            HN_ERROR("Could not start wsa, exiting...");
            return;
        }

        wsaSetup = true;
        startTime = std::chrono::high_resolution_clock::now();
    }
};

void hnWsaDestroy() {    
    if(wsaSetup) {
        WSACleanup();
        wsaSetup = false;
    }
};

void hnSocketCreateUdp(HnSocket* hnSocket) {    
    hnWsaSetup();

    hnSocket->status = HN_STATUS_ERROR;
    hnSocket->id     = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    hnSocket->type   = HN_PROTOCOL_UDP;
    if(hnSocket->id == INVALID_SOCKET)
        HN_ERROR("Could not create socket");
    else
        hnSocket->status = HN_STATUS_CONNECTED;    
};

void hnSocketCreateTcp(HnSocket* hnSocket) {    
    hnWsaSetup();

    hnSocket->status = HN_STATUS_ERROR;
    hnSocket->id     = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    hnSocket->type   = HN_PROTOCOL_TCP;
    if(hnSocket->id == INVALID_SOCKET)
        HN_ERROR("Could not create socket");
    else
        hnSocket->status = HN_STATUS_CONNECTED;    
};


// -- global scope

void hnSocketCreateClient(HnSocket* socket, std::string const& host, uint32_t const port) {    
    if(socket->type == HN_PROTOCOL_UDP)
        hnSocketCreateUdp(socket);
    else if(socket->type == HN_PROTOCOL_TCP)
        hnSocketCreateTcp(socket);
    else
        // Please specify a valid socket protocol
        assert(0);

    if(socket->status != HN_STATUS_ERROR) {
        struct addrinfo* ip = NULL, *ptr = NULL, hints;
        ZeroMemory(&hints, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        
        int32_t connResult = getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &ip);
        if(connResult != 0) {
            HN_ERROR("Could not resolve server domain!");
            hnSocketDestroy(socket);
            socket->status = HN_STATUS_ERROR;
        } else {
            ptr = ip;
            connResult = connect(socket->id, ptr->ai_addr, (int)ptr->ai_addrlen);
            if(connResult == SOCKET_ERROR) {
                HN_ERROR("Could not connect to server [" + host + ":" + std::to_string(port) + "]");
                hnSocketDestroy(socket);
                socket->status = HN_STATUS_ERROR;
            } else {
                // successful connection
                socket->status = HN_STATUS_CONNECTED;
                socket->udp.address.sa_family = ptr->ai_addr->sa_family;
                memcpy(socket->udp.address.sa_data, ptr->ai_addr->sa_data, 14);
            }
        }   
    }    
};

void hnSocketCreateServer(HnSocket* socket, uint32_t const port) {
    if(socket->type == HN_PROTOCOL_UDP)
        hnSocketCreateUdp(socket);
    else if(socket->type == HN_PROTOCOL_TCP)
        hnSocketCreateTcp(socket);
    else
        // Please specify a valid socket protocol
        assert(0);
    
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
                    HN_ERROR("Could not set up server socket: [" + std::to_string(WSAGetLastError()) + "]");
                } else
                    HN_LOG("Successfully set up server");
            } else
                HN_LOG("Successfully set up server");
        }
    }
};

void hnSocketDestroy(HnSocket* socket) {    
    closesocket(socket->id);
    socket->id     = 0;
    socket->status = HN_STATUS_CLOSED;    
};

void hnSocketSendData(HnSocket* socket, char const* data, uint32_t const dataSize) {
    if(socket->status != HN_STATUS_CONNECTED)
        return;
    
    if(socket->type == HN_PROTOCOL_TCP) {
        int32_t res = send(socket->id, data, dataSize, 0);
        if(res <= 0) {
            HN_LOG("Lost connection to socket while writing");
            socket->status = HN_STATUS_ERROR;
        }
    } else if(socket->type == HN_PROTOCOL_UDP) {
        SOCKADDR addr;
        addr.sa_family = socket->udp.address.sa_family;
        memcpy(addr.sa_data, socket->udp.address.sa_data, 14);
        int32_t res = sendto(socket->id, data, dataSize, 0, &addr, sizeof(addr));
        if(res <= 0) {
            HN_LOG("Lost connection to socket while writing");
            socket->status = HN_STATUS_ERROR;
        }    
    } else
        // Please specify a valid socket protocol
        assert(0);
};

void hnSocketReadData(HnSocket* socket, HnUdpConnection* connection) {
    if(socket->status != HN_STATUS_CONNECTED)
        return;
    
    socket->buffer.offset = 0;
    memset(socket->buffer.buffer, 0, HN_BUFFER_SIZE);

    if(socket->type == HN_PROTOCOL_TCP) {
        int32_t bytes = recv(socket->id, socket->buffer.buffer, HN_BUFFER_SIZE, 0);
        if(bytes <= 0) {
            HN_LOG("Lost connection to socket while reading");
            socket->status = HN_STATUS_ERROR;
            socket->buffer.currentSize = 0;
        } else {
            socket->buffer.currentSize = (uint32_t) bytes;
            socket->stats.lastPacketTime = hnPlatformGetCurrentTime();
        }
    } else if(socket->type == HN_PROTOCOL_UDP) {
        SOCKADDR fromWs;
        int32_t fromSize = sizeof(fromWs);
        int32_t bytes = recvfrom(socket->id, socket->buffer.buffer, HN_BUFFER_SIZE, 0, &fromWs, &fromSize);
        if(bytes <= 0) {
            HN_LOG("Lost connection to socket while reading (error [" + std::to_string(WSAGetLastError()) + "])");
            //socket->status = HN_STATUS_ERROR; 
            if(connection) {
                connection->status = HN_STATUS_ERROR;
                connection->address.sa_family = fromWs.sa_family;
                memcpy(connection->address.sa_data, fromWs.sa_data, 14);
            }
                
            socket->buffer.currentSize = 0;
        } else {
            socket->buffer.currentSize = bytes;
            socket->stats.lastPacketTime = hnPlatformGetCurrentTime();        

            if(connection) {
                connection->status = HN_STATUS_CONNECTED;
                connection->address.sa_family = fromWs.sa_family;
                memcpy(connection->address.sa_data, fromWs.sa_data, 14);
            }
        }

    } else
        // Please specify a valid socket protocol
        assert(0);
};

HnSocketId hnServerAcceptClient(HnSocket* serverSocket) {    
    HnSocketId socket = accept(serverSocket->id, nullptr, nullptr);
    if (socket == INVALID_SOCKET) {
        HN_ERROR("Connected to invalid socket");
        serverSocket->status = HN_STATUS_ERROR;
        socket = 0;
    }
    
    return socket;    
};


int64_t hnPlatformGetCurrentTime() {
    auto now  = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count();
};

void hnPlatformSleep(uint32_t const time) {
    std::chrono::milliseconds duration(time);
    std::this_thread::sleep_for(duration);
};

#endif
