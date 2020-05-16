#ifndef HN_SERVER_H
#define HN_SERVER_H

#include "hnConnection.h"
#include <map>
#include <thread>

struct HnRemoteClient {
	HnSocket    socket;
	HnClientId  clientId = 0;
	std::thread tcpThread;
};

struct HnServer {
	HnSocket serverSocket;

	std::map<HnClientId, HnRemoteClient> clients;
	HnClientId clientCounter = 0;

	// -- timing

	int64_t lastUpdate = 0;
	
	// -- settings

	uint32_t timeOut = 700000; // if a client hasnt sent a packet in this time (in ms), the connection is assumed to be timed out
	uint32_t pingCheckIntervall = 1000; // the intervall in between to send ping checks (after the last check returned) in ms
};

extern HN_API void hnServerCreate(HnServer* server, uint32_t const port, HnProtocolType const protocolType, HnProtocolId const protocolId);
extern HN_API void hnServerDestroy(HnServer* server);
extern HN_API void hnServerHandleInput(HnServer* server);
extern HN_API void hnServerUpdate(HnServer* server);
extern HN_API void hnServerBroadcastPacket(HnServer* server, HnPacket* packet);

extern HN_API void hnRemoteClientHandlePacket(HnServer* server, HnRemoteClient* client, HnPacket* packet);

#endif
