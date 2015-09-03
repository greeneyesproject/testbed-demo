/*
 * VisualFeatureDecoding.h
 */
#ifndef SRC_MULTIMEDIA_VISUALFEATUREDECODING_H_
#define SRC_MULTIMEDIA_VISUALFEATUREDECODING_H_

#include <iostream>
#include <stdio.h>
#include <math.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/features2d/features2d.hpp>

#include <coder/include/ac_extended.h>

#include <Multimedia/CodecParams.h>
#include <TestbedTypes.h>

using namespace cv;
using namespace std;

class VisualFeatureDecoding {

public:

	static VisualFeatureDecoding* getInstance();

	bool decodeKeyPoints(const vector<uchar>& bitstream, vector<KeyPoint>& kpts,
			const Size, const bool decodeAngles) const;
	bool decodeBinaryDescriptors(const DescriptorType,
			vector<uchar>& bitstream, Mat &features, const int Nfeats) const;
	bool decodeNonBinaryDescriptors(const DescriptorType descType, const vector<uchar>& bitstream,
			Mat& features) const;

	bool dummy_decodeKeyPoints(const vector<uchar>& bitstream,
			vector<KeyPoint>& kpts) const;
	bool dummy_decodeBinaryDescriptors(const DescriptorType,
			const vector<uchar>& bitstream, Mat& features) const;
	bool dummy_decodeNonBinaryDescriptors(const DescriptorType,
			const vector<uchar>& bitstream, Mat& features) const;

	bool dummy_decodeDescriptors(const DescriptorType,
			const vector<uchar>& bitstream, Mat& features,
			const cv::Size featuresSize, const int featuresType) const;

private:

	static VisualFeatureDecoding* _instance;

	VisualFeatureDecoding();

	bool _decodeBRISK(vector<uchar>& bitstream, Mat &features,
			const int nFeats) const;
};

#endif /* VISUALFEATUREDECODING_H_ */
