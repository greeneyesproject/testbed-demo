#include "GuiProcessingSystem.h"
#include "Messages/StartCTAMsg.h"
#include "Messages/StartATCMsg.h"
#include "Messages/StopMsg.h"
#include "Messages/DataCTAMsg.h"
#include "Messages/DataATCMsg.h"
#include "Messages/CoopInfoMsg.h"
#include <opencv2/highgui/highgui.hpp>
#include "Camera.h"
#include "Network/NetworkNode.h"
#include <GuiNetworkSystem.h>

GuiProcessingSystem* GuiProcessingSystem::_instance = NULL;

GuiProcessingSystem* GuiProcessingSystem::getInstance(){
    if (_instance == NULL){
        _instance = new GuiProcessingSystem();
    }
    return _instance;
}

GuiProcessingSystem::GuiProcessingSystem(){

    _decoder = VisualFeatureDecoding::getInstance();
    _briskExtractor = new VisualFeatureExtraction(DETECTORTYPE_BRISK,DESCRIPTORTYPE_BRISK,BRISK_detParams(),BRISK_descParams());
    _histExtractor = new VisualFeatureExtraction(DETECTORTYPE_NONE,DESCRIPTORTYPE_HIST,detParams(),HIST_descParams());
    _lastFrameId = new vector<unsigned char>(NetworkNode::getCameras().size(),-1);
    _pklotClassifier = new PKLotClassifier();
}

/**
 * @brief GuiProcessingSystem::processDataCTAMsg
 * @param camera
 * @param msg
 *
 * Deletes the message
 */
void GuiProcessingSystem::processDataCTAMsg(Camera *camera, DataCTAMsg* msg){

    if (camera->getCurrentMode()!=CAMERA_MODE_CTA){
        delete msg;
        sendStop(camera);
        endOfFrame(camera);
        return;
    }

    /* Reset restart timer */
    emit camera->startCameraTimerSignal();

    /* CV_8UC1 */
    cv::Mat slice = imdecode(*(msg->getData()),CV_LOAD_IMAGE_UNCHANGED);

    int R = slice.rows;
    int C = slice.cols;
    cv::Point top_left,bottom_right;
    top_left.x = msg->getTopLeftX();
    top_left.y = msg->getTopLeftY();
    bottom_right.x = top_left.x + C;
    bottom_right.y = top_left.y + R;

    camera->setTempProcTime(camera->getTempProcTime() + msg->getEncodingTime());
    camera->setTempTxTime(camera->getTempTxTime() + msg->getTxTime());

    if(msg->getSliceNumber()==0 || msg->getFrameId()!=_lastFrameId->at(camera->getGuiIdx())){
        //if (camera->getLinkType()==LINKTYPE_TELOS){
            /* For graphic purposes: clear the screen only in TELOS. No more */
            camera->newCTAFrame();
        //}
        _lastFrameId->at(camera->getGuiIdx()) = msg->getFrameId();
    }
    cv::Mat cur_img = camera->getCTAFrame();

    cout << "GuiProcessingSystem::processDataCTAMsg: setting slice from " << top_left.x << " " << top_left.y <<
            " to " << bottom_right.x << " " << bottom_right.y << endl;
    camera->setCTASlice(slice, top_left, bottom_right);

    //todo: define num slices in the menu
    cout << "GuiProcessingSystem::processDataCTAMsg: slice" << msg->getSliceNumber()+1 << "/" << camera->cta_param.num_slices << endl;
    if(msg->getSliceNumber() == camera->cta_param.num_slices-1){


        double energy=0;
        camera->setProcessingEnergy(camera->getTempProcTime() * CPU_POWER_BBB);    // switch to energy
        energy+=camera->getTempProcTime() * CPU_POWER_BBB;
        if (camera->getLinkType()==LINKTYPE_TELOS){
            camera->setTxEnergy(camera->getTempTxTime() * TX_POWER_TELOS_BBB);      // switch to energy
            energy+=camera->getTempTxTime() * TX_POWER_TELOS_BBB;
        }else{
            camera->setTxEnergy(camera->getTempTxTime() * TX_POWER_WIFI_BBB);
            energy+=camera->getTempTxTime() * TX_POWER_WIFI_BBB;
        }

        //***NBS***//
        camera->decrementEnergy(energy);
        //***NBS***//

        camera->setTempProcTime(0);
        camera->setTempTxTime(0);

        /* Features extraction */
        vector<cv::KeyPoint> kpts;
        cv::Mat desc;

        cv::Mat image;

        switch(msg->getOperativeMode()){
        case OPERATIVEMODE_OBJECT:{
            cv::cvtColor(camera->getCTAFrame(),image,CV_BGR2GRAY);
            _briskExtractor->extractKeypointsFeatures(image, kpts, desc);
            break;
        }
        case OPERATIVEMODE_PKLOT:{
            cv::Mat components[3];
            cv::cvtColor(camera->getCTAFrame(),image,CV_BGR2HSV);
            //cout << "CTA frame: type: " << camera->getCTAFrame().type() << " channels: " << camera->getCTAFrame().channels() << endl;
            //cout << "image: type: " << image.type() << " channels: " << image.channels() << endl;
            cv::split(image,components);
            cv::Mat hue = components[0];
            cout << "hue: type: " << hue.type() << " channels: " << hue.channels() << endl;
            //cv::imwrite("img.bmp",hue);
            //cv::imshow("img",camera->getCTAFrame());
            //cv::imshow("hue",hue);
            //cv::imshow("sat",components[1]);
            //cv::imshow("val",components[2]);
            //cout << "img first byte: " << (int)camera->getCTAFrame().at<uchar>(0,0) << endl;
            //cout << "hue first byte: " << (int)components[0].at<uchar>(0,0) << endl;
            //cout << "sat first byte: " << (int)components[1].at<uchar>(0,0) << endl;
            //cout << "val first byte: " << (int)components[2].at<uchar>(0,0) << endl;
            /* This copy is necessary since extractFeatures requires non-const value for keypoints*/
            vector<cv::KeyPoint> kpts = camera->getPKLotKpts();
            _histExtractor->setDescriptorParameters(DESCRIPTORTYPE_HIST,HIST_descParams(0,180));
            _histExtractor->extractFeatures(hue,kpts,desc);

            cout << "desc: " << desc.row(0) << endl;

            break;
        }
        default:
            break;
        }

        camera->setKeypoints(kpts);
        camera->setDescriptors(desc);

        camera->setGoodKeypoints(kpts);
        camera->setGoodDescriptors(desc);

        switch(msg->getOperativeMode()){
        case OPERATIVEMODE_OBJECT:
            //TODO should be uniformed
            break;
        case OPERATIVEMODE_PKLOT:
            if (camera->getRecognitionEnabled()){
                cv::Mat predictions;
                _pklotClassifier->predict(desc,predictions,0);
                camera->setPKLotPredictions(predictions);
                cv::Mat pklotImg = camera->getPklotOnlyOccupancy();
                cur_img += PKLOT_CTA_ALPHA*pklotImg;
                camera->showCTAFrame(cur_img);
                emit camera->recognitionCompletedSignal("Free parkings:",QString::number(predictions.rows - (int)(sum(predictions)[0])));
            }
            break;
        }

        emit camera->frameCompletedSignal(camera->getGuiIdx(),msg->getOperativeMode()); 
        endOfFrame(camera);
    }

    delete msg;
}

/**
 * @brief GuiProcessingSystem::processDataATCMsg
 * @param camera
 * @param msg
 *
 * Deletes the message
 */
void GuiProcessingSystem::processDataATCMsg(Camera* camera, DataATCMsg* msg){

    if (camera->getCurrentMode()!=CAMERA_MODE_ATC){
        delete msg;
        sendStop(camera);
        endOfFrame(camera);
        return;
    }

    cout << "Camera " << camera->getId() << endl;

    /* Reset restart timer */
    emit camera->startCameraTimerSignal();

    camera->setTempProcTime(
                camera->getTempProcTime() +
                msg->getDetTime() +
                msg->getDescTime() +
                msg->getKptsEncodingTime() +
                msg->getFeatEncodingTime());

    camera->setTempTxTime(
                camera->getTempTxTime() +
                msg->getTxTime());

    cv::Mat features;
    vector<cv::KeyPoint> keypoints;

    /* This copy is necessary since the ac_encoder doesn't accept const vector<uchar> */
    const vector<uchar>* kp_bitstream = msg->getKeypointsData();

    //cout << "GuiProcessingSystem::processDataATCMsg: decoding keypoints: expected " << msg->getNumKpts() << " keypoints" << endl;


    if (camera->atc_param.encodeKeypoints){
        _decoder->decodeKeyPoints(*kp_bitstream,keypoints, cv::Size(msg->getFrameWidth(),msg->getFrameHeight()), true);
    }else{
        _decoder->dummy_decodeKeyPoints(*kp_bitstream,keypoints);
    }

    /* This copy is necessary since the ac_encoder doesn't accept const vector<uchar> */
    vector<uchar> ft_bitstream = *(msg->getFeaturesData());

    cout << "GuiProcessingSystem::processDataATCMsg: decoded " << keypoints.size() << " keypoints" << endl;
    cout << "GuiProcessingSystem::processDataATCMsg: msg->getNumFeat(): " << msg->getNumFeat() << endl;

    if(msg->getNumFeat() == keypoints.size()){

        switch (msg->getOperativeMode()){
        case OPERATIVEMODE_OBJECT:
            if(camera->atc_param.encodeFeatures){
                cout << "GuiProcessingSystem::processDataATCMsg: EC decoding features" << endl;
                _decoder->decodeBinaryDescriptors(DESCRIPTORTYPE_BRISK, ft_bitstream, features, keypoints.size());
            }else{
                cout << "GuiProcessingSystem::processDataATCMsg: Dummy decoding features" << endl;
                _decoder->dummy_decodeBinaryDescriptors(DESCRIPTORTYPE_BRISK,ft_bitstream,features);
            }
            break;
        case OPERATIVEMODE_PKLOT:
            if(camera->atc_param.encodeFeatures){
                cout << "GuiProcessingSystem::processDataATCMsg: Dummy decoding features" << endl;
                //TODO arithmetic decoder to be implemented
                _decoder->dummy_decodeDescriptors(DESCRIPTORTYPE_HIST,ft_bitstream,features,cv::Size(((180-1) >> camera->atc_param.binShift)+1,msg->getNumFeat()),CV_16UC1);
            }else{
                cout << "GuiProcessingSystem::processDataATCMsg: Dummy decoding features" << endl;
                _decoder->dummy_decodeDescriptors(DESCRIPTORTYPE_HIST,ft_bitstream,features,cv::Size(((180-1) >> camera->atc_param.binShift)+1,msg->getNumFeat()),CV_16UC1);
            }
            break;
        }


        //***NBS***//
        //Compensate NBS offset
        if(camera->getNbs()){
            for(size_t i=0;i<keypoints.size();i++){
                keypoints.at(i).pt.x = keypoints.at(i).pt.x + camera->getNbsOffset();
            }
        }
        //***NBS***//



        if((msg->getBlockNumber()==0 /* First block of the frame */
            || msg->getFrameId()!=_lastFrameId->at(camera->getGuiIdx()) /* New frame */
            )&&(!(camera->getShowReconstruction()))){
            camera->newATCFrame();
            camera->resetFeatures();
            _lastFrameId->at(camera->getGuiIdx()) = msg->getFrameId();
        }

        camera->addFeatures(keypoints, features);

        /*Mat cur_img = camera->getATCFrameClean().clone();*/
        Mat cur_img = camera->getATCFrame();

        if(camera->getNbs()){
            int camera_offset = camera->getNbsOffset();
            if(camera_offset == 0){
                camera_offset = camera->getBargainCamera()->getNbsOffset();
                //overlap line
                cv::line(cur_img, cv::Point(camera_offset+(int)(NBS_OVL*FRAME_W),0), cv::Point(camera_offset+(int)(NBS_OVL*FRAME_W),FRAME_H), Scalar(220, 220, 220), 2);
            }
            //split line
            cv::line(cur_img, cv::Point(camera_offset,0), cv::Point(camera_offset,FRAME_H), Scalar(255, 0, 0), 2);
        }

        switch(camera->getOperativeMode()){
        case OPERATIVEMODE_OBJECT:
            try{
            cv::drawKeypoints(cur_img, camera->getKeypoints(), cur_img, Scalar::all(0), DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
        }catch(exception &e){
                if (_DEBUG)
                    cout << "GuiProcessingSystem::processDataATCMsg: error while drawing keypoints: " << e.what() << endl;
            }
            if (!camera->getShowReconstruction()){
                camera->showATCFrame(cur_img);
            }
            break;
        case OPERATIVEMODE_PKLOT:
            try{
                cv::drawKeypoints(cur_img, camera->getKeypoints(), cur_img, Scalar::all(0), DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
            }catch(exception &e){
                if (_DEBUG)
                    cout << "GuiProcessingSystem::processDataATCMsg: error while drawing keypoints: " << e.what() << endl;
            }
            if (!camera->getRecognitionEnabled()){
                camera->showATCFrame(cur_img);
            }
            break;
        }


        std::cout << "GuiProcessingSystem::processDataATCMsg: setting block" << std::endl;


        if(msg->getBlockNumber() == msg->getNumBlocks()-1){
            camera->setGoodDescriptors(camera->getDescriptors());
            camera->setGoodKeypoints(camera->getKeypoints());
            camera->setNbsRecReady(true);

            double energy=0;
            camera->setProcessingEnergy(camera->getTempProcTime() * CPU_POWER_BBB);    // switch to energy
            energy+=camera->getTempProcTime() * CPU_POWER_BBB;
            if (camera->getLinkType()==LINKTYPE_TELOS){
                camera->setTxEnergy(camera->getTempTxTime() * TX_POWER_TELOS_BBB);      // switch to energy
                energy+=camera->getTempTxTime() * TX_POWER_TELOS_BBB;
            }else{
                camera->setTxEnergy(camera->getTempTxTime() * TX_POWER_WIFI_BBB);
                energy+=camera->getTempTxTime() * TX_POWER_WIFI_BBB;
            }

            //***NBS***//
            camera->decrementEnergy(energy);
            //***NBS***//

            camera->setTempProcTime(0);
            camera->setTempTxTime(0);

            /* Notify ObjectTracking, PerformanceManager */
            emit camera->frameCompletedSignal(camera->getGuiIdx(),msg->getOperativeMode());

            if (camera->getShowReconstruction()){
                cout << "GuiProcessingSystem::processDataATCMsg: emitting reconstruct frame on camera " << *camera << endl;
                /* Notify InverseBrisk */
                emit camera->reconstructFrameSignal(camera->getGuiIdx());
            }

            switch(camera->getOperativeMode()){
            case OPERATIVEMODE_OBJECT:

                break;
            case OPERATIVEMODE_PKLOT:
                //TODO should be ported to a different thread, as recognition for objects
                if (camera->getRecognitionEnabled()){
                    cv::Mat predictions;
                    _pklotClassifier->predict(camera->getGoodDescriptors(),predictions,camera->atc_param.binShift);
                    cout << predictions.t() << endl;
                    camera->setPKLotPredictions(predictions);
                    cur_img = camera->getPklotWithOccupancy();
                    camera->showATCFrame(cur_img);
                    emit camera->recognitionCompletedSignal("Free parkings:",QString::number(predictions.rows - (int)(sum(predictions)[0])));
                }
                break;
            }

            endOfFrame(camera);
        }

    }
    else{
        cerr << "GuiProcessingSystem::processDataATCMsg: error in DATA_ATC message" << endl;
    }
    delete msg;
}

/**
 * @brief GuiProcessingSystem::processCoopInfoMsg
 * @param camera
 * @param msg
 *
 * Deletes the message
 */
void GuiProcessingSystem::processCoopInfoMsg(Camera* camera, CoopInfoMsg* msg){
    camera->setNumAvailableCooperatorsSlot(msg->getCooperators().size());
    emit camera->updateNumAvailableCooperatorsSignal(camera->getId(), camera->getNumAvailableCooperators());
    delete msg;
}

/**
 * @brief GuiProcessingSystem::endOfFrame
 * Send a Start__Msg or a Stop__Msg
 * @param camera
 */
void GuiProcessingSystem::endOfFrame(Camera* camera){
    if(camera->getActive()){
        switch(camera->getCurrentMode()){
        case CAMERA_MODE_CTA:{
            sendStartCTA(camera);
            break;
        }
        case CAMERA_MODE_ATC:{

            if(camera->getNbs()){
                //camera is ready to start nbs
                camera->setNbsReady(true);
                //if also the other camera is ready,start nbs!
                if(camera->getBargainCamera()->getNbsReady()){
                    camera->resetFeatures();
                    camera->getBargainCamera()->resetFeatures();
                    sendStartATCNBS(camera);
                }
            }
            else{
                camera->resetFeatures();
                sendStartATC(camera);
            }

            break;
        }
        default:{
            break;
        }
        }
    }else{
        sendStop(camera);
    }
}

bool GuiProcessingSystem::sendStop(Camera* camera){
    StopMsg* msg = new StopMsg(NetworkNode::getGui(),
                               NetworkNode::getCameraById(camera->getId()),
                               camera->getLinkType());
    GuiNetworkSystem::getInstance()->sendMessage(msg);
    emit camera->stoppedSignal(camera->getId());
    return true;
}


bool GuiProcessingSystem::sendStartCTA(Camera* camera){

    camera->storeParameters();

    cout << "GuiProcessingSystem::sendStartCTA: camera->cta_param.quality_factor: " << camera->cta_param.quality_factor << endl;

    StartCTAMsg* msg = new StartCTAMsg(NetworkNode::getGui(),
                                       NetworkNode::getCameraById(camera->getId()),
                                       camera->getLinkType(),
                                       camera->cta_param.quality_factor,
                                       cv::Size(FRAME_W,FRAME_H), camera->cta_param.num_slices,
                                       camera->getOperativeMode(),camera->getImageSource(),
                                       camera->getWifiBw()
                                       );
    return sendStart(camera,msg);
}

bool GuiProcessingSystem::sendStartATC(Camera* camera){

    camera->storeParameters();
    StartATCMsg* msg = NULL;

    uchar numCoops = camera->getDatc() ? camera->getNumCoop() : 0;

    ushort wifiBw = camera->getWifiBw();

    ImageSource imageSource = camera->getImageSource();

    switch (camera->getOperativeMode()){
    case OPERATIVEMODE_OBJECT:{
        Bitstream emptyBitstream;
        msg = new StartATCMsg(NetworkNode::getGui(),
                              NetworkNode::getCameraById(camera->getId()),
                              camera->getLinkType(),
                              DETECTORTYPE_BRISK, // Detector Type
                              camera->atc_param.detection_threshold, //Detector Threashold
                              DESCRIPTORTYPE_BRISK, // Descriptor Type
                              256, // Descriptor Length
                              camera->atc_param.max_features, // Max number of features
                              true, // Rotation inveriant
                              camera->atc_param.encodeKeypoints, //Encode keypoints
                              camera->atc_param.encodeFeatures, //Encode features
                              true, //Transfer coordinates
                              true, //Transfer scale
                              true, //Transfer orientation
                              camera->atc_param.num_feat_per_block, // Features per block
                              cv::Size(0,0), //topLeft
                              cv::Size(FRAME_W,FRAME_H), //bottomRight
                              0, //binshift
                              0, //valshift
                              numCoops, //Num cooperators
                              OPERATIVEMODE_OBJECT,
                              imageSource,
                              emptyBitstream,
                              wifiBw
                              );
        break;
    }
    case OPERATIVEMODE_PKLOT:{
        msg = new StartATCMsg(NetworkNode::getGui(),
                              NetworkNode::getCameraById(camera->getId()),
                              camera->getLinkType(),
                              DETECTORTYPE_BRISK, // Detector Type
                              camera->atc_param.detection_threshold, //Detector Threashold
                              DESCRIPTORTYPE_BRISK, // Descriptor Type
                              256, // Descriptor Length
                              camera->atc_param.max_features, // Max number of features
                              true, // Rotation inveriant
                              camera->atc_param.encodeKeypoints, //Encode keypoints
                              camera->atc_param.encodeFeatures, //Encode features
                              true, //Transfer coordinates
                              true, //Transfer scale
                              true, //Transfer orientation
                              camera->atc_param.num_feat_per_block, // Features per block
                              cv::Size(0,0), //topLeft
                              cv::Size(FRAME_W,FRAME_H), //bottomRight
                              camera->atc_param.binShift,
                              camera->atc_param.valShift,
                              numCoops, //Num cooperators
                              OPERATIVEMODE_PKLOT,
                              imageSource,
                              camera->getPKLotKptsBitstream(),
                              camera->getWifiBw()
                              );
        break;
    }
    default:
        break;
    }


    return sendStart(camera,msg);
}

//***NBS***//
bool GuiProcessingSystem::sendStartATCNBS(Camera* camera){


    camera->storeParameters();
    camera->getBargainCamera()->storeParameters();
    camera->setNbsReady(false);
    camera->setNbsReady(false);

    //    //stop selected camera
    //    StopMsg* msg1 = new StopMsg(NetworkNode::getGui(),
    //                               NetworkNode::getCameraById(camera->getId()),
    //                               camera->getLinkType());
    //    GuiNetworkSystem::getInstance()->sendMessage(msg1);
    //    emit camera->stoppedSignal(camera->getId());

    //    //stop bargaining camera
    Camera* bargainCamera = camera->getBargainCamera();
    //    StopMsg* msg2 = new StopMsg(NetworkNode::getGui(),
    //                               NetworkNode::getCameraById(bargainCamera->getId()),
    //                               bargainCamera->getLinkType());
    //    GuiNetworkSystem::getInstance()->sendMessage(msg2);
    //    emit bargainCamera->stoppedSignal(bargainCamera->getId());

    //compute NBS solution
    Camera* leftCamera;
    Camera* rightCamera;
    if(camera->getId()<bargainCamera->getId()){
        leftCamera = camera;
        rightCamera = bargainCamera;
    }
    else
    {
        rightCamera = camera;
        leftCamera = bargainCamera;
    }

    double splitL;
    //double splitR;
    double enL, enR;
    enL = leftCamera->getResidualEnergyJoule();
    enR = rightCamera->getResidualEnergyJoule();
    splitL = enL/(enL+enR);
    //splitR = enR/(enL+enR);

    ushort nbsOffset = ((((int)(FRAME_W*(splitL)))>>1)<<1);

    leftCamera->setNbsOffset(0);
    rightCamera->setNbsOffset(nbsOffset);

    uchar numCoops = leftCamera->getDatc() ? leftCamera->getNumCoop() : 0;

    ImageSource imageSource = camera->getImageSource();

    Bitstream emptyBitstream;

    //start both cameras in nbs mode
    StartATCMsg* start_msg1 = new StartATCMsg(NetworkNode::getGui(),
                                              NetworkNode::getCameraById(leftCamera->getId()),
                                              leftCamera->getLinkType(),
                                              DETECTORTYPE_BRISK, // Detector Type
                                              leftCamera->atc_param.detection_threshold, //Detector Threashold
                                              DESCRIPTORTYPE_BRISK, // Descriptor Type
                                              256, // Descriptor Length
                                              leftCamera->atc_param.max_features>>1, // Max number of features
                                              true, // Rotation inveriant
                                              leftCamera->atc_param.encodeKeypoints, //Encode keypoints
                                              leftCamera->atc_param.encodeFeatures, //Encode features
                                              true, //Transfer coordinates
                                              true, //Transfer scale
                                              true, //Transfer orientation
                                              leftCamera->atc_param.num_feat_per_block, // Features per block
                                              cv::Size(0,0), //topLeft
                                              cv::Size(((int)(FRAME_W*(splitL+NBS_OVL))>>1)<<1,FRAME_H), //bottomRight
                                              leftCamera->atc_param.binShift,
                                              leftCamera->atc_param.valShift,
                                              numCoops, //Num cooperators
                                              OPERATIVEMODE_OBJECT,
                                              imageSource,
                                              emptyBitstream, //pklot kpts bitstream
                                              leftCamera->getWifiBw()
                                              );

    numCoops = rightCamera->getDatc() ? leftCamera->getNumCoop() : 0;
    StartATCMsg* start_msg2 = new StartATCMsg(NetworkNode::getGui(),
                                              NetworkNode::getCameraById(rightCamera->getId()),
                                              rightCamera->getLinkType(),
                                              DETECTORTYPE_BRISK, // Detector Type
                                              rightCamera->atc_param.detection_threshold, //Detector Threashold
                                              DESCRIPTORTYPE_BRISK, // Descriptor Type
                                              256, // Descriptor Length
                                              rightCamera->atc_param.max_features>>1, // Max number of features
                                              true, // Rotation inveriant
                                              rightCamera->atc_param.encodeKeypoints, //Encode keypoints
                                              rightCamera->atc_param.encodeFeatures, //Encode features
                                              true, //Transfer coordinates
                                              true, //Transfer scale
                                              true, //Transfer orientation
                                              rightCamera->atc_param.num_feat_per_block, // Features per block
                                              cv::Size(nbsOffset,0), //topLeft
                                              cv::Size(FRAME_W,FRAME_H), //bottomRight
                                              rightCamera->atc_param.binShift,
                                              rightCamera->atc_param.valShift,
                                              numCoops, //Num cooperators
                                              OPERATIVEMODE_OBJECT,
                                              imageSource,
                                              emptyBitstream, //pklot kpts bitstream
                                              rightCamera->getWifiBw()
                                              );
    return (sendStart(leftCamera,start_msg1) && sendStart(rightCamera,start_msg2));
}
//***NBS***//

/**
 * Deletes the message
 * @param msg
 * @return
 */
bool GuiProcessingSystem::sendStart(Camera* camera, Message* msg){

    bool retVal = GuiNetworkSystem::getInstance()->sendMessage(msg);

    /* Do not cancel the history on every start, but only at first */
    emit camera->startSentSignal(camera->getId(), false);
    emit camera->startCameraTimerSignal();

    return retVal;
}
