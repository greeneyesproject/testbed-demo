#include <GuiNetworkSystem.h>
#include <Network/MessageParser.h>
#include <Network/Telosb/IncomingMessageQueue.h>
#include <Network/Tcp/ServerClientInterface.h>
#include <Network/Tcp/Server.h>
#include <Network/Tcp/Session.h>
#include <Network/NetworkNode.h>
#include <Messages/Message.h>
#include <Messages/DataATCMsg.h>
#include <Messages/DataCTAMsg.h>
#include <Messages/CoopInfoReqMsg.h>
#include "GuiProcessingSystem.h"
#include "Camera.h"

#include <boost/filesystem.hpp>
#include <cstddef>

#define COOPERATORS_PROBE_INTERVAL 300000 /* [ms] */

using namespace std;

GuiNetworkSystem* GuiNetworkSystem::_instance = NULL;

GuiNetworkSystem* GuiNetworkSystem::getInstance(
        boost::asio::io_service& ioService, unsigned short incomingPort) {
    cout << "GuiNetworkSystem::getInstance" << endl;
    if (_instance == NULL) {
        _instance = new GuiNetworkSystem(ioService, incomingPort);
    }
    return _instance;
}

GuiNetworkSystem::GuiNetworkSystem(boost::asio::io_service& ioService,
                                   unsigned short incomingPort) :
    _ioService(ioService) {
    cout << "GuiNetworkSystem::GuiNetworkSystem" << endl;

    assert(incomingPort);
    _server = new Server(_ioService, this, incomingPort);

    _guiProcessingSystem = GuiProcessingSystem::getInstance();

    _cooperatorsProbeTimer = new QTimer();
    _cooperatorsProbeTimer->setSingleShot(true);
    connect(_cooperatorsProbeTimer,SIGNAL(timeout()),this,SLOT(_cooperatorsProbeTimerFiredSlot()));
    connect(this,SIGNAL(_restartCooperatorsProbeTimerSignal()),this,SLOT(_restartCooperatorsProbeTimerSlot()));

}

void GuiNetworkSystem::shutdown(int) {
    cout << endl << "--------" << endl << "GuiNetworkSystem::shutdown" << endl;
    delete _instance->_server;
    exit(0);
}

/* ServerInterface methods */

void GuiNetworkSystem::serverAddSessionHandler(Session* session) {
    cout << "GuiNetworkSystem::serverAddSessionHandler: Sink connected from "
         << session->getSocket()->remote_endpoint() << " on "
         << session->getSocket()->local_endpoint() << endl;
    NetworkNode::getSink()->setSession(session);

    /* Ask cooperators information and start periodic probing */
    _probeCooperators();
    emit _restartCooperatorsProbeTimerSignal();

    emit sinkConnected(session->getSocket()->remote_endpoint().address().to_string().c_str(),
                       session->getSocket()->remote_endpoint().port());
}

void GuiNetworkSystem::serverRemoveSessionHandler(Session* /*session*/) {
    cout << "GuiNetworkSystem::serverRemoveSessionHandler: Sink disconnected" << endl;
    NetworkNode::getSink()->setSession(NULL);
    emit sinkDisconnected();
    _server->startAccept();
}

void GuiNetworkSystem::serverMessageHandler(Session*, Message* msg){
    if (msg==NULL){
        return;
    }

    Camera* camera = Camera::getCameraById(msg->getSrc()->getId());

    switch(msg->getType()){
    case MESSAGETYPE_DATA_ATC:{
        _guiProcessingSystem->processDataATCMsg(camera,(DataATCMsg*)msg);
        break;
    }
    case MESSAGETYPE_DATA_CTA:{
        _guiProcessingSystem->processDataCTAMsg(camera,(DataCTAMsg*)msg);
        break;
    }
    case MESSAGETYPE_COOP_INFO:
        _guiProcessingSystem->processCoopInfoMsg(camera,(CoopInfoMsg*)msg);
        break;
    case MESSAGETYPE_NODE_INFO:
        /* The sink notifies it has connected. Something to do with this? */
        break;
    default:
        cerr << "GuiNetworkSystem::serverMessageHandler: unexpected message type: " << msg->getType() << endl;
    }
}

/**
 * @brief GuiNetworkSystem::_probeCooperators
 * Ask each camera the number of available cooperators
 */
void GuiNetworkSystem::_probeCooperators() const{
    vector<NetworkNode*> cameras = NetworkNode::getCameras();
    for (vector<NetworkNode*>::iterator it = cameras.begin();
         it != cameras.end(); ++it){
        NetworkNode::getSink()->getSession()->writeMessage(new CoopInfoReqMsg(NetworkNode::getSink(),*it,LINKTYPE_TCP));
    }
}

/**
 * @brief GuiNetworkSystem::_cooperatorsProbeTimerFiredSlot
 * Called when the timer expires
 */
void GuiNetworkSystem::_cooperatorsProbeTimerFiredSlot(){
    if (NetworkNode::getSink()->getSession()){
        _probeCooperators();
        emit _restartCooperatorsProbeTimerSignal();
    }
}

/**
 * @brief GuiNetworkSystem::_restartCooperatorsProbeTimerSlot
 * Called to start or restart the cooperators probe timer.
 * The slot is necessary because the timer can be started only from the thread that created it.
 */
void GuiNetworkSystem::_restartCooperatorsProbeTimerSlot(){
    if (_cooperatorsProbeTimer->isActive()){
        _cooperatorsProbeTimer->stop();
    }
    _cooperatorsProbeTimer->start(COOPERATORS_PROBE_INTERVAL);
}

/**
 * @brief GuiNetworkSystem::sendMessage
 * Delete the message
 * @param msg
 */
bool GuiNetworkSystem::sendMessage(Message* msg){
    NetworkNode* sink = NetworkNode::getSink();
    if (!sink){
        delete msg;
        return false;
    }

    Session* ses = sink->getSession();
    if (!ses){
        delete msg;
        return false;
    }

    ses->writeMessage(msg);
    return true;
}
