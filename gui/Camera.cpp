#include "Camera.h"
#include <Network/NetworkNode.h>
#include "GuiProcessingSystem.h"
#include "Multimedia/VisualFeatureEncoding.h"
#include <QTimerEvent>

using namespace std;

std::vector<Camera*> Camera::_cameras;

Camera::Camera(QObject *parent):QObject(parent){}

Camera::Camera(uint id, uint am_address, string ip_address, NetworkItem* networkItem,
               double energyBudget, QObject *parent) :
    QObject(parent){

    _id = id;
    _amaddr = am_address;
    _ipaddr = ip_address;
    _networkItem = networkItem;
    _totalEnergyBudget = energyBudget;
    _residualEnergyBudget = energyBudget;
    _guiIdx = -1;
    _linkType=LINKTYPE_TCP;

    _operativeMode = OPERATIVEMODE_OBJECT;
    _imageSource = IMAGESOURCE_LIVE;
    _currentMode = CAMERA_MODE_CTA;
    _detThres = 60;
    _maxF = 50;
    _showR = false;
    _tracking = false;
    _recognition = true;
    _encodeKeypoints=true;
    _encodeFeatures=true;
    _datc = false;
    _nCoop = 1;
    _QF = 20;
    //_ATCCodingInfo = FULL;
    _active = false;
    _numFeatPerBlock = 20;
    _numSlices = 5;
    _nCoopAvailable = 0;
    _binShift = 0;
    _valShift = 0;

    _nbs = false;
    _nbsReady = false;
    _nbsRecReady = false;


    _networkNode = NetworkNode::getCameraById(_id);

    _tempProcTime = 0;
    _tempTxTime = 0;
    _processingEnergy = 0;
    _txEnergy = 0;


    _CTAFrame = cv::Mat(FRAME_H, FRAME_W, CV_8UC3);
    _CTAFrame = cv::Scalar::all(0);
    _CTAFrameClean = cv::Mat(FRAME_H, FRAME_W, CV_8UC3);
    _CTAFrameClean = cv::Scalar::all(0);

    _ATCFrame = cv::Mat(FRAME_H, FRAME_W, CV_8UC3);
    _ATCFrame = cv::Scalar::all(255);
    _ATCRecFrame = cv::Mat(FRAME_H, FRAME_W, CV_8UC3);
    _ATCRecFrame = cv::Scalar::all(255);
    /*_ATCFrameClean = cv::Mat(FRAME_H, FRAME_W, CV_8UC3);
    _ATCFrameClean = cv::Scalar(255,255,255);*/
    _ATCRecFrameClean = cv::Mat(FRAME_H, FRAME_W, CV_8UC3);
    _ATCRecFrameClean = cv::Scalar::all(255);

    _wifiBw = WIFI_BW_DEFAULT;

    _timer = new QTimer();
    _timer->setSingleShot(true);
    connect(_timer,SIGNAL(timeout()),this,SLOT(_timerFiredSlot()));
    connect(this,SIGNAL(startCameraTimerSignal()),this,SLOT(startCameraTimerSlot()));

}

/**
 * @brief Camera::setGuiIdx
 * Camera index inside the GUI. It corresponds to the Tab and View indexes.
 * @param tabIdx
 */
void Camera::setGuiIdx (uint guiIdx){
    _guiIdx = guiIdx;
    if (_networkItem){
        _networkItem->setGuiIdx(guiIdx);
    }
}


/**
 * @brief Camera::storeParameters
 * Store parameters of the GUI to maintain coherency w.r.t. the remote camera ones.
 * To be called only when a START message is sent.
 */
void Camera::storeParameters(){

    cta_param.quality_factor = _QF;
    cta_param.num_slices = _numSlices;
    cta_param.framePayloadSize = 0;

    atc_param.encodeKeypoints = _encodeKeypoints;
    atc_param.encodeFeatures = _encodeFeatures;
    atc_param.detection_threshold = _detThres;
    atc_param.max_features = _maxF;
    atc_param.num_feat_per_block = _numFeatPerBlock;

    atc_param.binShift = _binShift;
    atc_param.valShift = _valShift;

    atc_param.framePayloadSize = 0;
}

void Camera::setATCSlot(bool setter){
    if (setter){
        _currentMode = CAMERA_MODE_ATC;
    }
    else{
    }
}

void Camera::setATCRecFrame(const cv::Mat &ATCRecFrame)
{
    _ATCRecFrame = ATCRecFrame.clone();
    _ATCRecFrameClean = ATCRecFrame.clone();

    std::cout << "ATC Reconstructed frame updated" << std::endl;
    emit ATCRecFrameUpdatedSignal();
}

void Camera::setCTASlice(cv::Mat slice, cv::Point topLeft, cv::Point /*bottomRight*/){


    if (!slice.rows || !slice.cols){
        return;
    }

    cv::Mat dest = _CTAFrame.rowRange(topLeft.y,topLeft.y+slice.rows).colRange(topLeft.x,topLeft.x+slice.cols);
    if (slice.channels()!=3){
        cv::cvtColor(slice,dest,CV_GRAY2BGR);
    }
    slice.copyTo(dest);

    vector<cv::Point2f> pts_w = _bBox;
    if(_tracking && pts_w.size()>0){
        cv::line(_CTAFrame, pts_w.at(0), pts_w.at(1), cv::Scalar(0, 255, 0), 2);
        cv::line(_CTAFrame, pts_w.at(1), pts_w.at(2), cv::Scalar(0, 255, 0), 2);
        cv::line(_CTAFrame, pts_w.at(2), pts_w.at(3), cv::Scalar(0, 255, 0), 2);
        cv::line(_CTAFrame, pts_w.at(3), pts_w.at(0), cv::Scalar(0, 255, 0), 2);
    }

    emit CTAFrameUpdatedSignal();
}

void Camera::showCTAFrame(cv::Mat& slice){

    _CTAFrame = slice.clone();
    _CTAFrameClean = slice.clone();

    vector<cv::Point2f> pts_w = _bBox;
    if(_tracking && pts_w.size()>0){
        cv::line(_ATCFrame, pts_w.at(0), pts_w.at(1), cv::Scalar(0, 255, 0), 2);
        cv::line(_ATCFrame, pts_w.at(1), pts_w.at(2), cv::Scalar(0, 255, 0), 2);
        cv::line(_ATCFrame, pts_w.at(2), pts_w.at(3), cv::Scalar(0, 255, 0), 2);
        cv::line(_ATCFrame, pts_w.at(3), pts_w.at(0), cv::Scalar(0, 255, 0), 2);
    }

    std::cout << "Camera::showCTAFrame: CTA frame updated" << std::endl;

    emit CTAFrameUpdatedSignal();
}


void Camera::showATCFrame(cv::Mat& slice){

    _ATCFrame = slice.clone();
    _ATCFrameClean = slice.clone();

    vector<cv::Point2f> pts_w = _bBox;
    if(_tracking && pts_w.size()>0){
        cv::line(_ATCFrame, pts_w.at(0), pts_w.at(1), cv::Scalar(0, 255, 0), 2);
        cv::line(_ATCFrame, pts_w.at(1), pts_w.at(2), cv::Scalar(0, 255, 0), 2);
        cv::line(_ATCFrame, pts_w.at(2), pts_w.at(3), cv::Scalar(0, 255, 0), 2);
        cv::line(_ATCFrame, pts_w.at(3), pts_w.at(0), cv::Scalar(0, 255, 0), 2);
    }

    std::cout << "Camera::setATCBlock: ATC frame updated" << std::endl;

    /* Notify CameraView.showATCFrame() */
    emit ATCFrameUpdatedSignal();
}

void Camera::newATCFrame(){

    switch (_operativeMode){
    case OPERATIVEMODE_OBJECT:
        _ATCFrame = cv::Mat(FRAME_H, FRAME_W, CV_8UC3);
        _ATCFrame = cv::Scalar::all(255);
        _ATCFrameClean = cv::Mat(FRAME_H, FRAME_W, CV_8UC3);
        _ATCFrameClean = cv::Scalar::all(255);
        break;
    case OPERATIVEMODE_PKLOT:
        _ATCFrame = _pklotBase.clone();
        _ATCFrameClean = _pklotBase.clone();
        break;
    }


}

void Camera::newCTAFrame(){

    _CTAFrame = cv::Mat(FRAME_H, FRAME_W, CV_8UC3);
    _CTAFrame = cv::Scalar::all(0);
    _CTAFrameClean = cv::Mat(FRAME_H, FRAME_W, CV_8UC3);
    _CTAFrameClean = cv::Scalar::all(0);
}

void Camera::addFeatures(std::vector<cv::KeyPoint> kpts, cv::Mat desc){

    for (size_t k = 0; k < kpts.size(); k++){
        _keypoints.push_back(kpts[k]);
    }
    _descriptors.push_back(desc);

}

void Camera::resetFeatures(){

    _keypoints.clear();
    _descriptors.release();

}

vector<Camera*>* Camera::getCameras(){
    return &_cameras;
}


Camera* Camera::getCameraById(uint id){
    for (vector<Camera*>::iterator it = _cameras.begin(); it != _cameras.end(); ++it){
        if ((*it)->getId()==id){
            return *it;
        }
    }
    return NULL;
}

void Camera::printCameras(){
    for (vector<Camera*>::iterator camIt = _cameras.begin(); camIt != _cameras.end(); camIt++){
        cout << (**camIt) << endl;
    }
}

void Camera::_timerFiredSlot(){
    if (_active){
        GuiProcessingSystem::getInstance()->endOfFrame(this);
    }
}

void Camera::startCameraTimerSlot(){
    if (_timer->isActive()){
        _timer->stop();
    }
    _timer->start(_restartInterval);
}

/**
 * @brief Camera::setOperativeMode
 * Set the new operative mode for the camera.
 * @param om
 */
void Camera::setOperativeMode(OperativeMode om){
    if (om!=_operativeMode){
        _operativeMode = om;

        /* Reconstruction is not available for parkingLot */
        _showR = false;

        /* DATC not yet available for parking lot. Will be added */
        _datc = false;

        /* NBS not yet available for parking lot. Will be added */
        _nbs = false;

        /* Tracking doesn't make sense for pklot */
        _tracking = false;

    }
}

/**
 * @brief Camera::setPKLots
 * Sets the parking lots, the keypoints and the keypoints bitstream
 * @param pklots
 * @param imgSize
 */
void Camera::setPKLots(std::vector<PKLot>&pklots,cv::Size& imgSize){
    _pklotBase = cv::Mat(imgSize,CV_8UC3,PKLOT_BKG_COLOR);
    _pklot = pklots;
    for (ushort lotIdx = 0; lotIdx < _pklot.size(); ++lotIdx){
        PKLot pklot = pklots.at(lotIdx);

        cv::KeyPoint kpt;
        pklot.getKpt(kpt);
        _pklotKpts.push_back(kpt);

        pklot.drawLines(_pklotBase);
    }
    VisualFeatureEncoding* encoder = VisualFeatureEncoding::getInstance();
    encoder->encodeKeyPoints(codingParams(true),_pklotKpts,_pklotKptsBitstream,imgSize,true);

    stringstream ss;
    ss << "camera_" << _id << "_kptsBitstream.bmp";
    cv::imwrite(ss.str(),_pklotKptsBitstream);
}

/* Set new predictions and calculate new statistic prediction */
int Camera::setPKLotPredictions(const Mat &predictions){

    if (predictions.rows != _pklot.size()){
        // Wrong prediction data arrived here.
        return 1;
    }

    cv::Mat predictionsT = predictions.t();
    /* Store values in the history. Rows = #history predictions, Cols = #parking lots */
    if (_pklotPredictionsHistory.cols != predictionsT.cols){
        _pklotPredictionsHistory = predictionsT;
    }else{
        _pklotPredictionsHistory.push_back(predictionsT);
    }
    if (_pklotPredictionsHistory.rows > PKLOT_MEDIAN_FILTER_LENGTH){
        _pklotPredictionsHistory = _pklotPredictionsHistory.rowRange(_pklotPredictionsHistory.rows-PKLOT_MEDIAN_FILTER_LENGTH,_pklotPredictionsHistory.rows);
    }

    cv::Mat historyFloat;
    _pklotPredictionsHistory.convertTo(historyFloat,CV_32FC1);

    for (size_t idx = 0; idx < predictions.rows ; ++idx){
        float mean = cv::mean(historyFloat.col(idx))[0];
        _pklot.at(idx).setOccupied(mean>0.5);
    }
    return 0;
}

Mat Camera::getPklotWithOccupancy()
{
    cv::Mat img = _pklotBase.clone();

    for (uint idx = 0; idx < _pklotKpts.size() ; ++idx){
        _pklot.at(idx).fill(img);
    }
    for (uint idx = 0; idx < _pklotKpts.size() ; ++idx){
        _pklot.at(idx).drawLines(img);
    }

    return img;
}

Mat Camera::getPklotOnlyOccupancy()
{
    cv::Mat img(_pklotBase.size(),CV_8UC3,cv::Scalar::all(0));

    for (uint idx = 0; idx < _pklotKpts.size() ; ++idx){
        _pklot.at(idx).fill(img);
    }
    for (uint idx = 0; idx < _pklotKpts.size() ; ++idx){
        _pklot.at(idx).drawLines(img);
    }

    return img;
}
