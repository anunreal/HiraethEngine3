#include <iostream>
#include <windows.h>
#include <thread>

#ifdef HN_TEST_CLIENT
#include "src/hnClient.h"

struct vec2 {
    float x = 0.f, y = 0.f;
};

vec2 localTestVar;

struct Player {
    vec2 testVar;
};

std::unordered_map<HnClientId, Player> players;

void thread(HnClient* client) {
	while(client->socket.status == HN_STATUS_CONNECTED)
		hnClientHandleInput(client);
};

void clientConnect(HnClient* client, HnLocalClient* local) {
    Player* p = &players[local->clientId];
    hnLocalClientHookVariable(client, local, "testvar", &p->testVar);
};

void clientDisconnect(HnClient* client, HnLocalClient* local) {
    players.erase(local->clientId);
};

int main() {
	HnClient client;
    client.callbacks.clientConnect = clientConnect;
    client.callbacks.clientDisconnect = clientDisconnect;
	hnClientCreate(&client, "localhost", 9876, HN_PROTOCOL_UDP, 75);
    
	if(client.socket.status == HN_STATUS_ERROR) {
		system("pause");
		return -1;
	}

	std::thread t(thread, &client);

    hnClientCreateVariableFixed(&client, "testvar", HN_VARIABLE_TYPE_FLOAT, 2, 60);
    hnClientHookVariable(&client, "testvar", &localTestVar); 
    
	while(client.socket.status == HN_STATUS_CONNECTED) {		
        //hnClientSendVariable(&client, client.variableNames["testvar"]);
        hnClientUpdate(&client);
        //HN_LOG("Client stats: ping " + std::to_string(client.socket.stats.ping) + "ms, packet loss " + std::to_string((int) (client.socket.stats.packetLoss * 100)) + "%");

        localTestVar.x += 0.16f;
        if(localTestVar.x > 10.f) {
            localTestVar.x = 0.f;
            localTestVar.y++;
        }

        if(client.tick % 10 == 0) {
            HN_LOG("Local: " + std::to_string(localTestVar.x) + ", " + std::to_string(localTestVar.y));
            for(auto const& all : players)
                HN_LOG("Remote: " + std::to_string(all.second.testVar.x) + ", " + std::to_string(all.second.testVar.y));
        }
    }

	HN_DEBUG("Destroying client...");
	hnClientDestroy(&client);
	t.join();
	return 0;
};

#endif



#ifdef HN_TEST_SERVER
#include "src/hnServer.h"

void thread(HnServer* server) {
	while(server->serverSocket.status == HN_STATUS_CONNECTED) {
		hnServerHandleInput(server);
	}
};

int main() {
	HnServer server;
	hnServerCreate(&server, 9876, HN_PROTOCOL_UDP, 75);

	std::thread t(thread, &server);
	
	while(server.serverSocket.status == HN_STATUS_CONNECTED) {
		hnServerUpdate(&server);
	}

	
	hnServerDestroy(&server);
	t.join();
	return 0;
};

#endif

/*
int main() {
	HnClient client;
	HnServer server;
	hnServerCreate(&server, 9876, HN_PROTOCOL_UDP, 75);
	hnClientCreate(&client, "localhost", 9876, HN_PROTOCOL_UDP, 75);
	
	HnPacket sent;
	hnPacketCreate(&sent, HN_PACKET_CLIENT_DISCONNECT, &client.socket);
	hnPacketStoreInt(&sent, 42);
	hnPacketStoreString(&sent, "Klara");
	hnPacketStoreFloat(&sent, 1.85);
	hnPacketStoreInt(&sent, 987654);
	hnSocketSendPacket(&client.socket, &sent);

	while(client.socket.status == HN_STATUS_CONNECTED) {
		HnUdpConnection connection;
		HnPacket read;
		hnSocketReadPacket(&server.serverSocket, &read, &connection);
		HN_LOG("Read packet of type [" + std::to_string(read.type) + "]");
	}
	
	hnClientDestroy(&client);
	hnServerDestroy(&server);
};
*/
