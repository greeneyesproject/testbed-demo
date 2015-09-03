#include "IoThread.h"
#include "GuiNetworkSystem.h"

IoThread* IoThread::_instance = NULL;

IoThread* IoThread::getInstance(unsigned short incomingPort){
    if (_instance==NULL){
        _instance = new IoThread(incomingPort);
    }
    return _instance;
}

IoThread::IoThread(unsigned short incomingPort){
    _guiNetworkSystem = GuiNetworkSystem::getInstance(_ioService,incomingPort);
}
