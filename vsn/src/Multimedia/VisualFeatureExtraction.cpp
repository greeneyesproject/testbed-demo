#include <Multimedia/VisualFeatureExtraction.h>

VisualFeatureExtraction::VisualFeatureExtraction(const DetectorType detType_,
		const DescriptorType descType_, const detParams& detParms,
		const descParams& descParms) {

	_initDetector(detType_, detParms);
	_initDescriptor(descType_, descParms);
}

/*
 * Create the detector using the specified parameters
 */
bool VisualFeatureExtraction::_initDetector(const DetectorType encoderType,
		const detParams& params) {

	switch (encoderType) {
	case DETECTORTYPE_BRISK: {
		BRISK_detParams* const briskParams = (BRISK_detParams*) &params;
		_detector = new BriskFeatureDetector(briskParams->threshold,
				briskParams->octaves);
		break;
	}
	case DETECTORTYPE_NONE:
		/* Silence is golden */
		break;
	default:
		if (_DEBUG)
			cout
					<< "VisualFeatureExtraction::init_detector: encoderType not implemented: "
					<< encoderType << endl;
		return false;
	}
	return true;

}

bool VisualFeatureExtraction::setDetectorParameters(const DetectorType detType_,
		const detParams& detParms_) {

	switch (detType_) {
	case DETECTORTYPE_BRISK: {
		BRISK_detParams* const briskParms = (BRISK_detParams*) &detParms_;
		((Ptr<BriskFeatureDetector> ) _detector)->threshold =
				briskParms->threshold;
		break;
	}
	case DETECTORTYPE_NONE:{
			/* Silence is golden */
		break;
	}
	default:
		if (_DEBUG)
			cerr
					<< "VisualFeatureExtraction::setDetectorParameters: DetectorType not implemented: "
					<< detType_ << endl;
		return false;
	}
	return true;

}

bool VisualFeatureExtraction::setDescriptorParameters(DescriptorType descrType_,
		const descParams& descParms_) {

	HIST_descParams* const histParams = (HIST_descParams*) &descParms_;
	switch (descrType_) {
	case DESCRIPTORTYPE_HIST:
		((Ptr<HistDescriptor> ) _descriptor)->setBinshift(
				histParams->binshift);
		((Ptr<HistDescriptor> ) _descriptor)->setMaxvalue(
				histParams->maxvalue);
		break;
	case DESCRIPTORTYPE_BRISK:
		//TODO Select brisk pattern length
		break;
	default:
		if (_DEBUG)
			cerr
					<< "VisualFeatureExtraction::setDetThreshold: DescriptorType not implemented: "
					<< descrType_ << endl;
		return false;
	}
	return true;
}

/*
 * Create the detector using the specified parameters
 */
bool VisualFeatureExtraction::_initDescriptor(const DescriptorType encoderType,
		const descParams& params) {

	switch (encoderType) {
	case DESCRIPTORTYPE_BRISK: {
		BRISK_descParams* const briskParms = (BRISK_descParams*) &params;
		_descriptor = new BriskDescriptorExtractor(briskParms->inputPairs,
				briskParms->rotationInvariance, briskParms->scaleInvariance,
				briskParms->patternScale);
		break;
	}
	case DESCRIPTORTYPE_HIST: {
		HIST_descParams* const histParms = (HIST_descParams*) &params;
		_descriptor = new HistDescriptor(histParms->binshift,
				histParms->maxvalue);
		break;
	}
	default:
		if (_DEBUG)
			cout
					<< "VisualFeatureExtraction::setDescriptor: encoderType not implemented: "
					<< encoderType << endl;
		return false;
	}

	return true;

}

bool VisualFeatureExtraction::extractKeypoints(const cv::Mat &imgGray,
		std::vector<cv::KeyPoint> &kpts) const {
	cout << "VisualFeatureExtraction::extractKeypoints" << endl;
	_detector->detect(imgGray, kpts);
	return true;
}

bool VisualFeatureExtraction::extractFeatures(const cv::Mat &image,
		std::vector<cv::KeyPoint> &kpts, cv::Mat &features) const {
	cout << "VisualFeatureExtraction::extractFeatures" << endl;
	_descriptor->compute(image, kpts, features);
	return 0;
}

/**
 *  Extract keypoints and extracts features
 */
bool VisualFeatureExtraction::extractKeypointsFeatures(const cv::Mat& image,
		std::vector<cv::KeyPoint> &kpts, cv::Mat &features) const {
	_detector->detect(image, kpts);
	_descriptor->compute(image, kpts, features);
	return true;

}

bool VisualFeatureExtraction::cutFeatures(cv::vector<cv::KeyPoint> &kpts,
		cv::Mat &features, unsigned short maxFeats) const {

	// store hash values in a map
	std::map<size_t, unsigned int> keyp_hashes;
	cv::vector<cv::KeyPoint>::iterator itKeyp;

	cv::Mat sorted_features;

	unsigned int iLine = 0;
	for (itKeyp = kpts.begin(); itKeyp < kpts.end(); itKeyp++, iLine++)
		keyp_hashes[(*itKeyp).hash()] = iLine;

	// sort values according to the response
	std::sort(kpts.begin(), kpts.end(), greater_than_response());
	// create a new descriptor matrix with the sorted keypoints
	sorted_features.create(0, features.cols, features.type());
	sorted_features.reserve(features.rows);
	for (itKeyp = kpts.begin(); itKeyp < kpts.end(); itKeyp++)
		sorted_features.push_back(features.row(keyp_hashes[(*itKeyp).hash()]));

	features = sorted_features.clone();

	// select the first maxFeats features
	if (kpts.size() > maxFeats) {
		vector<KeyPoint> cutKpts(kpts.begin(), kpts.begin() + maxFeats);
		kpts = cutKpts;

		features = features.rowRange(0, maxFeats).clone();
	}

	return 0;

}
