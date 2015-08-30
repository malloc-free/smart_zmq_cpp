/*
 * Entity.h
 *
 *  Created on: 27/08/2015
 *      Author: michael
 */

#ifndef ENTITY_H_
#define ENTITY_H_

#include <shared_buffer.h>
#include <string>

namespace zmq_cpp {

class Entity {
public:
    inline Entity(mega_tools::SharedBuffer<> id) : id(id), address("") {}

    inline Entity(mega_tools::SharedBuffer<> id, std::string &address) :
            id(id),
            address(address) {}

    inline mega_tools::SharedBuffer<> getId() { return id; }

    inline std::string str() {
        return id.str();
    }

    inline void setAddress(const mega_tools::SharedBuffer<> &addBuff) {
        address = addBuff;
    }

    inline void setAddress(const mega_tools::SharedBuffer<> &&addBuff) {
        address = addBuff;
    }

    inline std::string getAddress() {
        return address.str();
    }

private:
    mega_tools::SharedBuffer<> id;
    mega_tools::SharedBuffer<> address;
};

}

#endif /* ENTITY_H_ */
