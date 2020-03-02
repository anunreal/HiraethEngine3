#include <iostream>
#include <windows.h>

#ifdef HN_TEST_CLIENT
#include "src/hnClient.h"

struct vec2 {
    float x, y;
};

struct vec3 {
    float x, y, z;
};

struct vec4 {
    float x = 0.0f, y = 0.0f, z = 0.0f, w = 0.0f;
};

struct Player {
    std::string name;
    HnLocalClient* client;
    float testvar;
    vec4 position;
};

// maps id to player
std::map<unsigned int, Player> players;

void _hnClientConnect(HnClient* client, HnLocalClient* local) {
    
    Player* p = &players[local->id];
    p->client = local;
    p->testvar = 0.0f;
    hnHookVariable(client, local, "testvar", &p->testvar);
    hnHookVariable(client, local, "position", &p->position);
    
};

void _hnClientDisconnect(HnClient* client, HnLocalClient* local) {
    
    players.erase(local->id);
    
};


int inputThread(HnClient* client) {
    while (client->socket.status == HN_STATUS_CONNECTED) {
        hnUpdateClientInput(client);
    }
    
    return 0;
};


void stressTest(HnSocket* serversocket) {
    
    for (int i = 0; i < 100; ++i) {
        hnSendPacket(serversocket, hnBuildCustomPacket(true, std::to_string(i).c_str()));
    }
    
}


vec3 position;
vec4 rotation;

int main() {
    
    std::cout << "Starting client..." << std::endl;
    HnClient client;
    
    client.callbacks.clientConnect = _hnClientConnect;
    client.callbacks.clientDisconnect = _hnClientDisconnect;
    
    hnConnectClient(&client, "localhost", 9876);
    hnSyncClient(&client);
    stressTest(&client.socket);
    
    std::thread in(inputThread, &client);
    
    hnCreateVariable(&client, "position", HN_DATA_TYPE_VEC3, 1);
    hnCreateVariable(&client, "rotation", HN_DATA_TYPE_VEC4, 1);
    hnHookVariable(&client, "position", &position);
    hnHookVariable(&client, "rotation", &rotation);
    
    std::cout << "Entering main loop" << std::endl;
    
    while (client.socket.status == HN_STATUS_CONNECTED) {
        position.x += 0.5f;
        
        if(position.x >= 10.f) {
            position.y++;
            position.x = 0.f;
            position.z -= 2;
        }
        
        rotation.y += 0.1f;
        rotation.z += 2.f;

        {
            
            // print other clients variables
            for(auto& all : client.clients) {
                std::cout << "Client(" << all.first << ": " << hnVariableDataToString(hnGetLocalClientVariable(&client, &all.second, "position"),
                    HN_DATA_TYPE_VEC4) << std::endl;
            }
            
        }
        
        
        hnUpdateClientVariables(&client);
        Sleep(16);
    }
    
    in.detach();
    hnDisconnectClient(&client);
    
};

#endif

#ifdef HN_TEST_SERVER
#include "src/hnServer.h"

void serverThread(HnServer* server) {
    
    while (server->serverSocket.status == HN_STATUS_CONNECTED) {
        hnServerAcceptClients(server);
    }
    
};

int main() {
    
    std::cout << "Starting hn server..." << std::endl;
    HnServer server;
    hnCreateServer(&server, 9876);
    
    std::thread in(serverThread, &server);
    
    while (server.serverSocket.status == HN_STATUS_CONNECTED) {
        // update server
        hnUpdateServer(&server);
        Sleep(16);
    }
    
    in.detach();
    hnDestroyServer(&server);
    return 0;
    
};

#endif