#include "MainWindow.h"
#include "CameraSettings.h"
#include "NetworkTopology.h"
#include "CameraView.h"
#include <QApplication>
#include <QIntValidator>
#include <QValidator>
#include "Global.h"
#include <sstream>
#include "ParseNetworkConfigurationXml.h"
#include "Camera.h"
#include <QDebug>
#include <QDesktopWidget>
#include <IoThread.h>
#include <Multimedia/VisualFeatureExtraction.h>
#include <InverseBrisk.h>
#include <PerformanceManager.h>
#include "GuiProcessingSystem.h"
#include <Plot.h>
#include "Camera.h"
#include "PKLot.h"
#include <signal.h>
#include "GuiNetworkSystem.h"

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <Multimedia/VisualFeatureDecoding.h>

#include <ObjectTracking.h>

using boost::asio::ip::tcp;

int main(int argc, char *argv[]){

    QApplication application(argc, argv);

    /*
     * XML configuration parser
     */
    ParseNetworkConfigurationXML parser;
    Sink* sink = new Sink();
    vector<Camera*>* cameras = Camera::getCameras();
    vector<Link*> links;
    vector<NetworkItem*> items;

    parser.parse(XML_NETWORK_PATH, items, links, cameras, sink);
    std::cout << "main: XML sink: " << *sink << endl;
    Camera::printCameras();

    /*
     * PKLot configuration parser
     */
    for (uint camIdx = 0;camIdx < cameras->size(); camIdx++){
        Camera* cam = cameras->at(camIdx);
        std::vector<PKLot> pklots;
        cv::Size imgSize(FRAME_W,FRAME_H);
        stringstream ss;
        ss << PKLOT_XML_PATH << cam->getId() << ".xml";
        PKLot::parse(ss.str(),pklots);
        cam->setPKLots(pklots,imgSize);
    }

    /*
     * GUI
     */
    QDesktopWidget desktop;
    QRect mainScreenSize = desktop.availableGeometry(desktop.primaryScreen());

    std::vector<CameraView*> cam_views(cameras->size());



    /* Create the server */
    IoThread* ioThread = IoThread::getInstance(GUI_LISTENING_TCP_PORT);

    //cam_views.reserve(cameras.size());
    for (uint camIdx = 0; camIdx < cameras->size(); camIdx++){

        Camera* curCam = cameras->at(camIdx);
        curCam->setGuiIdx(camIdx);

        std::stringstream ss;
        ss << "Camera " << camIdx+1;
        QString wTitle(ss.str().c_str());

        CameraView* curCamView = new CameraView(curCam, 0, camIdx);

        curCamView->setWindowTitle(wTitle);
        int cwTLx = (10 + curCamView->width())*camIdx;
        int cwTLy = 10;

        curCamView->setGeometry(cwTLx, cwTLy, curCamView->width(), curCamView->height());
        curCamView->show();

        cam_views[camIdx] = curCamView;
    }

    ObjectTracking * task_track = ObjectTracking::getObjectTracking(DB_PATH);

    task_track->setCameras(cameras);
    task_track->setCamViews(cam_views);

    for (vector<Camera*>::iterator camIt = cameras->begin(); camIt != cameras->end(); camIt++){
        task_track->connectTasks(*camIt);
    }

    GuiProcessingSystem* guiProcessingSistem = GuiProcessingSystem::getInstance();

    inverse_BRISK * invBRISK = inverse_BRISK::getinverse_BRISK(guiProcessingSistem->getBriskExtractor(),FRAME_W, FRAME_H);
    invBRISK->build_database(DB_IMAGE_PATH);
    invBRISK->setNCameras(cameras->size());
    invBRISK->setCameras(cameras);

    invBRISK->connectTasks(cameras->size());

    PerformanceManager * perf = PerformanceManager::getPerformanceManager(cameras, 40*1000/1000, 1000,5);

    CameraSettings camSet;

    NetworkTopology net(&camSet, &items, &links, 0);
    camSet.setNetworkTopology(&net);

    int netTLx = 10;
    int netTLy = mainScreenSize.height() - net.geometry().height();

    net.setGeometry(netTLx, netTLy, net.geometry().width(), net.geometry().height());
    /*net.show();*/

    int csTLy = 10;
    int csTLx = mainScreenSize.width() - camSet.geometry().width() - 10;

    camSet.setGeometry(csTLx, csTLy, camSet.geometry().width(), camSet.geometry().height());

    camSet.show();

    std::vector<Plot*> cam_plots(cameras->size());

    for (uint8_t cam = 0; cam < cameras->size(); cam++){

        Plot * p = new Plot(*perf, cam, *(cameras->at(cam)));
        p->setWindowTitle("Camera " + QString::number(cam + 1));
        int plotTLx = mainScreenSize.width() - (10 + p->width())*(cameras->size() - cam);
        int plotTLy = mainScreenSize.height() - p->height();

        p->setGeometry(plotTLx, plotTLy, p->width(), p->height());
        p->show();

        cam_plots.push_back(p);
    }

    MainWindow mainWindow(&camSet, cam_views, &net, cam_plots);

    int wTLx = 20 + net.geometry().width();
    int wTLy = mainScreenSize.height() - net.geometry().height();

    mainWindow.setGeometry(wTLx, wTLy, mainWindow.geometry().width(), mainWindow.geometry().height());
    mainWindow.show();

    // inverse BRISK thread
    QThread* inverseBriskThread = new QThread;
    invBRISK->moveToThread(inverseBriskThread);
    inverseBriskThread->start();

    // object tracking+recognition thread
    QThread* recognitionTrackingThread = new QThread;
    task_track->moveToThread(recognitionTrackingThread);
    recognitionTrackingThread->start();

    perf->connectTasks();

    /* Gentle shutdown */
    signal(SIGINT, GuiNetworkSystem::shutdown);
    signal(SIGQUIT, GuiNetworkSystem::shutdown);

    /* Start the network system */
    ioThread->start();

    return application.exec();
}

