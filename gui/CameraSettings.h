#ifndef CAMERASETTINGS_H
#define CAMERASETTINGS_H

#include <QWidget>
#include <QButtonGroup>
#include <opencv2/opencv.hpp>
#include <Camera.h>


class NetworkTopology;
class GuiProcessingSystem;
class GuiNetworkSystem;

namespace Ui {
class CameraSettings;
}

class CameraSettings : public QWidget
{
    Q_OBJECT

public:
    explicit CameraSettings(QWidget *parent = 0);
    ~CameraSettings();

    int activeCameraId() const;
    void setActiveCamera(Camera* camera);
    void setNetworkTopology(NetworkTopology * netTop);

private:
    Ui::CameraSettings *ui;

    bool sendStart(Message* msg);

    /* Camera selected in Camera Settings Tab View */
    Camera* _selectedCamera;

    std::vector<Camera*>* _cameras;

    NetworkTopology * _netTop;

    GuiProcessingSystem* _guiProcessingSystem;
    GuiNetworkSystem* _guiNetworkSystem;

    void setSelectedCameraParameters();

public slots:

    void atcCtaRadioSwitchSlot(QAbstractButton* button);
    void sinkCameraLinkRadioSwitchSlot(QAbstractButton*);
    void wifiBwModifiedSlot(int value);
    void entropyToggledSlot(bool value);
    void detectionThresholdModifiedSlot(int value);
    void maxFeaturesModifiedSlot(int value);
    void showReconstructionModifiedSlot(bool value);
    void datcToggledSlot(bool value);
    void numCooperatorsModifiedSlot(int value);
    void qfModifiedSlot(int value);
    void setSelectedCameraSlot(int guiIdx);
    void encodeKeypointsToggledSlot(bool value);
    //***NBS***//
    void nbsToggledSlot(bool value);
    void nbsSelectSlot();
    //***NBS***//


    void numFeatPerBlockModifiedSlot(int value);
    void numSlicesModifiedSlot(int value);

    void numAvailableCooperatorsModifiedSlot(uint id, int nCoop);

    void recognitionToggledSlot(bool checked);
    void trackingToggledSlot(bool checked);

    void toggleCameraSlot();

    void sinkDisconnectSlot();

private slots:

    void operativeModeToggledSlot(QAbstractButton*);

signals:

    void numCoopChangedSignal(int, int);
    void numAvailableCoopChangedSignal(int, int);

};

#endif // CAMERASETTINGS_H
