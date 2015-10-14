#include <boost/thread.hpp>
#include <boost/filesystem.hpp>
#include <Multimedia/ImageAcquisition.h>

using namespace std;
using namespace cv;

ImageAcquisition* ImageAcquisition::_instance = NULL;

ImageAcquisition* ImageAcquisition::getInstance(
		const CameraParameters& cameraParams_) {
	if (_instance == NULL) {
		_instance = new ImageAcquisition(cameraParams_);
	}
	return _instance;
}

ImageAcquisition::ImageAcquisition(const CameraParameters& cameraParams_) {

	_camId = cameraParams_.camId;
	_objPath = cameraParams_.objPath;
	_pklotPath = cameraParams_.pklotPath;
	_fallbackPath = cameraParams_.fallbackPath;

	_height = cameraParams_.size.height;
	_width = cameraParams_.size.width;

	_cameraFlip = cameraParams_.flip;

	if (_DEBUG) {
		cout << "ImageAcquisition::ImageAcquisition _camId: " << _camId << endl;
		cout << "ImageAcquisition::ImageAcquisition _objPath: " << _objPath
				<< endl;
		cout << "ImageAcquisition::ImageAcquisition _pklotPath: " << _pklotPath
				<< endl;
	}

	_videoCaptureObj = VideoCapture(_objPath);
	if (!_videoCaptureObj.isOpened())
		_videoCaptureObj = VideoCapture(_fallbackPath);

	_videoCapturePklot = VideoCapture(_pklotPath);
	if (!_videoCapturePklot.isOpened())
		_videoCapturePklot = VideoCapture(_fallbackPath);

	_videoCaptureLive = VideoCapture(_camId);
	if (_videoCaptureLive.isOpened()) {  // check if we succeeded

		if (_DEBUG)
			cout
					<< "ImageAcquisition::ImageAcquisition: set camera frame format"
					<< endl;
		_videoCaptureLive.set(CV_CAP_PROP_FOURCC,
				CV_FOURCC('M', 'J', 'P', 'G'));

		if (_DEBUG)
			cout
					<< "ImageAcquisition::ImageAcquisition: set camera frame per second"
					<< endl;
		_videoCaptureLive.set(CV_CAP_PROP_FPS, 5);

		if (_DEBUG)
			cout << "ImageAcquisition::ImageAcquisition: set camera frame width"
					<< endl;
		_videoCaptureLive.set(CV_CAP_PROP_FRAME_WIDTH, _width);

		if (_DEBUG)
			cout
					<< "ImageAcquisition::ImageAcquisition: set camera frame height"
					<< endl;
		_videoCaptureLive.set(CV_CAP_PROP_FRAME_HEIGHT, _height);

		boost::thread grabThread(&ImageAcquisition::_grabThread, this);
	} else {
		cout << "ImageAcquisition::ImageAcquisition: unable to open _livePath"
				<< endl;
		_videoCaptureLive = VideoCapture(_fallbackPath);
	}
}

void ImageAcquisition::_grabThread() {
	try {
		while (1) {
			/* Grab frames periodically */

			float time = cv::getTickCount();
			_videoCaptureLive.grab();
			if (_DEBUG > 1)
				cout << "ImageAcquisition::_grabThread: grabTime: "
						<< (cv::getTickCount() - time) / cv::getTickFrequency()
						<< endl;
		}
	} catch (exception &e) {
		cerr << "ImageAcquisition: _grabThread: " << e.what() << endl;
	}

}

bool ImageAcquisition::_takeFrame(const int camId, cv::VideoCapture &cap_, cv::Mat& picture) {

	int ret;
	try {
		ret = cap_.retrieve(picture);
		if (!ret)
			ret = _restartCap(cap_, camId, picture);
		if (_DEBUG)
			cout << "ImageAcquisition::_takeFrame: retrieve: " << ret << endl;
		_checkFrame(picture);
		return true;
	} catch (exception &e) {
		cerr << "ImageAcquisition::_takeFrame: " << e.what() << endl;
		return false;
	}
}

bool ImageAcquisition::_takeFrame(const std::string & path_, cv::VideoCapture & cap_,
		cv::Mat& picture) {

	int ret;
	try {
		ret = cap_.read(picture);
		if (!ret)
			ret = _restartCap(cap_, path_, picture);
		if (_DEBUG)
			cout << "ImageAcquisition::_takeFrame: retrieve: " << ret << endl;
		_checkFrame(picture);
		return true;
	} catch (exception &e) {
		cerr << "ImageAcquisition::_takeFrame: " << e.what() << endl;
		return false;
	}
}

bool ImageAcquisition::_checkFrame(cv::Mat & picture) {
	if (picture.empty()) {
		if (_DEBUG)
			cout << "ImageAcquisition::_checkFrame: creating a dummy image: "
					<< endl;
		/* Create a dummy image */
		picture = cv::Mat::zeros(Size(_width, _height), CV_8UC3);
		return false;
	} else {

		/* Resize the image */
		if (_DEBUG)
			cout << "ImageAcquisition::_checkFrame: acquired image size: "
					<< picture.size() << endl;
		if (_width && _height && picture.size() != Size(_width, _height)) {
			resize(picture, picture, Size(_width, _height));
		}
		if (_cameraFlip) {
			flip(picture, picture, 1);
		}
		return true;
	}
}

bool ImageAcquisition::_restartCap(cv::VideoCapture & cap,
		const std::string & path, cv::Mat & picture) {
	if (_DEBUG)
		cout << "ImageAcquisition::_restartCap: release and open " << endl;
	cap.release();
	cap.open(path);
	return cap.read(picture);
}
bool ImageAcquisition::_restartCap(cv::VideoCapture & cap, const int camId,
		cv::Mat & picture) {
	if (_DEBUG)
		cout << "ImageAcquisition::_restartCap: release and open " << endl;
	cap.release();
	cap.open(camId);
	return cap.read(picture);
}

bool ImageAcquisition::takeLiveFrame(Mat &frame) {
	return _takeFrame(_camId, _videoCaptureLive, frame);
}

bool ImageAcquisition::takeObjFrame(Mat &frame) {
	return _takeFrame(_objPath, _videoCaptureObj, frame);
}

bool ImageAcquisition::takePklotFrame(Mat &frame) {
	return _takeFrame(_pklotPath, _videoCapturePklot, frame);
}
