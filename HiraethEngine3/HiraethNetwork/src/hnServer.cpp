#include "hnServer.h"
#include <cassert>

void hnServerCreate(HnServer* server, uint32_t const port, HnProtocolType const protocolType, HnProtocolId const protocolId) {
	server->serverSocket.protocolId = protocolId;
	server->serverSocket.type       = protocolType;
	hnSocketCreateServer(&server->serverSocket, port);
};

void hnServerDestroy(HnServer* server) {
	hnSocketDestroy(&server->serverSocket);
};


void hnServerUpdate(HnServer* server) {
	if(server->serverSocket.type == HN_PROTOCOL_TCP) {
		
	} else if(server->serverSocket.type == HN_PROTOCOL_UDP) {

		HnPacket packet;
		HnUdpConnection connection;
		hnSocketReadPacket(&server->serverSocket, &packet, &connection);

		if(packet.protocolId != server->serverSocket.protocolId) {
			HN_DEBUG("Received packet with invalid protocol id [" + std::to_string(packet.protocolId) + "]");
			return;
		}

		HN_LOG("Recieved packet");
		
		if(packet.clientId == 0) {
			HnClientId id = ++server->clientCounter;
			HN_LOG("Connected to remote client [" + std::to_string(id) + "]");

			// broadcast client connect
			HnPacket connectPacket;
			hnPacketCreate(&connectPacket, HN_PACKET_CLIENT_CONNECT, &server->serverSocket);
			hnPacketStoreInt(&connectPacket, id);
			hnServerBroadcastPacket(server, &connectPacket);
			
			// registering new client
			HnRemoteClient* client = &server->clients[id];
			client->clientId            = id;
			client->socket.id           = server->serverSocket.id;
			client->socket.protocolId   = server->serverSocket.protocolId;
			client->socket.type         = HN_PROTOCOL_UDP;
			client->socket.status       = HN_STATUS_CONNECTED;
			client->socket.udp          = connection;
			client->socket.udp.clientId = id;
			
			// handle packet
			hnRemoteClientHandlePacket(client, &packet);
			
		} else {
			// handle packet
			HnRemoteClient* client = &server->clients[packet.clientId];
			hnRemoteClientHandlePacket(client, &packet);
		}
		
	} else
		// Please specify a valid socket protocol
		assert(0);
};

void hnServerBroadcastPacket(HnServer* server, HnPacket* packet) {
	for(auto& clients : server->clients) {
		packet->sequenceId = clients.second.socket.udp.sequenceId++;
		hnSocketSendPacket(&clients.second.socket, packet);
	}
};


void hnRemoteClientHandlePacket(HnRemoteClient* client, HnPacket* packet) {

};
