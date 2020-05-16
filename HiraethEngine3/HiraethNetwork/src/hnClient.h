#ifndef HN_CLIENT_H
#define HN_CLIENT_H

#include "hnConnection.h"
#include <unordered_map>

struct HnLocalClient {
	HnClientId clientId = 0;
};

struct HnClient {
	HnSocket   socket;
	HnClientId clientId = 0;
	std::unordered_map<HnClientId, HnLocalClient> clients;

	uint32_t timeOut = 7000; // if we havent read a packet from the server for this time (in ms), the connection is assumed to be timed out
};

extern HN_API void hnClientCreate(HnClient* client, std::string const& host, uint32_t const port, HnProtocolType const protocolType, HnProtocolId const protocolId);
extern HN_API void hnClientDestroy(HnClient* client);
extern HN_API void hnClientHandleInput(HnClient* client);
extern HN_API void hnClientHandlePacket(HnClient* client, HnPacket* packet);
extern HN_API void hnClientUpdate(HnClient* client);

#endif
