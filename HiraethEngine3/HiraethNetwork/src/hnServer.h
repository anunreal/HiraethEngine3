#pragma once
#include "hnConnection.h"
#include <map>
#include <thread>

struct HnRemoteData {
    unsigned int id;
    HnDataType type;
    
    union data {
        int _int;
        float _float;
        float _vec2[2];
    };
};

struct HnRemoteClient {
    // id of this client on the server
    unsigned int id;
    // socket of this client from server to client
    HnSocket socket;
    // the input thread of this client
    std::thread thread;
    // all data stored for this client, mapped to the id of it
    std::map<unsigned int, HnRemoteData> data;
    // all packets, public or private, sent by this client
    HnCustomPackets customPackets;
};

struct HnServer {
    // accepting socket of the server
    HnSocket serverSocket;
    
    // all connected clients, mapped to their ids
    std::map<unsigned int, HnRemoteClient> clients;
    // id of the last connected client. This will increase everytime a client connects in order to keep
    // client ids unique
    unsigned int clientCounter = 0;
    // a vector of sockets that connected to this server. Filled in the accepting thread, read and handled
    // in the server update
    std::vector<unsigned int> connectionRequests;
    // a vector of client ids that disconnected since the last update. Filled in the clients threads, read and
    // handled in the server update 
    std::vector<unsigned int> disconnectRequests;
    
    HnVariableInfoMap variableInfo;
    HnVariableLookupMap variableNames;
    // counts the number of variables requested in order to keep ids unique
    unsigned int variableCounter = 0;
};

// sets up a new server on given port
extern void hnCreateServer(HnServer* server, const unsigned int port);
// closes the server and deletes all associated data
extern void hnDestroyServer(HnServer* server);
// sends the client a disconnect message and updates its status
extern void hnKickRemoteClient(HnRemoteClient* client);
// Accepts an incoming client connection. This should be called in an external thread as it will
// block until a new client connects.
extern void hnServerAcceptClients(HnServer* server);
// updates the server by synchronizing data and running requests. Should be called from the main thread
extern void hnUpdateServer(HnServer* server);
// the input thread of any given remote client. Created when the client connects, runs until the clients
// socket is closed
extern void hnRemoteClientThread(HnServer* server, HnRemoteClient* client);
// handles an incoming packet from a client to the server. If this is a custom packet, it is added to
// the clients packet queue, else it will be handled internally (syncing...)
extern void hnHandleServerPacket(HnServer* server, HnRemoteClient* sender, const HnPacket& packet);
// sends given packet to all clients on given server
extern inline void hnBroadcastPacket(HnServer* server, const HnPacket& packet);
// gets the oldest custom packet sent from this client or an empty packet if no custom packet was sent since
// the last check. This will remove the packet from the clients list
extern HnPacket hnGetCustomPacket(HnRemoteClient* client);
