/*
 * Socket.h
 *
 *  Created on: 7/08/2015
 *      Author: michael
 */

#ifndef SOCKET_H_
#define SOCKET_H_

#include "ZmqError.h"
#include "Frame.h"
#include "Entity.h"
#include <memory>
#include <zmq.h>
#include <errno.h>
#include <tuple>
#include <string.h>
#include "shared_buffer.h"
#include <type_traits>
#include <unordered_map>

using namespace mega_tools;

namespace zmq_cpp {

enum class SocketType {
    NULL_SKT = -1,
    PAIR = ZMQ_PAIR,
    PUB = ZMQ_PUB,
    SUB = ZMQ_SUB,
    REQ = ZMQ_REQ,
    REP = ZMQ_REP,
    DEALER = ZMQ_DEALER,
    ROUTER = ZMQ_ROUTER,
    PULL = ZMQ_PULL,
    PUSH = ZMQ_PUSH,
    XPUB = ZMQ_XPUB,
    XSUB = ZMQ_XSUB,
    STREAM = ZMQ_STREAM,
    SERVER = ZMQ_SERVER,
    CLIENT = ZMQ_CLIENT
};

enum class SocketOption {
    AFFINITY = ZMQ_AFFINITY,
    IDENTITY = ZMQ_IDENTITY,
    SUBSCRIBE = ZMQ_SUBSCRIBE,
    UNSUBSCRIBE = ZMQ_UNSUBSCRIBE,
    RATE = ZMQ_RATE,
    RECOVERY_IVL = ZMQ_RECOVERY_IVL,
    SNDBUF = ZMQ_SNDBUF,
    RCVBUF = ZMQ_RCVBUF,
    RCVMORE = ZMQ_RCVMORE,
    FD = ZMQ_FD,
    EVENTS = ZMQ_EVENTS,
    TYPE = ZMQ_TYPE,
    LINGER = ZMQ_LINGER,
    RECONNECT_IVL = ZMQ_RECONNECT_IVL,
    BACKLOG = ZMQ_BACKLOG,
    RECONNECT_IVL_MAX = ZMQ_RECONNECT_IVL_MAX,
    MAXMSGSIZE = ZMQ_MAXMSGSIZE,
    SNDHWM = ZMQ_SNDHWM,
    RCVHWM = ZMQ_RCVHWM,
    MULTICAST_HOPS = ZMQ_MULTICAST_HOPS,
    RCVTIMEO = ZMQ_RCVTIMEO,
    SNDTIMEO = ZMQ_SNDTIMEO,
    LAST_ENDPOINT = ZMQ_LAST_ENDPOINT,
    ROUTER_MANDATORY = ZMQ_ROUTER_MANDATORY,
    TCP_KEEPALIVE = ZMQ_TCP_KEEPALIVE,
    TCP_KEEPALIVE_CNT = ZMQ_TCP_KEEPALIVE_CNT,
    TCP_KEEPALIVE_IDLE = ZMQ_TCP_KEEPALIVE_IDLE,
    TCP_KEEPALIVE_INTVL = ZMQ_TCP_KEEPALIVE_INTVL,
    IMMEDIATE = ZMQ_IMMEDIATE,
    XPUB_VERBOSE = ZMQ_XPUB_VERBOSE,
    ROUTER_RAW = ZMQ_ROUTER_RAW,
    IPV6 = ZMQ_IPV6,
    MECHANISM = ZMQ_MECHANISM,
    PLAIN_SERVER = ZMQ_PLAIN_SERVER,
    PLAIN_USERNAME = ZMQ_PLAIN_USERNAME,
    PLAIN_PASSWORD = ZMQ_PLAIN_PASSWORD,
    CURVE_SERVER = ZMQ_CURVE_SERVER,
    CURVE_PUBLICKEY = ZMQ_CURVE_PUBLICKEY,
    CURVE_SECRETKEY = ZMQ_CURVE_SECRETKEY,
    CURVE_SERVERKEY = ZMQ_CURVE_SERVERKEY,
    PROBE_ROUTER = ZMQ_PROBE_ROUTER,
    REQ_CORRELATE = ZMQ_REQ_CORRELATE,
    REQ_RELAXED = ZMQ_REQ_RELAXED,
    CONFLATE = ZMQ_CONFLATE,
    ZAP_DOMAIN = ZMQ_ZAP_DOMAIN,
    ROUTER_HANDOVER = ZMQ_ROUTER_HANDOVER,
    TOS = ZMQ_TOS,
    CONNECT_RID = ZMQ_CONNECT_RID,
    GSSAPI_SERVER = ZMQ_GSSAPI_SERVER,
    GSSAPI_PRINCIPAL = ZMQ_GSSAPI_PRINCIPAL,
    GSSAPI_SERVICE_PRINCIPAL = ZMQ_GSSAPI_SERVICE_PRINCIPAL,
    GSSAPI_PLAINTEXT = ZMQ_GSSAPI_PLAINTEXT,
    HANDSHAKE_IVL = ZMQ_HANDSHAKE_IVL,
    SOCKS_PROXY = ZMQ_SOCKS_PROXY,
    XPUB_NODROP = ZMQ_XPUB_NODROP,
    BLOCKY = ZMQ_BLOCKY,
    XPUB_MANUAL = ZMQ_XPUB_MANUAL,
    XPUB_WELCOME_MSG = ZMQ_XPUB_WELCOME_MSG,
    STREAM_NOTIFY = ZMQ_STREAM_NOTIFY,
    INVERT_MATCHING = ZMQ_INVERT_MATCHING,
    HEARTBEAT_IVL = ZMQ_HEARTBEAT_IVL,
    HEARTBEAT_TTL = ZMQ_HEARTBEAT_TTL,
    HEARTBEAT_TIMEOUT = ZMQ_HEARTBEAT_TIMEOUT,
    XPUB_VERBOSE_UNSUBSCRIBE = ZMQ_XPUB_VERBOSE_UNSUBSCRIBE,
    CONNECT_TIMEOUT = ZMQ_CONNECT_TIMEOUT,
    TCP_RETRANSMIT_TIMEOUT = ZMQ_TCP_RETRANSMIT_TIMEOUT,
};


struct Options : public std::tuple<SocketOption, const void*, size_t> {
    Options(SocketOption so, const void *data, size_t len) :
        std::tuple<SocketOption, const void*, size_t>(so, data, len){
    }

    inline SocketOption getOption() {
        return std::get<0>(*this);
    }

    inline const void* getValue() {
        return std::get<1>(*this);
    }

    inline size_t getSize() {
        return std::get<2>(*this);
    }
};

class Socket {

friend class Context;
friend class PollItem;
friend class RouterSocket;

public:
    Socket() : socket(nullptr, &deleteSocket), type(SocketType::NULL_SKT){}
    virtual ~Socket() {}

    void close() {
        zmq_close(socket.get());
    }

    inline SocketType getSocketType() {
        return type;
    }

    inline int setSockOpt(SocketOption &option, const void *optVal,
            size_t optValLen) {
        int error = zmq_setsockopt(this->socket.get(),
                static_cast<int>(option), optVal, optValLen);
        test(error);
        return error;
    }

    inline int setSockOpt(SocketOption &&option, const void *optVal,
            size_t optValLen) {
        return setSockOpt(option, optVal, optValLen);
    }

    inline int setSockOpt(Options &o) {
        return setSockOpt(o.getOption(), o.getValue(), o.getSize());
    }

    inline int setSockOpt(Options &&o) {
        return setSockOpt(o.getOption(), o.getValue(), o.getSize());
    }

    inline int connect(std::string &address) {
        return connect(address.c_str());
    }

    inline int connect(const char *address) {
        int error = zmq_connect(this->socket.get(), address);
        test(__FUNCTION__, error);
        return error;
    }

    inline int bind(std::string &address) {
       return bind(address.c_str());
    }

    inline int bind(const char *address) {
        int error = zmq_bind(this->socket.get(), address);
        test(error);
        return error;
    }

    inline int receive(SharedBuffer<> &buffer, int flags = 0) {
        int error = zmq_recv(this->socket.get(),
                buffer.get(), buffer.size, flags);
        test(error);
        return error;
    }

    inline Message receive() {
        Message message;
        bool more = false;
        do {
            Frame m;
            int error = zmq_msg_recv(m.getRawMessage(),
                    this->socket.get(), 0);
            test(__FUNCTION__, error);
            more = m.more();
            message.addFrame(m);
        }
        while(more);

        return message;
    }

    inline int send(SharedBuffer<> &buffer, int flags = 0) {
        int error = zmq_send(this->socket.get(),
                buffer.get(), buffer.size, flags);
        test(__FUNCTION__, error);
        return error;
    }

    inline int sendString(std::string &data, int flags = 0) {
        return sendString(data.c_str(), flags);
    }

    inline int sendString(const char *data, int flags = 0) {
        mega_tools::SharedBuffer<> str(data);
        return send(str, flags);
    }

    inline int sendMessage(Frame &message, int flags) {
        int error = zmq_msg_send(message.getRawMessage(),
                socket.get(), flags);
        test(__FUNCTION__, error);
        return error;
    }

    inline int sendMessage(Message &message) {
        int error = 0;
        int sentBytes = 0;

        for(unsigned int x = 0; x < message.size(); x++) {
            if(x < message.size() - 1) {
                error = sendMessage(message[x], ZMQ_SNDMORE);
            }
            else {
                error = sendMessage(message[x], 0);
            }

            test(__FUNCTION__, error);
            if(error >= 0) {
                sentBytes += error;
            }
            else {
                sentBytes = error;
                break;
            }
        }

        return sentBytes;
    }


protected:
    Socket(void *rawSocket, SocketType type) :
                socket(rawSocket, &deleteSocket),
                type(type){
    }

    std::shared_ptr<void> socket;
    inline static void deleteSocket(void *rawSocket) {}
    SocketType type;
};

class PubSocket : public Socket {
    int receive(SharedBuffer<> &buffer, int flags) = delete;
    Message receive() = delete;
};

class SubSocket : public Socket {
    int send(SharedBuffer<> buffer, int flags) = delete;
    int sendMessage(Message &message) = delete;
    int sendString(const char *data, int flags) = delete;
    int sendString(std::string &data, int flags) = delete;
    int sendMessage(Frame &message, int flags) = delete;
};

class RouterSocket : public Socket {
    friend class Context;
public:
    inline Message receiveRouterMessage() {
        RouterMessage m = receive();
        auto e = entities.find(m.getSenderId());
        if(e == entities.end()) {
            Entity entity(m.getSenderId());
            entities.insert({m.getSenderId(), entity});
        }
        return m;
    }

    inline unsigned int entityCount() {
        return entities.size();
    }

protected:
    RouterSocket(void *rawSocket) :
        Socket(rawSocket, SocketType::ROUTER),
        entities(0, idHash, idEq){

    }

    static bool idHash(const SharedBuffer<> buffer) {
        std::string value = std::string((char*)buffer.get(), buffer.size);
        return std::hash<std::string>()(value);
    }

    static bool idEq(const SharedBuffer<> &lhs, const SharedBuffer<> &rhs) {
        return lhs == rhs;
    }

    std::unordered_map<mega_tools::SharedBuffer<>, Entity,
    decltype(idHash)*, decltype(idEq)*> entities;
};

class ControlSocket : private Socket {
    friend class PollItem;
public:
    ControlSocket(void *rawSocket, const char *address) :
        Socket(rawSocket, SocketType::REP),
        address(address) {}

    std::string getAddress() {
        return address;
    }

    int getCommand() {
        Message m = Socket::receive();
        TLV tlv = m.getTLV(0);

        if(tlv.getType() != T_CONTROL) {
            throw ZmqError("Incorrect type for control socket.");
        }

        return (short)*tlv.getValue();
    }

    int confirm(bool confirmed) {
        if(confirmed) {
            return Socket::sendString("confirmed.");
        }
        else {
            return Socket::sendString("rejected.");
        }
    }

    void close() {
        Socket::close();
    }

    int bind() {
        return Socket::bind(address);
    }


private:
    std::string address;
};

class ControlCommand : private Socket {
    friend class PollItem;
public:
    ControlCommand(void *rawSocket, const char *address) :
        Socket(rawSocket, SocketType::REQ),
        address(address) {}

    Socket getSocket() {
        return reqSocket;
    }

    std::string getAddress() {
        return address;
    }

    int sendCommand(short command) {
        Message m({{Type::T_CONTROL, SharedBuffer<>((const unsigned char*)&command, sizeof(short))}});

        int error = Socket::sendMessage(m);
        if(error < 0) {
            return error;
        }
        Message msg = Socket::receive();
        std::cout << msg[0].str() << std::endl;

        return 0;
    }

    void close() {
        Socket::close();
    }

    int connect() {
        return Socket::connect(address);
    }


private:
    Socket reqSocket;
    std::string address;
};

} /* namespace zmq_cpp */

#endif /* SOCKET_H_ */
