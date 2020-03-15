#pragma once
#include "hnConnection.h"
#include "hnCallbacks.h"
#include <map>
#include <thread>
#include <chrono>

struct HnRemoteClient {
    // id of this client on the server
    unsigned long long id = 0;
    // socket of this client from server to client
    HnSocket socket;
    // the input thread of this client
    std::thread thread;
    // all data stored for this client, mapped to the id of it
    HnVariableDataMap variables;
    // all packets, public or private, sent by this client
    HnCustomPackets customPackets;
    // the ping in milliseconds (server -> client -> server)
    unsigned int ping = 0;
    // the time the last ping check was sent (from the server) in milliseconds, since program start or -1 if there
    // is currently no outgoing ping check
    long long pingCheck = 0;
};

struct HnServer {
    // accepting socket of the server
    HnSocket serverSocket;
    // all connected clients, mapped to their ids
    std::map<unsigned long long, HnRemoteClient> clients;
    // id of the last connected client. This will increase everytime a client connects in order to keep
    // client ids unique
    unsigned int clientCounter = 0;
    // a vector of sockets that connected to this server. Filled in the accepting thread, read and handled
    // in the server update
    std::vector<unsigned long long> connectionRequests;
    // a vector of client ids that disconnected since the last update. Filled in the clients threads, read and
    // handled in the server update 
    std::vector<unsigned long long> disconnectRequests;
    
    // the time (in milliseconds) after which a connection is seen as timed out. When a ping check takes longer than
    // this time to return, the client is assumed to be dead and disconnected
    unsigned int timeOut = 7000;
    // the time intervall (in milliseconds) in which a ping check is sent out
    unsigned int pingCheckIntervall = 500;
    // the time (in milliseconds) since the last update
    long long updateTime = 0;
    // stores the time point of the last server update
    std::chrono::high_resolution_clock::time_point lastUpdate;
    
    HnServerCallbacks callbacks;
    HnVariableInfoMap variableInfo;
    HnVariableLookupMap variableNames;
    // counts the number of variables requested in order to keep ids unique
    unsigned int variableCounter = 0;
};

// sets up a new server on given port
extern HN_API void hnCreateServer(HnServer* server, const unsigned int port);
// closes the server and deletes all associated data
extern HN_API void hnDestroyServer(HnServer* server);
// sends the client a disconnect message and updates its status
extern HN_API void hnKickRemoteClient(HnRemoteClient* client);
// Accepts an incoming client connection. This should be called in an external thread as it will
// block until a new client connects.
extern HN_API void hnServerAcceptClients(HnServer* server);
// updates the server by synchronizing data and running requests. Should be called from the main thread
extern HN_API void hnUpdateServer(HnServer* server);
// the input thread of any given remote client. Created when the client connects, runs until the clients
// socket is closed
extern HN_API void hnRemoteClientThread(HnServer* server, HnRemoteClient* client);
// handles an incoming packet from a client to the server. If this is a custom packet, it is added to
// the clients packet queue, else it will be handled internally (syncing...)
extern HN_API void hnHandleServerPacket(HnServer* server, HnRemoteClient* sender, const HnPacket& packet);
// sends given packet to all clients on given server
extern HN_API inline void hnBroadcastPacket(HnServer* server, const HnPacket& packet);
// gets the oldest custom packet sent from this client or an empty packet if no custom packet was sent since
// the last check. This will remove the packet from the clients list
extern HN_API HnPacket hnGetCustomPacket(HnRemoteClient* client);
// hooks a data pointer to a variable of a remote client. ptr must point to valid memory of the data type of
// the variable that is hooked
extern HN_API inline void hnHookVariable(HnServer* server, HnRemoteClient* client, const std::string& variable, void* ptr);
// sends a ping check to given client. If no answer is recieved within two seconds, the client is assumed to be disconnected (time out).
extern HN_API void hnPingCheck(HnRemoteClient* client);