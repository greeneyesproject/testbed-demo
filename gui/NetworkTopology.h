#ifndef NETWORKTOPOLOGY_H
#define NETWORKTOPOLOGY_H

#include <QWidget>
#include <vector>
#include "ParseNetworkConfigurationXml.h"
#include "Link.h"
#include "NetworkItem.h"
#include <Global.h>
#include <QPushButton>
#include "CameraSettings.h"


namespace Ui {
class NetworkTopology;
}

class NetworkTopology : public QWidget
{
    Q_OBJECT

public:
    NetworkTopology(CameraSettings * camSet, std::vector<NetworkItem*>* items, std::vector<Link*>*links, QWidget *parent = 0);
    ~NetworkTopology();

    void getLinkCells(Link* alink, int &s_x, int &s_y, int &d_x, int &d_y);

protected:
    void paintEvent(QPaintEvent *);

public slots:
    void buttonClicked(int a);
    void changeActiveCamera(int id);
    void changeCoop(int camId, int nCoop);
    void changeAvailCoop(int camId, int nCoop);
    void updateBandwidth(int cam_id, double bw);

private:
    Ui::NetworkTopology *ui;
    std::vector<NetworkItem*>* _items;
    std::vector<Link*>* _links;
    NetworkItem* _itemGrid[NETWORK_TOPOLOGY_ROWS][NETWORK_TOPOLOGY_COLS];
    QPushButton *_buttonGrid[NETWORK_TOPOLOGY_ROWS][NETWORK_TOPOLOGY_COLS];
    CameraSettings *_camSet;

signals:

    void cameraSelected(int);
};

#endif // NETWORKTOPOLOGY_H
