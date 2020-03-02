#pragma once

struct HnClient;
struct HnLocalClient;
struct HnServer;
struct HnRemoteClient;
struct HnPacket;

// event callbacks

// client side
typedef void(*HnLocalClientConnectCallback)(HnClient*, HnLocalClient*);
typedef void(*HnLocalClientDisconnectCallback)(HnClient*, HnLocalClient*);
typedef bool(*HnLocalClientCustomPacketCallback(HnClient*, HnLocalClient*, const HnPacket&);
             
             struct HnClientCallbacks {
                 HnLocalClientConnectCallback clientConnect = nullptr;
                 HnLocalClientDisconnectCallback clientDisconnect = nullptr;
                 HnLocalClientCustomPacketCallback customPacket = nullptr;
             };
             
             
             
             // server side
             typedef void(*HnRemoteClientConnectCallback)(HnServer*, HnRemoteClient*);
             typedef void(*HnRemoteClientDisconnectCallback)(HnServer*, HnRemoteClient*);
             
             struct HnServerCallbacks {
                 HnRemoteClientConnectCallback clientConnect = nullptr;
                 HnRemoteClientDisconnectCallback clientDisconnect = nullptr;
             };
             