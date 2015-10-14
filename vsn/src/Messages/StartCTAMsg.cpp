/*
 * StartCTAMsg.cpp
 *
 *  Created on: Jul 8, 2014
 *      Author: Ale
 */
#include <Messages/StartCTAMsg.h>
#include <Messages/Message.h>
#include <cassert>
#include <vector>

using namespace std;

StartCTAMsg::StartCTAMsg(NetworkNode* const src, NetworkNode* const dst,
		const LinkType linkType, const uchar qf, const cv::Size size,
		const uchar num_slices, const OperativeMode opMode, const ImageSource imageSource_,const ushort wifiBandwidth_) :
		Message(src, dst, linkType) {

	_msg_type = MESSAGETYPE_START_CTA;

	_qualityFactor = qf;
	_frameWidth = size.width;
	_frameHeight = size.height;
	_numSlices = num_slices;
	_operativeMode = (uchar) opMode;
	_imageSource = (uchar) imageSource_;
	_wifiBandwidth = wifiBandwidth_;
}

StartCTAMsg::StartCTAMsg(Header* const header, Bitstream* const bitstream) :
		Message(header) {
	_msg_type = MESSAGETYPE_START_CTA;

	/* Dummy initialization to avoid warnings */
	_qualityFactor = 0;
	_frameWidth = 0;
	_frameHeight = 0;
	_numSlices = 0;
	_operativeMode = 0;
	_imageSource = 0;
	_wifiBandwidth = 0;

	stringstream ss;
	ss.rdbuf()->pubsetbuf((char*) bitstream->data(), bitstream->size());
	cereal::BinaryInputArchive ia(ss);
	ia >> (*this);
}

template<typename Archive>
void StartCTAMsg::serialize(Archive &ar) {
	ar & _qualityFactor;
	ar & _frameWidth;
	ar & _frameHeight;
	ar & _numSlices;
	ar & _operativeMode;
	ar & _imageSource;
	ar & _wifiBandwidth;
}

Bitstream* StartCTAMsg::getBitStream() const {
	stringstream bitstream;
	cereal::BinaryOutputArchive oa(bitstream);
	oa << (*this);
	/* bitstreamString is needed since every call to bitstream.str() returns a different string*/
	string bitstreamString = bitstream.str();
	Bitstream* bitstreamVector(
			new vector<uchar>(bitstreamString.begin(), bitstreamString.end()));
	return bitstreamVector;
}
