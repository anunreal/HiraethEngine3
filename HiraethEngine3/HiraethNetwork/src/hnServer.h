#ifndef HN_SERVER_H
#define HN_SERVER_H

#include "hnConnection.h"
#include <map>

struct HnRemoteClient {
	HnSocket   socket;
	HnClientId clientId = 0;
};

struct HnServer {
	HnSocket serverSocket;

	std::map<HnClientId, HnRemoteClient> clients;
	HnClientId clientCounter = 0;
};

extern HN_API void hnServerCreate(HnServer* server, uint32_t const port, HnProtocolType const protocolType, HnProtocolId const protocolId);
extern HN_API void hnServerDestroy(HnServer* server);
extern HN_API void hnServerUpdate(HnServer* server);
extern HN_API void hnServerBroadcastPacket(HnServer* server, HnPacket* packet);

extern HN_API void hnRemoteClientHandlePacket(HnRemoteClient* client, HnPacket* packet);

#endif
