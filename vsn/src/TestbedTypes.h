/*
 * TestbedTypes.h
 *
 *  Created on: 06/apr/2015
 *      Author: luca
 */

#ifndef SRC_TESTBEDTYPES_H_
#define SRC_TESTBEDTYPES_H_

#include<string>
#include<opencv2/core/core.hpp>

enum NodeType {
	NODETYPE_UNDEF,
	NODETYPE_GUI,
	NODETYPE_SINK,
	NODETYPE_CAMERA,
	NODETYPE_COOPERATOR,
	NODETYPE_RELAY
};

enum MessageType {
	MESSAGETYPE_NONE,
	MESSAGETYPE_START_CTA,
	MESSAGETYPE_START_ATC,
	MESSAGETYPE_DATA_CTA,
	MESSAGETYPE_DATA_ATC,
	MESSAGETYPE_STOP,
	MESSAGETYPE_COOP_INFO,
	MESSAGETYPE_COOP_INFO_REQ,
	MESSAGETYPE_NODE_INFO,
	MESSAGETYPE_ACK
};

enum DetectorType {
	DETECTORTYPE_FAST = 0,
	DETECTORTYPE_START = 1,
	DETECTORTYPE_SIFT = 2,
	DETECTORTYPE_SURF = 3,
	DETECTORTYPE_ORB = 4,
	DETECTORTYPE_BRISK = 5,
	DETECTORTYPE_MSER = 6,
	DETECTORTYPE_NONE = -1
};

enum DescriptorType {
	DESCRIPTORTYPE_SIFT = 0,
	DESCRIPTORTYPE_SURF = 1,
	DESCRIPTORTYPE_BRIEF = 2,
	DESCRIPTORTYPE_BRISK = 3,
	DESCRIPTORTYPE_ORB = 4,
	DESCRIPTORTYPE_FREAK = 5,
	DESCRIPTORTYPE_HIST = 6,
	DESCRIPTORTYPE_NONE = -1
};

enum OperativeMode {
	OPERATIVEMODE_OBJECT,
	OPERATIVEMODE_PKLOT
};

enum ImageSource {
	IMAGESOURCE_LIVE,
	IMAGESOURCE_REC
};

class CameraParameters {
public:
	CameraParameters(int cameraId_, std::string objPath_,
			std::string pklotPath_,std::string fallbackPath_, cv::Size videoSize, bool cameraFlip) {
		camId = cameraId_;
		objPath = objPath_;
		pklotPath = pklotPath_;
		fallbackPath = fallbackPath_;
		size = videoSize;
		flip = cameraFlip;
	}
	int camId;
	std::string objPath;
	std::string pklotPath;
	std::string fallbackPath;
	cv::Size size;
	bool flip;
};

#endif /* SRC_TESTBEDTYPES_H_ */
