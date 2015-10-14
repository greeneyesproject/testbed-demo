#include "NetworkTopology.h"
#include "ui_camerasettings.h"
#include <sstream>
#include "Global.h"
#include <QDebug>
#include "Messages/StartCTAMsg.h"
#include "Messages/StartATCMsg.h"
#include "Messages/CoopInfoReqMsg.h"
#include "Messages/StopMsg.h"
#include "Network/NetworkNode.h"
#include "GuiProcessingSystem.h"
#include "GuiNetworkSystem.h"
#include <Network/Tcp/Session.h>

using namespace std;

CameraSettings::CameraSettings(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CameraSettings){

    _cameras = Camera::getCameras();
    _selectedCamera = _cameras->at(0);
    _guiProcessingSystem = GuiProcessingSystem::getInstance();
    _guiNetworkSystem = GuiNetworkSystem::getInstance();

    ui->setupUi(this);
    this->setWindowFlags(Qt::Window);

    ui->spinBox_CTAjpegQf->setMaximum(MAX_QF);
    ui->spinBox_CTAjpegQf->setMinimum(MIN_QF);

    ui->spinBox_ATCdetThNumBin->setMaximum(MAX_DETTH);
    ui->spinBox_ATCdetThNumBin->setMinimum(MIN_DETTH);

    ui->spinBox_ATCmaxFeatQuantStep->setMaximum(MAX_FEAT);
    ui->spinBox_ATCmaxFeatQuantStep->setMinimum(MIN_FEAT);

    ui->spinBox_ATCcooperators->setMinimum(1);
    ui->horizontalSlider_ATCcooperators->setMinimum(1);


    /*
    * With more than 1 cameras it's necessary to add new tabs
    */
    for (uint newCameraIdx = 1; newCameraIdx < _cameras->size(); newCameraIdx++){
        std::stringstream ss;
        ss << "Camera " << newCameraIdx+1;
        QWidget *newTab = new QWidget(ui->tabWidget);
        ui->tabWidget->addTab(newTab, tr(ss.str().c_str()));
    }


    connect(ui->tabWidget, SIGNAL(currentChanged(int)), this, SLOT(setSelectedCameraSlot(int)));

    /* Operative mode */
    connect(ui->buttonGroup_operativeMode,
            SIGNAL(buttonClicked(QAbstractButton*)),
            this,
            SLOT(operativeModeToggledSlot(QAbstractButton*)));

    /* Image source */
    connect(ui->buttonGroup_imageSource,
            SIGNAL(buttonClicked(QAbstractButton*)),
            this,
            SLOT(imageSourceToggledSlot(QAbstractButton*)));

    connect(ui->buttonGroup_ATC_CTA, SIGNAL(buttonClicked(QAbstractButton*)), this, SLOT(atcCtaRadioSwitchSlot(QAbstractButton*)));

    connect(ui->spinBox_ATCdetThNumBin, SIGNAL(valueChanged(int)), this, SLOT(detectionThresholdModifiedSlot(int)));
    connect(ui->horizontalSlider_ATCdetThNumBin, SIGNAL(valueChanged(int)), this, SLOT(detectionThresholdModifiedSlot(int)));

    connect(ui->spinBox_ATCmaxFeatQuantStep, SIGNAL(valueChanged(int)), this, SLOT(maxFeaturesModifiedSlot(int)));
    connect(ui->horizontalSlider_ATCmaxFeatQuantStep, SIGNAL(valueChanged(int)), this, SLOT(maxFeaturesModifiedSlot(int)));

    connect(ui->checkBox_ATCencodeKeypoints, SIGNAL(toggled(bool)), this, SLOT(encodeKeypointsToggledSlot(bool)));
    connect(ui->checkBox_ATCshowReconstruction, SIGNAL(clicked(bool)), this, SLOT(showReconstructionModifiedSlot(bool)));
    connect(ui->checkBox_ATCentropyCoding, SIGNAL(toggled(bool)), this, SLOT(entropyToggledSlot(bool)));

    //***NBS***//
    connect(ui->checkBox_ATCenableNBS,SIGNAL(toggled(bool)), this, SLOT(nbsToggledSlot(bool)));
    connect(ui->comboBox_ATCnbsSelectCamera,SIGNAL(currentIndexChanged(int)), this, SLOT(nbsSelectSlot()));
    //***NBS***//

    connect(ui->spinBox_ATCfeaturesPerBlock, SIGNAL(valueChanged(int)), this, SLOT(numFeatPerBlockModifiedSlot(int)));

    connect(ui->checkBox_ATCenableDatc, SIGNAL(clicked(bool)), this, SLOT(datcToggledSlot(bool)));

    connect(ui->spinBox_ATCcooperators, SIGNAL(valueChanged(int)), this, SLOT(numCooperatorsModifiedSlot(int)));
    connect(ui->horizontalSlider_ATCcooperators, SIGNAL(valueChanged(int)), this, SLOT(numCooperatorsModifiedSlot(int)));

    connect(ui->spinBox_CTAjpegQf, SIGNAL(valueChanged(int)), this, SLOT(qfModifiedSlot(int)));
    connect(ui->horizontalSlider_CTAjpegQf, SIGNAL(valueChanged(int)), this, SLOT(qfModifiedSlot(int)));

    connect(ui->spinBox_CTAslices, SIGNAL(valueChanged(int)), this, SLOT(numSlicesModifiedSlot(int)));
    connect(ui->horizontalSlider_CTAslices, SIGNAL(valueChanged(int)), this, SLOT(numSlicesModifiedSlot(int)));

    connect(ui->pushButton_startStop, SIGNAL(clicked()), this, SLOT(toggleCameraSlot()));

    connect(ui->checkBox_enableRecognition, SIGNAL(toggled(bool)), this, SLOT(recognitionToggledSlot(bool)));
    connect(ui->checkBox_enableTracking, SIGNAL(toggled(bool)), this, SLOT(trackingToggledSlot(bool)));

    connect(ui->buttonGroup_sinkCameraLink, SIGNAL(buttonClicked(QAbstractButton*)), this, SLOT(sinkCameraLinkRadioSwitchSlot(QAbstractButton*)));
    connect(ui->horizontalSlider_wifiBw, SIGNAL(valueChanged(int)), this, SLOT(wifiBwModifiedSlot(int)));
    connect(ui->spinBox_wifiBw, SIGNAL(valueChanged(int)), this, SLOT(wifiBwModifiedSlot(int)));

    connect(_guiNetworkSystem, SIGNAL(sinkDisconnected()), this, SLOT(sinkDisconnectSlot()));

    for(uint i=0; i < _cameras->size();i++){
        Camera *cur_cam = _cameras->at(i);
        connect(cur_cam, SIGNAL(updateNumAvailableCooperatorsSignal(uint,int)), this, SLOT(numAvailableCooperatorsModifiedSlot(uint, int)));
    }

    setSelectedCameraSlot(0);
}

CameraSettings::~CameraSettings(){
    delete ui;
}

/**
 * @brief CameraSettings::ATC_CTA_radio_switch
 * @param button
 */
void CameraSettings::atcCtaRadioSwitchSlot(QAbstractButton* button){

    //qDebug() << "ATC_CTA_radio_switch: button" << button;
    if (button == ui->radioButton_ATC){
        if (_selectedCamera->getCurrentMode()!=CAMERA_MODE_ATC){
            _selectedCamera->setCurrentModeSlot(CAMERA_MODE_ATC);
            setSelectedCameraParameters();
        }else{
            /* Needed to fix radio button change if already checked */
            ui->radioButton_ATC->setChecked(true);
            ui->radioButton_CTA->setChecked(false);
        }
    }else if (button == ui->radioButton_CTA){
        if (_selectedCamera->getCurrentMode()!=CAMERA_MODE_CTA){
            _selectedCamera->setCurrentModeSlot(CAMERA_MODE_CTA);
            setSelectedCameraParameters();
        }else{
            /* Needed to fix radio button change if already checked */
            ui->radioButton_ATC->setChecked(false);
            ui->radioButton_CTA->setChecked(true);
        }
    }

}

/**
 * @brief CameraSettings::sinkCameraLink_radio_switch
 * @param button
 */
void CameraSettings::sinkCameraLinkRadioSwitchSlot(QAbstractButton* button){

    if (button==ui->radioButton_sinkCameraTelos){
        if(_selectedCamera->getLinkType()!=LINKTYPE_TELOS){
            _selectedCamera->setLinkType(LINKTYPE_TELOS);
        }else{
            /* Needed to fix radio button change if already checked */
            ui->radioButton_sinkCameraTelos->setChecked(true);
            ui->radioButton_sinkCameraTcp->setChecked(false);
        }
    }else if(button==ui->radioButton_sinkCameraTcp){
        if(_selectedCamera->getLinkType()!=LINKTYPE_TCP){
            _selectedCamera->setLinkType(LINKTYPE_TCP);
        }else{
            /* Needed to fix radio button change if already checked */
            ui->radioButton_sinkCameraTelos->setChecked(false);
            ui->radioButton_sinkCameraTcp->setChecked(true);
        }
    }
    setSelectedCameraParameters();
}


void CameraSettings::entropyToggledSlot(bool value){

    _selectedCamera->setEncodeFeatures(value);

    setSelectedCameraParameters();
}

void CameraSettings::detectionThresholdModifiedSlot(int value){

    switch(_selectedCamera->getOperativeMode()){
    case OPERATIVEMODE_OBJECT:
        if (_selectedCamera->getDetThres()!=value){
            _selectedCamera->setDetThres(value);
        }
        break;
    case OPERATIVEMODE_PKLOT:
        if (_selectedCamera->getBinShift()!=value){
            _selectedCamera->setBinShift(value);
        }
        break;
    }

    setSelectedCameraParameters();

}

void CameraSettings::wifiBwModifiedSlot(int value){

    _selectedCamera->setWifiBw(value);
    setSelectedCameraParameters();

}

void CameraSettings::maxFeaturesModifiedSlot(int value){

    switch(_selectedCamera->getOperativeMode()){
    case OPERATIVEMODE_OBJECT:
        if (_selectedCamera->getMaxNumFeatures()!=value){
            _selectedCamera->setMaxNumFeaturesSlot(value);
        }
        break;
    case OPERATIVEMODE_PKLOT:
        if (_selectedCamera->getValShift()!=value){
            _selectedCamera->setValShift(value);
        }
        break;
    }

    setSelectedCameraParameters();
}

void CameraSettings::showReconstructionModifiedSlot(bool value){

    if (_selectedCamera->getShowReconstruction()!=value){
        _selectedCamera->setShowReconstructionSlot(value);
    }

    setSelectedCameraParameters();
}

void CameraSettings::datcToggledSlot(bool value){

    if (_selectedCamera->getDatc()!=value){
        _selectedCamera->setDatc(value);

    }
    if (value==false){
        //ui->spinBox_ATCcooperators->setMinimum(0);
        //ui->horizontalSlider_ATCcooperators->setMinimum(0);
        //ui->spinBox_ATCcooperators->setValue(1);

        emit numCoopChangedSignal(_selectedCamera->getId(), 0);
    }
    if (value==true){
        if (ui->spinBox_ATCcooperators->value() < 1)
            ui->spinBox_ATCcooperators->setValue(1);
        //ui->spinBox_ATCcooperators->setMinimum(1);
        //ui->horizontalSlider_ATCcooperators->setMinimum(1);
        //ui->spinBox_ATCcooperators->setMaximum(_selectedCamera->getNumAvailableCooperators());

        emit numCoopChangedSignal(_selectedCamera->getId(), _selectedCamera->getNumCoop());
    }

    setSelectedCameraParameters();

    /*if (!value)
        sendStartATC();*/

}

void CameraSettings::numCooperatorsModifiedSlot(int value){

    _selectedCamera->setNumCoopSlot(value);
    emit numCoopChangedSignal(_selectedCamera->getId(), value);
    setSelectedCameraParameters();

}

void CameraSettings::qfModifiedSlot(int value){
    _selectedCamera->setQfSlot(value);
    setSelectedCameraParameters();

}

void CameraSettings::numFeatPerBlockModifiedSlot(int value){

    _selectedCamera->setNumFeatPerBlock(value);
    setSelectedCameraParameters();

}

void CameraSettings::numSlicesModifiedSlot(int value){
    _selectedCamera->setNumSlices(value);
    setSelectedCameraParameters();
}

/**
 * @brief CameraSettings::setSelectedCamera
 * Set the _selectedCamera to the camera whose tab is currently displayed
 * @param guiIdx
 */
void CameraSettings::setSelectedCameraSlot(int guiIdx){
    _selectedCamera =_cameras->at(guiIdx);
    setSelectedCameraParameters();
}

/**
 * @brief CameraSettings::setSelectedCameraParameters
 * Set Camera Settings View parameters according to _selectedCamera properties
 */
void CameraSettings::setSelectedCameraParameters(){

    /* Set values */

    /* Operative mode */
    ui->buttonGroup_operativeMode->blockSignals(true);
    ui->radioButton_objects->setChecked(_selectedCamera->getOperativeMode()==OPERATIVEMODE_OBJECT);
    ui->radioButton_parkingLot->setChecked(_selectedCamera->getOperativeMode()==OPERATIVEMODE_PKLOT);
    ui->buttonGroup_operativeMode->blockSignals(false);

    /* Image source */
    ui->buttonGroup_imageSource->blockSignals(true);
    ui->radioButton_live->setChecked(_selectedCamera->getImageSource()==IMAGESOURCE_LIVE);
    ui->radioButton_playback->setChecked(_selectedCamera->getImageSource()==IMAGESOURCE_REC);
    ui->buttonGroup_imageSource->blockSignals(false);

    /* Detection threshold */
    ui->horizontalSlider_ATCdetThNumBin->blockSignals(true);
    ui->spinBox_ATCdetThNumBin->blockSignals(true);
    switch (_selectedCamera->getOperativeMode()){
    case OPERATIVEMODE_OBJECT:

        ui->label_ATCdetThNumBin->setText("Detection threshold");
        ui->horizontalSlider_ATCdetThNumBin->setMaximum(MAX_DETTH);
        ui->horizontalSlider_ATCdetThNumBin->setMinimum(MIN_DETTH);
        ui->horizontalSlider_ATCdetThNumBin ->setPageStep(10);

        ui->spinBox_ATCdetThNumBin->setMaximum(MAX_DETTH);
        ui->spinBox_ATCdetThNumBin->setMinimum(MIN_DETTH);

        ui->horizontalSlider_ATCdetThNumBin->setValue(_selectedCamera->getDetThres());
        ui->spinBox_ATCdetThNumBin->setValue(_selectedCamera->getDetThres());

        break;
    case OPERATIVEMODE_PKLOT:

        ui->label_ATCdetThNumBin->setText("Histogram bins");
        ui->horizontalSlider_ATCdetThNumBin->setMaximum(MAX_BINSHIFT);
        ui->horizontalSlider_ATCdetThNumBin->setMinimum(MIN_BINSHIFT);
        ui->horizontalSlider_ATCdetThNumBin ->setPageStep(1);


        ui->spinBox_ATCdetThNumBin->setMaximum(((180-1)>>MIN_BINSHIFT)+1);
        ui->spinBox_ATCdetThNumBin->setMinimum(((180-1)>>MAX_BINSHIFT)+1);

        ui->horizontalSlider_ATCdetThNumBin->setValue(_selectedCamera->getBinShift());
        ui->spinBox_ATCdetThNumBin->setValue(_selectedCamera->getNumBin());

        break;
    }
    ui->horizontalSlider_ATCdetThNumBin->blockSignals(false);
    ui->spinBox_ATCdetThNumBin->blockSignals(false);


    /* Max features */
    ui->horizontalSlider_ATCmaxFeatQuantStep->blockSignals(true);
    ui->spinBox_ATCmaxFeatQuantStep->blockSignals(true);
    switch (_selectedCamera->getOperativeMode()){
    case OPERATIVEMODE_OBJECT:

        ui->label_ATCmaxFeatQuantStep->setText("Max features");
        ui->horizontalSlider_ATCmaxFeatQuantStep->setMaximum(MAX_FEAT);
        ui->horizontalSlider_ATCmaxFeatQuantStep->setMinimum(MIN_FEAT);
        ui->horizontalSlider_ATCmaxFeatQuantStep->setPageStep(10);

        ui->spinBox_ATCmaxFeatQuantStep->setMaximum(MAX_FEAT);
        ui->spinBox_ATCmaxFeatQuantStep->setMinimum(MIN_FEAT);

        ui->horizontalSlider_ATCmaxFeatQuantStep->setValue(_selectedCamera->getMaxNumFeatures());
        ui->spinBox_ATCmaxFeatQuantStep->setValue(_selectedCamera->getMaxNumFeatures());

        break;
    case OPERATIVEMODE_PKLOT:

        ui->label_ATCmaxFeatQuantStep->setText("Quantization step");
        ui->horizontalSlider_ATCmaxFeatQuantStep->setMaximum(MAX_VALSHIFT);
        ui->horizontalSlider_ATCmaxFeatQuantStep->setMinimum(MIN_VALSHIFT);
        ui->horizontalSlider_ATCmaxFeatQuantStep->setPageStep(1);

        ui->spinBox_ATCmaxFeatQuantStep->setMaximum(1<<MAX_VALSHIFT);
        ui->spinBox_ATCmaxFeatQuantStep->setMinimum(1<<MIN_VALSHIFT);

        ui->horizontalSlider_ATCmaxFeatQuantStep->setValue(_selectedCamera->getValShift());
        ui->spinBox_ATCmaxFeatQuantStep->setValue(_selectedCamera->getQuantStep());

        break;
    }
    ui->spinBox_ATCmaxFeatQuantStep->blockSignals(false);
    ui->horizontalSlider_ATCmaxFeatQuantStep->blockSignals(false);

    /* Encode keypoints */
    ui->checkBox_ATCencodeKeypoints->blockSignals(true);
    ui->checkBox_ATCencodeKeypoints->setChecked(_selectedCamera->getEncodeKeypoints());
    ui->checkBox_ATCencodeKeypoints->blockSignals(false);
    /* Show reconstruction */
    ui->checkBox_ATCshowReconstruction->blockSignals(true);
    ui->checkBox_ATCshowReconstruction->setChecked(_selectedCamera->getShowReconstruction());
    ui->checkBox_ATCshowReconstruction->blockSignals(false);
    /* Entropy coding */
    ui->checkBox_ATCentropyCoding->blockSignals(true);
    ui->checkBox_ATCentropyCoding->setChecked(_selectedCamera->getEncodeFeatures());
    ui->checkBox_ATCentropyCoding->blockSignals(false);
    /* Features per block */
    ui->spinBox_ATCfeaturesPerBlock->blockSignals(true);
    ui->spinBox_ATCfeaturesPerBlock->setValue(_selectedCamera->getNumFeatPerBlock());
    ui->spinBox_ATCfeaturesPerBlock->blockSignals(false);
    /* Enable DATC */
    ui->checkBox_ATCenableDatc->blockSignals(true);
    ui->checkBox_ATCenableDatc->setChecked(_selectedCamera->getDatc());
    ui->checkBox_ATCenableDatc->blockSignals(false);
    /* Cooperators */
    ui->spinBox_ATCcooperators->blockSignals(true);
    ui->horizontalSlider_ATCcooperators->blockSignals(true);
    ui->spinBox_ATCcooperators->setValue(_selectedCamera->getNumCoop());
    uchar availableCoops = _selectedCamera->getNumAvailableCooperators() > 0 ? _selectedCamera->getNumAvailableCooperators() : 1;
    ui->spinBox_ATCcooperators->setMaximum(availableCoops);
    ui->horizontalSlider_ATCcooperators->setValue(_selectedCamera->getNumCoop());
    ui->horizontalSlider_ATCcooperators->setMaximum(availableCoops);
    ui->spinBox_ATCcooperators->blockSignals(false);
    ui->horizontalSlider_ATCcooperators->blockSignals(false);

    //***NBS***//
    ui->checkBox_ATCenableNBS->blockSignals(true);
    ui->comboBox_ATCnbsSelectCamera->blockSignals(true);
    ui->comboBox_ATCnbsSelectCamera->clear();
    //populate combo box
    for(int i=0;i<_cameras->size();i++){
        if(_selectedCamera->getId() != ((Camera*)(_cameras->at(i)))->getId()){
            ui->comboBox_ATCnbsSelectCamera->addItem(QString::number(((Camera*)(_cameras->at(i)))->getId()));
        }
    }
    //initialize camera to bargain with
    if(_cameras->size()>1){
        uint bargainCameraId = (uint)ui->comboBox_ATCnbsSelectCamera->currentText().toInt();
        Camera* bargainCamera;
        for(int i=0;i<_cameras->size();i++){
            if(((Camera*)(_cameras->at(i)))->getId() == bargainCameraId){
                bargainCamera = _cameras->at(i);
                break;
            }
        }
        _selectedCamera->setBargainCamera(bargainCamera);
    }
    ui->comboBox_ATCnbsSelectCamera->blockSignals(false);
    ui->checkBox_ATCenableNBS->blockSignals(false);
    //***NBS***//

    /* JPEG quality factor */
    ui->horizontalSlider_CTAjpegQf->blockSignals(true);
    ui->horizontalSlider_CTAjpegQf->setValue(_selectedCamera->getQf());
    ui->horizontalSlider_CTAjpegQf->blockSignals(false);
    ui->spinBox_CTAjpegQf->blockSignals(true);
    ui->spinBox_CTAjpegQf->setValue(_selectedCamera->getQf());
    ui->spinBox_CTAjpegQf->blockSignals(false);
    /* Slices */
    ui->horizontalSlider_CTAslices->blockSignals(true);
    ui->horizontalSlider_CTAslices->setValue(_selectedCamera->getNumSlices());
    ui->horizontalSlider_CTAslices->blockSignals(false);
    ui->spinBox_CTAslices->blockSignals(true);
    ui->spinBox_CTAslices->setValue(_selectedCamera->getNumSlices());
    ui->spinBox_CTAslices->blockSignals(false);

    /* Enable recognition */
    ui->checkBox_enableRecognition->blockSignals(true);
    ui->checkBox_enableRecognition->setChecked(_selectedCamera->getRecognitionEnabled());
    ui->checkBox_enableRecognition->blockSignals(false);

    /* Enable tracking */
    ui->checkBox_enableTracking->blockSignals(true);
    ui->checkBox_enableTracking->setChecked(_selectedCamera->getTrackingEnabled());
    ui->checkBox_enableTracking->blockSignals(false);

    /* Sink-Camera link type*/
    ui->buttonGroup_sinkCameraLink->blockSignals(true);
    ui->radioButton_sinkCameraTelos->setChecked(_selectedCamera->getLinkType()==LINKTYPE_TELOS);
    ui->radioButton_sinkCameraTcp->setChecked(_selectedCamera->getLinkType()==LINKTYPE_TCP);
    ui->buttonGroup_sinkCameraLink->blockSignals(false);

    /* WiFi Bandwidth */
    ui->horizontalSlider_wifiBw->blockSignals(true);
    ui->spinBox_wifiBw->blockSignals(true);
    ui->horizontalSlider_wifiBw->setValue(_selectedCamera->getWifiBw());
    ui->spinBox_wifiBw->setValue(_selectedCamera->getWifiBw());
    if (_selectedCamera->getLinkType()==LINKTYPE_TELOS){
        ui->horizontalSlider_wifiBw->setEnabled(false);
        ui->spinBox_wifiBw->setEnabled(false);
    }else{
        ui->horizontalSlider_wifiBw->setEnabled(true);
        ui->spinBox_wifiBw->setEnabled(true);
    }
    ui->horizontalSlider_wifiBw->blockSignals(false);
    ui->spinBox_wifiBw->blockSignals(false);

    ui->buttonGroup_ATC_CTA->blockSignals(true);
    /* Set ATC/CTA and enable/disable commands */
    if (_selectedCamera->getCurrentMode() == CAMERA_MODE_ATC){
        ui->radioButton_ATC->setChecked(true);
        ui->radioButton_CTA->setChecked(false);

        switch(_selectedCamera->getOperativeMode()){
        case OPERATIVEMODE_OBJECT:
            ui->label_ATCdetThNumBin->setEnabled(true);
            ui->horizontalSlider_ATCdetThNumBin->setEnabled(true);
            ui->spinBox_ATCdetThNumBin->setEnabled(true);
            ui->label_ATCmaxFeatQuantStep->setEnabled(true);
            ui->horizontalSlider_ATCmaxFeatQuantStep->setEnabled(true);
            ui->spinBox_ATCmaxFeatQuantStep->setEnabled(true);
            ui->checkBox_ATCencodeKeypoints->setEnabled(true);
            ui->checkBox_ATCshowReconstruction->setEnabled(true);
            ui->checkBox_ATCentropyCoding->setEnabled(true);
            ui->spinBox_ATCfeaturesPerBlock->setEnabled(true);

            /* Enable or disable DATC section */
            if (_selectedCamera->getNumAvailableCooperators()){
                ui->checkBox_ATCenableDatc->setEnabled(true);
                ui->spinBox_ATCcooperators->setEnabled(true);
                ui->horizontalSlider_ATCcooperators->setEnabled(true);
            }else{
                ui->checkBox_ATCenableDatc->setEnabled(false);
                ui->spinBox_ATCcooperators->setEnabled(false);
                ui->horizontalSlider_ATCcooperators->setEnabled(false);
            }

            //***NBS***//
            /* Enable NBS section only if other cameras are available */
            if(this->_cameras->size()>1){
                ui->checkBox_ATCenableNBS->setEnabled(true);
                ui->label_ATCbargainWithCamera->setEnabled(true);
                ui->comboBox_ATCnbsSelectCamera->setEnabled(true);
            }
            else{
                ui->checkBox_ATCenableNBS->setEnabled(false);
                ui->label_ATCbargainWithCamera->setEnabled(false);
                ui->comboBox_ATCnbsSelectCamera->setEnabled(false);
            }
            //***NBS***//
            break;
        case OPERATIVEMODE_PKLOT:
            ui->label_ATCdetThNumBin->setEnabled(true);
            ui->horizontalSlider_ATCdetThNumBin->setEnabled(true);
            ui->spinBox_ATCdetThNumBin->setEnabled(false);
            ui->label_ATCmaxFeatQuantStep->setEnabled(true);
            ui->horizontalSlider_ATCmaxFeatQuantStep->setEnabled(true);
            ui->spinBox_ATCmaxFeatQuantStep->setEnabled(false);
            ui->checkBox_ATCencodeKeypoints->setEnabled(true);
            ui->checkBox_ATCshowReconstruction->setEnabled(false);
            ui->checkBox_ATCentropyCoding->setEnabled(true);
            ui->spinBox_ATCfeaturesPerBlock->setEnabled(true);

            /* Disable DATC section */
            ui->checkBox_ATCenableDatc->setEnabled(false);
            ui->spinBox_ATCcooperators->setEnabled(false);
            ui->horizontalSlider_ATCcooperators->setEnabled(false);

            //***NBS***//
            /* Disable NBS */
            ui->checkBox_ATCenableNBS->setEnabled(false);
            ui->label_ATCbargainWithCamera->setEnabled(false);
            ui->comboBox_ATCnbsSelectCamera->setEnabled(false);
            //***NBS***//
            break;
        }

        ui->label_CTAjpegQf->setEnabled(false);
        ui->horizontalSlider_CTAjpegQf->setEnabled(false);
        ui->spinBox_CTAjpegQf->setEnabled(false);
        ui->label_CTAslices->setEnabled(false);
        ui->spinBox_CTAslices->setEnabled(false);

    }
    else{ //CAMERA_MODE_CTA

        ui->radioButton_ATC->setChecked(false);
        ui->radioButton_CTA->setChecked(true);

        ui->label_ATCdetThNumBin->setEnabled(false);
        ui->horizontalSlider_ATCdetThNumBin->setEnabled(false);
        ui->spinBox_ATCdetThNumBin->setEnabled(false);
        ui->label_ATCmaxFeatQuantStep->setEnabled(false);
        ui->horizontalSlider_ATCmaxFeatQuantStep->setEnabled(false);
        ui->spinBox_ATCmaxFeatQuantStep->setEnabled(false);
        ui->checkBox_ATCencodeKeypoints->setEnabled(false);
        ui->checkBox_ATCshowReconstruction->setEnabled(false);
        ui->checkBox_ATCentropyCoding->setEnabled(false);
        ui->spinBox_ATCfeaturesPerBlock->setEnabled(false);
        ui->checkBox_ATCenableDatc->setEnabled(false);
        ui->spinBox_ATCcooperators->setEnabled(false);
        ui->horizontalSlider_ATCcooperators->setEnabled(false);

        //***NBS***//
        ui->checkBox_ATCenableNBS->setEnabled(false);
        ui->label_ATCbargainWithCamera->setEnabled(false);
        ui->comboBox_ATCnbsSelectCamera->setEnabled(false);
        //***NBS***//

        ui->label_CTAjpegQf->setEnabled(true);
        ui->horizontalSlider_CTAjpegQf->setEnabled(true);
        ui->spinBox_CTAjpegQf->setEnabled(true);
        ui->label_CTAslices->setEnabled(true);
        ui->spinBox_CTAslices->setEnabled(true);
    }

    /* Recognition and tracking */
    switch(_selectedCamera->getOperativeMode()){
    case OPERATIVEMODE_OBJECT:
        ui->checkBox_enableTracking->setEnabled(true);
        break;
    case OPERATIVEMODE_PKLOT:
        ui->checkBox_enableTracking->setEnabled(false);
        break;
    }

    ui->buttonGroup_ATC_CTA->blockSignals(false);

    /* Start/Stop command */
    if (_selectedCamera->getActive()){
        ui->pushButton_startStop->setText("Stop camera");
    }else{
        ui->pushButton_startStop->setText("Start camera");
    }

}

void CameraSettings::setActiveCamera(Camera* camera){
    ui->tabWidget->setCurrentIndex(camera->getGuiIdx());
}

void CameraSettings::setNetworkTopology(NetworkTopology *netTop){

    _netTop = netTop;
    /* Camera tab change */
    connect(ui->tabWidget, SIGNAL(currentChanged(int)), _netTop, SLOT(changeActiveCamera(int)));
    connect(this, SIGNAL(numCoopChangedSignal(int,int)), _netTop, SLOT(changeCoop(int,int)));

    connect(this, SIGNAL(numAvailableCoopChangedSignal(int,int)), _netTop, SLOT(changeAvailCoop(int,int)));

    for (vector<Camera*>::iterator camIt = _cameras->begin(); camIt!=_cameras->end(); camIt++){
        connect(*camIt, SIGNAL(curBandwidthChangedSignal(int,double)), _netTop, SLOT(updateBandwidth(int,double)));
    }

    _netTop->changeActiveCamera(_selectedCamera->getGuiIdx());
}

/**
 * @brief sinkDisconnect
 * Slot activated from signal in Server when sink disconnects
 */
void CameraSettings::sinkDisconnectSlot(){
    for (vector<Camera*>::iterator it = Camera::getCameras()->begin();
         it != Camera::getCameras()->end();
         ++it){
        (*it)->setActive(false);
    }
    setSelectedCameraParameters();
}

/* Called when Start/Stop Camera button pressed */
void CameraSettings::toggleCameraSlot(){
    bool send_result = false;
    if (!_selectedCamera->getActive()){
        /* send start message */
        if (_selectedCamera->getCurrentMode()==CAMERA_MODE_CTA){
            send_result = _guiProcessingSystem->sendStartCTA(_selectedCamera);
        }else{
            if (_selectedCamera->getNbs()){
                send_result = _guiProcessingSystem->sendStartATCNBS(_selectedCamera);
            }else{
                send_result = _guiProcessingSystem->sendStartATC(_selectedCamera);
            }
        }

        if (send_result){
            _selectedCamera->setActive(true);
        }
    }else{
        /* send stop message */
        _guiProcessingSystem->sendStop(_selectedCamera);
        _selectedCamera->setActive(false);
    }
    setSelectedCameraParameters();
}

void CameraSettings::encodeKeypointsToggledSlot(bool value){

    _selectedCamera->setEncodeKeypoints(value);
    setSelectedCameraParameters();
}

//***NBS***//
void CameraSettings::nbsToggledSlot(bool value){

    _selectedCamera->setNbs(value);

    //qDebug() << "nbs is " << _selectedCamera->getNbs() << "on " << _selectedCamera;
    //if(_selectedCamera->getNbs())
    //    qDebug() << "camera " << _selectedCamera << " will bargain with " << _selectedCamera->getBargainCamera()->getId();

    setSelectedCameraParameters();

    //TRIGGER NBS SELECTION ALSO ON BARGAINING CAMERA
    Camera* bargainingCamera;
    for(int i=0;i<_cameras->size();i++){
        if(((Camera*)(_cameras->at(i)))->getId() == _selectedCamera->getBargainCamera()->getId()){
            bargainingCamera = _cameras->at(i);
            break;
        }
    }
    bargainingCamera->setNbs(value);
    bargainingCamera->setBargainCamera(_selectedCamera);
    if(value){
        bargainingCamera->setCurrentModeSlot(CAMERA_MODE_ATC);
        bargainingCamera->setDetThres(_selectedCamera->getDetThres());
        bargainingCamera->setEncodeFeatures(_selectedCamera->getEncodeFeatures());
        bargainingCamera->setMaxNumFeaturesSlot(_selectedCamera->getMaxNumFeatures());
        bargainingCamera->setNumFeatPerBlock(_selectedCamera->getNumFeatPerBlock());
    }


}

void CameraSettings::nbsSelectSlot(){
    if(_cameras->size()>1){
        uint bargainCameraId = (uint)ui->comboBox_ATCnbsSelectCamera->currentText().toInt();
        Camera* bargainCamera;
        for(int i=0;i<_cameras->size();i++){
            if(((Camera*)(_cameras->at(i)))->getId() == bargainCameraId){
                bargainCamera = _cameras->at(i);
                break;
            }
        }
        _selectedCamera->setBargainCamera(bargainCamera);
    }

    //qDebug() << "camera " << _selectedCamera << " will bargain with " << _selectedCamera->getBargainCamera()->getId();

    setSelectedCameraParameters();
}

//***NBS***//


/**
 * @brief CameraSettings::handleNCoopAvail
 * Called when CoopInfoMsg or CoopInfoRespMsg messages are received
 * @param camId
 * @param nCoop
 */
void CameraSettings::numAvailableCooperatorsModifiedSlot(uint camId, int nCoop){

    //qDebug() << "handling available cooperators: now camId " << camId << " has " << nCoop << " cooperators";
    if (camId != _selectedCamera->getId())
        return;

    setSelectedCameraParameters();

    /* Notify Network Topology about the change */
    emit numAvailableCoopChangedSignal(camId, nCoop);

}

void CameraSettings::recognitionToggledSlot(bool checked){

    _selectedCamera->setRecognitionEnabledSlot(checked);
    //qDebug() << "recognition is " << _selectedCamera->getRecognitionEnabled() << " on " << _selectedCamera;
    if (!checked)
        emit _selectedCamera->recognitionCompletedSignal("","--");

    setSelectedCameraParameters();
}

void CameraSettings::trackingToggledSlot(bool checked){

    _selectedCamera->setTrackingEnabledSlot(checked);
    //qDebug() << "tracking is " << _selectedCamera->getTrackingEnabled() << " on " << _selectedCamera;

    setSelectedCameraParameters();
}

/**
 * @brief CameraSettings::operativeModeToggledSlot
 * Called on operative mode radio button toggle
 * @param button
 */
void CameraSettings::operativeModeToggledSlot(QAbstractButton* button){
    if (button == ui->radioButton_objects){
        ui->radioButton_objects->setChecked(true);
        ui->radioButton_parkingLot->setChecked(false);
        _selectedCamera->setOperativeMode(OPERATIVEMODE_OBJECT);
    }else if(button == ui->radioButton_parkingLot){
        ui->radioButton_objects->setChecked(false);
        ui->radioButton_parkingLot->setChecked(true);
        _selectedCamera->setOperativeMode(OPERATIVEMODE_PKLOT);
    }

    setSelectedCameraParameters();
}

/**
 * @brief CameraSettings::imageSourceToggledSlot
 * Called on image source radio button toggle
 * @param button
 */
void CameraSettings::imageSourceToggledSlot(QAbstractButton* button){
    if (button == ui->radioButton_live){
        ui->radioButton_live->setChecked(true);
        ui->radioButton_playback->setChecked(false);
        _selectedCamera->setImageSource(IMAGESOURCE_LIVE);
    }else if(button == ui->radioButton_playback){
        ui->radioButton_live->setChecked(false);
        ui->radioButton_playback->setChecked(true);
        _selectedCamera->setImageSource(IMAGESOURCE_REC);
    }

    setSelectedCameraParameters();
}
