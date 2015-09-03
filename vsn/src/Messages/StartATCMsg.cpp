/*
 * StartATCMsg.cpp
 *
 *  Created on: Jul 8, 2014
 *      Author: Ale
 */
#include"StartATCMsg.h"
#include <TestbedTypes.h>
#include <Messages/Message.h>
#include <cassert>
#include <vector>

using namespace std;

StartATCMsg::StartATCMsg(NetworkNode* const src, NetworkNode* const dst,
		const LinkType linkType, const DetectorType det, const float det_thr,
		const DescriptorType desc, const unsigned int desc_length,
		const ushort max_feat, const uchar rotation_invariant,
		const bool encodeKeypoints, const bool encodeFeatures,
		const uchar transfer_kpt, const uchar transfer_scale,
		const uchar transfer_orientation, const ushort num_feat_per_blocks,
		const cv::Size topLeft, const cv::Size bottomRight,
		const uchar binShift, const uchar valShift, const uchar numCoops,
		const OperativeMode opMode, const Bitstream& keypoints, const ushort wifiBandwidth_) :
		Message(src, dst, linkType) {

	_msg_type = MESSAGETYPE_START_ATC;

	_detectorType = (uchar) det;
	_detectorThreshold = det_thr;
	_descriptorType = (uchar) desc;
	_descriptorLength = desc_length;
	_maxNumberOfFeatures = max_feat;
	_rotationInvariant = rotation_invariant;
	_encodeKeypoints = (ushort) encodeKeypoints;
	_encodeFeatures = (ushort) encodeFeatures;
	_transferCoordinates = transfer_kpt;
	_transferScale = transfer_scale;
	_transferOrientation = transfer_orientation;
	_numFeaturesPerBlock = num_feat_per_blocks;
	_topLeftX = topLeft.width;
	_topLeftY = topLeft.height;
	_bottomRightX = bottomRight.width;
	_bottomRightY = bottomRight.height;
	_binShift = binShift;
	_valShift = valShift;
	_numCooperators = numCoops;
	_operativeMode = (uchar) opMode;
	_keypoints = keypoints;
	_wifiBandwidth = wifiBandwidth_;
}

StartATCMsg::StartATCMsg(Header* const header, Bitstream* const bitstream) :
		Message(header) {
	_msg_type = MESSAGETYPE_START_ATC;
	/* Dummy initialization to avoid warnings */
	_detectorType = 0;
	_detectorThreshold = 0;
	_descriptorType = 0;
	_descriptorLength = 0;
	_maxNumberOfFeatures = 0;
	_rotationInvariant = 0;
	_encodeKeypoints = 0;
	_encodeFeatures = 0;
	_transferCoordinates = 0;
	_transferScale = 0;
	_transferOrientation = 0;
	_numFeaturesPerBlock = 0;
	_topLeftX = 0;
	_topLeftY = 0;
	_bottomRightX = 0;
	_bottomRightY = 0;
	_binShift = 0;
	_valShift = 0;
	_numCooperators = 0;
	_operativeMode = 0;
	_wifiBandwidth = 0;
	stringstream ss;
	ss.rdbuf()->pubsetbuf((char*) bitstream->data(), bitstream->size());
	cereal::BinaryInputArchive ia(ss);
	ia >> (*this);
}

StartATCMsg::StartATCMsg(NetworkNode* const src, NetworkNode* const dst,
		const LinkType linkType, Bitstream* const bitstream) :
		Message(src, dst, linkType) {
	_msg_type = MESSAGETYPE_START_ATC;
	/* Dummy initialization to avoid warnings */
	_detectorType = 0;
	_detectorThreshold = 0;
	_descriptorType = 0;
	_descriptorLength = 0;
	_maxNumberOfFeatures = 0;
	_rotationInvariant = 0;
	_encodeKeypoints = 0;
	_encodeFeatures = 0;
	_transferCoordinates = 0;
	_transferScale = 0;
	_transferOrientation = 0;
	_numFeaturesPerBlock = 0;
	_topLeftX = 0;
	_topLeftY = 0;
	_bottomRightX = 0;
	_bottomRightY = 0;
	_numCooperators = 0;
	_binShift = 0;
	_valShift = 0;
	_operativeMode = 0;
	_wifiBandwidth = 0;
	stringstream ss;
	ss.rdbuf()->pubsetbuf((char*) bitstream->data(), bitstream->size());
	cereal::BinaryInputArchive ia(ss);
	ia >> (*this);
}

template<typename Archive>
void StartATCMsg::serialize(Archive &ar) {
	ar & _detectorType;
	ar & _detectorThreshold;
	ar & _descriptorType;
	ar & _descriptorLength;
	ar & _maxNumberOfFeatures;
	ar & _rotationInvariant;
	ar & _encodeKeypoints;
	ar & _encodeFeatures;
	ar & _transferCoordinates;
	ar & _transferScale;
	ar & _transferOrientation;
	ar & _numFeaturesPerBlock;
	ar & _topLeftX;
	ar & _topLeftY;
	ar & _bottomRightX;
	ar & _bottomRightY;
	ar & _binShift;
	ar & _valShift;
	ar & _numCooperators;
	ar & _operativeMode;
	ar & _keypoints;
	ar & _wifiBandwidth;
}
Bitstream* StartATCMsg::getBitStream() const {
	stringstream bitstream;
	cereal::BinaryOutputArchive oa(bitstream);
	oa << (*this);
	/* bitstreamString is needed since every call to bitstream.str() returns a different string*/
	string bitstreamString = bitstream.str();
	Bitstream* bitstreamVector(
			new vector<uchar>(bitstreamString.begin(), bitstreamString.end()));
	return bitstreamVector;
}
