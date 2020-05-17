#include "hnClient.h"
#include <windows.h>

void hnClientCreate(HnClient* client, std::string const& host, uint32_t const port, HnProtocolType const protocolType, HnProtocolId const protocolId) {
	client->socket.type       = protocolType;
	client->socket.protocolId = protocolId;
	hnSocketCreateClient(&client->socket, host, port);

	if(client->socket.status != HN_STATUS_ERROR) {
		hnSocketSendPacket(&client->socket, HN_PACKET_SYNC_REQUEST);

		HnPacket response;
		hnSocketReadPacket(&client->socket, &response);
		while(response.type != HN_PACKET_SYNC_REQUEST) {
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

	// handle packet
	hnClientHandlePacket(client, &packet);	
};

void hnClientHandlePacket(HnClient* client, HnPacket* packet) {
	if(packet->type != HN_PACKET_PING_CHECK)
		HN_LOG("Received packet of type [" + std::to_string(packet->type) + "] from server");

	if(client->socket.type == HN_PROTOCOL_UDP) {
		int32_t dif = packet->sequenceId - client->socket.udp.remoteSequenceId;
		if(dif > 0) {
			client->socket.udp.remoteSequenceId = packet->sequenceId;
			//HN_LOG("Server [" + std::to_string(packet->clientId) + "] rsi : " + std::to_string(packet->sequenceId));

			// we received this packet, store local ack
			client->socket.udp.ack          = packet->sequenceId;
			client->socket.udp.ackBitfield  = client->socket.udp.ackBitfield << dif;
			client->socket.udp.ackBitfield |= 0x1;

			// check which packets the server received from us
			HnAcks acks = packet->ackBitfield;
			uint8_t count = (client->socket.udp.ack < 32) ? client->socket.udp.ack : 32;
			for(uint8_t i = 0; i < count; ++i) {
				// check if we have an ack for that packet
				if(!(acks & (0x1 << i))) {
					//HN_LOG("No ack for packet [" + std::to_string(client->socket.udp.sequenceId - i) + "]");
				}
			}
		}

		hnSocketUpdateReliable(&client->socket, packet->ack, packet->ackBitfield);
	}
	
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

		hnSocketSendPacket(&client->socket, HN_PACKET_PING_CHECK);
	} else if(packet->type == HN_PACKET_SYNC_REQUEST) {

		// done syncing, maybe print debug message? We just need this for acks
	} else if(packet->type == HN_PACKET_MESSAGE) {

		std::string msg = hnPacketGetString(packet);
		HN_DEBUG("Received message from server: " + msg);
	}
	else {
		HN_DEBUG("Received invalid packet of type [" + std::to_string(packet->type) + "] from server");
	}
};

void hnClientUpdate(HnClient* client) {
	// check for timeout
	int64_t now = hnPlatformGetCurrentTime();
	if((now - client->socket.lastPacketTime) > client->timeOut) {
		// assume this connection is dead
		HN_ERROR("Connection to server timed out");
		hnClientDestroy(client);
	} else {
		hnPlatformSleep(16);
	}
};
