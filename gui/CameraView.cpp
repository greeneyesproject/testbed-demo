#include "CameraView.h"
#include "ui_cameraview.h"
#include "Global.h"
#include <opencv2/opencv.hpp>

CameraView::CameraView(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CameraView)
{
    ui->setupUi(this);
}

CameraView::CameraView(Camera * camera, QWidget *parent, int tabIdx):
    QWidget(parent),
    ui(new Ui::CameraView){

    _cam = camera;
    _tabIdx = tabIdx;

    connect(_cam, SIGNAL(CTAFrameUpdatedSignal()), this, SLOT(showCTAFrame()));
    connect(_cam, SIGNAL(ATCFrameUpdatedSignal()), this, SLOT(showATCFrame()));
    connect(_cam, SIGNAL(ATCRecFrameUpdatedSignal()), this, SLOT(showATCRecFrame()));
    connect(_cam, SIGNAL(frameRateUpdatedSignal()), this, SLOT(updateFrameRate()));
    //***NBS***//
    connect(_cam, SIGNAL(residualEnergyUpdatedSignal()), this, SLOT(updateResidualEnergy()));
    //***NBS***//
    connect(_cam,SIGNAL(curBandwidthChangedSignal(int,double)),this,SLOT(updateBandwidthSlot(int,double)));
    connect (_cam,SIGNAL(recognitionCompletedSignal(QString)),SLOT(updateObjectSlot(QString)));

    ui->setupUi(this);

    ui->label_image->setText("");

}

CameraView::~CameraView(){
    delete ui;
}
Camera *CameraView::cam() const{
    return _cam;
}

void CameraView::setImage(QImage image){
    ui->label_image->setPixmap(QPixmap::fromImage(image));
    // ui->label_image->adjustSize();
}

void CameraView::showCTAFrame(){

    std::cout << "showing CTA frame..." << std::endl;

    cv::Mat convertedImg;
    cv::cvtColor(_cam->getCTAFrame(), convertedImg, CV_BGR2RGB);

    QImage im((uchar*)convertedImg.data, convertedImg.cols, convertedImg.rows, convertedImg.step, QImage::Format_RGB888);

    setImage(im);

}

/* Show _cam->getATCFrame() */
void CameraView::showATCFrame(){
    std::cout << "CameraView::showATCFrame" << std::endl;

    cv::Mat convertedImg;
    cv::cvtColor(_cam->getATCFrame(), convertedImg, CV_BGR2RGB);

    std::cout << "CameraView::showATCFrame: QImage" << std::endl;
    QImage im((uchar*)convertedImg.data, convertedImg.cols, convertedImg.rows, convertedImg.step, QImage::Format_RGB888);
    setImage(im);

}

void CameraView::showATCRecFrame(){

    std::cout << "showing ATCRec frame..." << std::endl;

    cv::Mat convertedImg = _cam->getATCRecFrame();
    cv::cvtColor(convertedImg, convertedImg, CV_BGR2RGB);

    QImage im((uchar*)convertedImg.data, convertedImg.cols, convertedImg.rows, convertedImg.step, QImage::Format_RGB888);

    setImage(im);

}

void CameraView::updateObjectSlot(QString objName){

    ui->label_object->setText(objName);

}

void CameraView::updateFrameRate(){

    ui->label_frameRate->setText(QString::number(_cam->getFrameRate(),'f',2) + " fps");
}

//***NBS***//
void CameraView::updateResidualEnergy(){
    ui->label_energy->setText(QString::number(_cam->getResidualEnergyPerc(),'f',0) + " %" + " (" + QString::number(_cam->getResidualEnergyJoule(),'f',0) + " J)");
}
//***NBS***//

void CameraView::updateBandwidthSlot(int /*cam Idx*/,double bandwidth){
    //XXX sistemare come per altri campi salvando sulla camera
    if (bandwidth > 0.001 && !std::isinf(bandwidth) && !std::isnan(bandwidth)){
        ui->label_bandwidth->setText(QString::number(bandwidth,'f',0) + " kbps");
    }else{
        ui->label_bandwidth->setText("--");
    }
}
