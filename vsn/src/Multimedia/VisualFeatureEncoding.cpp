/*
 * VisualFeatureEncoding.cpp
 */
#include <Multimedia/VisualFeatureEncoding.h>

using namespace std;
using namespace cv;

VisualFeatureEncoding* VisualFeatureEncoding::_instance = NULL;

VisualFeatureEncoding* VisualFeatureEncoding::getInstance() {
	if (_instance == NULL) {
		_instance = new VisualFeatureEncoding();
	}
	return _instance;
}

VisualFeatureEncoding::VisualFeatureEncoding() {
	BRISK_pModel::get_instance();
}

bool VisualFeatureEncoding::encodeKeyPoints(const codingParams& parms,
		const std::vector<cv::KeyPoint>& kpts, std::vector<uchar>& bitstream,
		const cv::Size size, const bool encodeAngles) const {
	if (parms.encode) {
		return _encodeKeyPoints(kpts, bitstream, size, encodeAngles);
	} else {
		return _dummyEncodeKeyPoints(kpts, bitstream);
	}
}

bool VisualFeatureEncoding::encodeDescriptors(const codingParams& parms,
		const DescriptorType descType, const cv::Mat& features,
		std::vector<uchar>& bitstream) const {
	if (parms.encode) {
		switch (descType) {
		case DESCRIPTORTYPE_BRISK:
			_encodeBRISK(features, bitstream);
			break;
		case DESCRIPTORTYPE_HIST:
			_encodeHist(features, bitstream, parms);
			break;
		default:
			if (_DEBUG)
				cerr
						<< "VisualFeatureEncoding::encodeDescriptors: DescriptorType not handled: "
						<< descType << endl;
		}
	} else {
		switch (descType) {
		case DESCRIPTORTYPE_BRISK:
			_dummyEncodeBinaryDescriptors(DESCRIPTORTYPE_BRISK, features,
					bitstream);
			break;
		case DESCRIPTORTYPE_HIST:
			_dummyEncodeDescriptors(features, bitstream);
			break;
		default:
			if (_DEBUG)
				cerr
						<< "VisualFeatureEncoding::encodeDescriptors: DescriptorType not handled: "
						<< descType << endl;
		}

	}
	return true;
}

// Encoder for the keypoints
bool VisualFeatureEncoding::_encodeKeyPoints(const vector<KeyPoint>& kpts,
		vector<uchar>& bitstream, const cv::Size size,
		const bool encodeAngles) const {

	// inputs:  kpts      -> vector of keypoints
	// outputs: bitstream <- the encoded bitstream

	bitstream.clear(); // clear the vector

	int nbits_x = ceil(log2(size.width)) + 2;
	int nbits_y = ceil(log2(size.height)) + 2;
	int nbits_s = 10; // bits for encoding the size
	int nbits_a = 0;  // bits for encoding the angle
	if (encodeAngles == true) {
		nbits_a = 9;
	}
	int nbits_kpt = nbits_x + nbits_y + nbits_s + nbits_a;

	uchar buffer = 0;

	// quantized values
	long qx, qy, qs;
	int qa = 0;

	int bit_idx = 7;
	int cur_bit = 0;

	for (int n = 0; n < (int) kpts.size(); n++) {

		// quantize the keypoints (precision 1/4 pixel)
		qx = round(kpts[n].pt.x * 4);
		qy = round(kpts[n].pt.y * 4);
		qs = round(kpts[n].size * 4);

		if (encodeAngles == true) {
			qa = round(kpts[n].angle);
			qa = qa % 360;
			if (qa > 180) {
				qa -= 180;
			}
		}
		// process the single bits
		for (int i = 0; i < nbits_kpt; i++) {

			if (i < nbits_x) {
				cur_bit = (qx >> (nbits_x - i - 1)) & 0x0001;
			} else if (i < nbits_x + nbits_y) {
				cur_bit = (qy >> (nbits_x + nbits_y - i - 1)) & 0x0001;
			} else if (i < nbits_x + nbits_y + nbits_s) {
				cur_bit = (qs >> (nbits_x + nbits_y + nbits_s - i - 1))
						& 0x0001;
			} else {
				if (encodeAngles == true) {
					if (i == nbits_x + nbits_y + nbits_s) {
						cur_bit = qa < 0;
					} else {
						if (qa < 0)
							qa = -qa;
						cur_bit = (qa
								>> (nbits_x + nbits_y + nbits_s + nbits_a - i
										- 1)) & 0x0001;
					}
				}
			}

			// update the 8-bits buffer
			buffer |= cur_bit << bit_idx;
			bit_idx--;

			// when the buffer is full, append it to the vector; then reset the buffer
			if (bit_idx < 0) {
				bit_idx = 7;
				bitstream.push_back(buffer);
				buffer = 0;
			}

		}

	}

	// append the remaining bits, if any
	if (bit_idx != 7) {
		bitstream.push_back(buffer);
	}

	return true;

}

// Dummy Encoder for the keypoints
bool VisualFeatureEncoding::_dummyEncodeKeyPoints(const vector<KeyPoint>& kpts,
		vector<uchar>& bitstream) const {

	// Define a contiguous representation for keypoints
	typedef struct {
		float x;
		float y;
		float size;
		float angle;
		float response;
	} my_kpt;

	// Array of keypoints
	my_kpt stream_kpts[kpts.size()];

	// Fill the array with needed information
	for (unsigned int i = 0; i < kpts.size(); i++) {
		stream_kpts[i].x = kpts[i].pt.x;
		stream_kpts[i].y = kpts[i].pt.y;
		stream_kpts[i].size = kpts[i].size;
		stream_kpts[i].angle = kpts[i].angle;
		stream_kpts[i].response = kpts[i].response;
	}

	// Read the array as a vector of char
	uchar *char_stream_kptsbitstream = (uchar*) &stream_kpts;

	// Creation of the bitstream
	int nBytes = kpts.size() * 5 * sizeof(float); // size of the bitstream (in bytes)
	for (int i = 0; i < nBytes; i++) { // fill the bitstream
		bitstream.push_back(char_stream_kptsbitstream[i]);
	}

	return true;

}

//todo rimuovere
bool VisualFeatureEncoding::_dummyEncodeBinaryDescriptors(
		const DescriptorType encoderType, const Mat& features,
		vector<uchar>& bitstream) const {

	bitstream.clear();

	if (_DEBUG)
		cout << "VisualFeatureEncoding::dummy_encodeBinaryDescriptors:  "
				<< encoderType << endl;

	int descSize = features.cols; // size of the descriptor
	int numDesc = features.rows; // number of descriptors

	// Create the bitstream
	for (int i = 0; i < descSize; i++) {
		for (int j = 0; j < numDesc; j++) {
			bitstream.push_back(features.at<uchar>(j, i));
		}
	}

	return true;

}

/* Byte-wise serialization */
bool VisualFeatureEncoding::_dummyEncodeDescriptors(const cv::Mat& features,
		vector<uchar>& bitstream) const {

	cout << "VisualFeatureEncoding::_dummyEncodeDescriptors: features: "
			<< features.row(0).colRange(0,
					(int) min((float) 10, (float) features.cols)) << endl;

	/* Necessary to use assign afterwards even if features is continue */

	cv::Mat featCont = features.clone();

	if (_DEBUG > 1)
		cout
				<< "VisualFeatureEncoding::_dummyEncodeDescriptors: bitstream.size() before "
				<< bitstream.size() << endl;
	bitstream.assign(featCont.datastart, featCont.dataend);

	if (_DEBUG > 1) {
		cout << "VisualFeatureEncoding::_dummyEncodeDescriptors: bitstream: ";
		for (int i = 0; i < min((float) 20, (float) bitstream.size()); ++i) {
			cout << (int) bitstream[i] << " ";
		}
		cout << endl;
	}

	if (_DEBUG > 1) {
		cout
				<< "VisualFeatureEncoding::_dummyEncodeDescriptors: featCont.total() "
				<< featCont.total() << endl;
		cout
				<< "VisualFeatureEncoding::_dummyEncodeDescriptors: featCont.elemSize() "
				<< featCont.elemSize() << endl;
		cout
				<< "VisualFeatureEncoding::_dummyEncodeDescriptors: bitstream.size() "
				<< bitstream.size() << endl;
	}
	return true;
}

/* Histogram encoding */
bool VisualFeatureEncoding::_encodeHist(const cv::Mat&features,
		vector<uchar>& bitstream, const codingParams& parms) const {

	HIST_codingParams* const histParms = (HIST_codingParams*) &parms;
	bitstream.clear();

	if (_DEBUG > 1)
			cout << "VisualFeatureEncoding::_encodeHist: histParms->valshift: " << (int)histParms->valshift << endl;

	if (_DEBUG > 1) {
		cout << "VisualFeatureEncoding::_encodeHist: features: ";
		for (int i = 0; i < min((float) 20, (float) features.cols); ++i) {
			cout << (int) features.at<ushort>(i) << " ";
		}
		cout << endl;
	}

	Mat reducedFeatures = features.clone();

	for (ushort dataIdx = 0; dataIdx < reducedFeatures.total(); ++dataIdx) {
		reducedFeatures.at<ushort>(dataIdx) = reducedFeatures.at<ushort>(
				dataIdx) >> histParms->valshift;
	}

	if (_DEBUG > 1) {
		cout << "VisualFeatureEncoding::_encodeHist: reducedFeatures: ";
		for (int i = 0; i < min((float) 20, (float) reducedFeatures.cols);
				++i) {
			cout << (int) reducedFeatures.at<ushort>(i) << " ";
		}
		cout << endl;
	}

	return _dummyEncodeDescriptors(reducedFeatures, bitstream);

	//TODO implement arithmetic coder
}

/* BRISK arithmetic coding */
bool VisualFeatureEncoding::_encodeBRISK(const Mat& features,
		vector<uchar>& bitstream) const {

	bitstream.clear();

	// load the BRISK probability model
	BRISK_pModel *pModel = BRISK_pModel::get_instance();

	if (_DEBUG)
		cout << "VisualFeatureEncoding::encodeBRISK" << endl;
	ac_encoder ace; // the encoder
	ac_model acm; // the probability model used by ace

	int freq[2]; // vector of frequency (dynamically updated)

	ac_encoder_init(&ace, bitstream); // init the encoder
	ac_model_init(&acm, 2, NULL, 0);  // init the model

	// start coding

	for (int n = 0; n < features.rows; n++) {

		int index = 0;
		int cur_bit = 0, prev_bit = 0;

		for (int i = 0; i < features.cols; i++) {

			// get the current element
			uchar cur_el = features.at<uchar>(n, i);

			//convert it into binary
			int temp_el[8];
			for (int j = 0; j < 8; j++) {
				temp_el[7 - j] = (cur_el >> j) & 0x0001;
			}

			// extract and decode the single bits
			//int cur_bit, prev_bit;

			for (int j = 0; j < 8; j++) {

				prev_bit = cur_bit;
				cur_bit = temp_el[j];			// (cur_el >> j) & 0x0001;
				// read the probabilities
				if (index == 0) {
					freq[0] = (int) max(1,
							(int) round(
									pModel->getP0(0) * (double) AC_PRECISION));
					freq[1] = AC_PRECISION - freq[0];
				} else {

					if (prev_bit == 0) {
						freq[0] =
								(int) max(1,
										(int) round(
												pModel->getP0c0(index,
														index
																- 1) * (double)AC_PRECISION));
						freq[1] = AC_PRECISION - freq[0];
					} else {
						freq[0] =
								(int) max(1,
										(int) round(
												pModel->getP0c1(index,
														index
																- 1) * (double)AC_PRECISION));
						freq[1] = AC_PRECISION - freq[0];
					}

				}
				if (freq[0] == AC_PRECISION) {
					freq[0] = AC_PRECISION - 1;
					freq[1] = 1;
				}

				ac_encode_symbol_updateModel(&ace, &acm, cur_bit, freq);

				index++;

			}

		}

	}

	ac_encoder_done(&ace);
	ac_model_done(&acm);

	return true;

}

