/*
 * Message.h
 *
 *  Created on: 8/08/2015
 *      Author: michael
 */

#ifndef MESSAGE_H_
#define MESSAGE_H_

#include <zmq.h>

#include "ZmqError.h"
#include "shared_buffer.h"

#include <memory>
#include <vector>
#include <utility>

namespace zmq_cpp {

enum Type {
    T_SOCKET_ID = 0,
    T_DATA = 1,
    T_CONTROL = 2,
    T_ENTITY = 3,
    T_ADDRESS = 4,
    T_EC_PUB_KEY = 5
};

static const unsigned int TYPE_SIZE = 2;
static const unsigned int LENGTH_SIZE = sizeof(short);

class TLV {
public:

//    TLV(short type, const mega_tools::SharedBuffer<> &value) :
//        type(type),
//        value(value),
//        size(value.size){}

    template<typename T>
    TLV(short type, T &&value) :
        type(type),
        value(std::forward<T>(value)),
        size(value.size) {}

    TLV(short type, const char* value) :
        type(type),
        value(value) {
        size = this->value.size;
    }

    const short &getType() const {
        return type;
    }

    const short &getLength() const {
        return size;
    }

    const mega_tools::SharedBuffer<> getValue() const {
        return value;
    }

private:
    short type;
    mega_tools::SharedBuffer<> value;
    short size;
};

/**
 * @brief Small class used to ensure that the buffer is not
 * deleted until zmq is finished with it.
 */
struct BufferSaver {
    BufferSaver(mega_tools::SharedBuffer<> buffer) :
        buffer(buffer) {}
    mega_tools::SharedBuffer<> buffer;
};

class Frame {
public:

    std::shared_ptr<zmq_msg_t> frame;

    Frame() : frame(new zmq_msg_t(), deleteMessage){
        zmq_msg_init(frame.get());
    }

    Frame(const mega_tools::SharedBuffer<> &buffer) noexcept :
        Frame() {
        BufferSaver *h = new BufferSaver(buffer);
        zmq_msg_init_data(frame.get(), buffer.get(), buffer.size,
                handleBuffer, (void*)h);
    }

    Frame(const mega_tools::SharedBuffer<> &&buffer) noexcept :
        Frame(buffer) {
    }

    inline static void handleBuffer(void *data, void *hint) {
        delete ((BufferSaver*)hint);
    }

    virtual ~Frame() {}

    static void deleteMessage(zmq_msg_t *msg) {
        delete msg;
    }

    inline bool more() {
        return zmq_msg_more(frame.get());
    }

    inline int close() {
        int error = zmq_msg_close(frame.get());
        test(__FUNCTION__, error);
        return error;
    }

    inline zmq_msg_t *getRawMessage() {
        return frame.get();
    };

    inline Frame clone() {
        Frame nMessage;
        memcpy(nMessage.frame.get(), this->frame.get(), sizeof(zmq_msg_t));
        return nMessage;
    }

    inline mega_tools::SharedBuffer<> getData() {
        mega_tools::SharedBuffer<> data(size());
        memcpy(data.get(), zmq_msg_data(frame.get()), data.size);
        return data;
    }

    inline void *getRawData() {
        return zmq_msg_data(frame.get());
    }

    inline int size() {
        return zmq_msg_size(frame.get());
    }

    inline std::string str() {
        return std::string((char*)getRawData(), size());
    }
};

class Message : std::shared_ptr<std::vector<Frame>> {
public:
    Message() : messages(new std::vector<Frame>()){}

    Message(std::initializer_list<TLV> items) : Message() {
        for(const TLV &tlv : items) {
            addTLV(tlv);
        }
    }

    virtual ~Message() {}

    inline void addFrame(Frame &message) {
        messages->push_back(message);
    }

    inline void addFrame(const mega_tools::SharedBuffer<> &data) {
        Frame frame(data);
        addFrame(frame);
    }

    inline void addFrame(const mega_tools::SharedBuffer<> &&data) {
        Frame frame(data);
        addFrame(frame);
    }

    inline void addFrameCopy(Frame &message) {
        messages->push_back(message.clone());
    }

    inline std::vector<Frame>::iterator begin() {
        return messages->begin();
    }

    inline std::vector<Frame>::iterator end() {
        return messages->end();
    }

    inline unsigned int size() { return messages->size(); }

    inline Frame &operator[](unsigned int index) {
        return (*messages)[index];
    }

    inline void close() {
        for(auto &f : *messages) {
            f.close();
        }
    }

    inline void addTLV(const TLV &&tlv) {
        addTLV(tlv);
    }

    inline void addTLV(const TLV &tlv) {
        Frame type(mega_tools::SharedBuffer<>((unsigned char*)&tlv.getType(), TYPE_SIZE));
        Frame value(tlv.getValue());

        addFrame(type);
        addFrame(value);
    }

    inline TLV getTLV(unsigned int index) {
        return getTLV(index, 0);
    }


    std::vector<TLV> getAllTLV(unsigned int index) {
        return getAllTLV(index, 0);
    }

protected:
    inline TLV getTLV(unsigned int index, unsigned int offset) {

        if(index + 1 + offset >= size()) {
            throw ZmqError("Index out-of-bounds.");
        }

        Frame type = (*messages)[index + offset];
        if(type.size() != 2) {
            std::string errstr =
                    std::string("Type frame size is not equal to 2 bytes: ")
                    .append(std::to_string(type.size()));
            throw ZmqError(errstr);
        }

        Frame value = (*messages)[index + offset + 1];
        TLV tlv(*(short*)type.getRawData(), value.getData());

        return tlv;
    }

    inline std::vector<TLV> getAllTLV(unsigned int startIndex, int offset) {
        if((size() - offset) % 2 != 0) {
            std::string eStr =
                    std::string("Size is not mod 2: ").append(std::to_string(size() - offset));
            throw ZmqError(eStr);
        }

        std::vector<TLV> tlvVec;
        for(unsigned int x = startIndex; x + 2 <= size(); x += 2) {
            tlvVec.push_back(getTLV(x, offset));
        }

        return tlvVec;
    }

    std::shared_ptr<std::vector<Frame>> messages;
};

class RouterMessage : public Message {
public:
    RouterMessage() : id() {

    }

    RouterMessage(Message &&m) : Message(m) {
        id = (*this)[0].getData();
    }

    RouterMessage(mega_tools::SharedBuffer<> &&id) :
        id(id) {
        Frame dlim;
        addFrame(id);
        addFrame(dlim);
    }

    /**
     * @brief Creates a router message with the given id.
     *
     * @param id The id for the recipient of the message.
     */
    RouterMessage(mega_tools::SharedBuffer<> &id) :
        Message(),
        id(id) {
        Frame dlim;
        addFrame(id);
        addFrame(dlim);
    }

    inline TLV getTLV(unsigned int index) {
        return Message::getTLV(index, 2);
    }

    inline std::vector<TLV> getAllTLV(unsigned int index) {
        return Message::getAllTLV(index, 2);
    }

    mega_tools::SharedBuffer<> getSenderId() { return id; }

private:
    mega_tools::SharedBuffer<> id;
};

} /* namespace zmq_cpp */

#endif /* MESSAGE_H_ */
