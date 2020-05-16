#ifndef HN_CONNECTION_H
#define HN_CONNECTION_H

#include <string>

#ifdef HN_EXPORTS
#define HN_API __declspec(dllexport)
#else
#define HN_API __declspec(dllimport)
#endif

#define HN_ENABLE_LOG_MSG
#define HN_ENABLE_ERROR_MSG
#define HN_ENABLE_DEBUG_MSG

#define HN_BUFFER_SIZE      4096
#define HN_PACKET_SIZE      256
#define HN_PACKET_DATA_SIZE 244 // HN_PACKET_SIZE - 12

// winsock socket ids
typedef unsigned long long HnSocketId;
// The protocol id should be unique for a program. This ensures that 
typedef uint8_t HnProtocolId;
// the sequence id is used to identify udp packets
typedef uint32_t HnSequenceId;
// stores the last sequence ids received over a socket
typedef uint32_t HnAcks;
// unique client id
typedef uint16_t HnClientId;

typedef enum HnProtocolType : uint8_t {	
    HN_PROTOCOL_NONE,
	HN_PROTOCOL_UDP,
	HN_PROTOCOL_TCP
} HnProtocolType;

typedef enum HnConnectionStatus : uint8_t {
    HN_STATUS_WAITING,
    HN_STATUS_CONNECTED,
    HN_STATUS_ERROR,
	HN_STATUS_CLOSED
} HnConnectionStatus;

typedef enum HnPacketType : uint8_t {
    HN_PACKET_NONE,
	HN_PACKET_CLIENT_CONNECT,
	HN_PACKET_CLIENT_DISCONNECT,
	HN_PACKET_SYNC_REQUEST,
	HN_PACKET_CLIENT_DATA,
	HN_PACKET_PING_CHECK,
} HnPacketType;

struct HnSocketBuffer {
	uint32_t currentSize   = 0;
	uint32_t offset        = 0;
	char buffer[HN_BUFFER_SIZE] = { 0 };
};

struct HnUdpConnection {
	HnConnectionStatus status      = HN_STATUS_WAITING;
    uint16_t           sa_family   = 0;
    char               sa_data[14] = {0};

	HnSequenceId sequenceId        = 0;
	HnSequenceId remoteSequenceId  = 0;
	HnClientId   clientId          = 0;
	HnAcks       acks              = 0;
};

struct HnSocket {
	HnSocketId         id         = 0;	
	HnProtocolId       protocolId = 0;
	HnProtocolType     type       = HN_PROTOCOL_NONE; 
	HnConnectionStatus status     = HN_STATUS_WAITING;
	HnSocketBuffer     buffer;
	
	HnUdpConnection    udp;

	int64_t lastPacketTime = 0; // time of the last packet arrival, in ms.
	int64_t pingCheck      = 0; // the time we started to sent the ping check since the program start in ms (if the value is positive) or time since the last successfull ping check in ms (negative)
	uint16_t ping          = 0; // the current ping of this socket in ms
};

struct HnPacket {
	HnProtocolId protocolId = 0;              //   1
	HnSequenceId sequenceId = 0;              // + 4
	HnAcks       acks       = 0;              // + 4
	HnClientId   clientId   = 0;              // + 2
	HnPacketType type       = HN_PACKET_NONE; // + 1 = 12
	
	char data[HN_PACKET_DATA_SIZE] = { 0 };
	uint8_t dataOffset = 0;
};


// -- platform dependant

// creates a client socket (tcp or udp) and tries to connect to the specified server. If the connection succeeds,
// this will instantly sync itself with the server and then return. If no connection could be established, an error
// message is printed and the sockets status is set to error
extern HN_API void hnSocketCreateClient(HnSocket* socket, std::string const& host, uint32_t const ort);
extern HN_API void hnSocketCreateServer(HnSocket* socket, uint32_t const port);
extern HN_API void hnSocketDestroy(HnSocket* socket);

extern HN_API void hnSocketSendData(HnSocket* socket, char const* data, uint32_t dataSize);
extern HN_API void hnSocketReadData(HnSocket* socket, HnUdpConnection* connection = nullptr);
extern HN_API HnSocketId hnServerAcceptClient(HnSocket* serverSocket);

// returns the current time since the program start in milliseconds.
extern HN_API int64_t hnPlatformGetCurrentTime();
// sleeps the current thread for given time (in ms)
extern HN_API void hnPlatformSleep(uint32_t const time);


// -- platform independant

extern HN_API void hnPacketCreate(HnPacket* packet, HnPacketType const type, HnSocket* connection = nullptr);
extern HN_API inline void hnPacketStoreData(HnPacket* packet, void* data, uint32_t const dataSize);
extern HN_API inline void hnPacketStoreString(HnPacket* packet, std::string const& _string);
extern HN_API inline void hnPacketStoreFloat(HnPacket* packet, float const _float);
template<typename T>
extern HN_API inline void hnPacketStoreInt(HnPacket* packet, T const _int);

extern HN_API void        hnPacketGetData(HnPacket* packet, void* out, uint32_t const dataSize);
extern HN_API std::string hnPacketGetString(HnPacket* packet);
extern HN_API float       hnPacketGetFloat(HnPacket* packet);
template<typename T>
extern HN_API T           hnPacketGetInt(HnPacket* packet);

extern HN_API void hnSocketGetData(HnSocket* socket, char* out, uint32_t requestedSize, HnUdpConnection* connection = nullptr);
extern HN_API void hnSocketSendPacket(HnSocket* socket, HnPacket* packet);
extern HN_API void hnSocketSendPacket(HnSocket* socket, HnPacketType const type);
extern HN_API void hnSocketReadPacket(HnSocket* socket, HnPacket* packet, HnUdpConnection* connection = nullptr);


// -- logging

// writes given message with the prefix into cout
extern HN_API void hnLogCout(std::string const& message, std::string const& prefix);

#ifdef HN_ENABLE_DEBUG_MSG
#define HN_DEBUG(msg) hnLogCout(msg, "[DEBUG]: ")
#else
#define HN_DEBUG(msg) {}
#endif

#ifdef HN_ENABLE_LOG_MSG
#define HN_LOG(msg) hnLogCout(msg, "[LOG  ]: ")
#else
#define HN_LOG(msg) {}
#endif

#ifdef HN_ENABLE_ERROR_MSG
#define HN_ERROR(msg) hnLogCout(msg, "[ERROR]: ")
#else
#define HN_ERROR(msg) {}
#endif


#endif
