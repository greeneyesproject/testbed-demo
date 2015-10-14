#ifndef SRC_MULTIMEDIA_IMAGEACQUISITION_H_
#define SRC_MULTIMEDIA_IMAGEACQUISITION_H_

#include <iostream>
#include <stdio.h>
#include <fstream>

#include <unistd.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <TestbedTypes.h>

class ImageAcquisition {

private:

	ImageAcquisition(const CameraParameters& cameraParams_);

	static ImageAcquisition* _instance;

	cv::VideoCapture _videoCaptureLive;
	cv::VideoCapture _videoCaptureObj;
	cv::VideoCapture _videoCapturePklot;

	void _grabThread();

	unsigned short _height;
	unsigned short _width;

	bool _cameraFlip;

	int _camId;
	std::string _fallbackPath;
	std::string _objPath;
	std::string _pklotPath;

	bool _takeFrame(const std::string & path_, cv::VideoCapture & cap_,
			cv::Mat& picture);
	bool _takeFrame(const int camId, cv::VideoCapture &cap_, cv::Mat& picture);

	bool _checkFrame(cv::Mat& picture);

	bool _restartCap(cv::VideoCapture &, const std::string &, cv::Mat &);
	bool _restartCap(cv::VideoCapture &, const int, cv::Mat &);

	static const uchar _DEBUG = 1;

public:

	static ImageAcquisition* getInstance(const CameraParameters& cameraParams_);

	bool takeLiveFrame(cv::Mat& picture);
	bool takePklotFrame(cv::Mat& picture);
	bool takeObjFrame(cv::Mat& picture);

};

#endif /*SRC_MULTIMEDIA_IMAGEACQUISITION_H_*/
