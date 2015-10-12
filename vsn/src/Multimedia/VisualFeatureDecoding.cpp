/*
 * VisualFeatureEncoding.cpp
 */
#include <Multimedia/VisualFeatureDecoding.h>

VisualFeatureDecoding* VisualFeatureDecoding::_instance = NULL;

VisualFeatureDecoding* VisualFeatureDecoding::getInstance() {
	if (_instance == NULL) {
		_instance = new VisualFeatureDecoding();
	}
	return _instance;
}

VisualFeatureDecoding::VisualFeatureDecoding() {
	BRISK_pModel::get_instance();
}

// Decoder for the keypoints
bool VisualFeatureDecoding::decodeKeyPoints(const vector<uchar>& bitstream,
		vector<KeyPoint>& kpts, const Size size,
		const bool decodeAngles) const {

	// inputs:  bitstream -> the encoded bitstream
	// outputs: kpts      -> the decoded vector of keypoints

	kpts.clear();

	int nbits_x = ceil(log2(size.width)) + 2;
	int nbits_y = ceil(log2(size.height)) + 2;
	int nbits_s = 10;
	int nbits_a = 0;

	if (decodeAngles == true) {
		nbits_a = 9;
	}

	int nbits_kpt = nbits_x + nbits_y + nbits_s + nbits_a;

	uchar cur_byte;

	int byte_idx = 0;
	int bit_idx = -1;
	int cur_bit;

	int nkpts = floor(((float) bitstream.size() * 8.0) / (float) nbits_kpt);

	for (int n = 0; n < nkpts; n++) {

		int qx = 0, qy = 0, qs = 0, qa = 0;
		int sign_a = 0;

		for (int i = 0; i < nbits_kpt; i++) {

			// reset bit counter
			if (bit_idx < 0) {
				bit_idx = 7;
				cur_byte = bitstream[byte_idx];
				byte_idx++;
			}

			// read the current bit
			cur_bit = (cur_byte >> bit_idx) & 0x01;
			bit_idx--;

			if (i < nbits_x) {
				qx |= (cur_bit << (nbits_x - i - 1));
			} else if (i < nbits_x + nbits_y) {
				qy |= (cur_bit << (nbits_x + nbits_y - i - 1));
			} else if (i < nbits_x + nbits_y + nbits_s) {
				qs |= (cur_bit << (nbits_x + nbits_y + nbits_s - i - 1));
			} else {
				if (decodeAngles == true) {
					if (i == nbits_x + nbits_y + nbits_s) {
						sign_a = cur_bit;
					} else {
						qa |= (cur_bit
								<< (nbits_x + nbits_y + nbits_s + nbits_a - i
										- 1));
					}
				}
			}

		}

		if (decodeAngles == false) {
			kpts.push_back(
					KeyPoint((float) qx / 4.0, (float) qy / 4.0,
							(float) qs / 4.0));
		} else {
			if (sign_a == 1) {
				qa = -qa;
			}
			kpts.push_back(
					KeyPoint((float) qx / 4.0, (float) qy / 4.0,
							(float) qs / 4.0, (float) qa));
		}

	}

	return 0;

}
;

// Decoder for binary descriptors
bool VisualFeatureDecoding::decodeBinaryDescriptors(
		const DescriptorType descType, vector<uchar>& bitstream, Mat &features,
		const int nFeats) const {

	// inputs:  descName  -> name of the binary descriptor (e.g., BRISK, ORB, ...)
	//          bitstream -> the bitstream to be decoded
	//			order     -> the order in which the elements of each descriptor are fetched
	// outputs: features  <- the decoded features

    bool retval;

    cout << "bitstream: ";
    for (int i = 0; i < 20; ++i){
        cout << (int)bitstream[i] << " ";
    }
    cout << endl;


	switch (descType) {
	case DESCRIPTORTYPE_BRISK:
        retval = _decodeBRISK(bitstream, features, nFeats);
		break;
	default:
        retval = false;
	}


    cout << "features: " << features.row(0).colRange(0,min(10,features.cols)) << endl;

    return retval;
}


// Decoder for non-binary descriptors
bool VisualFeatureDecoding::decodeNonBinaryDescriptors(
		const DescriptorType descType, const vector<uchar>& bitstream, Mat& features) const {

	switch (descType) {
		case DESCRIPTORTYPE_HIST:

			return true;
			break;
		default:
			return false;
		}


	return false;
}


// Dummy Decoder for the keypoints
bool VisualFeatureDecoding::dummy_decodeKeyPoints(
		const vector<uchar>& bitstream, vector<KeyPoint>& kpts) const {

	int nBytes = bitstream.size(); // size of the bitstream (in bytes)

	// convert from 'vector' to 'uchar array'
	uchar uchar_kpts_bitstream[nBytes];
	for (int i = 0; i < nBytes; i++) {
		uchar_kpts_bitstream[i] = bitstream[i];
	}

	// read the uchar array as a float array
	float *float_kpts_bitstream = (float*) &uchar_kpts_bitstream;

	int nFloats = (nBytes / sizeof(float)); // total number of floats

	// fill the vector of decoded keypoints
	for (int i = 0; i < nFloats; i = i + 5) {
		kpts.push_back(
				KeyPoint(float_kpts_bitstream[i], float_kpts_bitstream[i + 1],
						float_kpts_bitstream[i + 2],
						float_kpts_bitstream[i + 3],
						float_kpts_bitstream[i + 4]));
	}

	return true;

}
;

// Dummy Decoder for binary descriptors
bool VisualFeatureDecoding::dummy_decodeBinaryDescriptors(
		const DescriptorType descType, const vector<uchar>& bitstream,
		Mat& features) const {

	int descSize = 64;

	switch (descType) {
	case DESCRIPTORTYPE_BRISK:
		descSize = BRISK_LENGTH_BYTES;
		break;
	default:
		return false;
	}

	int nBytes = bitstream.size();  // size of the bitstream (in bytes)
	int nFeatures = (nBytes / descSize); // total number of features

	features = Mat(nFeatures, descSize, CV_8UC1);

	int count = 0;
	for (int i = 0; i < descSize; i++) {
		for (int j = 0; j < nFeatures; j++) {
			features.at<uchar>(j, i) = bitstream[count];
			count++;
		}
	}

	return 0;

}
;

// Dummy Decoder for non-binary descriptors
bool VisualFeatureDecoding::dummy_decodeNonBinaryDescriptors(
		const DescriptorType descType, const vector<uchar>& bitstream,
		Mat& features) const {

	int descSize = 64;
	switch (descType) {
	case DESCRIPTORTYPE_BRISK:
		descSize = BRISK_LENGTH_BYTES;
		break;
	default:
		return false;
	}

	int nBytes = bitstream.size();  // size of the bitstream (in bytes)
	int nFeatures = (nBytes / sizeof(float)) / descSize; // total number of features

	features.create(nFeatures, descSize, CV_32FC1);

	uchar float_uchar[sizeof(float)];

	int count = 0;
	for (int i = 0; i < descSize; i++) {
		for (int j = 0; j < nFeatures; j++) {

			for (unsigned int b = 0; b < sizeof(float); b++) {
				float_uchar[b] = bitstream[count + b];
			}

			features.at<float>(j, i) = *(float*) &float_uchar;

			count = count + sizeof(float);
		}
	}

	return 0;

}

bool VisualFeatureDecoding::dummy_decodeDescriptors(const DescriptorType,
		const vector<uchar>& bitstream, Mat& features,
		const cv::Size featuresSize, const int featuresType) const {

	features = cv::Mat::zeros(featuresSize,featuresType);

    cout << "features.total(): " << features.total() << endl;
    cout << "features.elemSize(): " << features.elemSize() << endl;
    cout << "bitstream.size(): " << bitstream.size() << endl;

    cout << "bitstream: ";
    for (int i = 0; i < 20; ++i){
        cout << (int)bitstream[i] << " ";
    }
    cout << endl;

	if (features.total()*features.elemSize() != bitstream.size()){
		cerr << "VisualFeatureDecoding::dummy_decodeDescriptors: bitstream and features dimensions are not coherent" << endl;
		return false;
	}

    for (unsigned int byteIdx = 0; byteIdx < bitstream.size(); ++byteIdx){
		features.data[byteIdx] = bitstream[byteIdx];
	}

    cout << "features: " << features.row(0).colRange(0,min(10,features.cols)) << endl;

	return true;

}

bool VisualFeatureDecoding::_decodeBRISK(vector<uchar>& bitstream,
		Mat &features, const int nFeats) const {

	features = Mat(nFeats, BRISK_LENGTH_BYTES, CV_8UC1);

	// load the BRISK probability model
	BRISK_pModel *pModel = BRISK_pModel::get_instance();

	ac_decoder acd; // the encoder
	ac_model acm; // the probability model used by ace

	int freq[2]; // vector of frequency (dynamically updated)

	ac_decoder_init(&acd, bitstream);
	ac_model_init(&acm, 2, NULL, 0);

	int decoded_bit;

	int index;

	for (int n = 0; n < nFeats; n++) {

		index = 0;
		freq[0] = (int) max(1,
				(int) round(pModel->getP0(0) * (double) AC_PRECISION));
		freq[1] = AC_PRECISION - freq[0];

		for (int i = 0; i < BRISK_LENGTH_BYTES; i++) {

			// extract and decode the single bits
			uchar cur_el = 0;
			for (int j = 0; j < 8; j++) {

				decoded_bit = ac_decode_symbol_updateModel(&acd, &acm, freq);

				cur_el |= ((uchar) decoded_bit << j); // compute the decimal representation

				// update index and read the current probabilities
				index++;
				if (index < BRISK_LENGTH_BITS) {
					if (decoded_bit == 0) {
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

			}

			// flip cur_el
			uchar revData = (cur_el & 0x80) >> 7 | (cur_el & 0x40) >> 5
					| (cur_el & 0x20) >> 3 | (cur_el & 0x10) >> 1
					| (cur_el & 0x08) << 1 | (cur_el & 0x04) << 3
					| (cur_el & 0x02) << 5 | (cur_el & 0x01) << 7;
			features.at<uchar>(n, i) = revData;

		}

	}

	ac_decoder_done(&acd);
	ac_model_done(&acm);

	return 0;

}

