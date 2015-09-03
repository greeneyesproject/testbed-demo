#ifndef SRC_MULTIMEDIA_VISUALFEATUREEXTRACTION_H_
#define SRC_MULTIMEDIA_VISUALFEATUREEXTRACTION_H_

#include <TestbedTypes.h>
#include <iostream>
#include <stdio.h>
#include <fstream>

#ifdef GUI
#include <Brisk.h>
#else
#include <Multimedia/Briskola.h>
#endif

#include <Multimedia/HistDescriptor.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <Multimedia/DetDescCoderParams.h>

// sorting predicate
struct greater_than_response {
	inline bool operator()(const cv::KeyPoint & kp1, const cv::KeyPoint & kp2) {
		return (kp1.response > kp2.response);
	}
};

class VisualFeatureExtraction {

private:

	const static unsigned char _DEBUG = 1;

	cv::Ptr<cv::FeatureDetector> _detector;
	cv::Ptr<cv::DescriptorExtractor> _descriptor;

	bool _initDetector(const DetectorType, const detParams& params);
	bool _initDescriptor(const DescriptorType, const descParams& params);

public:

	VisualFeatureExtraction(const DetectorType, const DescriptorType, const detParams&, const descParams&);


	bool setDetectorParameters(const DetectorType, const detParams&);
	bool setDescriptorParameters(const DescriptorType, const descParams&);

	bool extractKeypoints(const cv::Mat &imgGray,
			cv::vector<cv::KeyPoint> &kpts) const ;

	bool extractFeatures(const cv::Mat &img, std::vector<KeyPoint> &kpts,
			cv::Mat &features) const;

	bool extractKeypointsFeatures(const cv::Mat& image,
			std::vector<cv::KeyPoint> &kpts, cv::Mat &features) const;

	bool cutFeatures(cv::vector<cv::KeyPoint> &kpts, cv::Mat &features,
			const unsigned short maxFeats) const;

};

#endif
