#include "hnClient.h"
#include <windows.h>

void hnClientCreate(HnClient* client, std::string const& host, uint32_t const port, HnProtocolType const protocolType, HnProtocolId const protocolId) {
	client->socket.type       = protocolType;
	client->socket.protocolId = protocolId;
	client->socket.timeOut    = client->timeOut;
	hnSocketCreateClient(&client->socket, host, port);

	if(client->socket.status != HN_STATUS_ERROR) {
		hnSocketSendPacket(&client->socket, HN_PACKET_SYNC_REQUEST);

		HnPacket response;
		hnSocketReadPacket(&client->socket, &response);
		while(response.type != HN_PACKET_SYNC_REQUEST) {
			hnClientHandlePacket(client, &response);
			hnSocketReadPacket(&client->socket, &response);
		}

		if(client->socket.status == HN_STATUS_CONNECTED)
			HN_LOG("Successfully connected to server [" + host + "][" + std::to_string(port) + "]");
		else
			HN_ERROR("Could not connect to server [" + host + ":" + std::to_string(port) + "]");

	}
};

void hnClientDestroy(HnClient* client) {
	hnSocketSendPacket(&client->socket, HN_PACKET_CLIENT_DISCONNECT);
	hnSocketDestroy(&client->socket);
};

void hnClientUpdateInput(HnClient* client) {
	HnPacket packet;
	hnSocketReadPacket(&client->socket, &packet);

	// handle packet
	hnClientHandlePacket(client, &packet);	
};

void hnClientHandlePacket(HnClient* client, HnPacket* packet) {
	if(packet->type == HN_PACKET_CLIENT_CONNECT) {

		HnClientId clientId = hnPacketGetInt<HnClientId>(packet);
		HnLocalClient* localClient = &client->clients[clientId];
		localClient->clientId = clientId;
		HN_DEBUG("Connected to local client [" + std::to_string(clientId) + "]");
	} else if(packet->type == HN_PACKET_CLIENT_DISCONNECT) {

		HnClientId clientId = hnPacketGetInt<HnClientId>(packet);
		client->clients.erase(clientId);
		HN_DEBUG("Disconnected from local client [" + std::to_string(clientId) + "]");
	} else if(packet->type == HN_PACKET_CLIENT_DATA) {

		HnClientId clientId = hnPacketGetInt<HnClientId>(packet);
		client->clientId            = clientId;
		client->socket.udp.clientId = clientId;
		HN_DEBUG("Received client id [" + std::to_string(clientId) + "]");
	} else if(packet->type == HN_PACKET_PING_CHECK) {
		//Sleep(20);
		hnSocketSendPacket(&client->socket, HN_PACKET_PING_CHECK);
	} else {
		HN_DEBUG("Received invalid packet of type [" + std::to_string(packet->type) + "] from server");
	}
};
