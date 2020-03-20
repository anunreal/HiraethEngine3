#include "hnConnection.h"
#include <iostream>
#include <stdarg.h>
#include <chrono>

HiraethNetwork* hn = nullptr;
std::chrono::time_point<std::chrono::steady_clock> startTime;

void hnSocketCreate(HnSocket* socket) {
    
    if(socket->type == HN_PROTOCOL_UDP)
        hnSocketCreateUdp(socket);
    else if(socket->type == HN_PROTOCOL_TCP)
        hnSocketCreateTcp(socket);
    
};

void hnSocketSend(HnSocket* socket, const std::string& msg) {
    
    if(socket->type == HN_PROTOCOL_UDP)
        hnSocketSendDataUdp(socket, msg);
    else if(socket->type == HN_PROTOCOL_TCP)
        hnSocketSendDataTcp(socket, msg);
    
};

std::string hnSocketRead(HnSocket* socket, char* buffer, const uint32_t maxSize) {
    
    std::string msg;
    if(socket->type == HN_PROTOCOL_UDP)
        msg = hnSocketReadUdp(socket, buffer, maxSize, nullptr);
    else if(socket->type == HN_PROTOCOL_TCP)
        msg = hnSocketReadTcp(socket, buffer, maxSize);
    
    return msg;
    
};


std::string hnVariableDataToString(const void* ptr, const HnDataType dataType) {
    
    if(ptr == nullptr)
        return "";
    
    std::string data;
    
    switch(dataType) {
        
        case HN_DATA_TYPE_INT:
        data = std::to_string(*((int*)ptr));
        break;
        
        case HN_DATA_TYPE_FLOAT:
        data = std::to_string(*((float*)ptr));
        break;
        
        case HN_DATA_TYPE_VEC2:
        data = std::to_string(((float*)ptr)[0]) + '/' + std::to_string(((float*)ptr)[1]);
        break;
        
        case HN_DATA_TYPE_VEC3:
        data = std::to_string(((float*)ptr)[0]) + '/' + std::to_string(((float*)ptr)[1]) + '/' +
            std::to_string(((float*)ptr)[2]);
        break;
        
        case HN_DATA_TYPE_VEC4:
        data = std::to_string(((float*)ptr)[0]) + '/' + std::to_string(((float*)ptr)[1]) + '/' +
            std::to_string(((float*)ptr)[2]) + '/' + std::to_string(((float*)ptr)[3]);
        break;
        
    }
    
    return data;
    
};

std::string hnVariableDataToString(const HnVariableInfo* info) {
    
    return hnVariableDataToString(info->data, info->type);
    
};

void hnVariableParseFromString(void* ptr, const std::string& dataString, const HnDataType type) {
    
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
            arguments[0] = dataString.substr(0, index0);
            
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
    
};

void hnSocketSendPacket(HnSocket* socket, const HnPacket& packet) {
    
    if (socket->status != HN_STATUS_CONNECTED)
        return;
    
    hnSocketSend(socket, hnPacketGetContent(packet));
    if(packet.type != HN_PACKET_VAR_UPDATE)
        HN_LOG("Send Packet [" + hnPacketGetContent(packet) + "]");
    
};

HnPacket hnPacketBuildFromParameters(uint8_t numargs, const uint8_t type, ...) {
    
    HnPacket packet(type);
    va_list ap;
    
    va_start(ap, type);
    while(numargs--)
        packet.arguments.emplace_back(va_arg(ap, const char*));
    va_end(ap);
    
    return packet;
    
};

HnPacket hnPacketDecodeFromString(std::string& message) {
    
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
    
};

std::vector<HnPacket> hnPacketDecodeAllFromString(std::string& message) {
    
    std::vector<HnPacket> packets;
    while(message.find('!') != std::string::npos)
        packets.emplace_back(hnPacketDecodeFromString(message));
    
    return packets;
    
};

std::string hnPacketGetContent(const HnPacket& packet) {
    
    std::string data = std::to_string(packet.type);
    
    for (const std::string& all : packet.arguments)
        data += ":" + all;
    
    data += "!";
    return data;
    
};


void hnLogCout(const std::string& message, const std::string& prefix) {
    
    std::string output;
    output.reserve(message.length() + 9);
    output.append(prefix);
    output.append(message);
    output.push_back('\n');
    std::cout << output;
    std::cout.flush();
    
};

int64_t hnGetCurrentTime() {
    
    auto now = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count();
    
};

bool operator<(const HnSocketAddress& lhs, const HnSocketAddress& rhs) {
    
    return lhs.sa_family < rhs.sa_family;
    
};