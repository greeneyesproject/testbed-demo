#ifndef IOTHREAD_H
#define IOTHREAD_H

#include <QThread>
#include <boost/asio.hpp>

class GuiNetworkSystem;

class IoThread : public QThread{

public:
    static IoThread* getInstance(unsigned short incomingPort);

    ~IoThread(){

    }

    void run(){
        _ioService.run();
    }

private:

    static IoThread* _instance;

    IoThread(unsigned short incomingPort);

    boost::asio::io_service _ioService;
    GuiNetworkSystem* _guiNetworkSystem;

};

#endif // IOTHREAD_H
