#ifndef HN_CALLBACKS_H

struct HnClient;
struct HnLocalClient;
struct HnServer;
struct HnRemoteClient;

// -- client callbacks

// called after a successfull connection to a local client, either when a new client connected to the server or
// on client sync
typedef void(*HnLocalClientConnectCallback)(HnClient*, HnLocalClient*);
// called when a client disconnected from the server
typedef void(*HnLocalClientDisconnectCallback)(HnClient*, HnLocalClient*);

struct HnClientCallbacks {
    HnLocalClientConnectCallback    clientConnect    = nullptr;
    HnLocalClientDisconnectCallback clientDisconnect = nullptr;
};


// --- server side

typedef void(*HnRemoteClientConnectCallback)(HnServer*, HnRemoteClient*);
typedef void(*HnRemoteClientDisconnectCallback)(HnServer*, HnRemoteClient*);

struct HnServerCallbacks {
    HnRemoteClientConnectCallback    clientConnect    = nullptr;
    HnRemoteClientDisconnectCallback clientDisconnect = nullptr;
};

#endif
