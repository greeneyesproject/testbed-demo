#ifndef CAMERAVIEW_H
#define CAMERAVIEW_H

#include <QWidget>
#include <Camera.h>
#include <iostream>

using namespace std;

namespace Ui {
class CameraView;
}

class CameraView : public QWidget
{
    Q_OBJECT

public:
    explicit CameraView(QWidget *parent = 0);
    explicit CameraView(Camera * camera, QWidget *parent = 0, int id = 0);
    ~CameraView();

    Camera *cam() const;
//    void setCam(Camera *cam);

private:
    Ui::CameraView *ui;
    int _tabIdx;
    Camera * _cam;

public slots:
    void setImage(QImage image);
    void showCTAFrame();
    void showATCFrame();
    void showATCRecFrame();
    void updateObjectSlot(QString);
    void updateFrameRate();
    void updateBandwidthSlot(int,double);
    //***NBS***//
    void updateResidualEnergy();
    //***NBS***//

};

#endif // CAMERAVIEW_H
