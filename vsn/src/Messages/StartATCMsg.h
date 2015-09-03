#ifndef SRC_MESSAGES_STARTATCMSG_H
#define SRC_MESSAGES_STARTATCMSG_H

#include <Messages/Message.h>
#include <Network/LinkType.h>

class Header;

class StartATCMsg: public Message {
private:

	friend class cereal::access;
	template<typename Archive>
	void serialize(Archive &ar);

	uchar _detectorType;
	float _detectorThreshold;
	uchar _descriptorType;
	unsigned int _descriptorLength;
	ushort _maxNumberOfFeatures;
	uchar _rotationInvariant;
	bool _encodeKeypoints;
	bool _encodeFeatures;
	uchar _transferCoordinates;
	uchar _transferScale;
	uchar _transferOrientation;
	ushort _numFeaturesPerBlock;
	ushort _topLeftX;
	ushort _topLeftY;
	ushort _bottomRightX;
	ushort _bottomRightY;
	uchar _binShift;
	uchar _valShift;
	uchar _numCooperators;
	uchar _operativeMode;
	Bitstream _keypoints;
	ushort _wifiBandwidth;

public:
	StartATCMsg(NetworkNode* const src, NetworkNode* const dst,
			const LinkType linkType, const DetectorType det,
			const float det_thr, const DescriptorType desc,
			const unsigned int desc_length, const ushort max_feat,
			const uchar rotation_invariant, const bool encodeKeypoints,
			const bool encodeFeatures, const uchar transfer_kpt,
			const uchar transfer_scale, const uchar transfer_orientation,
			const ushort num_feat_per_blocks, const cv::Size topLeft,
			const cv::Size bottomRight, const uchar binShift,
			const uchar valShift, const uchar numCoops,
			const OperativeMode opMode, const Bitstream& keypoints,
			const ushort wifiBandwidth_);

	StartATCMsg(NetworkNode* const src, NetworkNode* const dst,
			const LinkType linkType, Bitstream* const bitstream);

	StartATCMsg(Header* const header, Bitstream* const bitstream);

	Bitstream* getBitStream() const;

	cv::Size getTopLeft() const {
		return cv::Size(_topLeftX, _topLeftY);
	}
	cv::Size getBottomRight() const {
		return cv::Size(_bottomRightX, _bottomRightY);
	}
	DetectorType getDetectorType() const {
		return (DetectorType) _detectorType;
	}
	float getDetectorThreshold() const {
		return _detectorThreshold;
	}
	DescriptorType getDescriptorType() const {
		return (DescriptorType) _descriptorType;
	}
	unsigned int getDescriptorLength() const {
		return _descriptorLength;
	}
	ushort getMaxNumFeat() const {
		return _maxNumberOfFeatures;
	}
	bool getKeypointsCoding() const {
		return _encodeKeypoints;
	}
	bool getFeaturesCoding() const {
		return _encodeFeatures;
	}
	uchar getTransferKpt() const {
		return _transferCoordinates;
	}
	uchar getTransferScale() const {
		return _transferScale;
	}
	uchar getTransferOrientation() const {
		return _transferOrientation;
	}
	ushort getNumFeatPerBlock() const {
		return _numFeaturesPerBlock;
	}
	uchar getBinShift() const {
		return _binShift;
	}
	uchar getValShift() const {
		return _valShift;
	}
	uchar getNumCooperators() const {
		return _numCooperators;
	}
	OperativeMode getOperativeMode() const {
		return (OperativeMode) _operativeMode;
	}
	Bitstream getKeypoints() const {
		return _keypoints;
	}
	ushort getWifiBandwidth() const {
		return _wifiBandwidth;
	}

};

#endif
