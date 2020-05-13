#ifndef HN_CLIENT_H
#define HN_CLIENT_H

#include "hnConnection.h"
#include "hnCallbacks.h"
#include <thread>
#include <map>
#include <vector>

struct HnLocalClient {
    // the client id
    uint16_t id = 0;
    // local variable pointers of this client
    HnVariableDataMap variableData;
    // a list of custom packets sent by this client
    HnCustomPackets customPackets;
};

struct HnClient {
    // the id of this client
    uint16_t id = 0;
    // the socket to the server
    HnSocket socket;
    // maps clients from the server to their id
    std::map<uint16_t, HnLocalClient> clients;
    HnVariableLookupMap variableNames;
    HnVariableInfoMap variableInfo;
    HnClientCallbacks callbacks;
    HnCustomPackets customPackets;
    
    // this map holds data hooks that are registered before the variable is officially registered by the server
    // Once the server responds and gives an id to the variable, this pointer will be moved into the variableInfo
    // map and removed from this
    std::map<std::string, void*> temporaryDataHooks;
    // input buffer for reading
    char inputBuffer[4096] = {0};
    // left over message from the last input if it didnt line up with the packet ending
    std::string lastInput = "";
};

// tries to connect the client to a given server. If the connection succeeds, data is synchronised between
// the client and the server, else the sockets status is set to error and a message will be printed
extern HN_API void hnClientConnect(HnClient* client, std::string const& host, uint32_t const port, HnProtocol const type);
// disconnects and cleans up given client
extern HN_API void hnClientDisconnect(HnClient* client);
// updates the input of given client. This should be called from a seperate thread as this will block until
// a message is read from the server or the connection is stopped
extern HN_API void hnClientUpdateInput(HnClient* client);
// updates all variables of given client if they need to be updated by sending their value to the server
extern HN_API void hnClientUpdateVariables(HnClient* client);
// handles a packet recieved for a client. If this is a custom packet, it is added to the packet queue of the
// client, else this will handle the packet internally (syncing...)
extern HN_API void hnClientHandlePacket(HnClient* client, HnPacket* packet);
// requests a new variable which will be registered anywhere. syncRate is the number of ticks after which the 
// variable should be synced with. If dataSize is zero, this tries to figure out the size of the variable. THIS
// ONLY WORKS FOR PRIMITIVE TYPES. Strings need to have their (maximum) size set!
extern HN_API void hnClientCreateVariable(HnClient* client, std::string const& name, HnDataType const type, uint16_t const syncRate, uint32_t const dataSize = 0);
// syncs a client to the current server state. This should be called after the connection was established.
// A sync request will be sent to the server and then this function blocks until the sync is ended by the server.
// This will synchronize remote clients and variables
extern HN_API void hnClientSync(HnClient* client);
// hooks a pointer to some data to the local variable of given client. This means that the given data pointer will
// be updated and modified when the variable is updated by the server, and changes made locally will also be
// updated towards the server. The data pointer must be a valid data type and it must exist for as long as the
// variable exists
extern HN_API inline void hnClientHookVariable(HnClient* client, std::string const& name, void* data);
// sends the variable to server for updating once.
extern HN_API void hnClientUpdateVariable(HnClient* client, std::string const& name);
// hooks a pointer to some data to the local variable of given local client. This means that the pointer will be
// modified whenever theres a variable update from the remote counter part. The data pointer must be the valid data
// type and it must exist for as long as the variable (and the local client) exist
extern HN_API inline void hnLocalClientHookVariable(HnClient* client, HnLocalClient* localclient, std::string const& name, void* data);
// waits for the next packet read by the client, parses the information and returns the packet
extern HN_API HnPacket hnClientReadPacket(HnClient* client);
// gets the oldest custom packet sent from this client or an empty packet if no custom packet was sent since
// the last check. This will remove the packet from the clients list
extern HN_API HnPacket hnLocalClientGetCustomPacket(HnLocalClient* client);
// gets the oldest custom packet sent from this client (ourselves) or an empty packet if no custom packet was sent
// since the last check. THis will remove the packet from the clients list
extern HN_API HnPacket hnClientGetCustomPacket(HnClient* client);
// returns the client with given index, NOT id. This is simply in the order of connection, as they are stored in the
// map. Indices start at 0. If given index is too big (not enough clients), nullptr is returned
extern HN_API HnLocalClient* hnClientGetLocalClientByIndex(HnClient* client, uint16_t const index);
// returns the pointer to the given variable of the local client. If that variable does not exist or was not
// hooked for given local client, nullptr is returned
extern HN_API void* hnClientGetLocalClientVariable(HnClient* client, HnLocalClient* localClient, std::string const& varName);

#endif
