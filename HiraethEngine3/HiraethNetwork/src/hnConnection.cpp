#include "hnConnection.h"
#include <iostream>
#include <ctime>


// -- platform independant
b8 HnUdpAddress::operator==(HnUdpAddress const& rhs) const {
    if(rhs.sa_family != sa_family)
        return false;

    b8 same = true;

    for(uint8_t i = 0; i < 14; ++i) {
        if(rhs.sa_data[i] != sa_data[i]) {
            same = false;
            break;
        }
    }

    return same;
};


void hnPacketCreate(HnPacket* packet, HnPacketType type, HnSocket* socket) {
	memset(packet, 0, HN_PACKET_SIZE);
	packet->dataOffset = 0;
	packet->type       = type;
		
	if(socket) {
		packet->protocolId = socket->protocolId;
		if(socket->type == HN_PROTOCOL_UDP) {
			packet->sequenceId  = ++socket->udp.sequenceId;
			packet->clientId    = socket->udp.clientId;
			packet->ackBitfield = socket->udp.ackBitfield;
			packet->ack         = socket->udp.remoteSequenceId;
		}
	}
};

void hnPacketCopy(HnPacket const* from, HnPacket* to, HnSocket* socket) {
	//	to->data       = from->data;
	memcpy(to->data, from->data, HN_PACKET_DATA_SIZE);
	to->dataOffset = from->dataOffset;
	to->type       = from->type;

	if(socket) {
		to->protocolId  = socket->protocolId;
		to->sequenceId  = ++socket->udp.sequenceId;
		to->ack         = socket->udp.ack;
		to->ackBitfield = socket->udp.ackBitfield;
		to->clientId    = socket->udp.clientId;
	} else {
		to->protocolId  = from->protocolId;
		to->sequenceId  = from->sequenceId;
		to->ack         = from->ack;
		to->ackBitfield = from->ackBitfield;
		to->clientId    = from->clientId;
	}
};

void hnPacketStoreData(HnPacket* packet, void* data, uint32_t const dataSize) {
	memcpy(&packet->data[packet->dataOffset], (char*) data, dataSize);
	packet->dataOffset += dataSize;
};

void hnPacketStoreString(HnPacket* packet, std::string const& _string) {
	uint32_t size = (uint32_t) _string.size() + 1;
	memcpy(&packet->data[packet->dataOffset], _string.c_str(), size);
	packet->dataOffset += size;
};

void hnPacketStoreFloat(HnPacket* packet, float const _float) {
	char bytes[4];
	*(float*) (bytes) = _float;
	memcpy(&packet->data[packet->dataOffset], bytes, 4);
	packet->dataOffset += 4;
};

template<>
void hnPacketStoreInt(HnPacket* packet, uint8_t const _int) {
	packet->data[packet->dataOffset++] = (char) _int;
};

template<>
void hnPacketStoreInt(HnPacket* packet, int8_t const _int) {
	packet->data[packet->dataOffset++] = (char) _int;
};

template<>
void hnPacketStoreInt(HnPacket* packet, uint16_t const _int) {
	packet->data[packet->dataOffset++] = (char) (_int >> 8);
	packet->data[packet->dataOffset++] = (char) _int;
};

template<>
void hnPacketStoreInt(HnPacket* packet, int16_t const _int) {
	packet->data[packet->dataOffset++] = (char) (_int >> 8);
	packet->data[packet->dataOffset++] = (char) _int;
};

template<>
void hnPacketStoreInt(HnPacket* packet, uint32_t const _int) {
	packet->data[packet->dataOffset++] = (char) (_int >> 24);
	packet->data[packet->dataOffset++] = (char) (_int >> 16);
	packet->data[packet->dataOffset++] = (char) (_int >> 8);
	packet->data[packet->dataOffset++] = (char) _int;
};

template<>
void hnPacketStoreInt(HnPacket* packet, int32_t const _int) {
	packet->data[packet->dataOffset++] = (char) (_int >> 24);
	packet->data[packet->dataOffset++] = (char) (_int >> 16);
	packet->data[packet->dataOffset++] = (char) (_int >> 8);
	packet->data[packet->dataOffset++] = (char) _int;
};


void hnPacketGetData(HnPacket* packet, void* out, uint32_t const dataSize) {
	memcpy((char*) out, &packet->data[packet->dataOffset], dataSize);
	packet->dataOffset += dataSize;
};

std::string hnPacketGetString(HnPacket* packet) {
	uint32_t index = 0;
	while(packet->data[packet->dataOffset + index] != '\0')
		index++;

	uint32_t oldOffset = packet->dataOffset;
	packet->dataOffset += index + 1;
	return std::string(&packet->data[oldOffset], index);
};

float hnPacketGetFloat(HnPacket* packet) {
	char bytes[4];
	memcpy(&bytes, &packet->data[packet->dataOffset], 4);
	packet->dataOffset += 4;
	return *(float*) (bytes);
};

template<>
uint8_t hnPacketGetInt(HnPacket* packet) {
	return (uint8_t) (packet->data[packet->dataOffset++]);
};

template<>
int8_t hnPacketGetInt(HnPacket* packet) {
	return (int8_t) (packet->data[packet->dataOffset++]);
};

template<>
uint16_t hnPacketGetInt(HnPacket* packet) {
	return (uint16_t) (packet->data[packet->dataOffset++] << 8 | packet->data[packet->dataOffset++]);
};

template<>
int16_t hnPacketGetInt(HnPacket* packet) {
	return (int16_t) (packet->data[packet->dataOffset++] << 8 | packet->data[packet->dataOffset++]);
};

template<>
uint32_t hnPacketGetInt(HnPacket* packet) {
	return (uint32_t) (packet->data[packet->dataOffset++] << 24 | packet->data[packet->dataOffset++] << 16 | packet->data[packet->dataOffset++] << 8 | packet->data[packet->dataOffset++]);
};

template<>
int32_t hnPacketGetInt(HnPacket* packet) {
	return (int32_t) (packet->data[packet->dataOffset++] << 24 | packet->data[packet->dataOffset++] << 16 | packet->data[packet->dataOffset++] << 8 | packet->data[packet->dataOffset++]);
};


void hnSocketGetData(HnSocket* socket, char* out, uint32_t requestedSize, HnUdpConnection* connection) {
	memset(out, 0, requestedSize);
	uint32_t outOffset = 0;
	
	while(socket->status == HN_STATUS_CONNECTED && requestedSize) {
		if(connection && connection->status == HN_STATUS_ERROR)
			return;
		
		uint32_t read = socket->buffer.currentSize - socket->buffer.offset;
		if(read > requestedSize)
			read = requestedSize;

		if(read > 0) {
			memcpy(&out[outOffset], &socket->buffer.buffer[socket->buffer.offset], read);
			socket->buffer.offset += read;
			requestedSize -= read;
		}
		
		if(requestedSize > 0)
			hnSocketReadData(socket, connection);
	};
};

void hnSocketSendPacket(HnSocket* socket, HnPacket* packet) {
	// DEV: little packet loss test
#ifdef HN_TEST_CLIENT
	if(std::rand() % 100 < 25) {
		HN_LOG("Dropped packet: " + std::to_string(packet->sequenceId));
		return;
	}
#endif
	
	char buffer[HN_PACKET_SIZE];
	memcpy(buffer, packet, HN_PACKET_SIZE);
	hnSocketSendData(socket, buffer, HN_PACKET_SIZE);
	//HN_LOG("Sent packet of type [" + std::to_string(packet->type) + "][" + std::to_string(packet->sequenceId) + "]");
};

void hnSocketSendPacket(HnSocket* socket, HnPacketType const type) {
	HnPacket packet;
	hnPacketCreate(&packet, type, socket);
	char buffer[HN_PACKET_SIZE];
	memcpy(buffer, &packet, HN_PACKET_SIZE);
	hnSocketSendData(socket, buffer, HN_PACKET_SIZE);	
	//HN_LOG("Sent packet of type [" + std::to_string(type) + "][" + std::to_string(packet.sequenceId) + "]");
};

void hnSocketSendPacketReliable(HnSocket* socket, HnPacket* packet) {
	hnSocketSendPacket(socket, packet);
	// add to reliable history
	if(socket->type == HN_PROTOCOL_UDP) {
		// find free spot in history
		int8_t index = -1;
		for(int8_t i = 0; i < HN_MAX_RELIABLE_PACKETS; ++i) {
			if(!socket->udp.reliablePacketsStatus[i]) {
				index = i;
				break;
			}
		}

		if(index == -1) {
			HN_ERROR("Reached max amount of reliable packets at a time, or having a really bad connection");
			return;
		}

		socket->udp.reliablePacketsStatus[index] = true;
		socket->udp.reliablePackets[index]       = *packet;
	}
};

void hnSocketSendPacketReliable(HnSocket* socket, HnPacketType const type) {
	HnPacket packet;
	hnPacketCreate(&packet, type, socket);
	hnSocketSendPacketReliable(socket, &packet);
};

void hnSocketReadPacket(HnSocket* socket, HnPacket* packet, HnUdpConnection* connection) {
	char buffer[HN_PACKET_SIZE];
	hnSocketGetData(socket, buffer, HN_PACKET_SIZE, connection);
	if(!connection || connection->status == HN_STATUS_CONNECTED) {
		memcpy(packet, buffer, HN_PACKET_SIZE);
		packet->dataOffset = 0;
	}
};

void hnSocketUpdateReliable(HnSocket* socket, HnAcks const ack, HnAcks const ackBitfield) {
	if(socket->type != HN_PROTOCOL_UDP)
		return;

	uint8_t sequenceOffset = 0;
	for(uint8_t i = 0; i < HN_MAX_RELIABLE_PACKETS; ++i) {
		if(socket->udp.reliablePacketsStatus[i]) {
			HnPacket* packet = &socket->udp.reliablePackets[i];

			// is the packet out of ack bounds by now (too old?)
			if(ack >= 32 && ack - 32 >= packet->sequenceId) {
				// resend packet
				uint32_t prev = packet->sequenceId;
				sequenceOffset++;
				packet->sequenceId  = socket->udp.sequenceId + sequenceOffset;
				packet->ackBitfield = socket->udp.ackBitfield;
				packet->ack         = socket->udp.remoteSequenceId;
				hnSocketSendPacket(socket, packet);
				//HN_LOG("Resending packet [" + std::to_string(prev) + "] as [" + std::to_string(packet->sequenceId) + "]");
			} else {	
				// check if we have an ack for that packet now
				int32_t offset = ack - packet->sequenceId; // if this is negative, we arent up to date with our acks -> no positive or negative ack for this packet so skip for now
				if(offset > 0 && offset < 32) {
					if(ackBitfield & (0x1 << offset)) {
						// we have an ack for this packet, remove from history
						socket->udp.reliablePacketsStatus[i] = false;
						//HN_LOG("Successfully sent reliable packet [" + std::to_string(packet->sequenceId) + "]");
					}
				}
			}
		}
	}

	socket->udp.sequenceId += sequenceOffset;
};


// -- logging

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
