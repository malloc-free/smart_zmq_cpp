/*
 * main.cpp
 *
 *  Created on: 7/08/2015
 *      Author: michael
 */

#include <zmq.h>
#include <thread>
#include <iostream>
#include <chrono>
#include <vector>
#include <unordered_map>
#include "Context.h"
#include "Socket.h"
#include "ZmqError.h"
#include "shared_buffer.h"
#include "ZmqFunctions.h"
#include "Frame.h"

using namespace zmq_cpp;
using namespace mega_tools;
using namespace std;

Context clientContext;
Context serverContext;

bool bound = false;
int ready = 0;
int id = 0;
bool stop = false;

void startClient() {
    Socket socket;
    try {
        socket = clientContext.createSocket(SocketType::REQ);
        std::string address("tcp://localhost:5555");
        socket.connect(address);
        for(int requestNumber = 0; requestNumber != 2; requestNumber++) {
            SharedBuffer<> buffer(10);
            std::cout << "Sending hello" << std::endl;
            SharedBuffer<> message("Hello");
            socket.send(message, 0);
            socket.receive(buffer, 0);
            std::cout << "Received World" << std::endl;
        }
        for(int x = 0; x < 2; x++) {
            SharedBuffer<> buffer(std::string("data"));
            std::cout << "sending " << buffer.str() << std::endl;
            Message m;
            Frame fOne;
            Frame fTwo(buffer);
            m.addFrame(fOne);
            m.addFrame(fTwo);
            socket.sendMessage(m);
            Message mTwo = socket.receive();
            std::cout << "client size = " << mTwo.size() << std::endl;
        }
        std::cout << "closing" << std::endl;
        socket.close();
        std::cout << "closed" << std::endl;
    }
    catch(ZmqError &e) {
        std::cout << "Error in client: " << e.what() << std::endl;
        if(socket.getSocketType() != SocketType::NULL_SKT) {
            socket.close();
        }
    }

    std::cout << "client exit" << std::endl;
}

void startServer() {
    Socket socket;
    try {
        socket = serverContext.createSocket(SocketType::REP);
        std::string address("tcp://*:5555");
        socket.bind(address);
        bound = true;
        for(int requestNumber = 0; requestNumber != 2; requestNumber++) {
            SharedBuffer<> buffer(10);
            socket.receive(buffer, 0);
            std::cout << "Received " << buffer.str() << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            SharedBuffer<> message("World");
            socket.send(message, 0);
        }
        for(int x = 0; x < 2; x++) {
            Message m = socket.receive();
            std::cout << "size = " << m.size() << std::endl;

            SharedBuffer<> buffer(std::string("data"));
            std::cout << "sending " << buffer.str() << std::endl;
            Message mTwo;
            Frame fOne;
            Frame fTwo(buffer);
            mTwo.addFrame(fOne);
            mTwo.addFrame(fTwo);
            socket.sendMessage(mTwo);
        }
        socket.close();
    }
    catch(ZmqError &e) {
        if(socket.getSocketType() != SocketType::NULL_SKT) {
            std::cout << "Error in server: " << e.what() << std::endl;
            socket.close();
        }
    }

    std::cout << "server exit" << std::endl;

}

#define randof(num)  (int) ((float) (num) * random () / (RAND_MAX + 1.0))
void weatherServer() {
    Context context;
    Socket publisher = context.createSocket(SocketType::PUB);
    std::string address("tcp://*:5556");
    publisher.bind(address);
    //  Initialize random number generator
    srandom ((unsigned) time (NULL));
    //while(1);
    while(!stop){
       //  Get values that will fool the boss
       int zipcode, temperature, relhumidity;
       zipcode     = randof (100000);
       temperature = randof (215) - 80;
       relhumidity = randof (50) + 10;

       //  Send message to all subscribers
       SharedBuffer<> update(20);
       sprintf ((char*)update.get(), "%05d %d %d", zipcode, temperature, relhumidity);
       publisher.send(update);
    }
    publisher.close();

    std::cout << "WeatherServer exit" << std::endl;
}

void taskVentalator() {
    Context context;
        //  Socket to send messages on
    Socket sender = context.createSocket(SocketType::PUSH);
    std::string address("tcp://*:5557");
    sender.bind(address);

    //  Initialize random number generator
    srandom ((unsigned) time (NULL));

    //  Send 100 tasks
    int task_nbr;
    int total_msec = 0;     //  Total expected cost in msecs
//    while(ready < 2) {
//        std::this_thread::sleep_for(std::chrono::seconds(1));
//    }
    for (task_nbr = 0; task_nbr < 100; task_nbr++) {
        int workload;
        //  Random workload from 1 to 100msecs
        workload = (int) ((float) (100) * random () / (RAND_MAX + 1.0)) + 1;
        total_msec += workload;
        SharedBuffer<> buffer(std::string("Hello"));
        sender.send(buffer);
    }
    printf ("Total expected cost: %d msec\n", total_msec);

    sender.close();

    std::cout << "TaskVentilator exit" << std::endl;
}

void multipleSocketPoller() {
    //  Connect to task ventilator
    Context context;
    Socket receiver = context.createSocket(SocketType::PULL);
    std::string address("tcp://localhost:5557");
    receiver.connect(address);

    //  Connect to weather server
    Socket subscriber = context.createSocket(SocketType::SUB);
    std::string subAddress("tcp://localhost:5556");
    subscriber.connect(subAddress);
    subscriber.setSockOpt(SocketOption::SUBSCRIBE, "10001 ", 6);


    SharedBuffer<> msg(256);
    PollItem pOne = {receiver, 0, { PollEvent::POLLIN },
            [receiver, msg](PollItem &i, void *v, short events) mutable {
        int size = receiver.receive(msg, 0);
        if (size != -1) {
            std::cout << "Task" << std::endl;
        }
        else {
            std::cout << "Nothing Task" << std::endl;
        }

        return 0;
    }};

    PollItem pTwo = {subscriber, 0, { PollEvent::POLLIN },
            [subscriber, msg](PollItem &i, void *v, short events) mutable {
        int size = subscriber.receive(msg, 0);
        if (size != -1) {
            std::cout << "Weather update" << std::endl;
        }
        else {
            std::cout << "nothing update" << std::endl;
        }
        return 0;
    }};
    PollItems items = { pTwo, pOne };
    ready++;
//    loop(items, -1);
    items.loop();
    receiver.close();
    subscriber.close();

    std::cout << "MultipleSocketPoller exit" << std::endl;
}

void request(const char *id, const char *address, bool connect) {
    Context context;
    Socket req = context.createSocket(SocketType::REQ);
    //req.setSockOpt(SocketOption::IDENTITY, id, strlen(id));
    req.setSockOpt({ SocketOption::IDENTITY, id, strlen(id)});
    std::string addrStr(address);
    std::cout << id << " connecting" << std::endl;
    req.connect(addrStr);
    std::cout << "Connected, sending message" << std::endl;
    SharedBuffer<> msg(std::string("Hello"));
    req.send(msg, 0);
    std::cout << id << " ready to receive" << std::endl;
    Message m = req.receive();
    std::cout << "msg size = " << m.size() << std::endl;
    TLV tlv(Type::T_DATA, msg);

    Message message;
    message.addTLV(tlv);
    req.sendMessage(message);
    Message m3 = req.receive();
    req.close();
    std::cout << id << "exiting" << std::endl;
}

enum DirControl {
    DC_SHUTDOWN = 0,
    DC_PUB_DIR = 1
};

void directory() {
    Context context;
    std::unordered_map<mega_tools::SharedBuffer<>, Entity,
        decltype(sb_hasher<>)*, decltype(sb_eqOp<>)*>
        entities(0, mega_tools::sb_hasher<>, mega_tools::sb_eqOp<>);
    Socket directoryList = context.createSocket(SocketType::REP);
    Socket registerSocket = context.createSocket(SocketType::REP);
    Socket publishDirectory = context.createSocket(SocketType::PUB);
    Socket control = context.createSocket(SocketType::REP);

    bool exit = false;
    cout << "Bind directoryList" << endl;
    directoryList.bind("tcp://*:5554");
    cout << "Bind registerSocket" << endl;
    registerSocket.bind("tcp://*:5555");
    cout << "Bind publishDirectory" << endl;
    publishDirectory.bind("tcp://*:5556");
    cout << "Bind control" << endl;
    control.bind("tcp://*:5557");

    auto createDir = [&entities]() mutable {
        Message dirMessage;
        for(auto p : entities) {
            short t = T_ENTITY;
            cout << "Adding tlv: " << p.first.str() << endl;
            dirMessage.addTLV({t, p.first});
        }

        return dirMessage;
    };

    auto pubDir = [&createDir, publishDirectory]() mutable {
        Message dirMessage = createDir();
        publishDirectory.sendMessage(dirMessage);
    };

    PollItem dirItem(directoryList, 0, { PollEvent::POLLIN },
            [&directoryList, &entities, &createDir](PollItem &item, void *data, short events) mutable {
        cout << "***** Directory request *****" << endl;
        Message m = directoryList.receive();
        Message dirMessage = createDir();
        cout << "Size of dirMessage = " << dirMessage.size() << endl;
        directoryList.sendMessage(dirMessage);
        return 0;
    });

    PollItem regItem(registerSocket, 0, { PollEvent::POLLIN },
            [registerSocket, &entities, &createDir, &publishDirectory, &pubDir](PollItem &item, void *data, short events) mutable {
        cout << "***** Registration *****" << endl;
        Message regMessage = registerSocket.receive();
        vector<TLV> tlvVec = regMessage.getAllTLV(0);
        TLV id = tlvVec[0];
        if(id.getType() != T_SOCKET_ID) {
            registerSocket.sendString("Error");
            return -1;
        }
        Entity e(id.getValue());
        for(unsigned int x = 1; x < tlvVec.size(); x++) {
            short type = tlvVec[x].getType();
            switch(type) {
                case T_ADDRESS : {
                    e.setAddress(tlvVec[x].getValue());
                    cout << "Address = " << e.getAddress() << endl;
                }
            }
        }

        entities.insert({e.getId(), e});
        cout << "id = " << id.getValue().str() << endl;
        registerSocket.sendString("Registration confirmed.");

        pubDir();
        return 0;
    });

    PollItem controlItem(control, 0, { PollEvent::POLLIN },
            [&control, &exit, &pubDir](PollItem &item, void *data, short events) mutable {
        try {
            cout << "[Directory] ***** Control item received *****" << endl;
            Message ctrlMsg = control.receive();
            TLV ctrl = ctrlMsg.getTLV(0);
            if(ctrl.getType() == T_CONTROL) {
                short val = (short)*ctrl.getValue();
                switch(val) {
                    case DC_SHUTDOWN : {
                        exit = true;
                        cout << "[Directory] Shutdown received." << endl;
                        control.sendString("Confirm shutdown.");
                        break;
                    }
                    case DC_PUB_DIR : {
                        pubDir();
                        cout << "[Directory] Pub received." << endl;
                        control.sendString("Confirm publish");
                        break;
                    }
                    default : {
                        cout << "[Directory] No command by that name" << endl;
                        control.sendString("Command error.");
                    }
                }
            }
            else {
                cout << "Non control message received: " << (int)*ctrl.getValue() << endl;
            }
        }
        catch(ZmqError &e) {
            cout << "Error in control: " << e.what() << endl;
        }

        return 0;
    });

    PollItems items({dirItem, regItem, controlItem});
    items.ctrlLoop([&exit](){
        return exit;
    });

    directoryList.close();
    registerSocket.close();
    publishDirectory.close();
    control.close();
    cout << "Directory exiting" << endl;
}

void talker(const char *id, const char *routeAddress, const char *ctrlAddress) {
    Context context;
    Socket directoryList = context.createSocket(SocketType::REQ);
    Socket registerSocket = context.createSocket(SocketType::REQ);
    Socket subscribeDirectory = context.createSocket(SocketType::SUB);
    subscribeDirectory.setSockOpt({SocketOption::SUBSCRIBE, "", 0});
    ControlSocket ctrl = context.createControlSocket(ctrlAddress);

    try {
        cout << "Connect directoryList" << endl;
        directoryList.connect("tcp://localhost:5554");
        cout << "Connect registerSocket" << endl;
        registerSocket.connect("tcp://localhost:5555");
        cout << "Connect subscribeDirectory" << endl;
        subscribeDirectory.connect("tcp://localhost:5556");
        cout << "Bind ctrlSocket" << endl;
        ctrl.bind();
    }
    catch(ZmqError &e) {
        cout << "talker " << id << " Error: " << e.what() << endl;
        return;
    }

    TLV reg(Type::T_SOCKET_ID, id);
    Message regMessage({
        {Type::T_SOCKET_ID, id},
        {Type::T_ADDRESS, routeAddress}
    });
    registerSocket.sendMessage(regMessage);
    Message retMessage = registerSocket.receive();
    cout << retMessage[0].str() << endl;
    bool exit = false;

    PollItem ctrlItem(ctrl, 0, {PollEvent::POLLIN},
            [&ctrl, &exit](PollItem &item, void *data, short events){
        short command = ctrl.getCommand();
        int error = 0;
        switch(command) {
           case DC_SHUTDOWN : {
               exit = true;
               cout << "Shutdown received." << endl;
               ctrl.confirm(true);
               break;
           }
           default : {
               cout << "No command by that name" << endl;
               ctrl.confirm(false);
               error = -1;
           }
        }

        return error;
    });

    PollItem dirSub(subscribeDirectory, 0, {PollEvent::POLLIN},
            [&subscribeDirectory, id](PollItem &item, void *data, short events){
        cout << "Directory subscription fired" << endl;
        Message dirMessage = subscribeDirectory.receive();
        vector<TLV> dir = dirMessage.getAllTLV(0);
        for(auto &e : dir) {
            cout << "talker: " << id << "Entity = " <<  e.getValue().str() << endl;
        }

        return 0;
    });

    PollItems items({ctrlItem, dirSub});
    items.ctrlLoop([&exit](){
        return exit;
    });

    directoryList.close();
    registerSocket.close();
    subscribeDirectory.close();
    ctrl.close();

    cout << "talker exiting" << endl;
}

void runDirectory() {
    cout << sizeof(Message) << endl;
    Context context;


    thread dir(directory);
    Socket ctrl = context.createSocket(SocketType::REQ);
    Socket dList = context.createSocket(SocketType::REQ);

    try {
        dList.connect("tcp://localhost:5554");
        ctrl.connect("tcp://localhost:5557");
    }
    catch(ZmqError &e) {
        cout << "Error: " << e.what() << endl;
        return;
    }

    try {
        thread talkerOne(talker, "talkerOne", "tcp://localhost:5558",
                "tcp://*:5561");
        ControlCommand oneCtrl = context.createControlCommand("tcp://localhost:5561");
        oneCtrl.connect();

        thread talkerTwo(talker, "talkerTwo", "tcp://localhost:5559",
                "tcp://*:5562");
        ControlCommand twoCtrl = context.createControlCommand("tcp://localhost:5562");
        twoCtrl.connect();

        thread talkerThree(talker, "talkerThree", "tcp://localhost:5560",
                "tcp://*:5563");
        ControlCommand threeCtrl = context.createControlCommand("tcp://localhost:5563");
        threeCtrl.connect();

        SharedBuffer<> bla("bla");
        Message dirReq;
        dirReq.addFrame(bla);
        cout << "Requesting directory" << endl;
        dList.sendMessage(dirReq);
        cout << "Waiting for reply" << endl;
        Message dirMessage = dList.receive();
        vector<TLV> dirVec = dirMessage.getAllTLV(0);
        cout << "size of directory = " << dirVec.size() << endl;
        for(TLV &t : dirVec) {
            cout << "Entity : " << t.getValue().str() << endl;
        }

        this_thread::sleep_for(chrono::seconds(1));
        cout << "Requesting publish" << endl;
        short pd = DC_PUB_DIR;
        Message ctrlPub({{Type::T_CONTROL, SharedBuffer<>((const unsigned char*)&pd, sizeof(short))}});
        ctrl.sendMessage(ctrlPub);
        Message retPubMsg = ctrl.receive();
        cout << "Ctrl return pub message: " << retPubMsg[0].str() << endl;
        this_thread::sleep_for(chrono::seconds(1));

        oneCtrl.sendCommand(DC_SHUTDOWN);
        talkerOne.join();
        oneCtrl.close();

        twoCtrl.sendCommand(DC_SHUTDOWN);
        talkerTwo.join();
        twoCtrl.close();

        threeCtrl.sendCommand(DC_SHUTDOWN);
        talkerThree.join();
        threeCtrl.close();

        short sd = DC_SHUTDOWN;
        Message ctrlMsg({ {Type::T_CONTROL, SharedBuffer<>((const unsigned char*)&sd, sizeof(short))}});
        ctrl.sendMessage(ctrlMsg);
        Message retMsg = ctrl.receive();
        cout << "Ctrl return message: " << retMsg[0].str() << endl;

        dir.join();
        ctrl.close();
        dList.close();

        cout << "runDirectory exit" << endl;
    }
    catch(ZmqError &e) {
        cout << "Error: " << e.what() << endl;
        ctrl.close();
        dList.close();
    }
}

void router(const char *id, const char *address, bool connect) {
    Context context;
    RouterSocket route = context.createRouter();
    std::string addrStr(address);
    if(connect) {
        std::cout << id << " connecting" << std::endl;
        route.connect(addrStr);
    }
    else {
        std::cout << id << " binding" << std::endl;
        route.bind(addrStr);
        bound = true;
    }
    std::cout << "Connection established" << std::endl;
    RouterMessage m = route.receiveRouterMessage();
    std::cout << "size = " << m.size();
    std::cout << "id = " << m.getSenderId() << std::endl;
    std::cout << "Data size =" << m[0].getData().size << std::endl;
    std::cout << "Data = " << m[1].getData().str() << std::endl;
    std::cout << "Data = " << m[2].getData().str() << std::endl;

    SharedBuffer<> dstr(std::string("goodbye"));
    Frame addr = m[0].clone();
    m.close();

    cout << "Creating message" << endl;
    Frame dlim;
    Frame data(dstr);
//    Message ms;
//    ms.addFrame(addr);
//    ms.addFrame(dlim);
//    ms.addFrame(data);

    RouterMessage ms(addr.getData());
    ms.addFrame(data);
    route.sendMessage(ms);
    RouterMessage m1 = route.receiveRouterMessage();

    try {
        cout << "Getting TLV, size of message = "
                << m1.size() << endl;
        TLV recvTlv = m1.getTLV(0);
        TLV tlv = TLV(Type::T_DATA, dstr);
        cout << "Value = " << tlv.getValue().str() << endl;
        RouterMessage m2(addr.getData());
        m2.addTLV(tlv);
        route.sendMessage(m2);
        cout << "Entity count = " << route.entityCount() << endl;
        route.close();
        std::cout << id << " exiting" << std::endl;
    }
    catch(ZmqError &e) {
        cout << "Error: " << e.what() << endl;
        route.close();
        return;
    }
}

void testBindConnect() {
    std::thread bind(router, "binder", "tcp://*:5557", false);
    while(!bound) {
       std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    std::thread connect(request, "connector", "tcp://localhost:5557", true);
    connect.join();
    bind.join();
    cout << "Quit" << endl;
}

void testPoll() {
    std::thread wServer(weatherServer);
    std::thread tVent(taskVentalator);
    std::thread pollerOne(multipleSocketPoller);
    std::thread pollerTwo(multipleSocketPoller);

    std::this_thread::sleep_for(std::chrono::seconds(5));
    stop = true;
    cout << "pollerOne join" << endl;
    pollerOne.join();
    cout << "pollerTwo join" << endl;
    pollerTwo.join();
    cout << "tVent join" << endl;
    tVent.join();
    cout << "wServer join" << endl;
    wServer.join();

    cout << "Quit" << endl;
}

int main() {
    runDirectory();
    return 0;
}

