#include "Plot.h"
#include "ui_plot.h"
#include <list>
#include <algorithm>
#include <iostream>

Plot::Plot(PerformanceManager &perf, int camId, Camera &cam, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Plot)
{
    _perf = &perf;
    _camId = camId;
    _cam = &cam;
    _upper = ENERGY;
    _lower = FRAMERATE;

    _isEnergyUp = (_upper == ENERGY);
    _isEnergyLo = (_lower == ENERGY);

    bars_proc = NULL;
    bars_tx = NULL;

    ui->setupUi(this);

     _initBarGraph(ui->widget_upper);

    ui->comboBox_upper->setCurrentIndex(ENERGY);
    ui->comboBox_lower->setCurrentIndex(FRAMERATE);

    connect(_perf, SIGNAL(frameRateUpdated()), this, SLOT(replot()));

    connect(ui->comboBox_upper, SIGNAL(currentIndexChanged(int)), this, SLOT(upperChangePlotVar(int)));
    connect(ui->comboBox_lower, SIGNAL(currentIndexChanged(int)), this, SLOT(lowerChangePlotVar(int)));

}

void Plot::replot(){

    replotSingle(ui->widget_upper, _upper, _isEnergyUp);

    replotSingle(ui->widget_lower, _lower, _isEnergyLo);

}

void Plot::upperChangePlotVar(int a){

    cout << "current var index on upper plot is " << a << endl;
    _upper = static_cast<plot_var>(a);
}

void Plot::lowerChangePlotVar(int a){

    cout << "current var index on lower plot is " << a << endl;
    _lower = static_cast<plot_var>(a);
}

Plot::~Plot()
{
    delete ui;
}

void Plot::_initBarGraph(QCustomPlot * customPlot){
    bars_proc = new QCPBars(customPlot->xAxis, customPlot->yAxis);
    bars_proc->setName("CPU");
    bars_proc->setPen(QPen(Qt::blue));
    customPlot->addPlottable(bars_proc);

    bars_tx = new QCPBars(customPlot->xAxis, customPlot->yAxis);
    bars_tx->setName("Tx");
    bars_tx->setPen(QPen(Qt::red));
    customPlot->addPlottable(bars_tx);
}

void Plot::replotSingle(QCustomPlot * customPlot, plot_var &plotType, bool &barGraph){


    // generate some data:
    QVector<double> x(_perf->time().size()), y(_perf->time().size()), y2(_perf->time().size()), y3((_perf->time().size())); // initialize with entries 0..100

    QVector<double> yProc(_perf->time().size()), yTx(_perf->time().size());

    vector<QVector<double> > xNew;
    vector<int> leg;
    vector<QVector<double> > yNew;

    vector<QVector<double> > yNew1;
    vector<QVector<double> > yNew2;


    assert(x.size() == y.size());

    int i = 0;

    list<double> temp = _perf->time();

    list<double> temp_proc;
    list<double> temp_tx;

    list<double> op_ATC = _perf->statusATC()[_camId];
    list<double> op_CTA = _perf->statusCTA()[_camId];
    // list<double> op_DATC = _perf->statusDATC()[_camId];

    QVector<double> tempx;
    QVector<double> tempy;

    QVector<double> tempy1;     // needed for Energy plot
    QVector<double> tempy2;


    int lastleg = -1;

    list<double>::reverse_iterator rit_ATC = op_ATC.rbegin();
    list<double>::reverse_iterator rit_CTA = op_CTA.rbegin();
    list<double>::reverse_iterator rit_DATC = op_CTA.rbegin();

    list<double>::reverse_iterator rit_tempProc;
    list<double>::reverse_iterator rit_tempTx;

    for (list<double>::reverse_iterator rit = temp.rbegin(); rit != temp.rend(); ++rit){

        if (i >= x.size())
            break;

        x[i] = *rit;
        //            y[i] = log2(x[i]); // let's plot a quadratic function
        i++;

        double cur_mode;
        if (*rit_DATC)
            cur_mode = 3; // DATC
        else if (*rit_ATC)
            cur_mode = 2; // ATC
        else
            cur_mode = 1; // CTA


        if (cur_mode != lastleg){   // new line to be drawn

            if (tempx.size())
                xNew.push_back(tempx);

            tempx.clear();
            tempx.push_back(*rit);

            leg.push_back(cur_mode); // new one
            lastleg = cur_mode;
        }
        else{
            tempx.push_back(*rit);
            lastleg = cur_mode;
        }

        rit_ATC++;
        rit_DATC++;
        rit_CTA++;

    }

    if (tempx.size()){
        xNew.push_back(tempx);
    }

    i = 0;

    string y_lab;

    switch (plotType){
    case OP_MODE:
        temp = (_perf->statusCTA())[_camId];
        y_lab = "Operation mode";
        break;
    case FRAMERATE:
        temp = (_perf->statusFrameRate())[_camId];
        y_lab = "Framerate [fps]";
        break;
    case DET_TH:
        temp = (_perf->statusDetTh())[_camId];
        y_lab = "Detection threshold";
        break;
    case MAX_F:
        temp = (_perf->statusMaxF())[_camId];
        y_lab = "Max number of features";
        break;
    case ENCODE_KP:
        temp = (_perf->statusEncodeKp())[_camId];
        y_lab = "Encode keypoint location [ON/OFF]";
        break;
    case ENTROPY:
        temp = (_perf->statusEntropy())[_camId];
        y_lab = "Entropy coding [ON/OFF]";
        break;
    case COOP:
        temp = (_perf->statusNCoop())[_camId];
        y_lab = "Number of cooperators";
        break;
    case QF:
        temp = (_perf->statusQF())[_camId];
        y_lab = "JPEG quality factor";
        break;
    case ENERGY:
        temp_proc = (_perf->statusProcTime())[_camId];
        temp_tx = (_perf->statusTxTime())[_camId];
        y_lab = "Energy [J]";
        temp = temp_proc; // useless
        //        default:
        //            temp = (_perf->statusOpMode())[0];
    }

    //        temp = (_perf->statusQF())[0];

    rit_ATC = op_ATC.rbegin();
    rit_CTA = op_CTA.rbegin();
    rit_DATC = op_CTA.rbegin();

    rit_tempProc = temp_proc.rbegin();
    rit_tempTx = temp_tx.rbegin();

    lastleg = -1;

    for (list<double>::reverse_iterator rit = temp.rbegin(); rit != temp.rend(); ++rit){

        if (i >= y.size())
            break;

        y[i] = *rit;
        yProc[i] = *rit_tempProc;
        yTx[i] = *rit_tempTx + *rit_tempProc;

        i++;

        double cur_mode;
        if (*rit_DATC)
            cur_mode = 3; // DATC
        else if (*rit_ATC)
            cur_mode = 2; // ATC
        else
            cur_mode = 1; // CTA

        if (plotType == ENERGY){

            if (cur_mode != lastleg){   // new line to be drawn

                if (tempy1.size()){
                    yNew1.push_back(tempy1);
                    yNew2.push_back(tempy2);
                }

                tempy1.clear();
                tempy1.push_back(*rit_tempProc);

                tempy2.clear();
                tempy2.push_back(*rit_tempTx);

                lastleg = cur_mode;
            }
            else{
                tempy1.push_back(*rit_tempProc);
                tempy2.push_back(*rit_tempTx);
                lastleg = cur_mode;
            }

        }
        else
        {

            if (cur_mode != lastleg){   // new line to be drawn

                if (tempy.size())
                    yNew.push_back(tempy);

                tempy.clear();
                tempy.push_back(*rit);

                lastleg = cur_mode;
            }
            else{
                tempy.push_back(*rit);
                lastleg = cur_mode;
            }
        }

        rit_ATC++;
        rit_DATC++;
        rit_CTA++;

        rit_tempProc++;
        rit_tempTx++;

    }

    if (plotType == ENERGY){
        if (tempy1.size()){
            yNew1.push_back(tempy1);
            yNew2.push_back(tempy2);
        }
    }
    else{
        if (tempy.size()){
            yNew.push_back(tempy);
        }
    }


    if (plotType == OP_MODE){

        temp = (_perf->statusATC())[_camId];

        i = 0;

        for (list<double>::reverse_iterator rit = temp.rbegin(); rit != temp.rend(); ++rit){

            if (i >= y.size())
                break;

            y2[i] = *rit;

            i++;

        }

        temp = (_perf->statusDATC())[_camId];

        i = 0;

        for (list<double>::reverse_iterator rit = temp.rbegin(); rit != temp.rend(); ++rit){

            if (i >= y.size())
                break;

            y3[i] = *rit;
            i++;

        }

    }

    // create graph and assign data to it:
    if (customPlot->graphCount() == 0)
        customPlot->addGraph();

    double mm = 99999;
    double MM = -99999;

    if (plotType == ENERGY){


        if (customPlot->graphCount() > 0){
            while (customPlot->graphCount() > 0){
                customPlot->removeGraph(customPlot->graphCount() - 1);
            }
        }

        if (!barGraph){
            _initBarGraph(customPlot);
        }
        barGraph = true;

        bars_proc->setData(x, yProc);
        bars_tx->setData(x, yTx);

        customPlot->legend->setVisible(true);


        mm = 0;

        MM = max(*std::max_element(yProc.begin(), yProc.end()), *std::max_element(yTx.begin(), yTx.end()));


    }else if (plotType == OP_MODE){

        if (barGraph){
            if (customPlot->plottableCount() > 0){
                while (customPlot->plottableCount() > 0){
                    customPlot->removePlottable(customPlot->plottableCount() - 1);
                }
            }
        }

        barGraph = false;

        while (customPlot->graphCount() < 3)
            customPlot->addGraph();
        customPlot->graph(0)->setPen(QPen(Qt::blue));
        customPlot->graph(0)->setData(x, y);
        customPlot->graph(0)->setName("CTA");
        customPlot->graph(1)->setPen(QPen(Qt::red));
        customPlot->graph(1)->setData(x, y2);
        customPlot->graph(1)->setName("ATC");
        customPlot->graph(2)->setPen(QPen(Qt::green));
        customPlot->graph(2)->setData(x, y3);
        customPlot->graph(2)->setName("DATC");
        customPlot->legend->setVisible(true);
        customPlot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignBottom|Qt::AlignLeft);

        mm = 0;
        MM = 2;
    }
    else{

        if (barGraph){
            if (customPlot->plottableCount() > 0){
                while (customPlot->plottableCount() > 0){
                    customPlot->removePlottable(customPlot->plottableCount() - 1);
                }
            }
        }

        barGraph = false;


        if (customPlot->graphCount() > xNew.size()){
            while (customPlot->graphCount() > xNew.size()){
                customPlot->removeGraph(customPlot->graphCount() - 1);
            }
        }
        else if (customPlot->graphCount() < xNew.size()){
            while (customPlot->graphCount() < xNew.size()){
                customPlot->addGraph();
            }
        }

        //qDebug() << customPlot->graphCount();

        for (int i = 0; i < xNew.size(); i++){

            switch(leg[i]){
            case 1:
                customPlot->graph(i)->setPen(QPen(Qt::green));
                break;
            case 2:
                customPlot->graph(i)->setPen(QPen(Qt::red));
                break;
            case 3:
                customPlot->graph(i)->setPen(QPen(Qt::blue));
                break;
            default:
                break;
            }

            customPlot->graph(i)->setData(xNew[i], yNew[i]);

            mm = min(mm, *std::min_element(yNew[i].begin(), yNew[i].end()));
            MM = max(MM, *std::max_element(yNew[i].begin(), yNew[i].end()));


        }

        customPlot->legend->setVisible(false);
    }


    customPlot->xAxis->setRange(x[0], x[x.size() - 1]);
    // customPlot->yAxis->setRange(mm - 1, MM + 1);
    customPlot->yAxis->setRange(0.0, 1.2 * MM);

    customPlot->xAxis->setLabel("time (s)");
    customPlot->yAxis->setLabel(y_lab.c_str());

    customPlot->replot();

}
