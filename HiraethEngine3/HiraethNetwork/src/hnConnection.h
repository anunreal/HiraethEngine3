#ifndef HN_CONNECTION_H
#define HN_CONNECTION_H

#include <string>
#include <vector>
#include <tuple> // for hnBuildPacket macro
#include <map>
#include <any>

#ifdef HN_EXPORTS
#define HN_API __declspec(dllexport)
#else
#define HN_API __declspec(dllimport)
#endif

#define HN_ENABLE_LOG_MSG
#define HN_ENABLE_ERROR_MSG
#define HN_ENABLE_DEBUG_MSG

typedef unsigned long long HnSocketId;

enum HnDataType : uint8_t {
    HN_DATA_TYPE_NONE = 0,
    HN_DATA_TYPE_INT,
    HN_DATA_TYPE_FLOAT,
    HN_DATA_TYPE_VEC2,
    HN_DATA_TYPE_VEC3,
    HN_DATA_TYPE_VEC4,
	HN_DATA_TYPE_STRING
};

enum HnStatus : uint8_t {
    // ready but not connected yet
    HN_STATUS_READY        = 0,
    // still connected, working fine
    HN_STATUS_CONNECTED    = 1,
    // disconnected correctly
    HN_STATUS_DISCONNECTED = 2,
    // disconnected with an error
    HN_STATUS_ERROR        = 3
};

enum HnPacketType : uint8_t {
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

struct HnUdpConnection {
	// the sequence id of the next-to-send packet. If this is 0, the sequence id is not used (tcp). When a udp
	// connection is created, this should be set to 1
	uint32_t sequenceId = 1;
	// the current sequence number of the latest packet received
	uint32_t remoteSequenceId = 0;
	// the client id to the server. Used for sending packets to the server_t clientId = 0;
	uint16_t clientId = 0;
};

// wrapper struct for winsock SOCKADDR
struct HnSocketAddress {
    uint16_t sa_family = 0;
    char sa_data[14]   = {0};

	bool operator==(HnSocketAddress const& rhs) const {
		return sa_family == rhs.sa_family && sa_data == rhs.sa_data;
	};
};

struct HnSocket {
    // the platform-specific id of this socket. 0 if this is the socket of a udp remote client 
    HnSocketId id;
    // the protocol of this socket, must be the same as the other end of this connection 
    HnProtocol type;
    // the status, should be HN_STATUS_CONNECTED
    HnStatus status;
	// keeps track of the sequence ids sent over this socket
	HnUdpConnection udp;
    // the other end of the connection if this is a udp socket
    HnSocketAddress destination;

	const uint32_t BUFFER_SIZE = 4096;
	char buffer[BUFFER_SIZE];
	
    HnSocket() : id(0), type(HN_PROTOCOL_NONE), status(HN_STATUS_READY) {};
    HnSocket(const HnSocketId id, const HnProtocol type) : id(id), type(type), status(HN_STATUS_CONNECTED) {};
};

struct HnPacket {
	// the size of this packets data in bytes. Set in hnPacketGetContent or when a packet is starting to get read.
	// This does not include the packets header (size, client and sequence id, type)
	uint8_t size = 0;
    // the client that sent this packet. Only used if this is udp and the packet is sent from a client to the
	// server. For the first connection packet, this should be zero so that the server knows this is a new client
	uint16_t clientId = 0;
	// the sequence id of this packet. Only used for udp. If this is 0, the sequence id is not sent in the packet
	uint32_t sequenceId = 0;
	// the id of this packet, ie the type of this packet. Used for faster recoginition. See above for
    // packet types
    uint8_t type = 0;
	// the data string where the arguments are written back to back
	std::string data;
	// an offset in the data string that is used for parsing arguments of the packet
	uint32_t dataOffset = 0;
    HnPacket() {};
    HnPacket(const uint8_t type) : type(type) {};

	HnPacket& operator=(HnPacket const& r) {
		this->sequenceId = r.sequenceId;
		this->type       = r.type;
		this->dataOffset = r.dataOffset;
		this->data.append(r.data.c_str(), r.data.size() + 1);
		return *this;
	};
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
	// the size of the data pointer in bytes
	uint32_t dataSize = 0;
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
// tries to connect given socket to given server. If the connection fails, the status of the socket is set to
// error. The type of the socket must be set before calling this method to either udp or tcp. The type must be the
// same as the server
extern HN_API void hnSocketCreateClient(HnSocket* socket, std::string const& host, uint32_t const port);
// sets up a server on given socket. If this fails (port already in use?), the status of the socket is set to error
extern HN_API void hnSocketCreateServer(HnSocket* socket, uint32_t const port);
// closes given socket and sets its status to disconnected
extern HN_API void hnSocketDestroy(HnSocket* socket);

// sends given data over given socket. If an error occurrs, the status of the socket is updated and an 
// error message is printed
extern HN_API void hnSocketSendDataTcp(HnSocket* socket, std::string const& data);
// sends given data from socket to the destination of the socket. If an error occurs, the status of the status of
// the socket is updated and an error message is printed
extern HN_API void hnSocketSendDataUdp(HnSocket* socket, std::string const& data);
// tries to read from given socket. If the connection is interrupted, an empty string will be returned, else
// the read message will be returned as a string (with any null chars removed). You can pass a buffer to avoid
// reallocating it every time this function is called or nullptr when this function should create its own buffer.
// maxSize is the maximum number of bytes read at once (if buffer is not nullptr, maxSize should be the length 
// of given buffer. If buffer is nullptr, a new buffer will be created with length maxSize)
extern HN_API std::string hnSocketReadTcp(HnSocket* socket);
// reads all data available in the stream to socket. If an error occurrs, a message will be printed, the sockets
// status will be set to error and an empty string will be returned. Else, the read message will be returned as
// astring (with any null chars removed). You can pass a buffer to avoid reallocating it every time this function
// is called or nullptr when this function should create its own buffer.
// maxSize is the maximum number of bytes read at once (if buffer is not nullptr, maxSize should be the length 
// of given buffer. If buffer is nullptr, a new buffer will be created with length maxSize).
// from will be filled with information about the sender of the read packet. This can be set to nullptr if that
// information is not needed (client-side)
extern HN_API std::string hnSocketReadUdp(HnSocket* socket, HnSocketAddress* from);
// waits for an incoming connection on given server socket and returns the incoming sockets id. If an error occurs,
// an error message is printed, the server sockets status is set to error and 0 is returned
extern HN_API HnSocketId hnServerAcceptClientTcp(HnSocket* serverSocket);


/* PLATFORM INDEPENDANT */

// converts the int to its bytes (1, 2 or 4) and puts them in out. Out must be a size of at least that byte count.
// offset is then increased by the size of T in bytes
template<typename T>
extern HN_API void hnIntToString(T const _int, char* out, uint32_t* offset);
// converts the int to its 4 bytes and puts them in out. Out must be a size of at least 4. offset is then
// increased by 4 
extern HN_API void hnFloatToString(float const _float, char* out, uint32_t* offset);
// parses a float from 4 bytes of in, starting at offset. Offset is then increased by 4
extern HN_API void hnStringToFloat(char const* in, float* out, uint32_t* offset);
// parses an int from 4 bytes of in, starting at offset. Offset is then increased by 4
template<typename T>
extern HN_API void hnStringToInt(char const* in, T* out, uint32_t* offset);

// creates a new socket depending on the type set in the socket
extern HN_API inline void hnSocketCreate(HnSocket* hnSocket);
// sends data over given socket, depending on its type (wrapper function for hnSocketSendDataTcp,
// hnSocketSendDataUdp)
extern HN_API inline void hnSocketSend(HnSocket* socket, std::string const& msg);
// reads data from given socket. This will either call the hnSocketReadTcp or hnSocketReadUdp function, depending
// on the type of the socket
extern HN_API inline std::string hnSocketRead(HnSocket* socket);

// returns the data represented in a string, multiple arguments of the variable (vec2, ...) will be split by a 
// forward dash ('/')
extern HN_API std::string hnVariableDataToString(void const* ptr, HnDataType const dataType, uint32_t const dataSize);
// returns the variables data formatted into a string. This depends on the variables type.
// Multiple arguments of the data (vec2...) will be split by a forward dash (/)
extern HN_API inline std::string hnVariableDataToString(HnVariableInfo const* variable);
// parses the data from given string and updates the data pointer, depending on the requested type
extern HN_API void hnVariableParseFromString(void* ptr, std::string const& dataString, HnDataType const type, uint32_t const dataSize);

extern HN_API b8 hnSocketReadPacket(HnSocket* socket, HnPacket* targetPacket);

// sends a packet over given socket
extern HN_API void hnSocketSendPacket(HnSocket* socket, HnPacket* packet);
// decodes a string recieved over a socket into a packet. This will cut the string after the packet ('!') 
extern HN_API void hnPacketDecodeFromString(HnPacket* packet, HnSocket* socket, std::string& message);
// decodes all packets read in this string. If the string does not end in the last packet (unfinished 
// packet), that rest will be left in message
extern HN_API std::vector<HnPacket> hnPacketDecodeAllFromString(HnSocket* socket, std::string& message);
// converts the packet to one string, with arguments split by a colon and with an exclamation mark at the end.
// If this packet will be sent over a udp connection, socket should be the sender socket so that the client id
// and the sequence id can be stored in that packet
extern HN_API std::string hnPacketGetContent(HnPacket* packet, HnSocket const* socket = nullptr);
// creates a new packet. This will set the type (and, if udp, the sequence id) of the packet. If this packet will
// be sent over a tcp socket, connection can be left as nullptr (no sequence id). Else this should be the
// the outgoing connection. If this packet will be broadcastet (server side) using hnServerBroadcastPacket over
// udp connections, the sequence id will be set later and therefore connection does not have to be provided here
extern HN_API void hnPacketCreate(HnPacket* packet, HnPacketType const type, HnSocket* socket = nullptr);
// adds a float argument to the given packet
extern HN_API void hnPacketAddFloat(HnPacket* packet, float const _float);
// adds an int argument to the given packet
template<typename T>
extern HN_API void hnPacketAddInt(HnPacket* packet, T const _int);
// adds a string to the packet
extern HN_API void hnPacketAddString(HnPacket* packet, std::string const& _string);
// parses the next 4 bytes from the packets data as float and stores that in result
extern HN_API void hnPacketGetFloat(HnPacket* packet, float* result);
// parses the next bytes from the packets data as integer and stores that in result. The actual byte count depends
// on the integer size (8_t, 16_t, 32_t)
template<typename T>
extern HN_API void hnPacketGetInt(HnPacket* packet, T* result);
// parses a string from the packet until a '\0' character is read
extern HN_API void hnPacketGetString(HnPacket* packet, std::string* result);


// writes given message with the prefix into cout
extern HN_API void hnLogCout(std::string const& message, std::string const& prefix);

// gets a string of characters (ints) 
extern HN_API std::string hnStringGetBytes(std::string const& message);
// returns the current time (in milliseconds) since the network was set up (hnCreateNetwork)
extern HN_API inline int64_t hnGetCurrentTime();

// used for sorting the map in the server
extern HN_API inline bool operator<(HnSocketAddress const& lhs, HnSocketAddress const& rhs);


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
