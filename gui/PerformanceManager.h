#ifndef PERFORMANCEMANAGER_H
#define PERFORMANCEMANAGER_H

#include <QTimer>
#include <QElapsedTimer>
#include <Camera.h>
#include <vector>
#include <list>

using namespace std;

class PerformanceManager : public QObject
{
    Q_OBJECT
public:

    static PerformanceManager* getPerformanceManager(
            vector<Camera *>*cameras,
            int historySize = 120,
            int timerFreq = 1000,
            int windowSize = 3);

    int nCameras() const;
    void setNCameras(int nCameras);
    void connectTasks();

    list<double> time() const;
    void setTime(const list<double> &time);



    vector<list<double> > statusDetTh() const;
    void setStatusDetTh(const vector<list<double> > &statusDetTh);

    vector<list<double> > statusMaxF() const;
    void setStatusMaxF(const vector<list<double> > &statusMaxF);

    vector<list<double> > statusEncodeKp() const;
    void setStatusEncodeKp(const vector<list<double> > &statusEncodeKp);

    vector<list<double> > statusEntropy() const;
    void setStatusEntropy(const vector<list<double> > &statusEntropy);

    vector<list<double> > statusDATC() const;
    void setStatusDATC(const vector<list<double> > &statusDATC);

    vector<list<double> > statusNCoop() const;
    void setStatusNCoop(const vector<list<double> > &statusNCoop);

    vector<list<double> > statusQF() const;
    void setStatusQF(const vector<list<double> > &statusQF);

    vector<list<double> > statusFrameRate() const;
    void setStatusFrameRate(const vector<list<double> > &statusFrameRate);

    vector<list<double> > statusCTA() const;
    void setStatusCTA(const vector<list<double> > &statusCTA);

    vector<list<double> > statusATC() const;
    void setStatusATC(const vector<list<double> > &statusATC);

    vector<list<double> > statusProcTime() const;
    void setStatusProcTime(const vector<list<double> > &statusProcTime);

    vector<list<double> > statusTxTime() const;
    void setStatusTxTime(const vector<list<double> > &statusTxTime);

private:

    static PerformanceManager* _instance;
    PerformanceManager(vector<Camera *>*cameras,
                       int historySize = 40,
                       int timerFreq = 1000,
                       size_t windowSize = 3);

    QTimer * _timer;
    vector<QElapsedTimer *> _curFrameTimer;

    // cyclic lists of camera status
    vector< list<double> > _statusCTA;
    vector< list<double> > _statusATC;
    vector< list<double> > _statusDATC;
    vector< list<double> > _statusDetTh;
    vector< list<double> > _statusMaxF;
    vector< list<double> > _statusEncodeKp;
    vector< list<double> > _statusEntropy;
    vector< list<double> > _statusNCoop;
    vector< list<double> > _statusQF;
    vector< list<double> > _statusFrameRate;
    vector< list<double> > _statusProcTime;
    vector< list<double> > _statusTxTime;

    /* Contains the last _windowSize values for ProcTIme and TxTime in order to perform a small low pass filtering */
    vector< vector<double> > _statusProcTimeLatest;
    vector< vector<double> > _statusTxTimeLatest;

    // cyclic list of time instants
    list<double> _time;

    int _t;
    /**
     * @brief _historySize
     * plot length in seconds
     */
    int _historySize;
    /**
     * @brief _timerFreq
     * sampling time of graph parameters
     */
    int _timerFreq; // ms
    /**
     * @brief _windowSize
     */
    size_t _windowSize; // number of Timer cycles on which mediate statistics
    //vector<list<unsigned int>> _completedFrame;
    vector<double> _curFrameRate;
    vector<Camera *> *_cameras;
    int _nCameras;

    vector<list<double> > _lastFrameTimings; // one list per camera with N last frame required computational time

public slots:

    void checkResetTimerCount();
    //void increaseFrameCount(int);
    void startLogging();
    void stopLogging();
    void takeSnapshot();

    void endOfFrame(int camGuiIdx,int);
    void resetFrameRateTimer(int cam_index,  bool reset_history);
    void stopFrameRateTimer(int cam_index);

signals:

    void frameRateUpdated();
};

#endif // PERFORMANCEMANAGER_H
