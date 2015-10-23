#ifndef SRC_MESSAGES_DATAATCMSG_H_
#define SRC_MESSAGES_DATAATCMSG_H_

#include <Messages/Message.h>
#include <Network/LinkType.h>

class Header;
class NetworkNode;

class DataATCMsg: public Message {
private:

	friend class cereal::access;
	template<typename Archive>
	void serialize(Archive &ar);

	uchar _frameID;
	ushort _blockNumber;
	ushort _numBlocks;
	float _detTime;
	float _descTime;
	float _kencTime;
	float _fencTime;
	float _txTime;
	ushort _numFeat;
	ushort _numKpts;
	ushort _frameWidth;
	ushort _frameHeight;
	std::vector<uchar> _featuresData;
	std::vector<uchar> _kptsData;
	uchar _operativeMode;
	int64 _startTick;

public:
	DataATCMsg(NetworkNode* const src, NetworkNode* const dst,
			const LinkType linkType, const uchar frameID,
			const ushort blockNumber, const ushort numBlocks,
			const float detTime, const float descTime, const float kptsEncTime,
			const float featEncTime, const float txTime, const ushort numFeat,
			const ushort numKpts, const ushort frameWidth_,
			const ushort frameHeight_, const Bitstream& features_data,
			const Bitstream& keypoints_data, const OperativeMode opMode,
			const int64 startTick);
	DataATCMsg(Header* const header, Bitstream* const bitstream);

	Bitstream* getBitStream() const;

	uchar getFrameId() const {
		return _frameID;
	}
	ushort getBlockNumber() const {
		return _blockNumber;
	}
	ushort getNumBlocks() const {
		return _numBlocks;
	}
	float getDetTime() const {
		return _detTime;
	}
	float getDescTime() const {
		return _descTime;
	}
	float getTxTime() const {
		return _txTime;
	}
	void setTxTime(const float tx_time) {
		_txTime = tx_time;
	}
	float getKptsEncodingTime() const {
		return _kencTime;
	}
	float getFeatEncodingTime() const {
		return _fencTime;
	}
	ushort getNumFeat() const {
		return _numFeat;
	}
	ushort getNumKpts() const {
		return _numKpts;
	}
	ushort getFrameWidth() const {
		return _frameWidth;
	}
	ushort getFrameHeight() const {
		return _frameHeight;
	}
	const std::vector<uchar>* getFeaturesData() const {
		return &_featuresData;
	}
	const std::vector<uchar>* getKeypointsData() const {
		return &_kptsData;
	}
	OperativeMode getOperativeMode() const {
		return (OperativeMode) _operativeMode;
	}
	int64 getStartTick() const {
		return _startTick;
	}
	void setStartTick(const int64 startTick) {
		_startTick = startTick;
	}
};

#endif /* DATAATCMSG_H_ */
