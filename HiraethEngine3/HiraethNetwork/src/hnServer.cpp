#include "hnServer.h"
#include <cassert>

// -- file scope

void hnClientDisconnect(HnServer* server, HnRemoteClient* client) {
	HnClientId id = client->clientId;

	if(client->socket.type == HN_PROTOCOL_TCP) {
		hnSocketDestroy(&client->socket);
		client->tcpThread.join();
	}
	
	server->clients.erase(id);
	
	HnPacket packet;
	hnPacketCreate(&packet, HN_PACKET_CLIENT_DISCONNECT, &server->serverSocket);
	hnPacketStoreInt(&packet, id);
	hnServerBroadcastPacket(server, &packet);
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
		hnServerBroadcastPacket(server, &connectPacket);
			
		// registering new client
		HnRemoteClient* client        = &server->clients[id];
		client->clientId              = id;
		client->socket.id             = socket;
		client->socket.protocolId     = server->serverSocket.protocolId;
		client->socket.type           = HN_PROTOCOL_TCP;
		client->socket.status         = HN_STATUS_CONNECTED;
		client->tcpThread             = std::thread(&hnRemoteClientInputThread, server, client);		
		client->socket.lastPacketTime = hnPlatformGetCurrentTime();
	} else if(server->serverSocket.type == HN_PROTOCOL_UDP) {

		HnPacket packet;
		HnUdpConnection connection;
		hnSocketReadPacket(&server->serverSocket, &packet, &connection);

		if(connection.status == HN_STATUS_ERROR)
			return;
		
		if(packet.protocolId != server->serverSocket.protocolId) {
			HN_DEBUG("Received packet with invalid protocol id [" + std::to_string(packet.protocolId) + "]");
			return;
		}
		
		if(packet.clientId == 0) {
			HnClientId id = ++server->clientCounter;
			HN_LOG("Connected to remote client [" + std::to_string(id) + "]");

			// broadcast client connect
			HnPacket connectPacket;
			hnPacketCreate(&connectPacket, HN_PACKET_CLIENT_CONNECT, &server->serverSocket);
			hnPacketStoreInt(&connectPacket, id);
			hnServerBroadcastPacket(server, &connectPacket);
			
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
			client->socket.lastPacketTime = hnPlatformGetCurrentTime();
			
			// handle packet
			hnRemoteClientHandlePacket(server, client, &packet);
			
		} else {
			// handle packet
			HnRemoteClient* client = &server->clients[packet.clientId];
			hnRemoteClientHandlePacket(server, client, &packet);
			client->socket.lastPacketTime = hnPlatformGetCurrentTime();
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
		if(client->socket.status != HN_STATUS_CONNECTED || nowTime - client->socket.lastPacketTime > server->timeOut) {
			HN_LOG("Lost connection to remote client [" + std::to_string(client->clientId) + "]");
			hnClientDisconnect(server, client);
		} else {
			
			// update client correctly (ping check)
			if(client->socket.pingCheck < 0 && -client->socket.pingCheck > server->pingCheckIntervall) {
				// send ping check
				//client->socket.pingCheck = nowTime;
				hnSocketSendPacket(&client->socket, HN_PACKET_PING_CHECK);
			} else {
				client->socket.pingCheck -= delta;
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


void hnRemoteClientHandlePacket(HnServer* server, HnRemoteClient* client, HnPacket* packet) {
	if(packet->type != HN_PACKET_PING_CHECK)
		HN_LOG("Remote client [" + std::to_string(client->clientId) + "] sent packet of type [" + std::to_string(packet->type) + "][" + std::to_string(packet->sequenceId) + "]");

	if(client->socket.type == HN_PROTOCOL_UDP) {
		int32_t dif = packet->sequenceId - client->socket.udp.remoteSequenceId;
		if(dif > 0) {
			if(dif > 1)
				HN_LOG("Remote client dropped a packet");
			
			client->socket.udp.remoteSequenceId = packet->sequenceId;
			//HN_LOG("Client [" + std::to_string(packet->clientId) + "] rsi : " + std::to_string(packet->sequenceId));

			// we received this packet, store local ack
			client->socket.udp.ack          = packet->sequenceId;
			client->socket.udp.ackBitfield  = client->socket.udp.ackBitfield << dif;
			client->socket.udp.ackBitfield |= 0x1;
			
			// check which packets the client received from us
			HnAcks acks = packet->ackBitfield;
			uint8_t count = (client->socket.udp.ack < 32) ? client->socket.udp.ack : 32;
			for(uint8_t i = 0; i < count; ++i) {
				// check if we have an ack for that packet
				if(!(acks & (0x1 << i))) {
					//HN_LOG("No ack for packet [" + std::to_string(client->socket.udp.sequenceId - i) + "]");
				}
			}

		} else
			HN_LOG("Wrong order");

		//hnSocketUpdateReliable(client->socket);
	}
			
	if(packet->type == HN_PACKET_SYNC_REQUEST) {

		HnPacket idPacket;
		hnPacketCreate(&idPacket, HN_PACKET_CLIENT_DATA, &client->socket);
		hnPacketStoreInt(&idPacket, client->clientId);
		hnSocketSendPacket(&client->socket, &idPacket);

		// existing clients
		for(auto const& all : server->clients) {
			if(all.second.clientId != client->clientId) {
				HnPacket connectPacket;
				hnPacketCreate(&connectPacket, HN_PACKET_CLIENT_CONNECT, &client->socket);
				hnPacketStoreInt(&connectPacket, all.second.clientId);
				hnSocketSendPacket(&client->socket, &connectPacket);
			}
		}

		// existing variables

		hnSocketSendPacket(&client->socket, HN_PACKET_SYNC_REQUEST);
	} else if(packet->type == HN_PACKET_CLIENT_DISCONNECT) {

		// this client disconnected
		HN_LOG("Remote client [" + std::to_string(client->clientId) + "] disconnected");
		hnClientDisconnect(server, client);
	} else if(packet->type == HN_PACKET_PING_CHECK) {

		// ping check returned, messure time
		int64_t now = hnPlatformGetCurrentTime();
		client->socket.ping = (uint16_t) (now - client->socket.pingCheck);
		client->socket.pingCheck = 0;
	} else if(packet->type == HN_PACKET_MESSAGE) {

		std::string msg = hnPacketGetString(packet);
		HN_DEBUG("Received message from client [" + std::to_string(client->clientId) + "]: " + msg);
	}
	else {
		HN_DEBUG("Received invalid packet of type [" + std::to_string(packet->type) + "] from remote client");
	}
};
