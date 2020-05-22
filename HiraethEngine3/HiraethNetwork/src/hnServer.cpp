#include "hnServer.h"
#include <cassert>

// -- file scope

void hnClientDisconnect(HnServer* server, HnRemoteClient* client) {
    HnClientId id = client->clientId;
    
    if(client->socket.type == HN_PROTOCOL_TCP) {
        hnSocketDestroy(&client->socket);
        if(client->tcpThread.joinable())   
            client->tcpThread.join();
    }
        
    server->clients.erase(id);
    
    HnPacket packet;
    hnPacketCreate(&packet, HN_PACKET_CLIENT_DISCONNECT, &server->serverSocket);
    hnPacketStoreInt(&packet, id);
    hnServerBroadcastPacketReliable(server, &packet);
};

void hnRemoteClientInputThread(HnServer* server, HnRemoteClient* client) {
    while(client->socket.status == HN_STATUS_CONNECTED) {
        HnPacket packet;
        hnSocketReadPacket(&client->socket, &packet);
        if(client->socket.status == HN_STATUS_CONNECTED) { // check that we succesfully read a packet
            HN_LOG("Read packet of type [" + std::to_string(packet.type) + "]");
            hnRemoteClientHandlePacket(server, client, &packet);
        }
    }
};


// -- global scope

void hnServerCreate(HnServer* server, uint32_t const port, HnProtocolType const protocolType, HnProtocolId const protocolId) {
    server->serverSocket.protocolId = protocolId;
    server->serverSocket.type       = protocolType;
    hnSocketCreateServer(&server->serverSocket, port);
};

void hnServerDestroy(HnServer* server) {
    hnSocketDestroy(&server->serverSocket);
};

void hnServerHandleInput(HnServer* server) {
    if(server->serverSocket.type == HN_PROTOCOL_TCP) {

        HnSocketId socket = hnServerAcceptClient(&server->serverSocket);
        HnClientId id = ++server->clientCounter;
        HN_LOG("Connected to remote client [" + std::to_string(id) + "]");

        // broadcast client connect
        HnPacket connectPacket;
        hnPacketCreate(&connectPacket, HN_PACKET_CLIENT_CONNECT, &server->serverSocket);
        hnPacketStoreInt(&connectPacket, id);
        hnServerBroadcastPacketReliable(server, &connectPacket);
            
        // registering new client
        HnRemoteClient* client        = &server->clients[id];
        client->clientId              = id;
        client->socket.id             = socket;
        client->socket.protocolId     = server->serverSocket.protocolId;
        client->socket.type           = HN_PROTOCOL_TCP;
        client->socket.status         = HN_STATUS_CONNECTED;
        client->tcpThread             = std::thread(&hnRemoteClientInputThread, server, client);        
        client->socket.stats.lastPacketTime = hnPlatformGetCurrentTime();
    } else if(server->serverSocket.type == HN_PROTOCOL_UDP) {

        HnPacket packet;
        HnUdpConnection connection;
        hnSocketReadPacket(&server->serverSocket, &packet, &connection);

        if(connection.status == HN_STATUS_ERROR) {
            HnClientId id = hnServerGetClientIdOfAddress(server, &connection.address);
            if(id > 0) {
                HN_DEBUG("Lost connection to remote client [" + std::to_string(id) + "] (socket close)");
                hnClientDisconnect(server, &server->clients[id]);
            }
            return;
        }
        
        if(packet.protocolId != server->serverSocket.protocolId) {
            HN_DEBUG("Received packet with invalid protocol id [" + std::to_string(packet.protocolId) + "]");
            return;
        }
        
        if(packet.clientId == 0) {
            HnClientId id = ++server->clientCounter;
            HN_DEBUG("Connected to remote client [" + std::to_string(id) + "]");

            // broadcast client connect
            HnPacket connectPacket;
            hnPacketCreate(&connectPacket, HN_PACKET_CLIENT_CONNECT, &server->serverSocket);
            hnPacketStoreInt(&connectPacket, id);
            hnServerBroadcastPacketReliable(server, &connectPacket);
            
            // registering new client
            HnRemoteClient* client        = &server->clients[id];
            client->clientId              = id;
            client->socket.id             = server->serverSocket.id;
            client->socket.protocolId     = server->serverSocket.protocolId;
            client->socket.type           = HN_PROTOCOL_UDP;
            client->socket.status         = HN_STATUS_CONNECTED;
            client->socket.udp            = connection;
            client->socket.udp.clientId   = id;
            client->socket.udp.status     = HN_STATUS_CONNECTED;
            client->socket.stats.lastPacketTime = hnPlatformGetCurrentTime();

            // handle packet
            hnRemoteClientHandlePacket(server, client, &packet);
            
        } else {
            // handle packet
            HnRemoteClient* client = &server->clients[packet.clientId];
            hnRemoteClientHandlePacket(server, client, &packet);
            client->socket.stats.lastPacketTime = hnPlatformGetCurrentTime();
        }
        
    } else
        // Please specify a valid socket protocol
        assert(0);
};

void hnServerUpdate(HnServer* server) {
    // check for disconnects and timeouts
    hnPlatformSleep(16);    

    int64_t nowTime = hnPlatformGetCurrentTime();
    int64_t delta = nowTime - server->lastUpdate;
    
    for(auto& all : server->clients) {
        HnRemoteClient* client = &all.second;

        // check for disconnect
        if(client->socket.status != HN_STATUS_CONNECTED || nowTime - client->socket.stats.lastPacketTime > server->timeOut) {
            HN_DEBUG("Lost connection to remote client [" + std::to_string(client->clientId) + "] (timed out)");
            hnClientDisconnect(server, client);
        } else {
            
            // update client correctly (ping check)
            if(client->socket.stats.pingCheck < 0 && -client->socket.stats.pingCheck > server->pingCheckIntervall) {
                // send ping check
                client->socket.stats.pingCheck = nowTime;
                HnPacket packet;
                hnPacketCreate(&packet, HN_PACKET_PING_CHECK, &client->socket);
                hnPacketStoreInt(&packet, client->socket.stats.ping);
                hnSocketSendPacketReliable(&client->socket, &packet);
            } else {
                client->socket.stats.pingCheck -= delta;
            }
        }       
    }
    
    nowTime = hnPlatformGetCurrentTime();
    server->lastUpdate = nowTime;
};

void hnServerBroadcastPacket(HnServer* server, HnPacket* packet) {
    for(auto& clients : server->clients) {
        //packet->sequenceId = ++clients.second.socket.udp.sequenceId;
        HnPacket clientPacket;
        hnPacketCopy(packet, &clientPacket, &clients.second.socket);
        hnSocketSendPacket(&clients.second.socket, (clients.second.socket.type == HN_PROTOCOL_UDP) ? &clientPacket : nullptr);
    }
};

void hnServerBroadcastPacketReliable(HnServer* server, HnPacket* packet) {
    for(auto& clients : server->clients) {
        //packet->sequenceId = ++clients.second.socket.udp.sequenceId;
        HnPacket clientPacket;
        hnPacketCopy(packet, &clientPacket, &clients.second.socket);
        hnSocketSendPacketReliable(&clients.second.socket, (clients.second.socket.type == HN_PROTOCOL_UDP) ? &clientPacket : nullptr);
    }
};

HnClientId hnServerGetClientIdOfAddress(HnServer const* server, HnUdpAddress const* address) {
    HnClientId id = 0;

    for(auto const& all : server->clients) {
        if(all.second.socket.udp.address == *address) {
            id = all.first;
            break;
        }
    }

    return id;
};


void hnRemoteClientHandlePacket(HnServer* server, HnRemoteClient* client, HnPacket* packet) {
    if(packet->type != HN_PACKET_PING_CHECK && packet->type != HN_PACKET_VAR_UPDATE)
        HN_LOG("Remote client [" + std::to_string(client->clientId) + "] sent packet of type [" + std::to_string(packet->type) + "][" + std::to_string(packet->sequenceId) + "]");

    hnSocketUpdateReliable(&client->socket, packet); // check for reliable packets that werent received

    if(packet->type == HN_PACKET_SYNC_REQUEST) {

        HnPacket idPacket;
        hnPacketCreate(&idPacket, HN_PACKET_CLIENT_DATA, &client->socket);
        hnPacketStoreInt(&idPacket, client->clientId);
        hnSocketSendPacketReliable(&client->socket, &idPacket);

        // existing variables
        for(auto const& all : server->variableNames) {
            HnVariableInfo* var = &server->variables[all.second];
            HnPacket variablePacket;
            hnPacketCreate(&variablePacket, HN_PACKET_VAR_NEW);
            hnPacketStoreString(&variablePacket, all.first);
            hnPacketStoreInt(&variablePacket, all.second);
            hnPacketStoreInt<uint8_t>(&variablePacket, var->type);
            hnPacketStoreInt<uint8_t>(&variablePacket, var->length);
            hnPacketStoreInt<uint8_t>(&variablePacket, var->syncRate);
            hnSocketSendPacketReliable(&client->socket, &variablePacket);
        }

        // existing clients
        for(auto const& all : server->clients) {
            if(all.second.clientId != client->clientId) {
                HnPacket connectPacket;
                hnPacketCreate(&connectPacket, HN_PACKET_CLIENT_CONNECT, &client->socket);
                hnPacketStoreInt(&connectPacket, all.second.clientId);
                hnSocketSendPacketReliable(&client->socket, &connectPacket);
            }
        }
        
        hnSocketSendPacketReliable(&client->socket, HN_PACKET_SYNC_REQUEST);
    } else if(packet->type == HN_PACKET_CLIENT_DISCONNECT) {

        // this client disconnected
        HN_DEBUG("Remote client [" + std::to_string(client->clientId) + "] disconnected");
        hnClientDisconnect(server, client);
    } else if(packet->type == HN_PACKET_PING_CHECK) {

        // ping check returned, messure time
        int64_t now = hnPlatformGetCurrentTime();
        client->socket.stats.ping = (uint16_t) (now - client->socket.stats.pingCheck);
        client->socket.stats.pingCheck = 0;
    } else if(packet->type == HN_PACKET_MESSAGE) {

        std::string msg = hnPacketGetString(packet);
        HN_DEBUG("Received message from client [" + std::to_string(client->clientId) + "]: " + msg);
    } else if(packet->type == HN_PACKET_VAR_NEW) {
        
        std::string name = hnPacketGetString(packet);
        HnVariableInfo* info = nullptr;

        auto it = server->variableNames.find(name);
        if(it == server->variableNames.end()) {
            // new var
            HnVariableId id = ++server->variableCounter;
            server->variableNames[name] = id;

            info = &server->variables[id];
            info->id       = id;
            info->type     = (HnVariableType) hnPacketGetInt<uint8_t>(packet);
            info->length   = hnPacketGetInt<uint8_t>(packet);
            info->syncRate = hnPacketGetInt<uint8_t>(packet);
        } else {
            // var already exists
            info = &server->variables[server->variableNames[name]];
        }

        HnPacket response;
        hnPacketCreate(&response, HN_PACKET_VAR_NEW, &client->socket);
        hnPacketStoreString(&response, name);
        hnPacketStoreInt(&response, info->id);
        hnPacketStoreInt<uint8_t>(&response, info->type);
        hnPacketStoreInt(&response, info->length);
        hnPacketStoreInt(&response, info->syncRate);
        hnSocketSendPacketReliable(&client->socket, &response);
    } else if(packet->type == HN_PACKET_VAR_UPDATE) {

        HnVariableId id = hnPacketGetInt<HnVariableId>(packet);
        HnVariableInfo* info = &server->variables[id];
        b8 reliable = hnPacketGetBool(packet);
        uint32_t size = 1;
        switch(info->type) {
        case HN_VARIABLE_TYPE_CHAR:
            size = 1;
            break;

        case HN_VARIABLE_TYPE_FLOAT:
            size = 4;
            break;

        case HN_VARIABLE_TYPE_INT32:
            size = 4;
            break;
        }

        size *= info->length;
        
        HnPacket response;
        hnPacketCreate(&response, HN_PACKET_VAR_UPDATE, &server->serverSocket);
        hnPacketStoreInt(&response, client->clientId);
        hnPacketStoreInt(&response, id);
        hnPacketStoreData(&response, &packet->data[packet->dataOffset], size);

        if(reliable)
            hnServerBroadcastPacketReliable(server, &response);
        else
            hnServerBroadcastPacket(server, &response);
    }
    else {
        HN_DEBUG("Received invalid packet of type [" + std::to_string(packet->type) + "] from remote client");
    }
};
