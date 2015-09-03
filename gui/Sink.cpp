#include "Sink.h"
#include <iostream>
using namespace std;

Sink::Sink(QObject *parent) : QObject(parent){
    _id = 0;
    _amaddr = 0;
    _ipaddr = "0.0.0.0";
}

Sink::~Sink(){

}

void Sink::setNetworkParameters(uint id, uint amaddr, string ipaddr, NetworkItem* networkItem){
    _id = id;
    _amaddr = amaddr;
    _ipaddr = string(ipaddr);
    _networkItem = networkItem;
}

uint Sink::getId(){
    return _id;
}

uint Sink::getAmAddr(){
    return _amaddr;
}

string Sink::getIpAddr(){
    return _ipaddr;
}

std::ostream& operator<<(std::ostream& os, const Sink& sink){
  return os << "Sink - id:" << sink._id << " am:"  << sink._amaddr << " ip:" << sink._ipaddr;
}
