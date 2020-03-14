#pragma once

struct HnClient;
struct HnLocalClient;
struct HnServer;
struct HnRemoteClient;
struct HnPacket;


// --- client side

// called after a successful connection to a client
typedef void(*HnLocalClientConnectCallback)(HnClient*, HnLocalClient*); 
// called when a client disconnected
typedef void(*HnLocalClientDisconnectCallback)(HnClient*, HnLocalClient*);
// called whenever another client sent a public custom packet. The packets first argument will be the real first 
// argument, not the clients id or any flags. If true is returned, the packet will be expected to be handled
// and not added to the clients custom packets list
typedef bool(*HnLocalClientCustomPacketCallback)(HnClient*, HnLocalClient*, const HnPacket&);

struct HnClientCallbacks {
    HnLocalClientConnectCallback clientConnect = nullptr;
    HnLocalClientDisconnectCallback clientDisconnect = nullptr;
    HnLocalClientCustomPacketCallback customPacket = nullptr;
};



// --- server side
typedef void(*HnRemoteClientConnectCallback)(HnServer*, HnRemoteClient*);
typedef void(*HnRemoteClientDisconnectCallback)(HnServer*, HnRemoteClient*);

struct HnServerCallbacks {
    HnRemoteClientConnectCallback clientConnect = nullptr;
    HnRemoteClientDisconnectCallback clientDisconnect = nullptr;
};
