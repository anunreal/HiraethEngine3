#include "hnConnection.h"
#include <ws2tcpip.h>
#include <iostream>
#include <stdarg.h>

#pragma comment(lib, "Ws2_32.lib")

HiraethNetwork* hn = nullptr;

void hnCreateNetwork() {
    
    if(hn == nullptr) {
        hn = new HiraethNetwork();
        hn->status = HN_STATUS_ERROR; // assume that something will go wrong
        
        // set up wsa
        WSAData wsData;
        WORD ver = MAKEWORD(2, 2);
        
        if(WSAStartup(ver, &wsData) != 0) {
            HN_ERROR("Could not start wsa, exiting...");
            return;
        }
        
        hn->status = HN_STATUS_CONNECTED; // successful start
        
    }
    
}

void hnDestroyNetwork() {
    WSACleanup();
}

void hnSendSocketData(HnSocket* socket, const std::string& data) {
    
    int res = send(socket->id, data.c_str(), (int) data.size() + 1, 0);
    if(res <= 0) {
        HN_ERROR("Lost connection to socket");
        socket->status = HN_STATUS_ERROR;
    }
    
}

void hnSendPacket(HnSocket* socket, const HnPacket& packet) {
    
    if (socket->status != HN_STATUS_CONNECTED)
        return;
    
    hnSendSocketData(socket, hnGetPacketContent(packet));
    
    HN_LOG("Send Packet [" + hnGetPacketContent(packet) + "]");
    
}


std::string hnVariableDataToString(const HnVariableInfo* info) {
    
    std::string data;
    
    switch(info->type) {
        
        case HN_DATA_TYPE_INT:
        data = std::to_string(*((int*)info->data));
        break;
        
        case HN_DATA_TYPE_FLOAT:
        data = std::to_string(*((float*)info->data));
        break;
        
        case HN_DATA_TYPE_VEC2:
        data = std::to_string(((float*)info->data)[0]) + '/' + std::to_string(((float*)info->data)[1]);
        break;
        
        case HN_DATA_TYPE_VEC3:
        data = std::to_string(((float*)info->data)[0]) + '/' + std::to_string(((float*)info->data)[1]) + '/' +
            std::to_string(((float*)info->data)[2]);
        break;
        
        case HN_DATA_TYPE_VEC4:
        data = std::to_string(((float*)info->data)[0]) + '/' + std::to_string(((float*)info->data)[1]) + '/' +
            std::to_string(((float*)info->data)[2]) + '/' + std::to_string(((float*)info->data)[3]);
        break;
        
    }
    
    return data;
    
}

void hnParseVariableString(void* ptr, const std::string& dataString, const HnDataType type) {
    
    switch(type) {
        case HN_DATA_TYPE_INT:
        *((int*) ptr) = std::stoi(dataString);
        break;
        
        case HN_DATA_TYPE_FLOAT:
        *((float*) ptr) = std::stof(dataString);
        break;
        
        case HN_DATA_TYPE_VEC2: {
            std::string arguments[2];
            size_t index = dataString.find('/');
            arguments[0] = dataString.substr(0, index);
            arguments[1] = dataString.substr(index + 1);
            ((float*)ptr)[0] = std::stof(arguments[0]);
            ((float*)ptr)[1] = std::stof(arguments[1]);
            break;
        }
        
        case HN_DATA_TYPE_VEC3: {
            std::string arguments[3];
            size_t index0 = dataString.find('/');
            arguments[0] = dataString.substr(0, index0);
            
            // we need the actual offset, find will only give us the offset in the substr (add 1 for the last /)
            size_t index1 = dataString.substr(index0 + 1).find('/') + index0 + 1;
            arguments[1] = dataString.substr(index0 + 1, index1);
            arguments[2] = dataString.substr(index1 + 1);
            
            ((float*)ptr)[0] = std::stof(arguments[0]);
            ((float*)ptr)[1] = std::stof(arguments[1]);
            ((float*)ptr)[2] = std::stof(arguments[2]);
            break;
        }
        
        case HN_DATA_TYPE_VEC4: {
            std::string arguments[4];
            size_t index0 = dataString.find('/');
            
            // we need the actual offset, find will only give us the offset in the substr (add 1 for the last /)
            size_t index1 = dataString.substr(index0 + 1).find('/') + index0 + 1;
            arguments[1] = dataString.substr(index0 + 1, index1);
            
            size_t index2 = dataString.substr(index1 + 1).find('/') + index1 + 1;
            arguments[2] = dataString.substr(index1 + 1, index2);
            arguments[3] = dataString.substr(index2 + 1);
            
            ((float*)ptr)[0] = std::stof(arguments[0]);
            ((float*)ptr)[1] = std::stof(arguments[1]);
            ((float*)ptr)[2] = std::stof(arguments[2]);
            ((float*)ptr)[3] = std::stof(arguments[3]);
            break;
        }
    }
    
}



HnPacket hnBuildPacketFromParameters(int numargs, const unsigned int type, ...) {
    
    HnPacket packet(type);
    va_list ap;
    
    va_start(ap, type);
    while(numargs--)
        packet.arguments.emplace_back(va_arg(ap, const char*));
    va_end(ap);
    
    return packet;
    
}

HnPacket hnDecodePacket(std::string& message) {
    
    HnPacket packet;
    std::string current;
    unsigned int charCount = 0;
    for(char all : message) {
        charCount++;
        if(all == '!') // end of packet 
            break;
        else if(all == ':') { // argument 
            if(packet.type == 0)
                packet.type = std::stoi(current); // first argument: type
            else
                packet.arguments.emplace_back(current);
            current.clear();
        } else // inside argument
            current += all;
    }
    
    if (packet.type == 0)
        packet.type = std::stoi(current); // this packet has no arguments, only a type
    else
        packet.arguments.emplace_back(current);
    message = message.substr(charCount);
    
    unsigned int offset = 0;
    while(message.size() > 0 && message[offset] == 0)
        offset++;
    
    if(offset > 0)
        message = message.substr(offset);
    
    return packet;
    
}

std::vector<HnPacket> hnDecodePackets(std::string& message) {
    
    std::vector<HnPacket> packets;
    while(message.find('!') != std::string::npos)
        packets.emplace_back(hnDecodePacket(message));
    
    return packets;
    
}

std::string hnGetPacketContent(const HnPacket& packet) {
    
    std::string data = std::to_string(packet.type);
    
    for (const std::string& all : packet.arguments)
        data += ":" + all;
    
    data += "!";
    return data;
    
}