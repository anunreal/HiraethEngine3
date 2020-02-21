#pragma once

struct HnClient;
struct HnLocalClient;

// event callbacks

typedef void(*HnClientConnectCallback)(HnClient*, HnLocalClient*);
typedef void(*HnClientDisconnectCallback)(HnClient*, HnLocalClient*);

struct HnClientCallbacks {
    HnClientConnectCallback clientConnect = nullptr;
    HnClientDisconnectCallback clientDisconnect = nullptr;
};
