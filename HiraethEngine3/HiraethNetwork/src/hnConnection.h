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

#define HN_BUFFER_SIZE          4096
#define HN_PACKET_SIZE           256
#define HN_PACKET_DATA_SIZE      240 // HN_PACKET_SIZE - 16
#define HN_MAX_RELIABLE_PACKETS    5

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
typedef bool b8;

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
    HN_PACKET_MESSAGE,
} HnPacketType;

struct HnPacket {
    HnProtocolId protocolId  = 0;              //   1
    HnSequenceId sequenceId  = 0;              // + 4
    HnAcks       ack         = 0;              // + 4
    HnAcks       ackBitfield = 0;              // + 4
    HnClientId   clientId    = 0;              // + 2
    HnPacketType type        = HN_PACKET_NONE; // + 1 = 16
    
    char data[HN_PACKET_DATA_SIZE] = { 0 };
    uint8_t dataOffset = 0;
};

struct HnSocketBuffer {
    uint32_t currentSize   = 0;
    uint32_t offset        = 0;
    char buffer[HN_BUFFER_SIZE] = { 0 };
};

struct HnUdpAddress {
    uint16_t           sa_family   = 0;
    char               sa_data[14] = {0};

    b8 operator==(HnUdpAddress const& rhs) const;
};

struct HnUdpConnection {
    HnConnectionStatus status      = HN_STATUS_WAITING;
    HnUdpAddress       address;
    
    HnSequenceId sequenceId        = 0;
    HnSequenceId remoteSequenceId  = 0;
    HnClientId   clientId          = 0;
    HnAcks       ack               = 0; // the last packet that received an ack
    HnAcks       ackBitfield       = 0; // a bitfield of the last 32 sequences where 1 means we received an ack
    
    // status of the packets in the reliablePackets array. If this is false, the packet at that index is invalid
    // (spot) is free. If a new reliable packet is sent, the status will be set to true until we are sure that that
    // packet arrived.
    b8 reliablePacketsStatus[HN_MAX_RELIABLE_PACKETS] = { false };  
    HnPacket reliablePackets[HN_MAX_RELIABLE_PACKETS] = { 0 };
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


// -- platform dependant

// creates a client socket (tcp or udp) and tries to connect to the specified server. If the connection succeeds,
// this will instantly sync itself with the server and then return. If no connection could be established, an error
// message is printed and the sockets status is set to error
extern HN_API void hnSocketCreateClient(HnSocket* socket, std::string const& host, uint32_t const ort);
// creates a server socket (tcp or udp). If the setup fails (i.e. the port is already in use), an error message
// is printed and the sockets status is set to error.
extern HN_API void hnSocketCreateServer(HnSocket* socket, uint32_t const port);
// destroys the given socket and sets all its data to 0. This works for servers and clients
extern HN_API void hnSocketDestroy(HnSocket* socket);

// sends given data over the given socket. This works for both udp and tcp
extern HN_API void hnSocketSendData(HnSocket* socket, char const* data, uint32_t dataSize);
// tries to read data from given socket and stores the read data in the sockets buffer. If this is a udp server
// socket, connection will be set to the incoming connection address
extern HN_API void hnSocketReadData(HnSocket* socket, HnUdpConnection* connection = nullptr);
// TCP: Accepts a connecting client and returns the socket id of that client. This blocks until a client connected
extern HN_API HnSocketId hnServerAcceptClient(HnSocket* serverSocket);

// returns the current time since the program start in milliseconds.
extern HN_API int64_t hnPlatformGetCurrentTime();
// sleeps the current thread for given time (in ms)
extern HN_API void hnPlatformSleep(uint32_t const time);


// -- platform independant

// creates a new paccket of given type. If socket is not nullptr and socket is a udp connection, the packets
// udp data (client id, sequence id and acks) will be set according to the sockets udp data
extern HN_API void hnPacketCreate(HnPacket* packet, HnPacketType const type, HnSocket* socket = nullptr);
// copies the content of the from packet into to. If socket is not nullptr, the sequence id, client id and acks
// are updated from the socket into to.
extern HN_API void hnPacketCopy(HnPacket const* from, HnPacket* to, HnSocket* socket = nullptr);
// copies data into the packets buffer. dataSize is the amount of bytes to copy from data into the buffer.
// Increases the packets dataOffset by dataSize
extern HN_API inline void hnPacketStoreData(HnPacket* packet, void* data, uint32_t const dataSize);
// copies a (c-style) string into the packets buffer. Increases the packets dataOffset by _string.size() + 1
extern HN_API inline void hnPacketStoreString(HnPacket* packet, std::string const& _string);
// copies the four bytes of the given float into the packets buffer. Increases the packets dataOffset by 4
extern HN_API inline void hnPacketStoreFloat(HnPacket* packet, float const _float);
// copies given int into the packets buffer. This works with 8, 16 and 32 bit ints. Increases the packets
// dataOffset by the size of that int
template<typename T>
extern HN_API inline void hnPacketStoreInt(HnPacket* packet, T const _int);

// copies dataSize amount of bytes from the packets buffer into out
extern HN_API void        hnPacketGetData(HnPacket* packet, void* out, uint32_t const dataSize);
// tries to find the next \0 char in the packets buffer and returns the byte chunk from the current packets
// dataOffset to that char 
extern HN_API std::string hnPacketGetString(HnPacket* packet);
// returns the next four bytes of the packets data, interpreted as float
extern HN_API float       hnPacketGetFloat(HnPacket* packet);
// returns an int of specified size
template<typename T>
extern HN_API T           hnPacketGetInt(HnPacket* packet);

// tries to read requestedSize bytes from the sockets buffer. If the buffer does not have enough valid data, this
// waits until new data can be read from the socket and then refills the buffer. 
extern HN_API void hnSocketGetData(HnSocket* socket, char* out, uint32_t requestedSize, HnUdpConnection* connection = nullptr);
// sends a packet over the socket (tcp or udp) once. If the socket is udp, this packet may get dropped
extern HN_API void hnSocketSendPacket(HnSocket* socket, HnPacket* packet);
// creates a packet of given type without arguments and sends it over the socket. If the socket is udp, this packet
// may get dropped
extern HN_API void hnSocketSendPacket(HnSocket* socket, HnPacketType const type);
// if this is tcp, this simply sends the packet. If this is udp, the packet is sent over the socket and added to
// the sockets reliable history. If we dont get an ack for this packet in the next 32 packets from the server, the
// packet is resent with a new sequence id. This is done in the sockets update function
extern HN_API void hnSocketSendPacketReliable(HnSocket* socket, HnPacket* packet);
// Creates a packet of given type without arguments. If this is tcp, this then simply sends that packet. If this
// is udp, the packet is sent over the socket and added to the sockets reliable history. If we dont get an ack
// for this packet in the next 32 packets from the server, the packet is resent with a new sequence id. This is
// done in the sockets update function
extern HN_API inline void hnSocketSendPacketReliable(HnSocket* socket, HnPacketType const type);
// tries to read HN_PACKET_SIZE bytes from the socket and build a packet from that data. If connection is not
// nullptr and the socket is udp, connection will be set to the address of the sender of the packet. This is used
// for a server socket.
extern HN_API void hnSocketReadPacket(HnSocket* socket, HnPacket* packet, HnUdpConnection* connection = nullptr);
// Only important for udp sockets. This checks wether we have acks for the reliable packets now. If so, the status
// of that packet in the reliable packet is cleared. If we dont have an ack, and the sequence id of the socket
// is 32 bigger than the sequence id of the original packet, we resent that packet with an updated sequence id.
// This should be called after every packet we receive over a socket (everytime the acks are updated)
extern HN_API void hnSocketUpdateReliable(HnSocket* socket, HnAcks const ack, HnAcks const ackBitfield);


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
