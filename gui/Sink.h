#ifndef SINK_H
#define SINK_H

#include <QObject>
#include "Global.h"

class NetworkItem;

class Sink: public QObject
{
    Q_OBJECT
public:
    explicit Sink(QObject *parent = 0);

    ~Sink();

    void setNetworkParameters(uint id, uint amaddr, std::string ipaddr, NetworkItem* networkItem);

    uint getId();
    uint getAmAddr();
    std::string getIpAddr();

    friend std::ostream& operator<<(std::ostream& os, const Sink& sink);


private:

    uint _id;
    uint _amaddr;
    std::string _ipaddr;
    NetworkItem* _networkItem;
};

#endif // SINK_H
