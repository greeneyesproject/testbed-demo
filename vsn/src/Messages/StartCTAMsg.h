#ifndef SRC_MESSAGES_STARTCTAMSG_H
#define SRC_MESSAGES_STARTCTAMSG_H

#include <Messages/Message.h>
#include <Network/LinkType.h>

class Header;

class StartCTAMsg: public Message {
private:

	friend class cereal::access;
	template<typename Archive>
	void serialize(Archive &ar);

	uchar _qualityFactor;
	ushort _frameWidth;
	ushort _frameHeight;
	uchar _numSlices;
	uchar _operativeMode;
	uchar _imageSource;
	ushort _wifiBandwidth;

public:
	StartCTAMsg(NetworkNode* const src, NetworkNode* const dst,
			const LinkType linkType, const uchar qf, const cv::Size size,
			const uchar num_slices, const OperativeMode opMode,
			const ImageSource imageSource_, const ushort wifiBandwidth_);

	StartCTAMsg(Header* const header, Bitstream* const bitstream);

	Bitstream* getBitStream() const;

	uchar getQualityFactor() const {
		return _qualityFactor;
	}
	ushort getFrameWidth() const {
		return _frameWidth;
	}
	ushort getFrameHeight() const {
		return _frameHeight;
	}
	uchar getNumSlices() const {
		return _numSlices;
	}
	OperativeMode getOperativeMode() const {
		return (OperativeMode) _operativeMode;
	}
	ImageSource getImageSource() const {
		return (ImageSource) _imageSource;
	}
	ushort getWifiBandwidth() const {
		return _wifiBandwidth;
	}

};

#endif
