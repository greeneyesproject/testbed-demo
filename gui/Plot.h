#ifndef PLOT_H
#define PLOT_H

#include <QWidget>
#include <PerformanceManager.h>
#include <Global.h>
#include "QCustomPlot.h"

namespace Ui {
class Plot;
}

class Plot : public QWidget
{
    Q_OBJECT

public:
    explicit Plot(PerformanceManager &perf, int camId, Camera &cam, QWidget *parent = 0);
    ~Plot();

    void replotSingle(QCustomPlot * customPlot, plot_var &plotType, bool &barGraph);

public slots:

    void replot();
    void upperChangePlotVar(int);
    void lowerChangePlotVar(int);

private:

    void _initBarGraph(QCustomPlot* customPlot);

    Ui::Plot *ui;
    PerformanceManager * _perf;
    int _camId;
    Camera * _cam;
    plot_var _upper;
    plot_var _lower;
    bool _isEnergyUp;
    bool _isEnergyLo;

    QCPBars * bars_proc;
    QCPBars * bars_tx;

};

#endif // PLOT_H
