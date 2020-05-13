#ifndef HN_CLIENT_H
#define HN_CLIENT_H

#include "hnConnection.h"

struct HnClient {
	HnSocket   socket;
	HnClientId clientId = 0;
};

extern HN_API void hnClientCreate(HnClient* client, std::string const& host, uint32_t const port, HnProtocolType const protocolType, HnProtocolId const protocolId);
extern HN_API void hnClientDestroy(HnClient* client);

#endif
