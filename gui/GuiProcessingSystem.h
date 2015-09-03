#ifndef GUIPROCESSINGSYSTEM_H
#define GUIPROCESSINGSYSTEM_H

#include <Multimedia/VisualFeatureDecoding.h>
#include <Multimedia/VisualFeatureExtraction.h>
#include <PKLotClassifier.h>

class Camera;
class DataCTAMsg;
class DataATCMsg;
class CoopInfoMsg;
class CoopInfoRespMsg;
class Message;

class GuiProcessingSystem{

public:
    static GuiProcessingSystem* getInstance();

    void processDataCTAMsg(Camera* camera, DataCTAMsg* msg);
    void processDataATCMsg(Camera* camera, DataATCMsg* msg);
    void processCoopInfoMsg(Camera* camera, CoopInfoMsg* msg);
    void processCoopInfoRespMsg(Camera* camera, CoopInfoRespMsg* msg);

    void endOfFrame(Camera* camera);

    bool sendStartCTA(Camera* camera);
    bool sendStartATC(Camera* camera);
    bool sendStartATCNBS(Camera* camera);
    bool sendStart(Camera* camera, Message* msg);
    bool sendStop(Camera* camera);

    VisualFeatureExtraction* getBriskExtractor(){
        return _briskExtractor;
    }


private:

    const static unsigned char _DEBUG = 1;
    static GuiProcessingSystem* _instance;
    GuiProcessingSystem();

    VisualFeatureDecoding* _decoder;
    VisualFeatureExtraction* _briskExtractor;
    VisualFeatureExtraction* _histExtractor;

    std::vector<unsigned char>* _lastFrameId;

    PKLotClassifier* _pklotClassifier;

};

#endif // GUIPROCESSINGSYSTEM_H
