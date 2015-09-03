#ifndef SRC_MULTIMEDIA_IMAGEACQUISITION_H_
#define SRC_MULTIMEDIA_IMAGEACQUISITION_H_

#include <iostream>
#include <stdio.h>
#include <fstream>

#include <unistd.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#define GRAB_DELAY 0 //us (5fps)

class ImageAcquisition {

private:

	ImageAcquisition(const int deviceID = 0, const std::string filePath = "",
			const cv::Size size = cv::Size(640, 480), bool cameraFlip = false);

	void _rebootCamera(cv::Mat frame);

	static ImageAcquisition* _instance;

	cv::VideoCapture _videoCaptureCamera;
	cv::VideoCapture _videoCaptureFile;

	cv::Mat _lastFrame;
	mutable boost::mutex _videoCaptureCameraMutex;

	void _grabThread();

	unsigned short _height;
	unsigned short _width;

	const static uint8_t _grabRep = 7;

	bool _cameraFlip;

	std::string _filePath;

	int _cameraID;

	static const uchar _DEBUG = 1;

public:

	static ImageAcquisition* getInstance(const int deviceID = 0,
			const std::string filePath = "",
			const cv::Size size = cv::Size(640, 480), bool cameraFlip = false);

	bool takeCameraPicture(cv::Mat &picture);

	bool takeFileFrame(cv::Mat& picture);

};

#endif /*SRC_MULTIMEDIA_IMAGEACQUISITION_H_*/
