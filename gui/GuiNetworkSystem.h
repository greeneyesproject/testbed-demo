#ifndef SRC_NETWORK_NODENETWORKSYSTEM_H_
#define SRC_NETWORK_NODENETWORKSYSTEM_H_

#include <Network/Tcp/ServerClientInterface.h>
#include <boost/asio.hpp>
#include <QObject>
#include <QString>
#include <QTimer>

#include <stdint.h>
#include <string>
#include <set>

class WiFiRadioSystem;
class Message;
class Connection;
class Session;
class Server;
class GuiProcessingSystem;

class GuiNetworkSystem:  public QObject, public ServerClientInterface {
    Q_OBJECT
public:

    static GuiNetworkSystem* getInstance(){
        return _instance;
    }

    static GuiNetworkSystem* getInstance(boost::asio::io_service& ioService,
                                         unsigned short incomingPort);

    static void shutdown(int signal);

    /* ServerInterface methods */
    void serverMessageHandler(Session* session, Message* msg);

    void serverAddSessionHandler(Session* session) ;

    void serverRemoveSessionHandler(Session* session) ;

    bool sendMessage(Message* msg);

private:

    GuiNetworkSystem(boost::asio::io_service& ioService,
                     unsigned short incomingPort);



    static GuiNetworkSystem* _instance;

    void _probeCooperators() const;

    GuiProcessingSystem* _guiProcessingSystem;

    boost::asio::io_service& _ioService;

    Server* _server;

    QTimer* _cooperatorsProbeTimer;


signals:
    void sinkDisconnected();
    void sinkConnected(QString, unsigned short);
    void _restartCooperatorsProbeTimerSignal();

private slots:
    void _cooperatorsProbeTimerFiredSlot();
    void _restartCooperatorsProbeTimerSlot();


};

#endif
