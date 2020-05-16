#include <iostream>
#include <windows.h>
#include "src/hnClient.h"
#include "src/hnServer.h"

#ifdef HN_TEST_CLIENT

void thread(HnClient* client) {
	while(client->socket.status == HN_STATUS_CONNECTED)
		hnClientHandleInput(client);
};

int main() {
	HnClient client;
	hnClientCreate(&client, "localhost", 9876, HN_PROTOCOL_UDP, 75);
	if(client.socket.status == HN_STATUS_ERROR) {
		system("pause");
		return -1;
	}
	
	std::thread t(thread, &client);

	while(client.socket.status == HN_STATUS_CONNECTED) {
		hnClientUpdate(&client);
	}

	HN_DEBUG("Destroying client...");
	hnClientDestroy(&client);
	t.join();
	return 0;
};

#endif



#ifdef HN_TEST_SERVER

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
