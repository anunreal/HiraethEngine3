#include "hnClient.h"

void hnClientCreate(HnClient* client, std::string const& host, uint32_t const port, HnProtocolType const protocolType, HnProtocolId const protocolId) {
	client->socket.type       = protocolType;
	client->socket.protocolId = protocolId;
	hnSocketCreateClient(&client->socket, host, port);

	hnSocketSendPacket(&client->socket, HN_PACKET_SYNC_REQUEST);
};

void hnClientDestroy(HnClient* client) {
	hnSocketDestroy(&client->socket);
};
