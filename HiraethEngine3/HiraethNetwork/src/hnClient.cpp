#include "hnClient.h"
#include <windows.h>

void hnClientCreate(HnClient* client, std::string const& host, uint32_t const port, HnProtocolType const protocolType, HnProtocolId const protocolId) {
    client->socket.type       = protocolType;
    client->socket.protocolId = protocolId;
    hnSocketCreateClient(&client->socket, host, port);

    hnSocketSendPacketReliable(&client->socket, HN_PACKET_SYNC_REQUEST);

    if(client->socket.status == HN_STATUS_CONNECTED) {
        HnPacket response;
        hnSocketReadPacket(&client->socket, &response);
        while(client->socket.status == HN_STATUS_CONNECTED && response.type != HN_PACKET_SYNC_REQUEST) {
            hnClientHandlePacket(client, &response);
            hnSocketReadPacket(&client->socket, &response);
        }

        hnClientHandlePacket(client, &response); // handle this packet for acks
            
        if(client->socket.status == HN_STATUS_CONNECTED)
            HN_LOG("Successfully connected to server [" + host + "][" + std::to_string(port) + "]");
        else
            HN_ERROR("Could not connect to server [" + host + ":" + std::to_string(port) + "]");
    }
};

void hnClientDestroy(HnClient* client) {
    if(client->socket.status == HN_STATUS_CONNECTED)
        hnSocketSendPacket(&client->socket, HN_PACKET_CLIENT_DISCONNECT);
    hnSocketDestroy(&client->socket);
    client->clients.clear();
    client->clientId = 0;
};

void hnClientHandleInput(HnClient* client) {
    HnPacket packet;
    hnSocketReadPacket(&client->socket, &packet);

    if(client->socket.status == HN_STATUS_CONNECTED) {
        // handle packet
        hnClientHandlePacket(client, &packet);
    }
};

void hnClientHandlePacket(HnClient* client, HnPacket* packet) {
    if(packet->type != HN_PACKET_PING_CHECK && packet->type != HN_PACKET_VAR_UPDATE)
        HN_LOG("Server sent packet of type [" + std::to_string(packet->type) + "][" + std::to_string(packet->sequenceId) + "]");

    hnSocketUpdateReliable(&client->socket, packet); // check for reliable packets that werent received
    
    if(packet->type == HN_PACKET_CLIENT_CONNECT) {

        HnClientId clientId = hnPacketGetInt<HnClientId>(packet);
        HnLocalClient* localClient = &client->clients[clientId];
        localClient->clientId = clientId;
        HN_DEBUG("Connected to local client [" + std::to_string(clientId) + "]");

        if(client->callbacks.clientConnect)
            client->callbacks.clientConnect(client, localClient);
    } else if(packet->type == HN_PACKET_CLIENT_DISCONNECT) {

        HnClientId clientId = hnPacketGetInt<HnClientId>(packet);

        HN_DEBUG("Disconnected from local client [" + std::to_string(clientId) + "]");

        if(client->callbacks.clientDisconnect)
            client->callbacks.clientDisconnect(client, &client->clients[clientId]);

        client->clients.erase(clientId);
    } else if(packet->type == HN_PACKET_CLIENT_DATA) {

        HnClientId clientId = hnPacketGetInt<HnClientId>(packet);
        client->clientId            = clientId;
        client->socket.udp.clientId = clientId;
        HN_DEBUG("Received client id [" + std::to_string(clientId) + "]");
    } else if(packet->type == HN_PACKET_PING_CHECK) {

        uint16_t ping = hnPacketGetInt<uint16_t>(packet);
        client->socket.stats.ping = ping;
        hnSocketSendPacketReliable(&client->socket, HN_PACKET_PING_CHECK);
    } else if(packet->type == HN_PACKET_SYNC_REQUEST) {

        // done syncing, maybe print debug message? We just need this for acks
    } else if(packet->type == HN_PACKET_MESSAGE) {

        std::string msg = hnPacketGetString(packet);
        HN_DEBUG("Received message from server: " + msg);
    } else if(packet->type == HN_PACKET_VAR_NEW) {

        std::string name = hnPacketGetString(packet);
        HnVariableId id  = hnPacketGetInt<HnVariableId>(packet);
        client->variableNames[name] = id;
        HnVariableInfo* info = &client->variables[id];
        info->id       = id;
        info->type     = (HnVariableType) hnPacketGetInt<uint8_t>(packet);
        info->length   = hnPacketGetInt<uint8_t>(packet);
        info->syncRate = hnPacketGetInt<uint8_t>(packet);
        HN_DEBUG("Registered variable [" + name + "] with id [" + std::to_string(id) + "]");

        auto it = client->variableTemporaryHooks.find(name);
        if(it != client->variableTemporaryHooks.end()) {
            client->variableData[id] = it->second;
            client->variableTemporaryHooks.erase(name);
        }
    } else if(packet->type == HN_PACKET_VAR_UPDATE) {

        HnClientId   cl  = hnPacketGetInt<HnClientId>(packet);
        HnVariableId var = hnPacketGetInt<HnVariableId>(packet);
        if(cl != client->clientId) {
            HnLocalClient* local = &client->clients[cl];
            if(local->variableData[var])
                hnPacketGetVariable(packet, &client->variables[var], local->variableData[var]);            
        }
    }
    else if(packet->type != 0) { // type zero means lost connection
        HN_DEBUG("Received invalid packet of type [" + std::to_string(packet->type) + "] from server");
    }
};

void hnClientUpdate(HnClient* client) {
    // check for timeout
    int64_t now = hnPlatformGetCurrentTime();
    if((now - client->socket.stats.lastPacketTime) > client->timeOut) {
        // assume this connection is dead
        HN_ERROR("Connection to server timed out");
        hnClientDestroy(client);
    } else {
        
        // determine which variables to update
        for(auto const& all : client->variables) {
            if(all.second.syncRate && client->tick % all.second.syncRate == 0)
                hnClientSendVariable(client, all.first);
        }
        
        client->tick++;
        //hnPlatformSleep(16);
    }
};


void hnClientCreateVariableFixed(HnClient* client, std::string const& name, HnVariableType const type, uint8_t length, uint8_t syncRate) {
    // send request to server
    HnPacket packet;
    hnPacketCreate(&packet, HN_PACKET_VAR_NEW, &client->socket);
    hnPacketStoreString(&packet, name);
    hnPacketStoreInt<uint8_t>(&packet, type);
    hnPacketStoreInt(&packet, length);
    hnPacketStoreInt(&packet, syncRate);
    hnSocketSendPacketReliable(&client->socket, &packet);
};

void hnClientHookVariable(HnClient* client, std::string const& name, void* hook) {
    auto it = client->variableNames.find(name);
    if(it == client->variableNames.end()) {
        // variable wasnt registered yet, store hook somewhere else
        client->variableTemporaryHooks[name] = hook;
    } else {
        client->variableData[it->second] = hook;
    }
};

void hnClientSendVariable(HnClient* client, HnVariableId const var) {
    HnVariableInfo* info = &client->variables[var];
    void* data = client->variableData[var];
    if(data == nullptr)
        return;
     
    HnPacket packet;
    hnPacketCreate(&packet, HN_PACKET_VAR_UPDATE, &client->socket);
    hnPacketStoreInt(&packet, var); // var id
    hnPacketStoreBool(&packet, 0);   // unreliable
    hnPacketStoreVariable(&packet, info, data); // actual data
    hnSocketSendPacket(&client->socket, &packet);
};

void hnClientSendVariableReliable(HnClient* client, HnVariableId const var) {
    HnVariableInfo* info = &client->variables[var];
    void* data = client->variableData[var];
    if(data == nullptr)
        return;
     
    HnPacket packet;
    hnPacketCreate(&packet, HN_PACKET_VAR_UPDATE, &client->socket);
    hnPacketStoreInt(&packet, var); // var id
    hnPacketStoreBool(&packet, 1);   // reliable
    hnPacketStoreVariable(&packet, info, data); // actual data
    hnSocketSendPacketReliable(&client->socket, &packet);
};


void hnLocalClientHookVariable(HnClient* client, HnLocalClient* local, std::string const& varName, void* hook) {
    HnVariableId id = client->variableNames[varName];
    local->variableData[id] = hook;
};
