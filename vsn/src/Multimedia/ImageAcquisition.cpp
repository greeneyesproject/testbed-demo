#include <boost/thread.hpp>
#include <boost/filesystem.hpp>
#include <Multimedia/ImageAcquisition.h>

using namespace std;
using namespace cv;

ImageAcquisition* ImageAcquisition::_instance = NULL;

ImageAcquisition* ImageAcquisition::getInstance(int deviceID, string filePath,
		Size size, bool cameraFlip) {
	if (_instance == NULL) {
		_instance = new ImageAcquisition(deviceID, filePath, size, cameraFlip);
	}
	return _instance;
}

ImageAcquisition::ImageAcquisition(int ID, string filePath, Size size,
		bool cameraFlip) {
	_cameraFlip = cameraFlip;
	_filePath = filePath;
	_cameraID = ID;
	if (filePath != "") {
		_videoCaptureFile = VideoCapture(_filePath);
	}
	_videoCaptureCamera = VideoCapture(_cameraID);
	if (_videoCaptureCamera.isOpened()) {  // check if we succeeded

		if (_DEBUG)
			cout
					<< "ImageAcquisition::ImageAcquisition: set camera frame format"
					<< endl;
		_videoCaptureCamera.set(CV_CAP_PROP_FOURCC,
				CV_FOURCC('M', 'J', 'P', 'G'));

		if (_DEBUG)
			cout
					<< "ImageAcquisition::ImageAcquisition: set camera frame per second"
					<< endl;
		_videoCaptureCamera.set(CV_CAP_PROP_FPS, 5);

		if (_DEBUG)
			cout << "ImageAcquisition::ImageAcquisition: set camera frame width"
					<< endl;
		_videoCaptureCamera.set(CV_CAP_PROP_FRAME_WIDTH, size.width);

		if (_DEBUG)
			cout
					<< "ImageAcquisition::ImageAcquisition: set camera frame height"
					<< endl;
		_videoCaptureCamera.set(CV_CAP_PROP_FRAME_HEIGHT, size.height);

		_height = size.height;
		_width = size.width;

		_lastFrame = cv::Mat(_height, _width, CV_8UC3, cv::Scalar::all(0));

		boost::thread grabThread(&ImageAcquisition::_grabThread, this);
	} else {
		cerr << "ImageAcquisition::ImageAcquisition: Error opening video device"
				<< endl;
		throw "ImageAcquisition::ImageAcquisition: Error opening video device";
	}
}

bool ImageAcquisition::takeCameraPicture(Mat &frame) {
	try {

		/*for (uchar grabIdx = 0; grabIdx < _grabRep; ++grabIdx) {
		 _videoCaptureCamera.grab();
		 }*/

		if (_DEBUG)
			cout << "ImageAcquisition::takeCameraPicture: retrieve frame"
					<< endl;
		//boost::mutex::scoped_lock lock(_videoCaptureCameraMutex);
		_videoCaptureCamera.retrieve(frame);
		//lock.unlock();

		/*if (_DEBUG)
		 cout << "ImageAcquisition::takeCameraPicture: check dimensions"
		 << endl;
		 if (_lastFrame.rows != frame.rows || _lastFrame.cols != frame.cols
		 || _lastFrame.channels() != frame.channels()) {
		 _rebootCamera(frame);
		 } else {

		 if (_DEBUG)
		 cout
		 << "ImageAcquisition::takeCameraPicture: calculate difference"
		 << endl;
		 cv::Mat diff;
		 cv::absdiff(frame, _lastFrame, diff);

		 if (_DEBUG)
		 cout << "ImageAcquisition::takeCameraPicture: split channels"
		 << endl;
		 cv::Mat channels[3];
		 cv::split(diff, channels);

		 if (_DEBUG)
		 cout << "ImageAcquisition::takeCameraPicture: count non zeros "
		 << endl;

		 if ((cv::countNonZero(channels[0]) + cv::countNonZero(channels[1])
		 + cv::countNonZero(channels[2])) == 0) {
		 _rebootCamera(frame);
		 } else {
		 if (_DEBUG)
		 cout
		 << "ImageAcquisition::takeCameraPicture: new frame correctly retrieved "
		 << endl;
		 _lastFrame = frame.clone();
		 }
		 }*/

		//_videoCaptureCamera.open(_cameraID);
		//_videoCaptureCamera.read(frame);
		//_videoCaptureCamera.release();
		/*

		 boost::mutex::scoped_lock lock(_frameMutex);
		 frame = _lastFrame.clone();
		 lock.unlock();
		 */

		//_videoCaptureCamera.retrieve(frame);
		if (frame.empty()) {
			/* Create a dummy image */
			frame = cv::Mat::zeros(Size(_width, _height), CV_8UC3);
		} else {

			/* Resize the image */
			if (_DEBUG)
				cout
						<< "ImageAcquisition::takeCameraPicture: acquired image size: "
						<< frame.size() << endl;
			if (_width && _height && frame.size() != Size(_width, _height)) {
				resize(frame, frame, Size(_width, _height));
			}
			if (_cameraFlip) {
				flip(frame, frame, 1);
			}
		}

		return true;
	} catch (exception &e) {
		cerr << "ImageAcquisition::takeCameraPicture: " << e.what() << endl;
		return false;
	}
}

/* UNUSED */
void ImageAcquisition::_rebootCamera(cv::Mat frame) {
	if (_DEBUG)
		cout << "ImageAcquisition::_rebootCamera" << endl;
	boost::mutex::scoped_lock lock(_videoCaptureCameraMutex);
	if (_DEBUG)
		cout << "ImageAcquisition::_rebootCamera: release" << endl;
	_videoCaptureCamera.release();
	if (_DEBUG)
		cout << "ImageAcquisition::_rebootCamera: create new VideoCapture"
				<< endl;
	_videoCaptureCamera = VideoCapture(_cameraID);
	if (_DEBUG)
		cout << "ImageAcquisition::_rebootCamera: read" << endl;
	_videoCaptureCamera.read(frame);
	lock.unlock();
}

void ImageAcquisition::_grabThread() {
	try {
		while (1) {
			/* Grab frames periodically */

			/*
			 _videoCaptureCamera.grab();
			 boost::mutex::scoped_lock lock(_frameMutex);
			 _videoCaptureCamera.retrieve(_lastFrame);
			 lock.unlock();
			 */

			float time = cv::getTickCount();
			_videoCaptureCamera.grab();
			if (_DEBUG > 1)
				cout << "ImageAcquisition::_grabThread: grabTime: "
						<< (cv::getTickCount() - time) / cv::getTickFrequency()
						<< endl;

			usleep(GRAB_DELAY);
		}
	} catch (exception &e) {
		cerr << "ImageAcquisition: _grabThread: " << e.what() << endl;
	}

}

bool ImageAcquisition::takeFileFrame(Mat& frame) {
	try {

		if (!_videoCaptureFile.isOpened()) {
			_videoCaptureFile.open(_filePath);
			frame = cv::Mat::zeros(Size(_width, _height), CV_8UC3);
			return false;
		}

		bool ret = _videoCaptureFile.read(frame);
		if (!ret) {
			_videoCaptureFile.release();
			_videoCaptureFile.open(_filePath);
			//_videoCaptureFile.set(CV_CAP_PROP_POS_FRAMES, 0);
			_videoCaptureFile.read(frame);
		}

		if (frame.empty()) {
			/* Create a dummy image */
			frame = cv::Mat::zeros(Size(_width, _height), CV_8UC3);
		} else {
			/* Resize the image */
			if (_width && _height && frame.size() != Size(_width, _height)) {
				resize(frame, frame, Size(_width, _height));
			}
		}

		return true;
	} catch (exception &e) {
		cerr << "ImageAcquisition::takeFileFrame: " << e.what() << endl;
		return false;
	}
}
