#include "PerformanceManager.h"
#include <iostream>

PerformanceManager* PerformanceManager::_instance=NULL;

PerformanceManager* PerformanceManager::getPerformanceManager(
        vector<Camera *> *cameras,
        int historySize,
        int timerFreq,
        int windowSize){
    if (_instance==NULL){
        _instance = new PerformanceManager(cameras,historySize,timerFreq,windowSize);
    }
    return _instance;
}

PerformanceManager::PerformanceManager(vector<Camera *>* cameras, int historySize, int timerFreq, size_t windowSize):
    _historySize(historySize),
    _timerFreq(timerFreq),
    _windowSize(windowSize)
{

    _cameras = cameras;

    _nCameras = _cameras->size();
    // _timer = new QTimer(this);

    for (int c = 0; c < _cameras->size(); c++){
        _curFrameTimer.push_back(new QElapsedTimer());
    }

    _lastFrameTimings.resize(_nCameras);

    _t = 0;

    _statusATC.resize(_nCameras);
    _statusCTA.resize(_nCameras);
    _statusDetTh.resize(_nCameras);
    _statusMaxF.resize(_nCameras);
    _statusEncodeKp.resize(_nCameras);
    _statusEntropy.resize(_nCameras);
    _statusDATC.resize(_nCameras);
    _statusNCoop.resize(_nCameras);
    _statusQF.resize(_nCameras);
    _statusFrameRate.resize(_nCameras);
    _statusProcTime.resize(_nCameras);
    _statusTxTime.resize(_nCameras);

    _statusProcTimeLatest.resize(_nCameras);
    _statusTxTimeLatest.resize(_nCameras);

    for (int c = 0; c < _nCameras; c++){
    //    _completedFrame.push_back(0);
        _curFrameRate.push_back(0);
    }


}

void PerformanceManager::takeSnapshot(){


    //qDebug() << "snapshot timer fired";
    vector<Camera*> temp;
    temp.resize(_nCameras);

    for (int c = 0; c < _nCameras; c++){

        Camera* cur_cam = (*(_cameras))[c];

        if (_statusATC[c].size() >= _historySize)
            _statusATC[c].pop_front();
        if ((cur_cam->getCurrentMode() == CAMERA_MODE_ATC)&&(!cur_cam->getDatc()))
            _statusATC[c].push_back(1.0);
        else
            _statusATC[c].push_back(0.0);

        if (_statusCTA[c].size() >= _historySize)
            _statusCTA[c].pop_front();
        if ((cur_cam->getCurrentMode() == CAMERA_MODE_CTA))
            _statusCTA[c].push_back(1.0);
        else
            _statusCTA[c].push_back(0.0);


        if (_statusDATC[c].size() >= _historySize)
            _statusDATC[c].pop_front();
        if (cur_cam->getDatc())
            _statusDATC[c].push_back(1.0);
        else
            _statusDATC[c].push_back(0.0);

        if (_statusDetTh[c].size() >= _historySize)
            _statusDetTh[c].pop_front();
        _statusDetTh[c].push_back((double) cur_cam->getDetThres());

        if (_statusMaxF[c].size() >= _historySize)
            _statusMaxF[c].pop_front();
        _statusMaxF[c].push_back((double) cur_cam->getMaxNumFeatures());

        if (_statusEncodeKp[c].size() >= _historySize)
            _statusEncodeKp[c].pop_front();
        _statusEncodeKp[c].push_back((double) cur_cam->getEncodeKeypoints());

        if (_statusEntropy[c].size() >= _historySize)
            _statusEntropy[c].pop_front();
        _statusEntropy[c].push_back((double) cur_cam->getEncodeFeatures());

        if (_statusNCoop[c].size() >= _historySize)
            _statusNCoop[c].pop_front();
        _statusNCoop[c].push_back((double) cur_cam->getNumCoop());

        if (_statusQF[c].size() >= _historySize)
            _statusQF[c].pop_front();
        _statusQF[c].push_back((double) cur_cam->getQf());

        if (_statusFrameRate[c].size() >= _historySize)
            _statusFrameRate[c].pop_front();
        _statusFrameRate[c].push_back((double) cur_cam->getFrameRate());

        if (_statusProcTime[c].size() >= _historySize)
            _statusProcTime[c].pop_front();
        /* We need to define what kind of energy to show. The theorically correct way is to multiply by the frame rate, but it is difficult to explain the results.
         * Let's show the energy for the last frame up the new one */
        //_statusProcTime[c].push_back((double) cur_cam->getProcessingEnergy()*(double)cur_cam->getFrameRate());
        /* Median filter of _windowSize */
        _statusProcTimeLatest[c].push_back((double) cur_cam->getProcessingEnergy());
        if (_statusProcTimeLatest[c].size() > _windowSize){
            _statusProcTimeLatest[c].erase(_statusProcTimeLatest[c].begin());
        }
        vector<double> statusProcTimeLatestSorted = _statusProcTimeLatest[c];
        sort(statusProcTimeLatestSorted.begin(),statusProcTimeLatestSorted.end());
        _statusProcTime[c].push_back((double) statusProcTimeLatestSorted[_windowSize>>1]);

        if (_statusTxTime[c].size() >= _historySize)
            _statusTxTime[c].pop_front();
        /* We need to define what kind of energy to show. The theorically correct way is to multiply by the frame rate, but it is difficult to explain the results.
         * Let's show the energy for the last frame up the new one */
        //_statusTxTime[c].push_back((double) cur_cam->getTxEnergy()*(double)cur_cam->getFrameRate());
        /* Median filter of _windowSize */
        _statusTxTimeLatest[c].push_back((double) cur_cam->getTxEnergy());
        if (_statusTxTimeLatest[c].size() > _windowSize){
            _statusTxTimeLatest[c].erase(_statusTxTimeLatest[c].begin());
        }
        vector<double> statusTxTimeLatestSorted = _statusTxTimeLatest[c];
        sort(statusTxTimeLatestSorted.begin(),statusTxTimeLatestSorted.end());
        _statusTxTime[c].push_back((double) statusTxTimeLatestSorted[_windowSize>>1]);

    }


    if (_time.size() >= _historySize){
        _time.pop_front();
    }
    _time.push_back( ((double) _t*_timerFreq)/1000.0 );

    _t++;
}
int PerformanceManager::nCameras() const
{
    return _nCameras;
}

void PerformanceManager::setNCameras(int nCameras)
{
    _nCameras = nCameras;
}


void PerformanceManager::checkResetTimerCount(){

    //cout << "Check reset timer" << endl;

    //if (_t >= _windowSize){
        //int timeInterval = _timerFreq * _windowSize;
        for (int c = 0; c < _nCameras; c++){

            Camera* cur_cam = (*(_cameras))[c];

            // compute framerate averaging the required time of last N frames
            /*_curFrameRate[c] = 0.0;
            if (_lastFrameTimings[c].size() > 0){
                for (list<double>::iterator it = _lastFrameTimings[c].begin(); it != _lastFrameTimings[c].end(); it++){
                    _curFrameRate[c] += ((double) 1000)/(((double) *it) * ((double) _lastFrameTimings[c].size()));
                }
            }

            */

            /* Median filter over last _windowSize values of _lastFrameTimings */
            vector<double> cameraLastTimings;
            for (list<double>::iterator it = _lastFrameTimings[c].begin(); it != _lastFrameTimings[c].end(); it++){
                cameraLastTimings.push_back(*it);
            }
            sort(cameraLastTimings.begin(),cameraLastTimings.end());

            if (cameraLastTimings.size()){
                _curFrameRate[c] = ((double) 1000)/(double)cameraLastTimings[cameraLastTimings.size()>>1];
            }else{
                _curFrameRate[c] = 0;
            }


            /* Set into the camera */
            cur_cam->setFrameRate(_curFrameRate[c]);
            /* Notify plot */
            emit frameRateUpdated();

        }
    //}
}

//void PerformanceManager::increaseFrameCount(int guiIdx){

//    cout << "frame count increased on camera " << guiIdx << endl;
//    _completedFrame[guiIdx]++;
//}

void PerformanceManager::startLogging(){

    cout << "logging started" << endl;
    _timer->start(_timerFreq);
    cout << "timer started" << endl;
    //qDebug() << "Parent of timer is" << _timer->parent();


}

void PerformanceManager::stopLogging(){

    _timer->stop();
}

void PerformanceManager::connectTasks(){

    cout << "connecting performance manager task" << endl;

    _timer = new QTimer(this);

    for (int c = 0; c < _nCameras; c++){
        Camera* cur_cam = (*(_cameras))[c];
        //connect(cur_cam, SIGNAL(frameCompletedSignal(int,int)), this, SLOT(increaseFrameCount(int,int)));
        connect(cur_cam, SIGNAL(stoppedSignal(int)), this, SLOT(stopFrameRateTimer(int)));
        connect(cur_cam, SIGNAL(startSentSignal(int, bool)), this, SLOT(resetFrameRateTimer(int, bool)));
        connect(cur_cam, SIGNAL(frameCompletedSignal(int,int)), this, SLOT(endOfFrame(int,int)));
    }

    connect(_timer, SIGNAL(timeout()), this, SLOT(takeSnapshot()));
    connect(_timer, SIGNAL(timeout()), this, SLOT(checkResetTimerCount()));

    startLogging();
    while (!_timer->isActive())
        startLogging();

}

list<double> PerformanceManager::time() const
{
    return _time;
}

void PerformanceManager::setTime(const list<double> &time)
{
    _time = time;
}

vector<list<double> > PerformanceManager::statusDetTh() const
{
    return _statusDetTh;
}

void PerformanceManager::setStatusDetTh(const vector<list<double> > &statusDetTh)
{
    _statusDetTh = statusDetTh;
}
vector<list<double> > PerformanceManager::statusMaxF() const
{
    return _statusMaxF;
}

void PerformanceManager::setStatusMaxF(const vector<list<double> > &statusMaxF)
{
    _statusMaxF = statusMaxF;
}
vector<list<double> > PerformanceManager::statusEncodeKp() const
{
    return _statusEncodeKp;
}

void PerformanceManager::setStatusEncodeKp(const vector<list<double> > &statusEncodeKp)
{
    _statusEncodeKp = statusEncodeKp;
}
vector<list<double> > PerformanceManager::statusEntropy() const
{
    return _statusEntropy;
}

void PerformanceManager::setStatusEntropy(const vector<list<double> > &statusEntropy)
{
    _statusEntropy = statusEntropy;
}
vector<list<double> > PerformanceManager::statusDATC() const
{
    return _statusDATC;
}

void PerformanceManager::setStatusDATC(const vector<list<double> > &statusDATC)
{
    _statusDATC = statusDATC;
}
vector<list<double> > PerformanceManager::statusNCoop() const
{
    return _statusNCoop;
}

void PerformanceManager::setStatusNCoop(const vector<list<double> > &statusNCoop)
{
    _statusNCoop = statusNCoop;
}
vector<list<double> > PerformanceManager::statusQF() const
{
    return _statusQF;
}

void PerformanceManager::setStatusQF(const vector<list<double> > &statusQF)
{
    _statusQF = statusQF;
}
vector<list<double> > PerformanceManager::statusFrameRate() const
{
    return _statusFrameRate;
}

void PerformanceManager::setStatusFrameRate(const vector<list<double> > &statusFrameRate)
{
    _statusFrameRate = statusFrameRate;
}
vector<list<double> > PerformanceManager::statusCTA() const
{
    return _statusCTA;
}

void PerformanceManager::setStatusCTA(const vector<list<double> > &statusCTA)
{
    _statusCTA = statusCTA;
}
vector<list<double> > PerformanceManager::statusATC() const
{
    return _statusATC;
}

void PerformanceManager::setStatusATC(const vector<list<double> > &statusATC)
{
    _statusATC = statusATC;
}
vector<list<double> > PerformanceManager::statusProcTime() const
{
    return _statusProcTime;
}

void PerformanceManager::setStatusProcTime(const vector<list<double> > &statusProcTime)
{
    _statusProcTime = statusProcTime;
}
vector<list<double> > PerformanceManager::statusTxTime() const
{
    return _statusTxTime;
}

void PerformanceManager::setStatusTxTime(const vector<list<double> > &statusTxTime)
{
    _statusTxTime = statusTxTime;
}



/**
 * @brief PerformanceManager::endOfFrame
 * Store time interval between two frames in ticks
 * @param cam
 */
void PerformanceManager::endOfFrame(int camGuiIdx, int opmode){

    //qDebug() << "end of frame, performance handler";

    unsigned int time = _curFrameTimer[camGuiIdx]->elapsed();
    if (_lastFrameTimings[camGuiIdx].size() >= _windowSize){
        _lastFrameTimings[camGuiIdx].pop_front();
    }
    _lastFrameTimings[camGuiIdx].push_back(time);

    //qDebug() << "Frame finished: " << time << " ms elapsed.";
    //qDebug() << "ok";
}

/**
 * @brief PerformanceManager::resetFrameRateTimer
 * Called when start message is sent
 * @param camId
 * @param reset_history
 */
void PerformanceManager::resetFrameRateTimer(int camId, bool reset_history){

    ushort cam_index = Camera::getCameraById(camId)->getGuiIdx();
//    if (reset_history){
//        _lastFrameTimings[cam_index].clear();
//    }
    _curFrameTimer[cam_index]->restart();
    //qDebug() << "New start frame message: Framerate timer started on camera " << cam_index;
}

void PerformanceManager::stopFrameRateTimer(int camId){
    _lastFrameTimings[Camera::getCameraById(camId)->getGuiIdx()].clear();
}




















