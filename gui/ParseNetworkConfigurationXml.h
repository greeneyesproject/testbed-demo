#ifndef PARSENETWORKCONFIGURATIONXML_H
#define PARSENETWORKCONFIGURATIONXML_H

#include <QString>
#include <vector>
#include "Link.h"
#include "Camera.h"
#include "Sink.h"
#include "NetworkItem.h"

class ParseNetworkConfigurationXML
{
public:
    ParseNetworkConfigurationXML();

    int parse(std::string filename,
              std::vector<NetworkItem*> &items,
              std::vector<Link*> &links,
              std::vector<Camera*>* cameras,
              Sink* sink);

};



#endif // PARSENETWORKCONFIGURATIONXML_H
