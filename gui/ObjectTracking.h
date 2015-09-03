#ifndef OBJECTTRACKING_H
#define OBJECTTRACKING_H

#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <Camera.h>
#include <CameraView.h>
#include <fstream>
#include <numeric>
#include "Brisk.h"

using namespace std;
using namespace cv;
class ObjectTracking : public QObject
{
    Q_OBJECT
public:
    static ObjectTracking* getObjectTracking(std::string dbPath);
 //   ~ObjectTracking ();

    string db_path() const;
    void setDb_path(const string &db_path);

    std::vector<Camera *> *cameras() const;
    void setCameras(std::vector<Camera *> *cameras);

    std::vector<CameraView *> *camViews() const;
    void setCamViews(std::vector<CameraView *> &camViews);

    void connectTasks(Camera* camera);

    void setDb_bounding_box(vector<vector<Point2f> > Db_bounding_box);
    void setH_out(cv::Mat H_out);
    void setBest_img(int best_img);
    void setB_Box(vector<Point2f>);
    vector<vector<Point2f> > getDb_bounding_box() const;
    cv::Mat getH_out() const;
    int getBest_ing() const;
    vector<Point2f> getB_Box() const;
public slots:

    int trackObject(int camIdx,int operativeMode);
    void draw_bounding_box( vector<vector<Point2f > > Db_bounding_box, Mat& img_to_show, Mat& H, int best_img );
private:

    ObjectTracking(string path);
    static ObjectTracking* _instance;

    int _best_img;
    vector<vector<Point2f> > _Db_bounding_box;
    vector<Point2f> _B_Box;
    vector<Point2f> _B_Box_old;
    cv::Mat _H_out;

    string _db_path;
    std::vector<Camera*> *_cameras;
    std::vector<CameraView*> *_camViews;

    std::vector<Mat> _dbDesc;
    std::vector<std::vector<cv::KeyPoint> > _dbKpts;
    std::vector<string> _dbObj;
    std::vector<int> _dbClass;


signals:

    void objectTracked(string, int);

};

#endif // OBJECTTRACKING_H
