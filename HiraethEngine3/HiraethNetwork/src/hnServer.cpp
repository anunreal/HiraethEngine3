#include "hnServer.h"
#include "hnConnection.h"
#include <ws2tcpip.h>
#include <iostream>

void hnCreateServer(HnServer* server, const unsigned int port) {
    
    if(server->serverSocket.status) {
        HN_ERROR("Server already started");
        return;
    }
    
    hnCreateNetwork(); // make sure wsa is started
    
    server->serverSocket.id = socket(AF_INET, SOCK_STREAM, 0);
    if(server->serverSocket.id == INVALID_SOCKET) {
        server->serverSocket.status = HN_STATUS_ERROR;
        HN_ERROR("Could not create socket");
        return;
    }
    
    sockaddr_in hint;
    hint.sin_family = AF_INET;
    hint.sin_port = htons(port);
    hint.sin_addr.S_un.S_addr = INADDR_ANY;
    
    bind(server->serverSocket.id, (sockaddr*) &hint, sizeof(hint));
    
    if(listen(server->serverSocket.id, SOMAXCONN) != 0) {
        server->serverSocket.status = HN_STATUS_ERROR;
        HN_ERROR("Port already in use");
        return;
    }
    
    server->serverSocket.status = HN_STATUS_CONNECTED;
    
}

void hnDestroyServer(HnServer* server) {
    
    for(auto& all : server->clients)
        hnKickRemoteClient(&all.second);
    
    if(server->serverSocket.id != 0)
        closesocket(server->serverSocket.id);
    server->clients.clear();
    server->serverSocket.status = HN_STATUS_DISCONNECTED;
    
}

void hnKickRemoteClient(HnRemoteClient* client) {
    
    hnSendSocketData(&client->socket, "DIS");
    client->socket.status = HN_STATUS_DISCONNECTED;
    client->thread.detach();
    closesocket(client->socket.id);
    
}

void hnServerAcceptClients(HnServer* server) {
    
    unsigned long long socket = accept(server->serverSocket.id, nullptr, nullptr);
    if (socket == INVALID_SOCKET) {
        HN_ERROR("Connected to invalid socket");
        return;
    }
    
    server->connectionRequests.emplace_back(socket);
    
}

void hnUpdateServer(HnServer* server) {
    
    // connecting clients
    while(server->connectionRequests.size() > 0) {
        unsigned int id = ++server->clientCounter;
        
        // send connect packet before new client is on list
        hnBroadcastPacket(server, hnBuildPacket(HN_PACKET_CLIENT_CONNECT, std::to_string(id).c_str()));
        
        HnRemoteClient* client = &server->clients[id];
        client->id = id;
        client->socket = HnSocket(server->connectionRequests[0]);
        client->thread = std::thread(hnRemoteClientThread, server, client);
        HN_DEBUG("Connected to RemoteClient [" + std::to_string(id) + "]");
        server->connectionRequests.erase(server->connectionRequests.begin());
    }
    
    while(server->disconnectRequests.size() > 0) {
        unsigned long long id = server->disconnectRequests[0];
        
        HnRemoteClient* cl = &server->clients[id];
        cl->thread.detach();
        server->clients.erase(server->clients.find(id));
        cl = nullptr;
        hnBroadcastPacket(server, hnBuildPacket(HN_PACKET_CLIENT_DISCONNECT, std::to_string(id).c_str()));
        HN_DEBUG("Disconnected from RemoteClient [" + std::to_string(id) + "]");
        server->disconnectRequests.erase(server->disconnectRequests.begin());
    }
    
}

void hnRemoteClientThread(HnServer* server, HnRemoteClient* client) {
    
    // input buffer for current message
    char buffer[4096];
    // leftover of last message if it didnt line up with packet ending (start of next packet was sent in last
    // message)
    std::string lastInput;
    
    while(client->socket.status == HN_STATUS_CONNECTED) {
        ZeroMemory(buffer, 4096);
        size_t bytes = (size_t) recv(client->socket.id, buffer, 4096, 0);
        
        if(bytes > 0) {
            // input
            std::string msg = std::string(buffer, bytes - 1);
            
            HN_LOG("Read from client [" + msg + "]");
            
            msg = lastInput + msg;
            std::vector<HnPacket> packets = hnDecodePackets(msg);
            for(const HnPacket& all : packets)
                hnHandleServerPacket(server, client, all);
            lastInput = msg;
        } else {
            // lost connection
            client->socket.status = HN_STATUS_DISCONNECTED;
        }
    }
    
    HN_DEBUG("Lost connection to remote client [" + std::to_string(client->id) + "]");
    server->disconnectRequests.emplace_back(client->id);
    
}

void hnHandleServerPacket(HnServer* server, HnRemoteClient* sender, const HnPacket& packet) {
    
    if(packet.type == HN_PACKET_VAR_NEW) {
        // request new variable
        std::string name = packet.arguments[0];
        
        if (server->variableNames.find(name) != server->variableNames.end()) {
            // variable was already registered before
            unsigned int id = server->variableNames[name];
            hnSendPacket(&sender->socket, hnBuildPacket(HN_PACKET_VAR_NEW,
                                                        std::to_string(id).c_str(), 
                                                        name.c_str(),
                                                        packet.arguments[1].c_str(),
                                                        std::to_string(server->variableInfo[id].syncRate).c_str()));
        } else {
            // new variable
            unsigned int id = ++server->variableCounter;
            server->variableNames[name] = id;
            
            HnVariableInfo* info = &server->variableInfo[id];
            info->syncRate       = std::stoi(packet.arguments[2]);
            info->type           = (HnDataType) std::stoi(packet.arguments[1]);
            
            hnBroadcastPacket(server, hnBuildPacket(HN_PACKET_VAR_NEW, 
                                                    std::to_string(id).c_str(),    // id of the variable
                                                    name.c_str(),                  // name of the variable
                                                    packet.arguments[1].c_str(),   // type
                                                    packet.arguments[2].c_str())); // tick rate
        }
        
        return;
    }
    
    if(packet.type == HN_PACKET_VAR_UPDATE) {
        hnBroadcastPacket(server, hnBuildPacket(HN_PACKET_VAR_UPDATE,
                                                std::to_string(sender->id).c_str(),
                                                packet.arguments[0].c_str(),
                                                packet.arguments[1].c_str()));
        return;
    }
    
    if (packet.type == HN_PACKET_SYNC_REQUEST) {
        // sync client, send all important data
        hnSendPacket(&sender->socket, hnBuildPacket(HN_PACKET_CLIENT_DATA, std::to_string(sender->id).c_str()));
        
        // first send vars then clients so we can hook vars to clients
        
        for (const auto& all : server->variableNames) {
            const HnVariableInfo* info = &server->variableInfo[all.second];
            hnSendPacket(&sender->socket, hnBuildPacket(HN_PACKET_VAR_NEW, 
                                                        std::to_string(all.second).c_str(),       // id
                                                        all.first.c_str(),                        // name
                                                        std::to_string(info->type).c_str(),       // type
                                                        std::to_string(info->syncRate).c_str())); // sync rate
            
            for (const auto& all : server->clients)
                if (&all.second != sender)
                hnSendPacket(&sender->socket, hnBuildPacket(HN_PACKET_CLIENT_CONNECT, std::to_string(all.first).c_str()));
            
            
        }
        
        // finish sync
        hnSendPacket(&sender->socket, hnBuildPacket(HN_PACKET_SYNC_REQUEST));
        HN_LOG("Synced with client");
        return;
    }
    
    if(packet.type == HN_PACKET_CUSTOM) {
        
        // recieved custom packet from client
        bool isPrivate = (packet.arguments[0][0] == '1');
        if(!isPrivate) {
            // broadcast packet
            HnPacket newPacket;
            newPacket.type = HN_PACKET_CUSTOM;
            newPacket.arguments.emplace_back(std::to_string(sender->id));
            for(size_t i = 1; i < packet.arguments.size(); ++i)
                newPacket.arguments.emplace_back(packet.arguments[i]);
            hnBroadcastPacket(server, newPacket);
        }
        
        sender->customPackets.emplace_back(packet);
        
        return;
        
    }
    
    HN_DEBUG("Recieved invalid packet: " + hnGetPacketContent(packet));
    
}

void hnBroadcastPacket(HnServer* server, const HnPacket& packet) {
    
    for(auto& all : server->clients)
        hnSendPacket(&all.second.socket, packet);
    
}

HnPacket hnGetCustomPacket(HnRemoteClient* client) {
    
    if(client->customPackets.size() == 0)
        return HnPacket();
    
    HnPacket packet = client->customPackets[0];
    client->customPackets.erase(client->customPackets.begin());
    return packet;
    
}

void hnHookVariable(HnServer* server, HnRemoteClient* client, const std::string& variable, void* ptr) {
    
}