#ifndef SRC_MULTIMEDIA_OBJRECOGNITION_H_
#define SRC_MULTIMEDIA_OBJRECOGNITION_H_

#include <Multimedia/Briskola.h>
#include <iostream>
#include <stdio.h>
#include <fstream>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/imgproc/imgproc.hpp>
/*#include <opencv2/nonfree/nonfree.hpp>*/
#include <opencv2/legacy/legacy.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <Multimedia/VisualFeatureExtraction.h>

using namespace cv;
using namespace std;

// relevant (default) parameters
#define DEFAULT_RANSAC_MIN_MATCHES 8   // number minimum of matches for RANSAC computation
#define DEFAULT_NND_BIN_THR 90         // distance threshold for nearest neighbor test (for binary descriptors)
#define DEFAULT_NNDR_RATIO 0.7f        // ratio test threshold
#define DEFAULT_RANSAC_THRESHOLD 10.0f // maximum allowed reprojection error to treat a point pair as an inlier

class objRecognition {
	// feature type: 1 -> binary descriptor, 0 -> non binary descriptor	
	int feature_type;
	// Database: keypoints and descriptors
	std::vector<std::vector<KeyPoint> > m_DbKeyp;
	std::vector<cv::Mat> m_DbDescr;
	// number of images in the database
	unsigned int m_nDBImages;
	// bitrate per descriptor
	int m_bitrate;

	VisualFeatureExtraction* m_pFeatExtract;
	Ptr<DescriptorMatcher> m_pMatcher;

	void sortDescriptor(std::vector<KeyPoint>& _keyp, Mat& _descr);

public:

	// keypoints and descriptors of the query
	std::vector<KeyPoint> q_keyp;
	Mat q_descr;

	objRecognition(VisualFeatureExtraction* featExtract, bool isBinary,
			const int _nDBImages);

	void createDescriptor(const std::string& qImgPath = "",
			const std::string& dbImgPath = "");
	void cutDBDescriptors(const int _nDescriptors);

	//void dCreateAddToObjDB(std::vector<string> &imgs);
	void dCreateAddToDB(const std::string& dbImgPath);

	std::vector<unsigned int> rankedQueryDB(std::vector<KeyPoint> &kpts,
			Mat &features, std::vector<vector<float> > &distances,
			bool do_bin_NNDR = true, int NND_bin_thr = DEFAULT_NND_BIN_THR,
			float NNDR_ratio = DEFAULT_NNDR_RATIO, bool do_ransac = true,
			int ransac_min_matches = DEFAULT_RANSAC_MIN_MATCHES,
			float ransac_threshold = DEFAULT_RANSAC_THRESHOLD);

};

#endif
