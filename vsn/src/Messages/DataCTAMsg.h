#ifndef SRC_MESSAGES_DATACTAMSG_H_
#define SRC_MESSAGES_DATACTAMSG_H_

#include <Messages/Message.h>
#include <Network/LinkType.h>
#include <vector>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

class NetworkNode;
class Archive;

class DataCTAMsg: public Message {
private:

	friend class cereal::access;
	template<typename Archive>
	void serialize(Archive &ar);

	uchar _frameID;
	uchar _sliceNumber;
	uchar _totNumSlices;
	ushort _topLeft_x;
	ushort _topLeft_y;
	uint32_t _dataSize;
	float _encTime;
	float _txTime;
	Bitstream _data;
	uchar _operativeMode;
	int64 _startTick;

public:
	DataCTAMsg(NetworkNode* const src, NetworkNode* const dst,
			LinkType const linkType, const uchar frameID,
			const uchar sliceNumber, const uchar _totNumSlices,
			const ushort topLeft_x, const ushort topLeft_y,
			const uint32_t dataSize, const float encTime, const float txTime,
			const Bitstream& data, const OperativeMode opMode,
			const int64 _startTick);

	DataCTAMsg(Header* const header, Bitstream* const bitstream);

	Bitstream* getBitStream() const;

	int getFrameId() const {
		return _frameID;
	}
	int getSliceNumber() const {
		return _sliceNumber;
	}
	int getTotNumSlices() const {
		return _totNumSlices;
	}
	ushort getTopLeftX() const {
		return _topLeft_x;
	}
	ushort getTopLeftY() const {
		return _topLeft_y;
	}
	uint32_t getDataSize() const {
		return _dataSize;
	}
	float getEncodingTime() const {
		return _encTime;
	}
	float getTxTime() const {
		return _txTime;
	}
	void setTxTime(const float tx_time) {
		_txTime = tx_time;
	}
	const std::vector<uchar>* getData() const {
		return &_data;
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

#endif /* DATACTAMSG_H_ */
