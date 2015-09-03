#include "PKLotClassifier.h"
#include <boost/filesystem.hpp>
#include "Camera.h"

PKLotClassifier::PKLotClassifier(QObject *parent) : QObject(parent){
    for (uchar binShift = MIN_BINSHIFT; binShift <= MAX_BINSHIFT; ++binShift){
        stringstream ss;
        ss << PKLOT_SVM_PATH_PREFIX << (int)binShift << PKLOT_SVM_PATH_SUFFIX;
        if (boost::filesystem::exists((ss.str()))){
            _svm[binShift].load(ss.str().c_str());
        }else{
            cerr << "PKLotClassifier::PKLotClassifier: SVM not found: " << ss.str() << endl;
        }
    }
}

PKLotClassifier::~PKLotClassifier(){

}

int PKLotClassifier::predict(const cv::Mat &features, cv::Mat &predictions, const uchar binShift_){

    cv::Mat featuresFloat;
    features.convertTo(featuresFloat,CV_32FC1);

    cout << "PKLotClassifier::predict: features.rows: " << features.rows
            << " features.cols: " << features.cols << " features.type: "
            << features.type() << endl;

    cout << "PKLotClassifier::predict: features first bin: " << features.at<uint16_t>(0,0) << endl;
    cout << "PKLotClassifier::predict: featuresFloat first bin: " << featuresFloat.at<float>(0,0) << endl;

    for (int featIdx = 0; featIdx < featuresFloat.rows; ++featIdx){
        cv::Mat featRow = featuresFloat.row(featIdx);
        float featSum = cv::sum(featRow)[0];
        featRow /= featSum;
        featRow.copyTo(featuresFloat.row(featIdx));
    }

    cout << "PKLotClassifier::predict: featuresFloat first bin after normalization: " << featuresFloat.at<float>(0,0) << endl;

    predictions = cv::Mat(features.rows, 1, CV_32FC1,1);
    try{
    _svm[binShift_].predict(featuresFloat,predictions);
    }catch(exception &e){
        cout << "PKLotClassifier::predict: predict exception: " << e.what() << endl;
    }

    predictions = predictions > 0;
    predictions /= 255;

    return 1;
}
