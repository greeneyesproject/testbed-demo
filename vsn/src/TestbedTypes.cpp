/*
 * TestbedTypes.cpp
 *
 *  Created on: 06/apr/2015
 *      Author: luca
 */
#include <TestbedTypes.h>
#include <iostream>

using namespace std;

ostream& operator<<(ostream& os, const NodeType& nt) {
	switch (nt) {
	case NODETYPE_UNDEF:
		os << "UNDEF";
		break;
	case NODETYPE_GUI:
		os << "GUI";
		break;
	case NODETYPE_SINK:
		os << "SINK";
		break;
	case NODETYPE_CAMERA:
		os << "CAMERA";
		break;
	case NODETYPE_COOPERATOR:
		os << "COOPERATOR";
		break;
	case NODETYPE_RELAY:
		os << "RELAY";
		break;
	}
	return os;
}

ostream& operator<<(ostream& os, const MessageType& mt) {
	switch (mt) {
	case MESSAGETYPE_NONE:
		os << "NONE";
		break;
	case MESSAGETYPE_START_CTA:
		os << "START_CTA";
		break;
	case MESSAGETYPE_START_ATC:
		os << "START_ATC";
		break;
	case MESSAGETYPE_DATA_CTA:
		os << "DATA_CTA";
		break;
	case MESSAGETYPE_DATA_ATC:
		os << "DATA_ATC";
		break;
	case MESSAGETYPE_STOP:
		os << "STOP";
		break;
	case MESSAGETYPE_COOP_INFO:
		os << "COOP_INFO";
		break;
	case MESSAGETYPE_COOP_INFO_REQ:
		os << "COOP_INFO_REQ";
		break;
	case MESSAGETYPE_NODE_INFO:
		os << "NODE_INFO";
		break;
	case MESSAGETYPE_ACK:
		os << "ACK";
		break;
	}
	return os;
}


ostream& operator<<(ostream& os, const OperativeMode& om) {
	switch (om) {
	case OPERATIVEMODE_OBJECT:
		os << "OBJECT";
		break;
	case OPERATIVEMODE_PKLOT:
		os << "PKLOT";
		break;

	}
	return os;
}

