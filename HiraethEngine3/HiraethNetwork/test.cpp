#include <iostream>
#include <windows.h>

#ifdef HN_TEST_CLIENT
#include "src/hnClient.h"

struct vec2 {
    float x, y;
};

struct vec3 {
    float x = 0.0f, y = 0.0f, z = 0.0f;
};

struct Player {
    std::string name;
    HnLocalClient* client;
    float testvar;
    vec3 position;
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
        
        for(auto& all : client->clients) {
            Player* p = &players[all.first];
            HnPacket customPacket = hnGetCustomPacket(&all.second);
            
            while(customPacket.type != 0) {
                if(customPacket.arguments[0] == "name") {
                    p->name = customPacket.arguments[1];
                    std::cout << "Set player name of [" << all.first << "] to " << p->name << std::endl;
                }
                
                customPacket = hnGetCustomPacket(&all.second);
            }
        }
    }
    
    return 0;
};


float testvar = 0.f;
vec3 position;

int main() {
    
    std::cout << "Starting client..." << std::endl;
    HnClient client;
    
    client.callbacks.clientConnect = _hnClientConnect;
    client.callbacks.clientDisconnect = _hnClientDisconnect;
    
    hnConnectClient(&client, "localhost", 9876);
    hnSyncClient(&client);
    
    std::thread in(inputThread, &client);
    
    hnCreateVariable(&client, "testvar", HN_DATA_TYPE_FLOAT, 60);
    hnCreateVariable(&client, "position", HN_DATA_TYPE_VEC3, 10);
    hnHookVariable(&client, "testvar", &testvar);
    hnHookVariable(&client, "position", &position);
    
    hnSendPacket(&client.socket, hnBuildCustomPacket(true, "Yoo server bro"));
    hnSendPacket(&client.socket, hnBuildCustomPacket(false, "Yoo client bro"));
    
    std::cout << "Entering main loop" << std::endl;
    
    while (client.socket.status == HN_STATUS_CONNECTED) {
        //std::string line;
        //std::getline(std::cin, line);
        //hnSendPacket(&client.socket, hnBuildCustomPacket(false, "name", line.c_str()));
        testvar += 0.1f;
        position.x += 0.5f;
        
        if(position.x >= 10.f) {
            position.y++;
            position.x = 0.f;
            position.z-=2;
        }
        
        //std::cout << "Local testvar: " << testvar << std::endl;
        hnUpdateClientVariables(&client);
        Sleep(16);
        
        for(const auto& all : players)
            std::cout << "ClientPos: " << all.second.position.x << ", " << all.second.position.y << ", " << all.second.position.z << std::endl;
        
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