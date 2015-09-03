#ifndef PKLOT_H
#define PKLOT_H

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/features2d/features2d.hpp>

class PKLot{

public:
    PKLot(int id,cv::RotatedRect& rotatedRect,std::vector<cv::Point> rotatedContour);

    int getId() const { return _id; }
    cv::RotatedRect getRotatedRect() const { return _rotatedRect; }
    std::vector<cv::Point> getRotatedContour() const { return _rotatedContour; }
    void setOccupied(bool occ) { _occupied=occ; }

    void drawLines(cv::Mat& img);
    void fill(cv::Mat& img);

    int getKpt(cv::KeyPoint& kpt);

    static int parse(std::string filename,std::vector<PKLot>& parkingLots);

private:
    int _id;
    bool _occupied;
    cv::RotatedRect _rotatedRect;
    std::vector<cv::Point> _rotatedContour;

};

#endif // PKLOT_H
