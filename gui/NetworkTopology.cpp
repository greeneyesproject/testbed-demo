#include "NetworkTopology.h"
#include "ui_networktopology.h"
#include "Global.h"
#include <QPushButton>
#include <QPainter>
#include <QSignalMapper>
#include <QDebug>


using namespace std;

NetworkTopology::NetworkTopology(CameraSettings * camSet,
                                 std::vector<NetworkItem*> *items,
                                 std::vector<Link*>*links,
                                 QWidget *parent) :
    QWidget(parent),
    ui(new Ui::NetworkTopology)
{
    ui->setupUi(this);

    _camSet = camSet;

    _links = links;

    for (int r = 0; r < NETWORK_TOPOLOGY_ROWS; r++){
        for (int c = 0; c < NETWORK_TOPOLOGY_COLS; c++){
            _itemGrid[r][c]=NULL;
        }
    }

    for (vector<NetworkItem*>::iterator itemIt = items->begin(); itemIt != items->end(); itemIt++){
        _itemGrid[(*itemIt)->row()][(*itemIt)->col()] = *itemIt;
    }

    QSignalMapper *signalMapper = new QSignalMapper(this);
    connect(signalMapper, SIGNAL(mapped(int)), this, SLOT(buttonClicked(int)));

    for (int r = 0; r < NETWORK_TOPOLOGY_ROWS; r++){
        for (int c = 0; c < NETWORK_TOPOLOGY_COLS; c++){
            NetworkItem* item = _itemGrid[r][c];

            if (item){

                QPushButton * button = new QPushButton();
                button->setFlat(true);

                if (item->getGuiIdx() < 0){
                    button->setDisabled(true);
                }

                if (!item->enabled()){
                    button->setDisabled(true);
                }

                ui->gridLayout->addWidget(button, r, c, 1, 1);

                _buttonGrid[r][c] = button;

                signalMapper->setMapping(button, r*NETWORK_TOPOLOGY_COLS+c);
                connect(button, SIGNAL(clicked()), signalMapper, SLOT(map()));
            }

        }
    }

    connect(this, SIGNAL(cameraSelected(int)), camSet, SLOT(setSelectedCameraSlot(int)));
}

NetworkTopology::~NetworkTopology(){
    delete ui;
}


void NetworkTopology::paintEvent(QPaintEvent *){
    // draw links

    QPen pen;  // creates a default pen

    pen.setStyle(Qt::DashDotLine);

    QPainter painter;
    painter.begin(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(pen);

    QImage camera(CAMERA_IM_PATH);
    QImage relay(RELAY_IM_PATH);
    QImage cooperator(COOP_IM_PATH);
    QImage sink(SINK_IM_PATH);

    for (uint l = 0; l < _links->size(); l++){

        if ((*_links)[l]->enabled()){

            int s_x, s_y, d_x, d_y;
            getLinkCells((*_links)[l], s_x, s_y, d_x, d_y);

            int tx, ty, bx, by;

            QLayoutItem* a = ui->gridLayout->itemAtPosition(s_y, s_x);
            tx = a->geometry().topLeft().x();
            ty = a->geometry().topLeft().y();

            bx = a->geometry().bottomRight().x();
            by = a->geometry().bottomRight().y();

            int sx = (tx + bx)*0.5;
            int sy = (ty + by)*0.5;

            QLayoutItem* b = ui->gridLayout->itemAtPosition(d_y, d_x);
            tx = b->geometry().topLeft().x();
            ty = b->geometry().topLeft().y();

            bx = b->geometry().bottomRight().x();
            by = b->geometry().bottomRight().y();

            int dx = (tx + bx)*0.5;
            int dy = (ty + by)*0.5;

            painter.drawLine(sx, sy, dx, dy);

            if ((*_links)[l]->bw() > 0){
                int x = (min(sx, dx) + ((max(sx, dx) - min(sx, dx)) / 2));
                int y = (min(sy, dy) + ((max(sy, dy) - min(sy, dy)) / 2) + 3);

                painter.drawText(x, y, QString::number((int) (*_links)[l]->bw()) + " kbps");
            }

        }
    }

    for (int r = 0; r < NETWORK_TOPOLOGY_ROWS; r++){
        for (int c = 0; c < NETWORK_TOPOLOGY_COLS; c++){

            NetworkItem* item = _itemGrid[r][c];

            if (item){

                string cur_im_path;

                QPushButton * button = _buttonGrid[r][c];

                switch (item->itemType()){
                case NODETYPE_CAMERA:{
                    if (item->selected())
                        cur_im_path = CAMERA_IM_PATH_SELECTED;
                    else
                        cur_im_path = CAMERA_IM_PATH;
                    break;
                }
                case NODETYPE_RELAY:{
                    if (item->selected())
                        cur_im_path = RELAY_IM_PATH_SELECTED;
                    else
                        cur_im_path = RELAY_IM_PATH;
                    break;
                }
                case NODETYPE_SINK:{
                    if (item->selected())
                        cur_im_path = SINK_IM_PATH_SELECTED;
                    else
                        cur_im_path = SINK_IM_PATH;
                    break;
                }
                case NODETYPE_COOPERATOR:{
                    if (item->selected())
                        cur_im_path = COOP_IM_PATH_SELECTED;
                    else if (item->enabled())
                        cur_im_path = COOP_IM_PATH;
                    else
                        cur_im_path = COOP_IM_PATH_AVAILABLE;
                    break;
                }
                default:
                    break;
                }

                QImage im(cur_im_path.c_str());
                QIcon ButtonIcon(QPixmap::fromImage(im));
                button->setIcon(ButtonIcon);
                button->setIconSize(button->size());
                button->setHidden(false);



                if (item->itemType()==NODETYPE_COOPERATOR){
                    if (!item->available()){
                        QPushButton * button = _buttonGrid[r][c];
                        button->setVisible(false);
                        button->setIconSize(button->size());
                    }
                }else{
                    if (!item->enabled()){
                        QPushButton * button = _buttonGrid[r][c];
                        button->setVisible(false);
                        button->setIconSize(button->size());
                    }
                }
            }
        }
    }

    painter.end();

}

/**
 * @brief NetworkTopology::getLinkCells
 * Given a Link fills source (s_x,s_y) and destination (d_x,d_y) coordinates
 * @param alink
 * @param s_x
 * @param s_y
 * @param d_x
 * @param d_y
 */
void NetworkTopology::getLinkCells(Link* alink, int &s_x, int &s_y, int &d_x, int &d_y){

    int s_id = alink->source_id();
    int d_id = alink->dest_id();


    for (int r = 0; r < NETWORK_TOPOLOGY_ROWS; r++){
        for (int c = 0; c < NETWORK_TOPOLOGY_COLS; c++){

            NetworkItem *cur = _itemGrid[r][c];

            if (cur && cur->getId()==s_id){
                s_x = cur->col();
                s_y = cur->row();
            }
            if (cur && cur->getId()==d_id){
                d_x = cur->col();
                d_y = cur->row();
            }
        }
    }
}

void NetworkTopology::buttonClicked(int a){

    for (int r = 0; r < NETWORK_TOPOLOGY_ROWS; r++){
        for (int c = 0; c < NETWORK_TOPOLOGY_COLS; c++){

            if (_itemGrid[r][c]){
                _itemGrid[r][c]->setSelected(false);
            }
        }
    }

    int r = a/NETWORK_TOPOLOGY_COLS;
    int c = a%NETWORK_TOPOLOGY_COLS;

    if (_itemGrid[r][c]){
        _itemGrid[r][c]->setSelected(true);

        if (_itemGrid[r][c]->itemType() == NODETYPE_CAMERA){
            emit cameraSelected(_itemGrid[r][c]->getGuiIdx());
        }
    }

    this->repaint();

}

/* Given the GUI id of the camera changes the active camera. Callback from GUI tabs */
void NetworkTopology::changeActiveCamera(int guiIdx){

    int rr;
    int cc;

    for (int r = 0; r < NETWORK_TOPOLOGY_ROWS; r++){
        for (int c = 0; c < NETWORK_TOPOLOGY_COLS; c++){

            if (_itemGrid[r][c] && _itemGrid[r][c]->getGuiIdx()==guiIdx){
                rr = r;
                cc = c;
            }
        }
    }

    int a = cc + rr*NETWORK_TOPOLOGY_COLS;

    buttonClicked(a);
}

void NetworkTopology::changeCoop(int camId, int nCoop){

    qDebug() << "Network topology: changing cooperator status";
    vector<int> RtoBeActivated;
    vector<int> CtoBeActivated;

    int counter = 0;

    for (int r = 0; r < NETWORK_TOPOLOGY_ROWS; r++){
        for (int c = 0; c < NETWORK_TOPOLOGY_COLS; c++){
            if (_itemGrid[r][c]){
                int coop_id = _itemGrid[r][c]->cooperant_id();

                if (coop_id == camId){
                    _itemGrid[r][c]->setEnabled(false);
                    _buttonGrid[r][c]->setEnabled(false);
                }
            }
        }
    }

    for (int r = 0; r < NETWORK_TOPOLOGY_ROWS; r++){
        for (int c = 0; c < NETWORK_TOPOLOGY_COLS; c++){
            if (_itemGrid[r][c]){
                if (counter == nCoop)
                    break;

                int coop_id = _itemGrid[r][c]->cooperant_id();

                if (coop_id == camId){
                    RtoBeActivated.push_back(r);
                    CtoBeActivated.push_back(c);
                    _itemGrid[r][c]->setEnabled(true);
                    _buttonGrid[r][c]->setEnabled(true);
                    counter++;
                }

            }
        }
        if (counter == nCoop)
            break;
    }

    // find links

    this->update();
}


void NetworkTopology::changeAvailCoop(int camId, int nCoop){

    qDebug() << "Network topology: changing available cooperator status";
    qDebug() << "Camera " << camId << " now has " << nCoop << " available coops";

    vector<int> RtoBeActivated;
    vector<int> CtoBeActivated;

    int counter = 0;

    for (int r = 0; r < NETWORK_TOPOLOGY_ROWS; r++){
        for (int c = 0; c < NETWORK_TOPOLOGY_COLS; c++){
            if (_itemGrid[r][c]){
                int coop_id = _itemGrid[r][c]->cooperant_id();

                if (coop_id == camId){
                    _itemGrid[r][c]->setAvailable(false);
                    _buttonGrid[r][c]->setEnabled(false);
                }
            }
        }
    }

    for (int r = 0; r < NETWORK_TOPOLOGY_ROWS; r++){
        for (int c = 0; c < NETWORK_TOPOLOGY_COLS; c++){

            if (counter == nCoop)
                break;
            if (_itemGrid[r][c]){
                int coop_id = _itemGrid[r][c]->cooperant_id();

                if (coop_id == camId){
                    RtoBeActivated.push_back(r);
                    CtoBeActivated.push_back(c);
                    _itemGrid[r][c]->setAvailable(true);
                    // _buttonGrid[r][c]->setEnabled(true);
                    counter++;
                }

            }
        }
        if (counter == nCoop)
            break;
    }

    // find links

    this->update();
}

void NetworkTopology::updateBandwidth(int cam_id, double bw){

    vector<Link*> tempLinks;
    vector<int> idxs;

    for (uint i = 0; i < _links->size(); i++){
        Link* tmp = (*_links)[i];
        if ((tmp->source_id() == cam_id)||(tmp->dest_id() == cam_id)){
            tempLinks.push_back(tmp);
            idxs.push_back(i);
        }
    }

    int good_idx;
    bool found = false;
    if (tempLinks.size() < 1)
        qDebug() << "error setting link bandwidth label";
    else if (tempLinks.size() == 1){
        good_idx = 0;
        found = true;
    }
    else{

        for (uint i = 0; i <tempLinks.size(); i++){
            int partner;
            Link *tmp = tempLinks[i];
            if (tmp->source_id() == cam_id)
                partner = tmp->dest_id();
            else
                partner = tmp->source_id();

            for (int r = 0; r < NETWORK_TOPOLOGY_ROWS; r++){
                for (int c = 0; c < NETWORK_TOPOLOGY_COLS; c++){
                    NetworkItem *tmp_i = _itemGrid[r][c];
                    if (tmp_i && tmp_i->getGuiIdx() == partner){
                        if (tmp_i->itemType()==NODETYPE_RELAY){
                            good_idx = idxs[i];
                            found = true;
                            break;
                        }
                    }
                }
                if (found){
                    break;
                }
            }
            if (found){
                break;
            }
        }
    }

    if (!found){
        qDebug() << "error setting link bandwidth label";
    }
    else{
        (*_links)[good_idx]->setBw(bw);
    }

    this->update();

}

