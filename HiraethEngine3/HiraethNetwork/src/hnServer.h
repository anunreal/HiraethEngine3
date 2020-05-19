#ifndef HN_SERVER_H
#define HN_SERVER_H

#include "hnConnection.h"
#include <unordered_map>
#include <thread>

struct HnRemoteClient {
    HnSocket    socket;
    HnClientId  clientId = 0;
    std::thread tcpThread;
    std::unordered_map<HnVariableId, void*> variableData;
};

struct HnServer {
    HnSocket serverSocket;

    std::unordered_map<HnClientId, HnRemoteClient>   clients;
    std::unordered_map<HnVariableId, HnVariableInfo> variables;
    std::unordered_map<std::string, HnVariableId>    variableNames; 

    HnClientId clientCounter = 0;
    HnVariableId variableCounter = 0;
    
    // -- timing

    int64_t lastUpdate = 0;
    
    // -- settings

    uint32_t timeOut = 7000; // if a client hasnt sent a packet in this time (in ms), the connection is assumed to be timed out
    uint32_t pingCheckIntervall = 500; // the intervall in between to send ping checks (after the last check returned) in ms
};

extern HN_API void hnServerCreate(HnServer* server, uint32_t const port, HnProtocolType const protocolType, HnProtocolId const protocolId);
extern HN_API void hnServerDestroy(HnServer* server);
extern HN_API void hnServerHandleInput(HnServer* server);
extern HN_API void hnServerUpdate(HnServer* server);
extern HN_API void hnServerBroadcastPacket(HnServer* server, HnPacket* packet);
extern HN_API void hnServerBroadcastPacketReliable(HnServer* server, HnPacket* packet);
extern HN_API HnClientId hnServerGetClientIdOfAddress(HnServer const* server, HnUdpAddress const* address);

extern HN_API void hnRemoteClientHandlePacket(HnServer* server, HnRemoteClient* client, HnPacket* packet);


#endif
