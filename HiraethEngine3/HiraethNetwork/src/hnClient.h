#ifndef HN_CLIENT_H
#define HN_CLIENT_H

#include "hnConnection.h"
#include "hnCallbacks.h"
#include <unordered_map>

struct HnLocalClient {
    HnClientId clientId = 0;
    std::unordered_map<HnVariableId, void*> variableData;
};

struct HnClient {
    HnSocket   socket;
    HnClientId clientId = 0;
    std::unordered_map<HnClientId, HnLocalClient>    clients;
    std::unordered_map<HnVariableId, HnVariableInfo> variables;
    std::unordered_map<HnVariableId, void*>          variableData;
    std::unordered_map<std::string, HnVariableId>    variableNames; 
    std::unordered_map<std::string, void*>           variableTemporaryHooks;
    
    uint32_t timeOut = 7000; // if we havent read a packet from the server for this time (in ms), the connection is assumed to be timed out
    uint32_t tick = 0; // this tick counter goes up on every update. Its used to determine which variables to send to the server
    
    HnClientCallbacks callbacks;
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

// If a variable with that name does not yet exist, this send a request to the server for a new variable with given
// information. The variable can not be used until confirmation from the server was received.
extern HN_API void hnClientCreateVariableFixed(HnClient* client, std::string const& name, HnVariableType const type, uint8_t length, uint8_t syncRate);
// hooks a memory pointer to a variable. This pointer is then stored in the client and will be used when updating
// the variable. The pointer is read only by hn
extern HN_API void hnClientHookVariable(HnClient* client, std::string const& name, void* hook);
// sends the variables data to the server. This just sends an update packet, it doesnt care about update ticks
// and shit
extern HN_API void hnClientSendVariable(HnClient* client, HnVariableId const variable);
// sends the variables data to the server. This just sends a reliable update packet, it doesnt care about update
// ticks and shit
extern HN_API void hnClientSendVariableReliable(HnClient* client, HnVariableId const variable);

// hooks a memory pointer to a variable. This pointer is then stored in the local client and will be updated by hn.
// This variable is the remote part of the actual client
extern HN_API inline void hnLocalClientHookVariable(HnClient* client, HnLocalClient* local, std::string const& varName, void* hook);

#endif
