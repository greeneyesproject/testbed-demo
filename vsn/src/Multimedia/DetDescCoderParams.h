/*
 * DetDescParams.h
 */

#ifndef SRC_MULTIMEDIA_DETDESCPARAMS_H_
#define SRC_MULTIMEDIA_DETDESCPARAMS_H_

#ifdef GUI
#include <Brisk.h>
#include <Global.h>
#else
#include <Multimedia/Briskola.h>
#endif
#include <opencv2/core/core.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <fstream>

using namespace cv;
using namespace std;

// abstract structure for detector parameters
struct detParams {

	detParams(){

	}
};

// abstract structure for descriptor parameters
struct descParams {

	descParams(){

	}
};

// abstract structure for coding parameters
struct codingParams {

	bool encode;

	codingParams(bool encode_=false) {
		encode = encode_;
	}

};
/*
 * Parameters for AGAST detector
 */
struct AGAST_detParams: detParams {

	int threshold;

	AGAST_detParams(int _threshold = 60) {
		threshold = _threshold;
	}

};

/*
 * Parameters for BRISK detector
 */
struct BRISK_detParams: detParams {

	int threshold;
	int octaves;

	BRISK_detParams(int _threshold = 60, int _octaves = 4) {
		threshold = _threshold;
		octaves = _octaves;
	}

};

/*
 * Parameters for BRISK descriptor
 */
struct BRISK_descParams: descParams {

	vector<int> inputPairs;
	bool rotationInvariance;
	bool scaleInvariance;
	float patternScale;

	BRISK_descParams(string rankingFile = "", bool _rotationInvariance = true,
			bool _scaleInvariance = true, float _patternScale = 1.0f) {
		rotationInvariance = _rotationInvariance;
		scaleInvariance = _scaleInvariance;
		patternScale = _patternScale;

		string filename(PATH_BRISK_FILES);
		if (rankingFile == "") { // use the default ranking files
			if ( BRISK_LENGTH_BITS == 512) {
				filename += string("/ranking_original_optimized512.bin");
			} else if (BRISK_LENGTH_BITS == 256) {
				filename += string("/ranking_original_optimized256.bin");
			}
		} else {
			filename = rankingFile;
		}

		inputPairs.clear();
		int aux_pairs[BRISK_LENGTH_BITS];
		ifstream fileRank;
		fileRank.open(filename.c_str(), ios::in | ios::binary);
		fileRank.read((char*) aux_pairs, sizeof(int) * BRISK_LENGTH_BITS);
		fileRank.close();
		for (int i = 0; i < BRISK_LENGTH_BITS; i++) {
			inputPairs.push_back(aux_pairs[i]);
		}

	}

};

/*
 * Parameters for BRIEF descriptor
 */
struct BRIEF_descParams: descParams {

	int bytes;

	BRIEF_descParams(int _bytes = 32) {
		bytes = _bytes;
	}

};

/*
 * Parameters for ORB descriptor
 */
struct ORB_descParams: descParams {

	int WTA_K;
	int patchSize;

	ORB_descParams(int _WTA_K = 2, int _patchSize = 31) {
		WTA_K = _WTA_K;
		patchSize = _patchSize;
	}

};

/*
 * Parameters for FREAK descriptor
 */
struct FREAK_descParams: descParams {

	bool orientationNormalized;
	bool scaleNormalized;
	float patternScale;
	int nOctaves;

	FREAK_descParams(bool _orientationNormalized = true, bool _scaleNormalized =
			true, float _patternScale = 22.0f, int _nOctaves = 4) {
		orientationNormalized = _orientationNormalized;
		scaleNormalized = _scaleNormalized;
		patternScale = _patternScale;
		nOctaves = _nOctaves;
	}

};

/*
 * Parameters for SURF (64floats) descriptor
 */
struct SURF_descParams: descParams {

	bool upright;

	SURF_descParams(bool _upright = false) {
		upright = _upright;
	}

};

/*
 * Parameters for ExtendedSURF (128floats) descriptor
 */
struct ExtendedSURF_descParams: descParams {

	bool upright;

	ExtendedSURF_descParams(bool _upright = false) {
		upright = _upright;
	}

};

/*
 * Parameters for SIFT descriptor
 */
struct SIFT_descParams: descParams {

	SIFT_descParams() {
	}

};

/*
 * Parameters for histogram descriptor
 */
struct HIST_descParams: descParams {

	uchar binshift; /* Bitshift to the left applied to pixel value */
	ushort maxvalue; /* Maximum pixel value */

	HIST_descParams(uchar binshift_ = 0, ushort maxvalue_ = 255) {
		binshift = binshift_;
		maxvalue = maxvalue_;
	}

};

struct HIST_codingParams: codingParams {

	uchar valshift;

	HIST_codingParams(bool encode_, uchar valshift_) :
			codingParams(encode_) {

		valshift = valshift_;
	}

};

#endif /* DETDESCPARAMS_H_ */
