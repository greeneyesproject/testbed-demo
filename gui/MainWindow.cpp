#include "MainWindow.h"
#include "ui_mainwindow.h"
#include <sstream>
#include <QSignalMapper>
#include <QTimer>
#include <QDebug>
#include <sstream>
#include <Global.h>
#include "GuiNetworkSystem.h"

using namespace std;



MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //    QTimer::singleShot(1000, this, SLOT(test()));
}

MainWindow::~MainWindow()
{
    delete ui;
}


MainWindow::MainWindow(CameraSettings * camSet,
                       std::vector<CameraView*> &camViews,
                       NetworkTopology * netTop,
                       std::vector<Plot*> &camPlot,
                       QWidget *parent):
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    _camSet(camSet),
    _netTop(netTop){

    connect(GuiNetworkSystem::getInstance(), SIGNAL(sinkConnected(QString, unsigned short)),
            this, SLOT(showGuiConnect(QString, unsigned short)));
    connect(GuiNetworkSystem::getInstance(), SIGNAL(sinkDisconnected()),
            this, SLOT(showGuiDisconnect()));
    ui->setupUi(this);

    // ui->lineEdit->setText(DB_PATH);

    connect(ui->actionView_camera_settings, SIGNAL(triggered(bool)), this, SLOT(toggleCameraSettings()));
    connect(ui->actionView_network_topology, SIGNAL(triggered(bool)), this, SLOT(toggleNetworkTopology()));
    connect(ui->actionHide_main_window, SIGNAL(triggered(bool)), this, SLOT(toggleMainWindow()));

    ui->actionView_network_topology->setText("Toggle network topology");
    ui->actionView_camera_settings->setText("Toggle camera settings");
    ui->actionHide_main_window->setText("Toggle main window");

    QSignalMapper *signalMapper = new QSignalMapper(this);
    connect(signalMapper, SIGNAL(mapped(int)), this, SLOT(toggleCameraView(int)));

    QSignalMapper *signalMapperPerf = new QSignalMapper(this);
    connect(signalMapperPerf, SIGNAL(mapped(int)), this, SLOT(toggleCameraPlot(int)));

    for (size_t cc = 0; cc < camViews.size(); cc++){

        _camViews.push_back(camViews[cc]);
        _camPlots.push_back(camPlot[cc]);


        stringstream ss;
        ss << "Toggle camera view " << cc+1;
        QAction* cameraView = new QAction(ui->menuCamera_views);
        cameraView->setText(ss.str().c_str());

        ss.str("");
        ss << "Toggle camera performance " << cc+1;
        QAction* cameraPerformance = new QAction(ui->menuCamera_performance);
        cameraPerformance->setText(ss.str().c_str());

        signalMapper->setMapping(cameraView, cc);
        connect(cameraView, SIGNAL(triggered()), signalMapper, SLOT(map()));
        _cameraViewActions.push_back(cameraView);

        ui->menuCamera_views->addAction(cameraView);

        signalMapperPerf->setMapping(cameraPerformance, cc);
        connect(cameraPerformance, SIGNAL(triggered()), signalMapperPerf, SLOT(map()));
        _cameraPerformanceActions.push_back(cameraPerformance);

        ui->menuCamera_performance->addAction(cameraPerformance);
    }

}

void MainWindow::toggleCameraSettings(){

    if (_camSet->isHidden()){
        _camSet->show();
        ui->actionView_camera_settings->setText("Toggle camera settings");
    }
    else{
        _camSet->hide();
        ui->actionView_camera_settings->setText("Toggle camera settings");
    }
}

void MainWindow::toggleCameraView(int a){

    CameraView * cur = _camViews[a];
    if (cur->isHidden()){
        cur->show();
        stringstream ss;
        ss << "Toggle camera view " << a+1;
        _cameraViewActions[a]->setText(ss.str().c_str());
    }
    else{
        cur->hide();
        stringstream ss;
        ss << "Toggle camera view " << a+1;
        _cameraViewActions[a]->setText(ss.str().c_str());
    }
}

void MainWindow::toggleNetworkTopology(){

    if (_netTop->isHidden()){
        _netTop->show();
        ui->actionView_network_topology->setText("Toggle network topology");
    }
    else{
        _netTop->hide();
        ui->actionView_network_topology->setText("Toggle network topology");
    }
}

void MainWindow::toggleMainWindow(){

    if (this->isHidden()){
        this->show();
        ui->actionHide_main_window->setText("Toggle main window");
    }
    else{
        this->hide();
        ui->actionHide_main_window->setText("Toggle main window");
    }
}

void MainWindow::test(){

    qDebug() << "timer fired";

}

void MainWindow::showGuiConnect(QString address, unsigned short port){
    stringstream ss;
    ss << "Connection from " << address.toStdString() << ":" << port;
    ui->label->setText(ss.str().c_str());

}

void MainWindow::showGuiDisconnect(){
    stringstream ss;
    ss << "Server is ready...";
    ui->label->setText(ss.str().c_str());
}

std::vector<Plot *> MainWindow::camPlots() const
{
    return _camPlots;
}

void MainWindow::setCamPlots(const std::vector<Plot *> &camPlots)
{
    _camPlots = camPlots;
}

void MainWindow::toggleCameraPlot(int a){

    Plot * cur = _camPlots[a];
    if (cur->isHidden()){
        cur->show();
        stringstream ss;
        ss << "Toggle camera performance " << a+1;
        _cameraPerformanceActions[a]->setText(ss.str().c_str());
    }
    else{
        cur->hide();
        stringstream ss;
        ss << "Toggle camera performance " << a+1;
        _cameraPerformanceActions[a]->setText(ss.str().c_str());
    }
}
