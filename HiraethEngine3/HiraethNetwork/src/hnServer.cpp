#include "hnServer.h"
#include "hnConnection.h"
#include <iostream>
#include <algorithm>

void hnServerCreate(HnServer* server, const unsigned int port, const HnSocketType type) {
    
    if(server->socket.status) {
        HN_ERROR("Server already started");
        return;
    }
    
    hnNetworkCreate(); // make sure wsa is started
    server->socket.type = type;
    hnSocketCreateServer(&server->socket, port);
    
    server->lastUpdate = std::chrono::high_resolution_clock::now();
    
};

void hnServerDestroy(HnServer* server) {
    
    hnSocketDestroy(&server->socket);
    server->clients.clear();
    server->socket.status = HN_STATUS_DISCONNECTED;
    
};

void hnServerHandleRequests(HnServer* server) {
    
    // check for timeouts
    long long now = hnGetCurrentTime();
    for(auto& all : server->clients) {
        if(all.second.pingCheck > 0) {
            if(now - all.second.pingCheck > server->timeOut) {
                HN_LOG("Connection to RemoteClient [" + std::to_string(all.second.id) + "] timed out");
                server->disconnectRequests.emplace_back(all.first);
                all.second.socket.status = HN_STATUS_DISCONNECTED;
            }
        } else {
            // time since last succesfull ping check is stored as negative value, pingCheckIntervall is unsigned
            if(-all.second.pingCheck > server->pingCheckIntervall)
                hnRemoteClientPingCheck(&all.second);
            else {
                // increase time since last ping check
                all.second.pingCheck -= server->updateTime;
            }
        }
    }
    
    // connecting clients
    while(server->connectionRequests.size() > 0) {
        unsigned int id = ++server->clientCounter;
        
        HN_DEBUG("Connected to RemoteClient [" + std::to_string(id) + "]");
        // send connect packet before new client is on list
        hnServerBroadcastPacket(server, hnPacketBuild(HN_PACKET_CLIENT_CONNECT, std::to_string(id).c_str()));
        
        HnRemoteClient* client = &server->clients[id];
        client->id = id;
        client->socket = HnSocket(server->connectionRequests[0], HN_SOCKET_TYPE_UDP);
        client->thread = std::thread(hnRemoteClientThreadTcp, server, client);
        
        if(server->callbacks.clientConnect != nullptr)
            server->callbacks.clientConnect(server, client);
        
        server->connectionRequests.erase(server->connectionRequests.begin());
    }
    
    // disconnecting clients
    while(server->disconnectRequests.size() > 0) {
        unsigned long long id = server->disconnectRequests[0];
        
        HnRemoteClient* cl = &server->clients[id];
        
        if(server->callbacks.clientDisconnect != nullptr)
            server->callbacks.clientDisconnect(server, cl);
        
        if(cl->thread.joinable())
            cl->thread.detach();
        server->clients.erase(server->clients.find(id));
        cl = nullptr;
        hnServerBroadcastPacket(server, hnPacketBuild(HN_PACKET_CLIENT_DISCONNECT, std::to_string(id).c_str()));
        HN_DEBUG("Disconnected from RemoteClient [" + std::to_string(id) + "]");
        server->disconnectRequests.erase(server->disconnectRequests.begin());
    }
    
    auto nowTime = std::chrono::high_resolution_clock::now();
    server->updateTime = std::chrono::duration_cast<std::chrono::milliseconds>(nowTime - server->lastUpdate).count();
    server->lastUpdate = nowTime;
    
};


void hnServerUpdateTcp(HnServer* server) {
    
    server->connectionRequests.emplace_back(hnServerAcceptClientTcp(&server->socket));
    
};

void hnRemoteClientThreadTcp(HnServer* server, HnRemoteClient* client) {
    
    // input buffer for current message
    char buffer[4096];
    // leftover of last message if it didnt line up with packet ending (start of next packet was sent in last
    // message)
    std::string lastInput;
    
    while(client->socket.status == HN_STATUS_CONNECTED) {
        std::string msg = hnSocketReadTcp(&client->socket, buffer, 4096);
        msg = lastInput + msg;
        msg.erase(std::remove(msg.begin(), msg.end(), '\0'), msg.end());
        
        std::vector<HnPacket> packets = hnPacketDecodeAllFromString(msg);
        for(const HnPacket& all : packets)
            hnRemoteClientHandlePacket(server, client, all);
        lastInput = msg;
    }
    
    HN_DEBUG("Lost connection to remote client [" + std::to_string(client->id) + "]");
    server->disconnectRequests.emplace_back(client->id);
    
};


void hnServerUpdateUdp(HnServer* server) {
    
};


void hnServerBroadcastPacket(HnServer* server, const HnPacket& packet) {
    
    for(auto& all : server->clients)
        hnSocketSendPacket(&all.second.socket, packet);
    
};

void hnRemoteClientHandlePacket(HnServer* server, HnRemoteClient* sender, const HnPacket& packet) {
    
    HN_LOG("Handling remote client (" + std::to_string(sender->id) + ") packet [" + hnPacketGetContent(packet) + "]");
    
    if(packet.type == HN_PACKET_VAR_NEW) {
        // request new variable
        std::string name = packet.arguments[0];
        
        if (server->variableNames.find(name) != server->variableNames.end()) {
            // variable was already registered before
            unsigned int id = server->variableNames[name];
            hnSocketSendPacket(&sender->socket, hnPacketBuild(HN_PACKET_VAR_NEW,
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
            
            hnServerBroadcastPacket(server, hnPacketBuild(HN_PACKET_VAR_NEW, 
                                                          std::to_string(id).c_str(),    // id of the variable
                                                          name.c_str(),                  // name of the variable
                                                          packet.arguments[1].c_str(),   // type
                                                          packet.arguments[2].c_str())); // tick rate
        }
        
        return;
    }
    
    if(packet.type == HN_PACKET_VAR_UPDATE) {
        hnServerBroadcastPacket(server, hnPacketBuild(HN_PACKET_VAR_UPDATE,
                                                      std::to_string(sender->id).c_str(),
                                                      packet.arguments[0].c_str(),
                                                      packet.arguments[1].c_str()));
        return;
    }
    
    if (packet.type == HN_PACKET_SYNC_REQUEST) {
        // sync client, send all important data
        hnSocketSendPacket(&sender->socket, hnPacketBuild(HN_PACKET_CLIENT_DATA, std::to_string(sender->id).c_str()));
        
        // first send vars then clients so we can hook vars to clients
        
        for (const auto& all : server->variableNames) {
            const HnVariableInfo* info = &server->variableInfo[all.second];
            hnSocketSendPacket(&sender->socket, hnPacketBuild(HN_PACKET_VAR_NEW, 
                                                              std::to_string(all.second).c_str(),       // id
                                                              all.first.c_str(),                        // name
                                                              std::to_string(info->type).c_str(),       // type
                                                              std::to_string(info->syncRate).c_str())); // sync rate 
        }
        
        for (const auto& all : server->clients)
            if (&all.second != sender)
				hnSocketSendPacket(&sender->socket, hnPacketBuild(HN_PACKET_CLIENT_CONNECT, std::to_string(all.first).c_str()));
        
        // finish sync
        hnSocketSendPacket(&sender->socket, hnPacketBuild(HN_PACKET_SYNC_REQUEST));
        HN_LOG("Synced with client");
        return;
    }
    
    if(packet.type == HN_PACKET_PING_CHECK) {
        long long now = hnGetCurrentTime();
        sender->ping = (unsigned int) (now - sender->pingCheck);
        sender->pingCheck = 0;
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
            hnServerBroadcastPacket(server, newPacket);
        }
        
        sender->customPackets.emplace_back(packet);
        return;
        
    }
    
    HN_DEBUG("Recieved invalid packet: " + hnPacketGetContent(packet));
    
};

void hnRemoteClientHookVariable(HnServer* server, HnRemoteClient* client, const std::string& variable, void* ptr) {
    
};

void hnRemoteClientPingCheck(HnRemoteClient* client) {
    
    client->pingCheck = hnGetCurrentTime();
    hnSocketSendPacket(&client->socket, hnPacketBuild(HN_PACKET_PING_CHECK));
    
};

HnPacket hnRemoteClientGetCustomPacket(HnRemoteClient* client) {
    
    if(client->customPackets.size() == 0)
        return HnPacket();
    
    HnPacket packet = client->customPackets[0];
    client->customPackets.erase(client->customPackets.begin());
    return packet;
    
};
