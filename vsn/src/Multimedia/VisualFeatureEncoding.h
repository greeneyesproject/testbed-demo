/*
 * VisualFeatureEncoding.h
 */
#ifndef SRC_MULTIMEDIA_VISUALFEATUREENCODING_H_
#define SRC_MULTIMEDIA_VISUALFEATUREENCODING_H_

#include <iostream>
#include <stdio.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/features2d/features2d.hpp>

#include <coder/include/ac_extended.h>
#include <Multimedia/DetDescCoderParams.h>
#include <Multimedia/CodecParams.h>
#include <TestbedTypes.h>

class VisualFeatureEncoding {

public:

	static VisualFeatureEncoding* getInstance();

	bool encodeKeyPoints(const codingParams& parms, const std::vector<cv::KeyPoint>& kpts,
			std::vector<uchar>& bitstream, const cv::Size size,
			const bool encodeAngles) const;

	bool encodeDescriptors(const codingParams& parms, const DescriptorType descType,
			const cv::Mat& features, std::vector<uchar>& bitstream) const;

private:

	VisualFeatureEncoding();

	const static unsigned char _DEBUG = 2;

	static VisualFeatureEncoding* _instance;

	bool _encodeKeyPoints(const std::vector<cv::KeyPoint>& kpts,
			std::vector<uchar>& bitstream, const cv::Size size,
			const bool encodeAngles) const;
	bool _encodeBRISK(const cv::Mat& features, vector<uchar>& bitstream) const;
	bool _encodeHist(const cv::Mat&features, vector<uchar>& bitstream,
			const codingParams& parms) const;

	bool _dummyEncodeKeyPoints(const std::vector<cv::KeyPoint>& kpts,
			vector<uchar>& bitstream) const;
	bool _dummyEncodeBinaryDescriptors(const DescriptorType descType,
			const cv::Mat& features, vector<uchar>& bitstream) const;

	bool _dummyEncodeDescriptors(const cv::Mat& features, vector<uchar>& bitstream) const;

};

#endif /* VISUALFEATUREENCODING_H_ */
