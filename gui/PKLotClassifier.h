#ifndef PKLOTCLASSIFIER_H
#define PKLOTCLASSIFIER_H

#include <QObject>
#include <opencv2/ml/ml.hpp>
#include <Global.h>

class PKLotClassifier : public QObject
{
    Q_OBJECT
public:
    explicit PKLotClassifier(QObject *parent = 0);
    ~PKLotClassifier();

    int predict(const cv::Mat& features, cv::Mat& predictions, const uchar binShift_);

private:

    cv::SVM _svm[MAX_BINSHIFT-MIN_BINSHIFT+1];


signals:

public slots:

};

#endif // PKLOTCLASSIFIER_H
