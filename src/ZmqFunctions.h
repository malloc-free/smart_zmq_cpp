/*
 * ZmqFunctions.h
 *
 *  Created on: 7/08/2015
 *      Author: michael
 */

#ifndef ZMQFUNCTIONS_H_
#define ZMQFUNCTIONS_H_

#include "zmq.h"
#include "Socket.h"
#include "ZmqError.h"

#include <vector>
#include <functional>
#include <initializer_list>
#include <unordered_map>

namespace zmq_cpp {

enum class PollEvent {
    POLLIN = ZMQ_POLLIN,
    POLLOUT = ZMQ_POLLOUT,
    POLLERR = ZMQ_POLLERR,
    POLLPRI = ZMQ_POLLPRI
};

enum class PollState {
    STOPPED = 0,
    STARTED = 1
};

class PollItem;
using EventFunct = std::function<int(PollItem&, void*, short events)>;
const EventFunct NULL_FUNCT;

struct PollItem {
    void *socket;
    short *revents;
    short events;
    EventFunct eventFunct;
    int fd;
    void *arg;
    std::vector<PollEvent> eventList;

    template<class S>
    PollItem(S socket, int fd, std::vector<PollEvent> eventList,
            EventFunct eventFunct = NULL_FUNCT) :
        socket(socket.socket.get()),
        revents(nullptr),
        events(0),
        eventFunct(eventFunct),
        fd(fd),
        arg(nullptr),
        eventList(eventList) {
        for(auto e : eventList) {
            events |= static_cast<short>(e);
        }
    }

    inline short getEvents() {
        short ev = 0;
        for(auto e : eventList) {
            ev |= static_cast<short>(e);
        }

        return ev;
    }

    inline bool eventTriggered(PollEvent event) {
        return *revents & static_cast<int>(event);
    }

    /**
     * @brief Call event function if events triggered.
     *
     * If any of the events for this PollItem are triggered,
     * then call the event function.
     */
    inline void eventsTriggered() {
        short triggered = 0;
        for(auto e : eventList) {
            short tEvent = static_cast<short>(e);
            if(tEvent & *revents) {
                triggered |= tEvent;
            }
        }

        if(triggered) {
            eventFunct(*this, arg, triggered);
        }
    }

    inline int call(short events) {
        return eventFunct(*this, arg, events);
    }
};

using CtrlFunction = std::function<bool()>;

struct PollItems {
    PollItems(std::initializer_list<PollItem> items) :
        items(items),
        _stop(false),
        state(PollState::STOPPED){

        for(unsigned int x = 0; x < items.size(); x++) {
            PollItem &pItem = this->items[x];
            zmq_pollitem_t i = { pItem.socket, pItem.fd,
                    pItem.events, 0 };
            itemArray.push_back(i);
            pItem.revents = &itemArray[x].revents;
        }

        for(unsigned int x = 0; x < itemArray.size(); x++) {
           this->items[x].revents = &itemArray[x].revents;
        }

    }

    std::vector<PollItem> items;
    std::vector<zmq_pollitem_t> itemArray;
    bool _stop;
    PollState state;

    inline void stop() { _stop = true; }

    inline void reset() { _stop = false; }

    inline unsigned int size() { return items.size(); }

    inline PollItem &operator[](unsigned int index) {
        return items[index];
    }

    inline void eventsTriggered() {
        for(unsigned int x = 0; x < items.size(); x++) {
            items[x].eventsTriggered();
        }
    }

    inline int loop(long timeout = -1) {
        int error = 0;
        while(_stop) {
            error = zmq_poll(itemArray.data(), itemArray.size(),
                    timeout);
            test(error);
            eventsTriggered();
        }

        return error;
    }

    inline int ctrlLoop(CtrlFunction funct, long timeout = -1) {
        int error = 0;
        while(!funct()) {
            error = zmq_poll(itemArray.data(), itemArray.size(), timeout);
            test(error);
            eventsTriggered();
        }

        return error;
    }
};

static inline int poll(PollItems &items, long timeout) {
    int error = zmq_poll(items.itemArray.data(), items.size(), timeout);
    test(error);
    return error;
}

static inline int loop(PollItems &items, long timeout) {
    int error = 0;
    while(1) {

        error = zmq_poll(items.itemArray.data(),
                items.itemArray.size(), timeout);

        test(error);
        items.eventsTriggered();
    }

    return error;
}

}


#endif /* ZMQFUNCTIONS_H_ */
