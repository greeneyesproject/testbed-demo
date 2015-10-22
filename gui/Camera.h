#ifndef CAMERA_H
#define CAMERA_H

#include <opencv2/core/core.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <QObject>
#include "Global.h"
#include <QDebug>
#include <iostream>
#include <opencv2/highgui/highgui.hpp>
#include <Messages/Message.h>
#include "NetworkItem.h"
#include <QTimer>
#include "PKLot.h"

class SessionOld;
class CameraView;
class NetworkItem;
class NetworkNode;

using namespace std;

typedef enum {
    CAMERA_MODE_ATC,
    CAMERA_MODE_CTA
}CameraModeType;

typedef struct CTA_param{
    int quality_factor;
    int num_slices;
    size_t framePayloadSize;
    //.... other params such as num slices (w and h?)
} CTA_param_t;

typedef struct ATC_param{
    int max_features;
    DetectorType det;
    DescriptorType desc;
    double detection_threshold;
    int desc_length;
    bool rotation_invariant;
    bool encodeKeypoints;
    bool encodeFeatures;
    bool transmit_keypoints;
    bool transmit_scale;
    bool transmit_orientation;
    int num_feat_per_block;
    uchar binShift;
    uchar valShift;
    size_t framePayloadSize;
    // .... other params such as num blocks
} ATC_param_t;

class Camera : public QObject
{
    Q_OBJECT
public:

    CTA_param_t cta_param;
    ATC_param_t atc_param;

    static void printCameras();
    static std::vector<Camera*>* getCameras();

    static Camera* getCameraById(uint id);

    explicit Camera(QObject *parent = 0);
    explicit Camera(uint id, uint am_address, string ip_address, NetworkItem* networkItem, double energyBudget, QObject *parent = 0);

    friend std::ostream& operator<<(std::ostream& os, const Camera& camera){
        return os << "Camera - id:" << camera._id <<
                     " am:"  << camera._amaddr <<
                     " ip:" << camera._ipaddr <<
                     " gui:" << camera._guiIdx;
    }

    void setGuiIdx (uint guiIdx);

    void setATCRecFrame(const cv::Mat &ATCRecFrame);

    void newATCFrame();
    void newCTAFrame();

    void setCTASlice(cv::Mat slice, cv::Point topLeft, cv::Point bottomRight);
    void showATCFrame(cv::Mat &slice);
    void showCTAFrame(cv::Mat &slice);

    void addFeatures(std::vector<cv::KeyPoint> kpts, cv::Mat desc);
    void resetFeatures();

    void storeParameters();

    CameraModeType getCurrentMode() const{ return _currentMode;}

    uint getGuiIdx (){ return _guiIdx; }

    NetworkNode* getNetworkNode(){ return _networkNode; }

    void setLinkType(LinkType linkType){ _linkType=linkType; }
    LinkType getLinkType() const{ return _linkType; }

    cv::Mat getCTAFrame() const { return _CTAFrame; }

    cv::Mat getATCFrame() const { return _ATCFrame; }

    uint getId() const { return _id; }

    string getIpAddr(){return _ipaddr;}

    int getQf() const { return _QF; }

    int getNumCoop() const { return _nCoop; }

    bool getDatc() const { return _datc; }
    void setDatc(bool datc) { _datc = datc; }

    OperativeMode getOperativeMode(){return _operativeMode;}
    void setOperativeMode(OperativeMode);

    ImageSource getImageSource(){return _imageSource;}
    void setImageSource(ImageSource imgSrc_){ _imageSource = imgSrc_;}

    //***NBS***/
    bool getNbsReady() {return _nbsReady;}
    void setNbsReady(bool nbsReady) {_nbsReady = nbsReady;}
    bool getNbsRecReady() {return _nbsRecReady;}
    void setNbsRecReady(bool nbsRecReady) {_nbsRecReady = nbsRecReady;}
    int getNbsOffset() {return _nbsOffset;}
    void setNbsOffset(int nbsOffset) {_nbsOffset = nbsOffset;}
    bool getNbs() const {return _nbs;}
    void setNbs(bool nbs) {_nbs = nbs; }
    Camera* getBargainCamera() const {return _bargainCamera;}
    void setBargainCamera(Camera* bargainCamera) { _bargainCamera = bargainCamera;}
    uint getResidualEnergyPerc(){return _residualEnergy;}
    double getResidualEnergyJoule(){return _residualEnergyBudget;}
    void setResidualEnergy(uint residualEnergy) {
        _residualEnergy = residualEnergy;
         emit residualEnergyUpdatedSignal();
    }
    void decrementEnergy(double energy){
        uint residualEnergy;
        _residualEnergyBudget-=energy;
        if (_residualEnergyBudget <= 0){
            _residualEnergyBudget = _totalEnergyBudget;
        }
        residualEnergy = (uint)(100*_residualEnergyBudget/_totalEnergyBudget);
        setResidualEnergy(residualEnergy);
    }
    //***NBS***//

    bool getEncodeKeypoints() const { return _encodeKeypoints; }
    void setEncodeKeypoints(bool encodeKeypoints) { _encodeKeypoints = encodeKeypoints; }

    bool getEncodeFeatures() const { return _encodeFeatures; }
    void setEncodeFeatures(bool encodeFeatures) { _encodeFeatures = encodeFeatures; }

    bool getShowReconstruction() const { return _showR; }

    bool getTrackingEnabled() const { return _tracking; }

    int getMaxNumFeatures() const { return _maxF; }

    int getDetThres() const { return _detThres; }
    void setDetThres(int detThres) { _detThres = detThres; }

    int getNumBin()const {return ((180-1) >> _binShift)+1;}
    int getBinShift() const {return _binShift;}
    void setBinShift(int binShift_){_binShift = binShift_;}

    int getQuantStep()const {return 1 << _valShift;}
    int getValShift() const {return _valShift;}
    void setValShift(int valShift_){_valShift = valShift_;}

    bool getActive() const { return _active; }
    void setActive(bool active) { _active = active; }

    int getNumFeatPerBlock() const { return _numFeatPerBlock; }
    void setNumFeatPerBlock(int numFeatPerBlock) { _numFeatPerBlock = numFeatPerBlock; }

    int getNumSlices() const { return _numSlices; }
    void setNumSlices(int numSlices) { _numSlices = numSlices; }

    int getNumAvailableCooperators() const { return _nCoopAvailable; }

    cv::Mat getATCFrameClean() const { return _ATCFrameClean; }
    cv::Mat getCTAFrameClean() const { return _CTAFrameClean; }
    cv::Mat getATCRecFrameClean() const {return _ATCRecFrameClean; }

    bool getRecognitionEnabled() const { return _recognition; }

    double getTempTxTime() const { return _tempTxTime; }
    void setTempTxTime(double tempTxTime) { _tempTxTime = tempTxTime; }

    double getTempProcTime() const { return _tempProcTime;}
    void setTempProcTime(double tempProcTime) { _tempProcTime = tempProcTime; }

    double getTxEnergy() const { return _txEnergy; }
    void setTxEnergy(double txEnergy) { _txEnergy = txEnergy; }

    double getProcessingEnergy() const { return _processingEnergy; }
    void setProcessingEnergy(double processingEnergy) { _processingEnergy = processingEnergy; }

    double getFrameRate() const { return _frameRate; }
    void setFrameRate(double frameRate) {
        _frameRate = frameRate;
        emit frameRateUpdatedSignal();
    }

    cv::Mat getGoodDescriptors() const { return _goodDescriptors; }
    void setGoodDescriptors(const cv::Mat &goodDescriptors) { _goodDescriptors = goodDescriptors; }

    std::vector<cv::KeyPoint> getGoodKeypoints() const { return _goodKeypoints; }
    void setGoodKeypoints(const std::vector<cv::KeyPoint> &goodKeypoints) { _goodKeypoints = goodKeypoints; }

    cv::Mat getDescriptors() const { return _descriptors; }
    void setDescriptors(const cv::Mat &descriptors) { _descriptors = descriptors; }

    std::vector<cv::KeyPoint> getKeypoints() const { return _keypoints; }
    void setKeypoints(const std::vector<cv::KeyPoint> &keypoints) { _keypoints = keypoints; }

    cv::Mat getATCRecFrame() const { return _ATCRecFrame; }

    std::vector<cv::Point2f> getBoundingBoxes() const {return _bBox;}
    void setBoundingBoxes(const std::vector<cv::Point2f> &boundingBoxes){ _bBox = boundingBoxes; }

    Bitstream getPKLotKptsBitstream() const {return _pklotKptsBitstream;}
    void setPKLots(std::vector<PKLot>&pklots, cv::Size &imgSize);

    std::vector<PKLot> getPKLots() const{ return _pklot; }
    std::vector<cv::KeyPoint> getPKLotKpts() const{ return _pklotKpts; }

    cv::Mat getPklotBase() const{ return _pklotBase;}

    int setPKLotPredictions(const cv::Mat &predictions);

    cv::Mat getPklotWithOccupancy();
    cv::Mat getPklotOnlyOccupancy();

    void setWifiBw(int bw_){ _wifiBw = bw_;}
    int getWifiBw(){return _wifiBw;}

signals:

    void CTAFrameUpdatedSignal();
    void ATCFrameUpdatedSignal();
    void ATCRecFrameUpdatedSignal();

    /**
     * @brief frameCompletedSignal
     * Emitted once every frame completion
     */
    void frameCompletedSignal(int,int);
    void reconstructFrameSignal(int);

    void frameRateUpdatedSignal();
    //***NBS***//
    void residualEnergyUpdatedSignal();
    //***NBS***//

    void recognitionCompletedSignal(QString label,QString result);

    void updateNumAvailableCooperatorsSignal(uint, int);
    void startSentSignal(int, bool); // if true, camera mode has been toggled by user
    void stoppedSignal(int);

    void curBandwidthChangedSignal(int, double);

    void startCameraTimerSignal();

public slots:

    void setCurrentModeSlot(CameraModeType currentMode){ _currentMode = currentMode;}

    void setATCSlot(bool);
    void setCTAFrameSlot(const cv::Mat &CTAFrame){_CTAFrame = CTAFrame;}
    void setATCFrameSlot(const cv::Mat &ATCFrame){_ATCFrame = ATCFrame;}

    void setQfSlot(int QF) { _QF = QF; }
    void setNumCoopSlot(int nCoop) { _nCoop = nCoop; }
    void setShowReconstructionSlot(bool showR) { _showR = showR; }
    void setTrackingEnabledSlot(bool tracking) { _tracking = tracking; }

    void setRecognitionEnabledSlot(bool recognition) {
        _recognition = recognition;
    }
    void setMaxNumFeaturesSlot(int maxF) { _maxF = maxF; }
    void setObjectSlot(const string &object){ _object=object; }
    void setNumAvailableCooperatorsSlot(int nCoopAvailable){
        _nCoopAvailable = nCoopAvailable;
        if (!_nCoopAvailable){
            _datc = false;
        }
        if (_nCoop > _nCoopAvailable){
            _nCoop = _nCoopAvailable;
        }
        else if ((_nCoop == 0) & (_nCoopAvailable > 0)){
            _nCoop = 1;
        }
    }
    void startCameraTimerSlot();

private slots:
    void _timerFiredSlot();

private:

    const static unsigned int _restartInterval = 10000; /* [ms] */



    static std::vector<Camera*> _cameras;

    //***NBS***/
    double _totalEnergyBudget; /*Joule*/
    double _residualEnergyBudget; /*Joule*/
    //***NBS***/

    NetworkNode* _networkNode;

    uint _id;
    string _ipaddr;
    uint _amaddr;
    NetworkItem* _networkItem;
    int _guiIdx;

    QTimer* _timer;

    LinkType _linkType;

    int _wifiBw;

    CameraModeType _currentMode;
    int _detThres;
    int _maxF;
    int _binShift;
    int _valShift;
    bool _showR;

    bool _encodeKeypoints;
    bool _encodeFeatures;
    bool _datc;
    int _nCoop;
    int _QF;
    cv::Mat _ATCFrame;
    cv::Mat _ATCFrameClean;

    cv::Mat _CTAFrame;
    cv::Mat _CTAFrameClean;

    cv::Mat _ATCRecFrame;
    cv::Mat _ATCRecFrameClean;

    vector<cv::Point2f > _bBox;
    bool _active;
    cv::Mat _descriptors;
    cv::Mat _goodDescriptors;
    std::vector<cv::KeyPoint> _keypoints;
    std::vector<cv::KeyPoint> _goodKeypoints;

    std::string _object;
    double _frameRate;
    int _numFeatPerBlock;
    int _numSlices;

    //***NBS***//
    bool _nbs ;
    bool _nbsReady ;
    bool _nbsRecReady ;
    Camera* _bargainCamera;
    uint _residualEnergy;
    int _nbsOffset;
    //***NBS***//

    double _processingEnergy;
    double _txEnergy;

    double _tempProcTime;
    double _tempTxTime;

    int _nCoopAvailable;

    bool _tracking;
    bool _recognition;

    std::vector<cv::Point2f> _track_points;

    OperativeMode _operativeMode;
    ImageSource _imageSource;

    cv::Mat _pklotPredictionsHistory;

    std::vector<PKLot> _pklot;
    std::vector<cv::KeyPoint> _pklotKpts;
    Bitstream _pklotKptsBitstream;
    cv::Mat _pklotBase;

};

#endif // CAMERA_H
