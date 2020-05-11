#include "hnClient.h"
#include <iostream>
#include <algorithm>


void hnClientConnect(HnClient* client, std::string const& host, uint32_t const port, HnProtocol const type) {   
    hnNetworkCreate(); // make sure wsa is started
    client->socket.type = type;
    hnSocketCreateClient(&client->socket, host, port);    
};

void hnClientDisconnect(HnClient* client) {    
    hnSocketDestroy(&client->socket);
    client->clients.clear();
    client->variableNames.clear();
    client->variableInfo.clear();    
};

void hnClientUpdateInput(HnClient* client) {    
    std::string msg = hnSocketRead(&client->socket, client->inputBuffer, 4096);
    if(msg.size() > 0) {
		HN_LOG("Msg: " + msg);
        msg = client->lastInput + msg;
        msg.erase(std::remove(msg.begin(), msg.end(), '\0'), msg.end());
        std::vector<HnPacket> packets = hnPacketDecodeAllFromString(msg);
		HN_LOG("Packets: " + std::to_string(packets.size()));
        for (const HnPacket& all : packets)
            hnClientHandlePacket(client, all);
        client->lastInput = msg;
    }    
};

void hnClientUpdateVariables(HnClient* client) {    
	if(client->socket.status == HN_STATUS_CONNECTED) {
		for (auto& map : client->variableInfo) {
			HnVariableInfo* all = &map.second;
			++all->lastSync;
			if (all->lastSync >= all->syncRate && all->data != nullptr) {
				// update variable to server
				HnPacket packet = hnPacketBuild(HN_PACKET_VAR_UPDATE, 
												std::to_string(all->id).c_str(), 
												hnVariableDataToString(all).c_str());
            
				hnSocketSendPacket(&client->socket, packet);

				// reset last sync
				all->lastSync = 0;
			}
		}
	}
};

void hnClientHandlePacket(HnClient* client, HnPacket const& packet) {    
    HN_LOG("Handling packet from server [" + hnPacketGetContent(packet) + "]");
    
    if(packet.type == HN_PACKET_CLIENT_DATA) {
        // general client data
        client->id = std::stoi(packet.arguments[0]);
        HN_DEBUG("Recieved client id [" + std::to_string(client->id) + "]");
        return;
    }
    
    if(packet.type == HN_PACKET_CLIENT_CONNECT) {
        // create new client with given information
        unsigned int id = std::stoi(packet.arguments[0]);
        HnLocalClient* cl = &client->clients[id];
        cl->id = id;
        HN_DEBUG("Connected to local client [" + std::to_string(id) + "]");
        
        HnLocalClientConnectCallback c = client->callbacks.clientConnect;
        if(c != nullptr)
            c(client, cl);
        
        return;
    }
    
    if(packet.type == HN_PACKET_CLIENT_DISCONNECT) {
        // remove local client
        unsigned int id = std::stoi(packet.arguments[0]);
        
        HnLocalClientDisconnectCallback c = client->callbacks.clientDisconnect;
        HnLocalClient* cl = &client->clients[id];
        if(c != nullptr)
            c(client, cl);
        
        client->clients.erase(id);
        HN_DEBUG("Disconnected from local client [" + std::to_string(id) + "]");
        return;
    }
    
    if(packet.type == HN_PACKET_VAR_NEW) {
        // variable request returned, store information in map
        const unsigned int varId    = std::stoi(packet.arguments[0]);
        const std::string name      = packet.arguments[1];
        const HnDataType type       = (HnDataType) std::stoi(packet.arguments[2]);
        const unsigned int syncRate = std::stoi(packet.arguments[3]);
		const unsigned int dataSize = std::stoi(packet.arguments[4]);
		
        client->variableNames[name] = varId;
        HnVariableInfo* varInfo     = &client->variableInfo[varId];
        varInfo->id                 = varId;
        varInfo->syncRate           = syncRate;
        varInfo->type               = type; 
		varInfo->dataSize           = dataSize;
		
        // perhaps a data hook was already created on the local side
        auto it = client->temporaryDataHooks.find(name);
        if(it != client->temporaryDataHooks.end()) {
            varInfo->data = it->second;
            client->temporaryDataHooks.erase(name);
        }
        
        HN_DEBUG("Registered Variable " + name + "(" + std::to_string(type) + ") with id " + std::to_string(varId));
        return;
    }
    
    if(packet.type == HN_PACKET_VAR_UPDATE) {
        const unsigned int cid = std::stoi(packet.arguments[0]);
        if(cid != client->id) {
            const unsigned int vid = std::stoi(packet.arguments[1]);
            const std::string data = packet.arguments[2];
            
            HnLocalClient* cl = &client->clients[cid];
            if(cl->variableData[vid] != nullptr)
                hnVariableParseFromString(cl->variableData[vid], data, client->variableInfo[vid].type, client->variableInfo[vid].dataSize);
        }
        
        return;
    }
    
    if(packet.type == HN_PACKET_PING_CHECK) {
        hnSocketSendPacket(&client->socket, hnPacketBuild(HN_PACKET_PING_CHECK));
        return;
    }
    
    if(packet.type == HN_PACKET_CUSTOM) {
        unsigned int clientId = std::stoi(packet.arguments[0]);
        
        // copy the contents of the original packet but without the client id
        HnPacket fakePacket;
        fakePacket.type = HN_PACKET_CUSTOM;
        
        for(size_t i = 1; i < packet.arguments.size(); ++i)
            fakePacket.arguments.emplace_back(packet.arguments[i]);
        
        if(clientId == client->id) {
            // own custom packet
            client->customPackets.emplace_back(fakePacket);
        } else { 
            // local client
            HnLocalClient* cl = &client->clients[clientId];
            
            HnLocalClientCustomPacketCallback callback = client->callbacks.customPacket;
            if(callback == nullptr || !callback(client, cl, fakePacket))
                cl->customPackets.emplace_back(fakePacket);
        }
        return;
    }
    
    HN_DEBUG("Recieved invalid packet of type [" + std::to_string(packet.type) + "]");    
};

void hnClientCreateVariable(HnClient* client, std::string const& name, HnDataType const type, uint16_t const tickRate, uint32_t const dataSize) {    
    if (client->variableNames.find(name) != client->variableNames.end())
        // variable already registered
        return;
    

	uint32_t size = dataSize;
	if(size == 0) {
		if(type == HN_DATA_TYPE_STRING) {
			HN_ERROR("You need to specify a max char limit for strings (variable [" + name + "])");
			return;
		}

		switch(type) {
		case HN_DATA_TYPE_INT:
			size = sizeof(int32_t);
			break;

		case HN_DATA_TYPE_FLOAT:
			size = sizeof(float);
			break;

		case HN_DATA_TYPE_VEC2:
			size = sizeof(float) * 2;
			break;

		case HN_DATA_TYPE_VEC3:
			size = sizeof(float) * 3;
			break;

		case HN_DATA_TYPE_VEC4:
			size = sizeof(float) * 4;
			break;
		
		}
	}
	
	HnPacket packet = hnPacketBuild(HN_PACKET_VAR_NEW, 
                                    name.c_str(), 
                                    std::to_string(type).c_str(), 
                                    std::to_string(tickRate).c_str(),
									std::to_string(size).c_str());
    hnSocketSendPacket(&client->socket, packet);
};

void hnClientSync(HnClient* client) {    
    HN_LOG("Syncing client...");
    hnSocketSendPacket(&client->socket, hnPacketBuild(HN_PACKET_SYNC_REQUEST));
	
    HnPacket incoming = hnClientReadPacket(client);
    while (client->socket.status == HN_STATUS_CONNECTED && incoming.type != HN_PACKET_SYNC_REQUEST) {
        hnClientHandlePacket(client, incoming);
        incoming = hnClientReadPacket(client);
    }
    
    // sync end
    HN_LOG("Successfully finished client sync");    
};

void hnClientHookVariable(HnClient* client, std::string const& name, void* data) {    
    // this can only work if the variable is already registered locally. The server might not respond that fast
    auto it = client->variableNames.find(name);
    if(it != client->variableNames.end())
        client->variableInfo[it->second].data = data;
    else
        client->temporaryDataHooks[name] = data;    
};

void hnClientUpdateVariable(HnClient* client, std::string const& name) {	
	HnVariableInfo* all = &client->variableInfo[client->variableNames[name]];
	HnPacket packet = hnPacketBuild(HN_PACKET_VAR_UPDATE, 
									std::to_string(all->id).c_str(), 
									hnVariableDataToString(all).c_str());            
	hnSocketSendPacket(&client->socket, packet);
};

void hnLocalClientHookVariable(HnClient* client, HnLocalClient* localclient, std::string const& name, void* data) {
    localclient->variableData[client->variableNames[name]] = data;
};

HnPacket hnClientReadPacket(HnClient* client) {    
    HnPacket packet;
    
    size_t index = client->lastInput.find('!');
    if(index != std::string::npos) {
        // we still have input in the buffer
        packet = hnPacketDecodeFromString(client->lastInput);
    } else {
        // buffer is empty, try to read
        std::string msg = hnSocketReadTcp(&client->socket, client->inputBuffer, 4096); 
        if (msg.size() > 0) {
            msg = client->lastInput + msg;
            packet = hnPacketDecodeFromString(msg);
            client->lastInput = msg;
        }
    }
    
    return packet;    
};

HnPacket hnLocalClientGetCustomPacket(HnLocalClient* client) {    
    if(client->customPackets.size() == 0)
        return HnPacket();
    
    HnPacket packet = client->customPackets[0];
    client->customPackets.erase(client->customPackets.begin());
    return packet;    
};

HnPacket hnClientGetCustomPacket(HnClient* client) {    
    if(client->customPackets.size() == 0)
        return HnPacket();
    
    HnPacket packet = client->customPackets[0];
    client->customPackets.erase(client->customPackets.begin());
    return packet;    
};

HnLocalClient* hnClientGetLocalClientByIndex(HnClient* client, uint16_t const index) {    
    if(index >= client->clients.size())
        return nullptr;
    
    HnLocalClient* localclient = nullptr;
    unsigned int counter = 0;
    for(auto& all : client->clients) {
        if(counter++ == index) {
            localclient = &all.second;
            break;
        }
    }
    
    return localclient;    
};

void* hnClientGetLocalClientVariable(HnClient* client, HnLocalClient* localClient, std::string const& variable) {  
    auto it = client->variableNames.find(variable);
    if(it == client->variableNames.end()) 
        // no variable with that name registered
        return nullptr;
    
    unsigned int id = it->second;
    return localClient->variableData[id];    
};
