#include <iostream>
#include <windows.h>

#ifdef HN_TEST_CLIENT
#include "src/hnClient.h"

float testvar = 0.f;

void clientThread(HnClient* client) {
    while(client->socket.status == HN_STATUS_CONNECTED)
        hnClientUpdateInput(client);
};

int main() {
    
    HnClient client;
    hnClientConnect(&client, "localhost", 9876, HN_PROTOCOL_UDP);
    hnClientSync(&client);
    hnClientCreateVariable(&client, "testvar", HN_DATA_TYPE_FLOAT, 10);
    hnClientHookVariable(&client, "testvar", &testvar);
    
    std::thread t(clientThread, &client);
    
    while(client.socket.status == HN_STATUS_CONNECTED) {
        hnClientUpdateVariables(&client);
        testvar += 0.16f;
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
        hnServerUpdate(server);
};

int main() {
    
    HnServer server;
    hnServerCreate(&server, 9876, HN_PROTOCOL_UDP);
    
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