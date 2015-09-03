/*
 * ServerClientInterface.h
 *
 *  Created on: 17/feb/2015
 *      Author: luca
 */

#ifndef SRC_NETWORK_TCP_SERVERCLIENTINTERFACE_H_
#define SRC_NETWORK_TCP_SERVERCLIENTINTERFACE_H_

class Message;
class Session;

class ServerClientInterface {

public:

	virtual void clientMessageHandler(Session*, Message*) {

	}

	virtual void clientConnectHandler(Session*) {

	}

	virtual void serverMessageHandler(Session*, Message*) {

	}

	virtual void serverAddSessionHandler(Session*) {

	}

	virtual void serverRemoveSessionHandler(Session*) {

	}

	virtual ~ServerClientInterface() {

	}

};

#endif /* SRC_NETWORK_TCP_SERVERCLIENTINTERFACE_H_ */
