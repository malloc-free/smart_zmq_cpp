/*
 * Context.h
 *
 *  Created on: 7/08/2015
 *      Author: michael
 */

#ifndef CONTEXT_H_
#define CONTEXT_H_

#include <zmq.h>
#include <memory>
#include "ZmqError.h"
#include "Socket.h"

namespace zmq_cpp {

enum class SocketType;

enum class ContextOption {
    IO_THREADS = ZMQ_IO_THREADS,
    MAX_SOCKETS = ZMQ_MAX_SOCKETS,
    SOCKET_LIMIT = ZMQ_SOCKET_LIMIT,
    THREAD_PRIORITY = ZMQ_THREAD_PRIORITY,
    THREAD_SHED_POLICY = ZMQ_THREAD_SCHED_POLICY
};

class Context {
public:
    Context() : context(zmq_ctx_new(), &deleteContext){

    }

    ~Context() {

    }

    Socket createSocket(SocketType type) {
        return Socket(zmq_socket(this->context.get(),
                static_cast<int>(type)),
                type);
    }

    inline void *createRawSocket(SocketType type) {
        return zmq_socket(this->context.get(), static_cast<int>(type));
    }

    inline RouterSocket createRouter() {
        return RouterSocket(zmq_socket(this->context.get(),
                static_cast<int>(SocketType::ROUTER)));
    }

    inline int setContextOptions(ContextOption option, int value) {
        int error = zmq_ctx_set(this->context.get(),
                static_cast<int>(option), value);
        test(error);
        return error;
    }

    inline ControlSocket createControlSocket(const char *address) {
        return ControlSocket(createRawSocket(SocketType::REP), address);
    }

    inline ControlCommand createControlCommand(const char *address) {
        return ControlCommand(createRawSocket(SocketType::REQ),address);
    }

private:
    std::shared_ptr<void> context;

    inline static void deleteContext(void *context) {
        zmq_ctx_destroy(context);
    }
};

} /* namespace zmq_cpp */

#endif /* CONTEXT_H_ */
