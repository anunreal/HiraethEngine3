#include <iostream>
#include <windows.h>

#ifdef HN_TEST_CLIENT
#include "src/hnClient.h"

float testvar;

void clientThread(HnClient* client) {
    while(client->socket.status == HN_STATUS_CONNECTED)
        hnClientUpdateInputTcp(client);
};

int main() {
    
    HnClient client;
    hnClientConnect(&client, "localhost", 9876, HN_SOCKET_TYPE_TCP);
    hnClientSync(&client);
    hnClientCreateVariable(&client, "testvar", HN_DATA_TYPE_FLOAT, 10);
    hnClientHookVariable(&client, "testvar", &testvar);
    
    std::thread t(clientThread, &client);
    
    while(client.socket.status == HN_STATUS_CONNECTED) {
        hnClientUpdateVariables(&client);
        Sleep(16);
    }
    
    hnClientDisconnect(&client);
    if(t.joinable())
        t.join();
    
    return 0;
    
};

#endif



#ifdef HN_TEST_SERVER
#include "src/hnServer.h"

void serverThread(HnServer* server) {
    while(server->socket.status == HN_STATUS_CONNECTED)
        hnServerUpdateTcp(server);
};

int main() {
    
    HnServer server;
    hnServerCreate(&server, 9876, HN_SOCKET_TYPE_TCP);
    
    std::thread t(serverThread, &server);
    
    while(server.socket.status == HN_STATUS_CONNECTED) {
        hnServerHandleRequests(&server);
        Sleep(16);
    }
    
    hnServerDestroy(&server);
    if(t.joinable())
        t.join();
    
    return 0;
    
};

#endif