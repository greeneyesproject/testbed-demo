#include "ParseNetworkConfigurationXml.h"
#include <Network/NetworkNode.h>
#include <QXmlStreamReader>
#include <QFile>
#include <iostream>
#include <QtXml>
#include <cstring>

using namespace std;

ParseNetworkConfigurationXML::ParseNetworkConfigurationXML()
{
}

int ParseNetworkConfigurationXML::parse(string filename,
                                        vector<NetworkItem*> &items,
                                        vector<Link*> &links,
                                        vector<Camera*>* cameras,
                                        Sink* sink){

    //The QDomDocument class represents an XML document.
    QDomDocument xmlBOM;

    // Load xml file as raw data
    QFile f(filename.c_str());
    if (!f.open(QIODevice::ReadOnly )){
        // Error while loading file
        std::cerr << "ParseNetworkConfigurationXML: Error while loading " << filename << std::endl;
        exit(-1);
    }

    // Set data into the QDomDocument before processing
    xmlBOM.setContent(&f);
    f.close();

    // Extract the root markup
    QDomElement root=xmlBOM.documentElement();

    // Get root names and attributes
    //n_cameras = root.attribute("n_cameras").toInt();

    QDomElement item =root.firstChild().toElement();
    while(!item.isNull()){
        if (item.tagName() == "item"){
            int row = item.attribute("row").toInt();
            int col = item.attribute("col").toInt();
            string item_class = item.attribute("class").toStdString();
            uint id = item.attribute("id").toUInt();
            QString enabledStr = item.attribute("enabled","");
            bool enabled = enabledStr!="" ? enabledStr.toUInt()>0 : true;

            uint amaddr = 0;
            string ipaddr = "0.0.0.0";
            double energyBudget = 0;
            if (item.attribute("am_address","null")!="null"){
                amaddr = item.attribute("am_address").toUInt();
            }
            if (item.attribute("ip_address","null")!="null"){
                ipaddr = item.attribute("ip_address").toStdString();
            }
            if(item.attribute("energy_budget","null")!="null"){
                energyBudget = item.attribute("energy_budget").toDouble();
            }

            int coop_id = -1;

            NodeType itemType = NODETYPE_UNDEF;

            if (item_class=="gui"){
                coop_id = id;
                itemType = NODETYPE_GUI;
            }else if (item_class=="cooperator"){
                coop_id = id;
                itemType = NODETYPE_COOPERATOR;
            }
            else if(item_class=="camera"){
                itemType = NODETYPE_CAMERA;
            }
            else if(item_class=="sink"){
                itemType = NODETYPE_SINK;
            }
            else if(item_class=="relay"){
                itemType = NODETYPE_RELAY;
            }

            if(itemType != NODETYPE_UNDEF){
                NetworkItem* networkItem = new NetworkItem (id, itemType, row, col, false, enabled, coop_id);
                items.push_back(networkItem);

                switch(itemType){
                case NODETYPE_GUI:
                    NetworkNode::setMyself(NetworkNode::setGui(ipaddr,0));
                    break;
                case NODETYPE_CAMERA:
                    NetworkNode::addCamera(id,amaddr,ipaddr);
                    cameras->push_back(new Camera(id,amaddr,ipaddr,networkItem,energyBudget));
                    break;
                case NODETYPE_SINK:
                    NetworkNode::setSink(id,amaddr,ipaddr);
                    sink->setNetworkParameters(id,amaddr,ipaddr,networkItem);
                    break;
                case NODETYPE_COOPERATOR:
                    NetworkNode::addCooperator(id);
                default:
                    break;
                }
            }

        }
        else if (item.tagName() == "link"){

            uint source_id = item.attribute("source_id").toUInt();
            uint dest_id = item.attribute("dest_id").toUInt();
            bool enabled = item.attribute("enabled").toUInt()>0;

            links.push_back(new Link(source_id, dest_id, enabled));
        }

        item = item.nextSibling().toElement();
    }

    return 0;
}
