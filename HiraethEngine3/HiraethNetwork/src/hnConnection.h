#pragma once
#include <string>
#include <vector>
#include <tuple> // for hnBuildPacket macro
#include <map>

#ifdef HN_EXPORTS
#define HN_API __declspec(dllexport)
#else
#define HN_API __declspec(dllimport)
#endif

#define HN_ENABLE_LOG_MSG
#define HN_ENABLE_ERROR_MSG
#define HN_ENABLE_DEBUG_MSG

typedef unsigned long long HnSocketId;

enum HnDataType {
    HN_DATA_TYPE_NONE = 0,
    HN_DATA_TYPE_INT,
    HN_DATA_TYPE_FLOAT,
    HN_DATA_TYPE_VEC2,
    HN_DATA_TYPE_VEC3,
    HN_DATA_TYPE_VEC4
};

enum HnStatus {
    // ready but not connected yet
    HN_STATUS_READY        = 0,
    // still connected, working fine
    HN_STATUS_CONNECTED    = 1,
    // disconnected correctly
    HN_STATUS_DISCONNECTED = 2,
    // disconnected with an error
    HN_STATUS_ERROR        = 3
};

enum HnPacketType {
    // another client has connected
    HN_PACKET_CLIENT_CONNECT = 1,
    // another client has disconnected
    HN_PACKET_CLIENT_DISCONNECT,
    // sent to indicate synchronization between client and server
    HN_PACKET_SYNC_REQUEST,
    // information about the local client (id...)
    HN_PACKET_CLIENT_DATA,
    // a new variable request, going to server first and then coming back to client
    HN_PACKET_VAR_NEW,
    // a variable sync, either client->server or server->client
    HN_PACKET_VAR_UPDATE,
    // sent from the server to the client (and is then returned) to check if the connection is still active
    HN_PACKET_PING_CHECK,
    // a custom packet message
    HN_PACKET_CUSTOM
};

enum HnProtocol {
    HN_PROTOCOL_NONE,
    HN_PROTOCOL_UDP,
    HN_PROTOCOL_TCP
};

// wrapper struct for winsock SOCKADDR
struct HnSocketAddress {
    uint16_t sa_family;
    char sa_data[14];
};

struct HnSocket {
    // the platform-specific id of this socket. 0 if this is the socket of a udp remote client 
    HnSocketId id;
    // the protocol of this socket, must be the same as the other end of this connection 
    HnProtocol type;
    // the status, should be HN_STATUS_CONNECTED
    HnStatus status;
    
    // the other end of the connection if this is a udp socket
    HnSocketAddress destination;
    
    HnSocket() : id(0), type(HN_PROTOCOL_NONE), status(HN_STATUS_READY) {};
    HnSocket(const HnSocketId id, const HnProtocol type) : id(id), type(type), status(HN_STATUS_CONNECTED) {};
};

struct HnPacket {
    // the id of this packet, ie the type of this packet. Used for faster recoginition. See above for
    // packet types
    uint8_t type = 0;
    std::vector<std::string> arguments;
    
    HnPacket() {};
    HnPacket(const uint8_t type) : type(type) {};
};

struct HnVariableInfo {
    // the data type of this variable
    HnDataType type = HN_DATA_TYPE_NONE;
    // the id of this variable, as defined by the server
    uint16_t id = 0;
    // how often to update this variable to the server (tick rate)
    uint16_t syncRate = 0;
    // when was the last update in ticks? Compared to the syncRate
    uint16_t lastSync = 0;
    // a pointer to data representing this variable on the local side (local client or remote client). This
    // must point to a valid memory location of the correct data type
    void* data = nullptr;
};

// maps a variable id to its info
typedef std::map<uint16_t, HnVariableInfo> HnVariableInfoMap;
// maps the name of a variable to its id as defined by the server
typedef std::map<std::string, uint16_t> HnVariableLookupMap;
// stores a pointer to some data with the variable id as key, only used for local clients
typedef std::map<uint16_t, void*> HnVariableDataMap;
// a list of custom packets sent over a socket. They are saved in the remote counter parts of each client
typedef std::vector<HnPacket> HnCustomPackets;

struct HiraethNetwork {
    HnStatus status = HN_STATUS_READY;
};

extern HiraethNetwork* hn;


/* PLATFORM DEPENDANT, see abstractions */

// sets up the HiraethNetwork api by creating a new pointer. This can be called more than once but will 
// only have an effect on the first time. This will set up wsa. If the network could not be created,
// status of the network will be set to 2
extern HN_API void hnNetworkCreate();
// cleans up the HiraethNetwork api. This only has an effect if the network is currently set up
// This must be called at the end of the program
extern HN_API void hnNetworkDestroy();

// creates a new udp socket (for both server and clients). If this fails, the status of the socket is set to error
extern HN_API void hnSocketCreateUdp(HnSocket* hnSocket);
// creates a new tcp socket (for both server and clients). If this fails, the status of the socket is set to error
extern HN_API void hnSocketCreateTcp(HnSocket* hnSocket);
// tries to connect given socket to given server. If the connection fails, the status of the socket is set to error.
// The type of the socket must be set before calling this method to either udp or tcp. The type must be the same 
// as the server
extern HN_API void hnSocketCreateClient(HnSocket* socket, const std::string& host, const uint32_t port);
// sets up a server on given socket. If this fails (port already in use?), the status of the socket is set to error
extern HN_API void hnSocketCreateServer(HnSocket* socket, const uint32_t port);
// closes given socket and sets its status to disconnected
extern HN_API void hnSocketDestroy(HnSocket* socket);

// sends given data over given socket. If an error occurrs, the status of the socket is updated and an 
// error message is printed
extern HN_API void hnSocketSendDataTcp(HnSocket* socket, const std::string& data);
// sends given data from socket to the destination of the socket. If an error occurs, the status of the status of the 
// socket is updated and an error message is printed
extern HN_API void hnSocketSendDataUdp(HnSocket* socket, const std::string& data);
// tries to read from given socket. If the connection is interrupted, an empty string will be returned, else
// the read message will be returned as a string (with any null chars removed). You can pass a buffer to avoid
// reallocating it every time this function is called or nullptr when this function should create its own buffer.
// maxSize is the maximum number of bytes read at once (if buffer is not nullptr, maxSize should be the length 
// of given buffer. If buffer is nullptr, a new buffer will be created with length maxSize)
extern HN_API std::string hnSocketReadTcp(HnSocket* socket, char* buffer, const uint32_t maxSize);
// reads all data available in the stream to socket. If an error occurrs, a message will be printed, the sockets
// status will be set to error and an empty string will be returned. Else, the read message will be returned as
// astring (with any null chars removed). You can pass a buffer to avoid reallocating it every time this function is 
// called or nullptr when this function should create its own buffer.
// maxSize is the maximum number of bytes read at once (if buffer is not nullptr, maxSize should be the length 
// of given buffer. If buffer is nullptr, a new buffer will be created with length maxSize).
// from will be filled with information about the sender of the read packet. This can be set to nullptr if that
// information is not needed (client-side)
extern HN_API std::string hnSocketReadUdp(HnSocket* socket, char* buffer, const uint32_t maxSize, HnSocketAddress* from);
// waits for an incoming connection on given server socket and returns the incoming sockets id. If an error occurs,
// an error message is printed, the server sockets status is set to error and 0 is returned
extern HN_API HnSocketId hnServerAcceptClientTcp(HnSocket* serverSocket);


/* PLATFORM INDEPENDANT */

// creates a new socket depending on the type set in the socket
extern HN_API inline void hnSocketCreate(HnSocket* hnSocket);
// sends data over given socket, depending on its type (wrapper function for hnSocketSendDataTcp, hnSocketSendDataUdp)
extern HN_API inline void hnSocketSend(HnSocket* socket, const std::string& msg);
// reads data from given socket. This will either call the hnSocketReadTcp or hnSocketReadUdp function, depending
// on the type of the socket
extern HN_API inline std::string hnSocketRead(HnSocket* socket, char* buffer, const uint32_t maxSize);

// returns the data represented in a string, multiple arguments of the variable (vec2, ...) will be split by a 
// forward dash ('/')
extern HN_API std::string hnVariableDataToString(const void* ptr, const HnDataType dataType);
// returns the variables data formatted into a string. This depends on the variables type.
// Multiple arguments of the data (vec2...) will be split by a forward dash (/)
extern HN_API inline std::string hnVariableDataToString(const HnVariableInfo* variable);
// parses the data from given string and updates the data pointer, depending on the requested type
extern HN_API void hnVariableParseFromString(void* ptr, const std::string& dataString, const HnDataType type);

// sends a packet over given socket
extern HN_API void hnSocketSendPacket(HnSocket* socket, const HnPacket& packet);
// builds a new packet with given arguments. numargs is the number of parameters of the packet (...),
// type is the type of the packet (see HnPacketType)
extern HN_API HnPacket hnPacketBuildFromParameters(uint8_t numargs, const uint8_t type, ...);
// decodes a string recieved over a socket into a packet. This will cut the string after the packet ('!') 
extern HN_API HnPacket hnPacketDecodeFromString(std::string& message);
// decodes all packets read in this string. If the string does not end in the last packet (unfinished 
// packet), that rest will be left in message
extern HN_API std::vector<HnPacket> hnPacketDecodeAllFromString(std::string& message);
// converts the packet to one string, with arguments split by a colon and with an exclamation mark at the end
extern HN_API std::string hnPacketGetContent(const HnPacket& packet);

// writes given message with the prefix into cout
extern HN_API void hnLogCout(const std::string& message, const std::string& prefix);
// returns the current time (in milliseconds) since the network was set up (hnCreateNetwork)
extern HN_API inline int64_t hnGetCurrentTime();

extern HN_API inline bool operator<(const HnSocketAddress& lhs, const HnSocketAddress& rhs);

// builds a packet of given type from given parameters. All additional arguments must be a c style string 
// (no std::string, numbers...)
#define hnPacketBuild(type, ...) (hnPacketBuildFromParameters(std::tuple_size<decltype(std::make_tuple(__VA_ARGS__))>::value, type, __VA_ARGS__))

// builds a custom message with given arguments. If isPrivate is true, this packet will only be sent to the server,
// public packets will be sent to all other clients as well
#define hnPacketBuildCustom(isPrivate, ...) (hnPacketBuildFromParameters(std::tuple_size<decltype(std::make_tuple(__VA_ARGS__))>::value + 1, HN_PACKET_CUSTOM, isPrivate ? "1" : "0", __VA_ARGS__))

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