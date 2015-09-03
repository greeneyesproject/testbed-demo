#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <CameraSettings.h>
#include <CameraView.h>
#include <NetworkTopology.h>
#include <Plot.h>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);

    explicit MainWindow(CameraSettings * camSet,
                        std::vector<CameraView*> &camViews,
                        NetworkTopology * netTop,
                        std::vector<Plot*> &camPlot,
                        QWidget *parent = 0);

    ~MainWindow();

    std::vector<Plot *> camPlots() const;
    void setCamPlots(const std::vector<Plot *> &camPlots);

private slots:

    void toggleCameraSettings();
    void toggleMainWindow();
    void toggleNetworkTopology();
    void toggleCameraView(int a);
    void test();

    void toggleCameraPlot(int a);

    void showGuiConnect(QString address, unsigned short port);
    void showGuiDisconnect();

private:

    Ui::MainWindow *ui;
    CameraSettings * _camSet;
    std::vector<CameraView*> _camViews;
    std::vector<Plot*> _camPlots;
    NetworkTopology * _netTop;
    std::vector<QAction*> _cameraViewActions;
    std::vector<QAction*> _cameraPerformanceActions;

};

#endif // MAINWINDOW_H
