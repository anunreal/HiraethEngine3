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

// tries to connect the client to given server.  If the connection fails, the sockets status is set to error. If
// the connection succeeds, the client will be synced with existing clients and variables
extern HN_API void hnClientCreate(HnClient* client, std::string const& host, uint32_t const port, HnProtocolType const protocolType, HnProtocolId const protocolId);
// destroys the client by disconnecting from the server and closing the socket. This will also reset all data
extern HN_API void hnClientDestroy(HnClient* client);
// reads a packet from the clients socket and then handles that packet. This should be called from a seperated
// thread as it will block until a packet is read
extern HN_API void hnClientHandleInput(HnClient* client);
// handles a packet. If this is udp, acks will be handled and reliable packets that got dropped will be resent.
extern HN_API void hnClientHandlePacket(HnClient* client, HnPacket* packet);
// updates the clients connection. This checks for timeouts. If the connection is still valid, this sleeps 16 ms
// for now
extern HN_API void hnClientUpdate(HnClient* client);

#endif
