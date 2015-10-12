/*
 * NodeProcessingSystem.h
 *
 *  Created on: 27/feb/2015
 *      Author: luca
 */

#ifndef SRC_NODEPROCESSINGSYSTEM_H_
#define SRC_NODEPROCESSINGSYSTEM_H_

#include <Multimedia/VisualFeatureEncoding.h>
#include <Multimedia/VisualFeatureExtraction.h>
#include <Multimedia/VisualFeatureDecoding.h>
#include <Network/OffloadingManager.h>
#include <TestbedTypes.h>
#include <Network/Queue.h>
#include <boost/thread.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <gpio/BlackGPIO.h>

class Message;
class ImageAcquisition;

struct DatcParams {
	unsigned short maxNumFeatures;
	float detectionThreshold;
};

class CameraParameters {
public:
	CameraParameters(unsigned char id, std::string file, cv::Size videoSize,
			bool cameraFlip) {
		camId = id;
		filePath = file;
		size = videoSize;
		flip = cameraFlip;
	}
	unsigned char camId;
	std::string filePath;
	cv::Size size;
	bool flip;
};

class NodeNetworkSystem;

class NodeProcessingSystem {

private:

	static const unsigned char _DEBUG = 2;

	/**
	 * Queue deadlock prevention period [ms]
	 */
	static const unsigned int _waitPeriod = 1000;

	/**
	 * Processing queue, used for time consuming operations.
	 * Only one operation can be queued at time
	 */
	Queue<Message> _processingQueue;

	/**
	 * Service queue, used for fast operations
	 */
	Queue<Message> _serviceQueue;

	boost::thread* _processingThread;
	boost::thread* _serviceThread;

	NodeType _nodeType;

	NodeNetworkSystem* _nodeNetworkSystem;

	OffloadingManager* _offloadingManager;

	VisualFeatureDecoding* _decoder;

	//TODO to be modified as vector when multiple cameras used
	DatcParams _datcParams;

	bool _oneShot;

	BlackLib::BlackGPIO** _gpios;

	NodeProcessingSystem(NodeNetworkSystem*, const NodeType,
			const CameraParameters& cameraParameters, const bool oneShot,
			BlackLib::BlackGPIO** gpios);

	void _processingThreadHandler();
	void _serviceThreadHandler();

	void _cameraProcessing(Message*);
	void _cooperatorProcessing(Message*);

	void _sinkService(Message*);
	void _cameraService(Message*);
	void _cooperatorService(Message*);

	void _initSink();
	void _initCamera(const CameraParameters&);
	void _initCooperator();

	/* Processing attributes and methods */
	unsigned char _frameId;

	ImageAcquisition* _imageAcquisition;

	float _acquireImage(cv::Mat& image, const cv::Size& topLeft,
			const cv::Size& bottomRight, bool fromFile);

	cv::Size _imageSlice(const cv::Mat& src, cv::Mat& dst,
			unsigned char sliceIdx, unsigned char numSlices) const;

	float _jpegEncode(const cv::Mat& image, Bitstream& bitstream,
			unsigned char qf) const;

	VisualFeatureExtraction* _briskExtractor;
	float _briskExtractKeypoints(const cv::Mat& image,
			std::vector<cv::KeyPoint>& keypoints,
			float detectorThreshold) const;

	float _briskExtractFeatures(const cv::Mat& image,
			std::vector<cv::KeyPoint>& keypoints, cv::Mat& features,
			unsigned short maxFeatures) const;

	VisualFeatureExtraction* _histExtractor;
	float _histExtractFeatures(const cv::Mat& image,
			std::vector<cv::KeyPoint>& keypoints, cv::Mat& features,
			const uchar binshift) const;

	VisualFeatureEncoding* _encoder;
	float _encodeKeypoints(const std::vector<cv::KeyPoint>& keypoints,
			Bitstream& bitstream, const cv::Size imageSize, bool encodeAngles,
			bool encode) const;

	float _encodeFeatures(const cv::Mat& features, Bitstream& bitstream,
			const DescriptorType encoderType, const codingParams& parms) const;

	void _sendATC(const std::vector<cv::KeyPoint>& keypoints,
			const cv::Mat& features, NetworkNode* dst, const LinkType linkType,
			const cv::Size imageSize, const unsigned short featuresPerBlock,
			const bool keypointsCoding, const float detectionTime,
			const float descriptionTime, const DescriptorType,
			const codingParams&, const OperativeMode);

public:

	static NodeProcessingSystem* _instance;

	static NodeProcessingSystem* getInstance(NodeNetworkSystem*, NodeType const,
			const CameraParameters& cameraParameters = CameraParameters(0, "",
					cv::Size(640, 480), false), const bool oneShot = false,
			BlackLib::BlackGPIO** gpios = NULL);

	NodeType getNodeType() const {
		return _nodeType;
	}

	void queueMessage(Message*);

};

#endif /* SRC_NODEPROCESSINGSYSTEM_H_ */
