#include <Messages/DataATCMsg.h>
#include <Messages/Message.h>
#include <vector>
#include <cassert>

using namespace std;
DataATCMsg::DataATCMsg(NetworkNode* const src, NetworkNode* const dst,
		const LinkType linkType, const uchar frameID, const ushort blockNumber,
		const ushort numBlocks, const float detTime, const float descTime,
		const float kptsEncTime, const float featEncTime, const float txTime,
		const ushort numFeat, const ushort numKpts, const ushort frameWidth_, const ushort frameHeight_,
		const Bitstream& features_data, const Bitstream& keypoints_data, const OperativeMode opMode, const int64 startTick) :
		Message(src, dst, linkType) {

	_msg_type = MESSAGETYPE_DATA_ATC;

	_frameID = frameID;
	_blockNumber = blockNumber;
	_numBlocks = numBlocks;
	_detTime = detTime;
	_descTime = descTime;
	_kencTime = kptsEncTime;
	_fencTime = featEncTime;
	_txTime = txTime;
	_numFeat = numFeat;
	_numKpts = numKpts;
	_frameWidth = frameWidth_;
	_frameHeight = frameHeight_;
	_featuresData = features_data;
	_kptsData = keypoints_data;
	_operativeMode = opMode;
	_startTick = startTick;
}

DataATCMsg::DataATCMsg(Header* const header, Bitstream* const bitstream) :
		Message(header) {
	_msg_type = MESSAGETYPE_DATA_ATC;
	/* Dummy initialization to avoid warnings */
	_detTime = 0;
	_kencTime = 0;
	_numKpts = 0;
	_blockNumber = 0;
	_txTime = 0;
	_numFeat = 0;
	_numBlocks = 0;
	_fencTime = 0;
	_descTime = 0;
	_frameID = 0;
	_blockNumber = 0;
	_operativeMode = 0;
	_frameWidth = 0;
	_frameHeight = 0;
	_startTick = 0;


	stringstream ss;
	ss.rdbuf()->pubsetbuf((char*) bitstream->data(), bitstream->size());
	cereal::BinaryInputArchive ia(ss);
	ia >> (*this);

	_bitstreamSize = bitstream->size();
}

template<typename Archive>
void DataATCMsg::serialize(Archive &ar) {
	ar & _frameID;
	ar & _blockNumber;
	ar & _numBlocks;
	ar & _detTime;
	ar & _descTime;
	ar & _kencTime;
	ar & _fencTime;
	ar & _txTime;
	ar & _numFeat;
	ar & _numKpts;
	ar & _frameWidth;
	ar & _frameHeight;
	ar & _featuresData;
	ar & _kptsData;
	ar & _operativeMode;
	ar & _startTick;
}

Bitstream* DataATCMsg::getBitStream() const {
	stringstream bitstream;
	cereal::BinaryOutputArchive oa(bitstream);
	oa << (*this);
	/* bitstreamString is needed since every call to bitstream.str() returns a different string*/
	string bitstreamString = bitstream.str();
	Bitstream* bitstreamVector(
			new vector<uchar>(bitstreamString.begin(), bitstreamString.end()));
	return bitstreamVector;
}
