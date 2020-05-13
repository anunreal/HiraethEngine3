#include "hnConnection.h"
#include <iostream>
#include <stdarg.h>
#include <chrono>
#include <ctime>

HiraethNetwork* hn = nullptr;
std::chrono::time_point<std::chrono::steady_clock> startTime;


template<>
void hnIntToString(uint8_t const _int, char* out, uint32_t* offset) {
	*out = _int;
	offset++;
};

template<>
void hnIntToString(int8_t const _int, char* out, uint32_t* offset) {
	*out = _int;
	offset++;
};

template<>
void hnIntToString(uint16_t const _int, char* out, uint32_t* offset) {
	out[(*offset)++] = (char) (_int >> 8);
	out[(*offset)++] = (char) _int;
};

template<>
void hnIntToString(int16_t const _int, char* out, uint32_t* offset) {
	out[(*offset)++] = (char) (_int >> 8);
	out[(*offset)++] = (char) _int;
};

template<>
void hnIntToString(uint32_t const _int, char* out, uint32_t* offset) {
	out[(*offset)++] = (char) (_int >> 24);
	out[(*offset)++] = (char) (_int >> 16);
	out[(*offset)++] = (char) (_int >> 8);
	out[(*offset)++] = (char) _int;
};

template<>
void hnIntToString(int32_t const _int, char* out, uint32_t* offset) {
	out[(*offset)++] = (char) (_int >> 24);
	out[(*offset)++] = (char) (_int >> 16);
	out[(*offset)++] = (char) (_int >> 8);
	out[(*offset)++] = (char) _int;
};


void hnFloatToString(float const _float, char* out) {
	char bytes[4];
	*(float*) (bytes) = _float;
	memcpy(out, bytes, 4);
};

void hnStringToFloat(char const* in, float* out, uint32_t* offset) {
	char bytes[4];
	memcpy(bytes, &in[*offset], 4);
	*offset += 4;
	*out = *(float*) (bytes);
};

template<>
void hnStringToInt(char const* in, uint8_t* out, uint32_t* offset) {
	*out = (uint8_t) in[*offset];
	*offset += 1;
};

template<>
void hnStringToInt(char const* in, uint16_t* out, uint32_t* offset) {
	char bytes[2];
	memcpy(bytes, &in[*offset], 2);
	*out = (bytes[0] << 8) | bytes[1];
	*offset += 2;
};

template<>
void hnStringToInt(char const* in, uint32_t* out, uint32_t* offset) {
	char bytes[4];
	memcpy(bytes, &in[*offset], 4);
	*out = (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];
	*offset += 4;
};


void hnSocketCreate(HnSocket* socket) {    
    if(socket->type == HN_PROTOCOL_UDP)
        hnSocketCreateUdp(socket);
    else if(socket->type == HN_PROTOCOL_TCP)
        hnSocketCreateTcp(socket);    
};

void hnSocketSend(HnSocket* socket, std::string const& msg) {    
    if(socket->type == HN_PROTOCOL_UDP)
        hnSocketSendDataUdp(socket, msg);
    else if(socket->type == HN_PROTOCOL_TCP)
        hnSocketSendDataTcp(socket, msg);
};

std::string hnSocketRead(HnSocket* socket) {    
	std::string msg;
    if(socket->type == HN_PROTOCOL_UDP)
        msg = hnSocketReadUdp(socket nullptr);
    else if(socket->type == HN_PROTOCOL_TCP)
        msg = hnSocketReadTcp(socket);
    
    return msg;    
};


std::string hnVariableDataToString(void const* ptr, HnDataType const dataType, uint32_t const dataSize) {    
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

	case HN_DATA_TYPE_STRING:
		//memcpy(data.c_str(), ptr, dataSize);
		data = std::string((char*) ptr, dataSize);
		break;
    }
    
    return data;    
};

std::string hnVariableDataToString(HnVariableInfo const* info) {
    return hnVariableDataToString(info->data, info->type, info->dataSize);
};

void hnVariableParseFromString(void* ptr, std::string const& dataString, HnDataType const type, uint32_t const dataSize) {   
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

	case HN_DATA_TYPE_STRING: {
	    memset(ptr, '\0', dataSize);
		memcpy(ptr, &dataString[0], dataString.size());
		break;
	}
    }	
};

void hnSocketReadPacket() {

};

void hnSocketSendPacket(HnSocket* socket, HnPacket* packet) {    
    if (socket->status != HN_STATUS_CONNECTED)
        return;
	
    hnSocketSend(socket, hnPacketGetContent(packet, socket));
    if(packet->type != HN_PACKET_VAR_UPDATE) {
        HN_LOG("Sent Packet of type [" + std::to_string(packet->type) + "]:");
		//hnStringPrintBytes(hnPacketGetContent(packet));
	}
};

void hnPacketDecodeFromString(HnPacket* packet, HnSocket* socket, std::string& message) {
	uint8_t bytes;
	hnStringToInt(message.data(), &bytes, &packet->dataOffset);

	if(socket->type == HN_PROTOCOL_UDP) {
		//hnStringToInt(message.data, &packet->clientId, &packet->dataOffset);
		// parse sequence id
		hnStringToInt(message.data(), &packet->sequenceId, &packet->dataOffset);
		if(packet->sequenceId > socket->udp.remoteSequenceId)
			socket->udp.remoteSequenceId = packet->sequenceId;
	}

	hnStringToInt(message.data(), &packet->type, &packet->dataOffset);
	packet->data = message.substr(packet->dataOffset, bytes);
	packet->dataOffset = 0;
	message = message.substr(bytes);
};

std::vector<HnPacket> hnPacketDecodeAllFromString(HnSocket* socket, std::string& message) {    
    std::vector<HnPacket> packets;
	while(message.find(-1) != std::string::npos) {
		HnPacket* packet = &packets.emplace_back();
		hnPacketDecodeFromString(packet, socket, message);
	}
		
    return packets;    
};

std::string hnPacketGetContent(HnPacket* packet, HnSocket const* socket) {
	std::string data;
	uint32_t offset = 0;
	uint32_t size = 2;
	if(socket->type == HN_PROTOCOL_UDP)
		size += 6;

	data.resize(size);

	packet->size = packet->data.size(); 
	hnIntToString(packet->size, data.data(), &offset);
	
	if(socket && socket->type == HN_PROTOCOL_UDP) {
		hnIntToString(packet->clientId, data.data(), &offset); 
		offset += 2;
		hnIntToString(packet->sequenceId, data.data(), &offset);
		offset += 4;
	}
		
	hnIntToString(packet->type, data.data(), &offset);
	offset += 1;

	if(packet->data.size() > 0) {
		//data.append(packet->data);
		data.append(packet->data);
		offset += (uint32_t) packet->data.size();
	}
	
	//data.push_back(-1);
	return data;    
};


void hnPacketCreate(HnPacket* packet, HnPacketType const type, HnSocket* socket) {
	if(socket && socket->type == HN_PROTOCOL_UDP) {
		packet->sequenceId = socket->udp.sequenceId++;
		packet->clientId   = socket->udp.clientId;
	}
		
	packet->type = type;
};

void hnPacketAddFloat(HnPacket* packet, float const _float) {
	char bytes[4];
	hnFloatToString(_float, bytes, &packet->dataOffset);
	//packet->data.append(bytes);
	packet->data += std::string(bytes);
};

template<>
void hnPacketAddInt(HnPacket* packet, uint8_t const _int) {
	char bytes;
	hnIntToString(_int, &bytes, &packet->dataOffset);
	packet->data.push_back(bytes);
};

template<>
void hnPacketAddInt(HnPacket* packet, uint16_t const _int) {
	char bytes[2];
	hnIntToString(_int, bytes, &packet->dataOffset);
	//packet->data += std::string(bytes);
	packet->data.append(bytes, 2);
};

template<>
void hnPacketAddInt(HnPacket* packet, uint32_t const _int) {
	char bytes[4];
	hnIntToString(_int, bytes, &packet->dataOffset);
	packet->data.append(bytes, 4);
	//packet->data += std::string(bytes);
};

void hnPacketAddString(HnPacket* packet, std::string const& _string) {
	packet->data.append(_string.c_str(), _string.size() + 1);
	//packet->data += _string;
	//packet->data.push_back('\0');
};

void hnPacketGetFloat(HnPacket* packet, float* result) {
	hnStringToFloat(packet->data.data(), result, &packet->dataOffset);
};

template<>
void hnPacketGetInt(HnPacket* packet, uint8_t* result) {
	hnStringToInt(packet->data.data(), result, &packet->dataOffset);
};

template<>
void hnPacketGetInt(HnPacket* packet, uint16_t* result) {
	hnStringToInt(packet->data.data(), result, &packet->dataOffset);
};

template<>
void hnPacketGetInt(HnPacket* packet, uint32_t* result) {
	hnStringToInt(packet->data.data(), result, &packet->dataOffset);
};

void hnPacketGetString(HnPacket* packet, std::string* result) {
	size_t end = packet->data.find('\0', packet->dataOffset);
	//result->clear();
	//std::string arg = ; 
	result->assign(packet->data.substr(packet->dataOffset, end - packet->dataOffset));
	packet->dataOffset = (uint32_t) (end + 1);
};


void hnLogCout(std::string const& message, std::string const& prefix) {
    // get time
    auto time = std::time(nullptr);
    struct tm buf;
    localtime_s(&buf, &time);
    char timeBuf[11];
    std::strftime(timeBuf, 11, "[%H:%M:%S]", &buf);
    std::string timeString(timeBuf, 10);
    
    std::string output;
    output.reserve(20 + message.size());
    output.append(timeString);
    output.append(prefix);
    output.push_back(' ');
    output.append(message);
	output.push_back('\n');
    
    std::cout << output;
    std::cout.flush();   
};

std::string hnStringGetBytes(std::string const& message) {
	std::string string;
	string.reserve(message.size() * 3);
	
	for(char all : message) {
		int i = (int) all;
		if(i < 10 && i >= 0)
			string.append("00");
		else if(i < 100)
			string.append("0");
		string.append(std::to_string(i) + ' ');
	}

	return string;
};

int64_t hnGetCurrentTime() {    
    auto now = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count();    
};

bool operator<(const HnSocketAddress& lhs, const HnSocketAddress& rhs) {    
    return lhs.sa_family < rhs.sa_family;    
};
