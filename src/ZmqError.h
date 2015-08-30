/*
 * ZmqError.h
 *
 *  Created on: 7/08/2015
 *      Author: michael
 */

#ifndef ERROR_H_
#define ERROR_H_

#include <stdexcept>
#include <errno.h>
#include <string.h>
#include <zmq.h>
#include <iostream>

class ZmqError : public std::runtime_error {
public:
    ZmqError(const std::string &what) : std::runtime_error(what) {}

    ZmqError(const char *what) : std::runtime_error(what) {}

    ZmqError(const int err) : std::runtime_error(zmq_strerror(err)) {}

    virtual ~ZmqError() {}
};

static inline void test(const char *msg, int value) {
#ifndef __NO_EXCEPT
    if(value < 0) {
        std::cerr << "Error: " << msg << " value: " << value << std::endl;
        throw ZmqError(std::string(msg) + " : " + zmq_strerror(errno));
    }
#endif
}

static inline void test(int value) {
#ifndef __NO_EXCEPT
    if(value < 0) {
        throw ZmqError(zmq_strerror(errno));
    }
#endif
}

#endif /* ERROR_H_ */
