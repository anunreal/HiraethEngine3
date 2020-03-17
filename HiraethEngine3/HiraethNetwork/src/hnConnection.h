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

enum HnSocketType {
    HN_SOCKET_TYPE_NONE,
    HN_SOCKET_TYPE_UDP,
    HN_SOCKET_TYPE_TCP
};

struct HnSocket {
    unsigned long long id;
    HnSocketType type;
    HnStatus status;
    
    HnSocket() : id(0), type(HN_SOCKET_TYPE_NONE), status(HN_STATUS_READY) {};
    HnSocket(const unsigned long long id, const HnSocketType type) : id(id), type(type), status(HN_STATUS_CONNECTED) {};
};

struct HnPacket {
    // the id of this packet, ie the type of this packet. Used for faster recoginition. See above for
    // packet types
    unsigned int type = 0;
    std::vector<std::string> arguments;
    
    HnPacket() {};
    HnPacket(const unsigned int type) : type(type) {};
};

struct HnVariableInfo {
    // the data type of this variable
    HnDataType type = HN_DATA_TYPE_NONE;
    // the id of this variable, as defined by the server
    unsigned int id = 0;
    // how often to update this variable to the server (tick rate)
    unsigned int syncRate = 0;
    // when was the last update in ticks? Compared to the syncRate
    unsigned int lastSync = 0;
    // a pointer to data representing this variable on the local side (local client or remote client). This
    // must point to a valid memory location of the correct data type
    void* data = nullptr;
};

// maps a variable id to its info
typedef std::map<unsigned int, HnVariableInfo> HnVariableInfoMap;
// maps the name of a variable to its id as defined by the server
typedef std::map<std::string, unsigned int> HnVariableLookupMap;
// stores a pointer to some data with the variable id as key, only used for local clients
typedef std::map<unsigned int, void*> HnVariableDataMap;
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
extern HN_API void hnCreateNetwork();
// cleans up the HiraethNetwork api. This only has an effect if the network is currently set up
// This must be called at the end of the program
extern HN_API void hnDestroyNetwork();
// sends given data over given socket. If an error occurrs, the status of the socket is updates and an 
// error message is printed
extern HN_API void hnSendSocketData(HnSocket* socket, const std::string& data);
// creates a new udp socket (for both server and clients). If this fails, the status of the socket is set to error
extern HN_API void hnCreateUdpSocket(HnSocket* hnSocket);
// creates a new tcp socket (for both server and clients). If this fails, the status of the socket is set to error
extern HN_API void hnCreateTcpSocket(HnSocket* hnSocket);
// closes given socket and sets its status to disconnected
extern HN_API void hnDestroySocket(HnSocket* socket);
// sets up a server on given socket. If this fails (port already in use?), the status of the socket is set to error
extern HN_API void hnCreateServerSocket(HnSocket* socket, const unsigned int port);
// tries to connect given socket to given server. If the connection fails, the status of the socket is set to error.
// This will create a new udp socket
extern HN_API void hnCreateClientSocket(HnSocket* socket, const std::string& host, const unsigned int port);
// waits for an incoming connection on given server socket and returns the incoming sockets id. If an error occurs,
// an error message is printed, the server sockets status is set to error and 0 is returned
extern HN_API unsigned long long hnServerAcceptClient(HnSocket* serverSocket);
// tries to read from given socket. If the connection is interrupted, an empty string will be returned, else
// the read message will be returned as a string (with any null chars removed). You can pass a buffer to avoid
// reallocation it every time this function is called or nullptr when this function should create its own buffer.
// maxSize is the maximum number of bytes read at once (if buffer is not nullptr, maxSize should be the length of given
// buffer. If buffer is nullptr, a new buffer will be created with length maxSize)
extern HN_API std::string hnReadFromSocket(HnSocket* socket, char* buffer, const int maxSize);


/* PLATFORM INDEPENDANT */

// creates a new socket depending on the type set in the socket
extern HN_API void hnCreateSocket(HnSocket* hnSocket);

// returns the data represented in a string, multiple arguments of the variable (vec2, ...) will be split by a 
// forward dash ('/')
extern HN_API std::string hnVariableDataToString(const void* ptr, const HnDataType dataType);
// returns the variables data formatted into a string. This depends on the variables type.
// Multiple arguments of the data (vec2...) will be split by a forward dash (/)
extern HN_API inline std::string hnVariableDataToString(const HnVariableInfo* variable);
// parses the data from given string and updates the data pointer, depending on the requested type
extern HN_API void hnParseVariableString(void* ptr, const std::string& dataString, const HnDataType type);

// sends a packet over given socket
extern HN_API void hnSendPacket(HnSocket* socket, const HnPacket& packet);
// builds a new packet with given arguments. numargs is the number of parameters of the packet (...),
// type is the type of the packet (see HnPacketType)
extern HN_API HnPacket hnBuildPacketFromParameters(int numargs, const unsigned int type, ...);
// decodes a string recieved over a socket into a packet. This will cut the string after the packet ('!') 
extern HN_API HnPacket hnDecodePacket(std::string& message);
// decodes all packets read in this string. If the string does not end in the last packet (unfinished 
// packet), that rest will be left in message
extern HN_API std::vector<HnPacket> hnDecodePackets(std::string& message);
// converts the packet to one string, with arguments split by a colon and with an exclamation mark at the end
extern HN_API std::string hnGetPacketContent(const HnPacket& packet);

// writes given message with the prefix into cout
extern HN_API void hnLogCout(const std::string& message, const std::string& prefix);
// returns the current time (in milliseconds) since the network was set up (hnCreateNetwork)
extern HN_API inline long long hnGetCurrentTime();

// builds a packet of given type from given parameters. All additional arguments must be a c style string 
// (no std::string, numbers...)
#define hnBuildPacket(type, ...) (hnBuildPacketFromParameters(std::tuple_size<decltype(std::make_tuple(__VA_ARGS__))>::value, type, __VA_ARGS__))

// builds a custom message with given arguments. If isPrivate is true, this packet will only be sent to the server,
// public packets will be sent to all other clients as well
#define hnBuildCustomPacket(isPrivate, ...) (hnBuildPacketFromParameters(std::tuple_size<decltype(std::make_tuple(__VA_ARGS__))>::value + 1, HN_PACKET_CUSTOM, isPrivate ? "1" : "0", __VA_ARGS__))

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