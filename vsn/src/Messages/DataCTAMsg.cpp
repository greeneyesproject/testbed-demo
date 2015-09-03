/*
 * DataCTAMsg.cpp
 *
 *  Created on: Jul 9, 2014
 *      Author: Ale
 */
#include <Messages/DataCTAMsg.h>
#include <Messages/Message.h>
#include <Messages/Header.h>

#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream_buffer.hpp>

#include <iostream>
#include <cassert>
#include <vector>

using namespace std;

DataCTAMsg::DataCTAMsg(NetworkNode* const src, NetworkNode* const dst,
		const LinkType linkType, const uchar frameID, const uchar sliceNumber,
		const ushort topLeft_x, const ushort topLeft_y, const uint32_t dataSize,
		const float encTime, const float txTime, const Bitstream& data, const OperativeMode opMode) :
		Message(src, dst, linkType) {

	_msg_type = MESSAGETYPE_DATA_CTA;
	_frameID = frameID;
	_sliceNumber = sliceNumber;
	_topLeft_x = topLeft_x;
	_topLeft_y = topLeft_y;
	_dataSize = dataSize;
	_encTime = encTime;
	_txTime = txTime;
	_data = data;
	_operativeMode = opMode;
}

DataCTAMsg::DataCTAMsg(Header* const header, Bitstream* const bitstream) :
		Message(header) {
	_msg_type = MESSAGETYPE_DATA_CTA;
	/* Dummy initialization to avoid warnings */
	_encTime = 0;
	_dataSize = 0;
	_txTime = 0;
	_frameID = 0;
	_sliceNumber = 0;
	_topLeft_x = 0;
	_topLeft_y = 0;
	_operativeMode = 0;

	double tick = cv::getTickCount();

	stringstream ss;
	ss.rdbuf()->pubsetbuf((char*) bitstream->data(), bitstream->size());
	cereal::BinaryInputArchive ia(ss);
	ia >> (*this);

	double time = ((double) (cv::getTickCount() - tick))
			/ cv::getTickFrequency();
	cout << "DataCTAMsg::DataCTAMsg: time: " << time << endl;

	_bitstreamSize = bitstream->size();
}

template<typename Archive>
void DataCTAMsg::serialize(Archive &ar) {
	ar & _frameID;
	ar & _sliceNumber;
	ar & _topLeft_x;
	ar & _topLeft_y;
	ar & _dataSize;
	ar & _encTime;
	ar & _txTime;
	ar & _data;
	ar & _operativeMode;
}
Bitstream* DataCTAMsg::getBitStream() const {
	double tick = cv::getTickCount();
	stringstream bitstream;
	cereal::BinaryOutputArchive oa(bitstream);
	oa << (*this);
	/* bitstreamString is needed since every call to bitstream.str() returns a different string*/
	string bitstreamString = bitstream.str();
	Bitstream* bitstreamVector = new vector<uchar>(bitstreamString.begin(),
			bitstreamString.end());

	double time = ((double) (cv::getTickCount() - tick))
			/ cv::getTickFrequency();
	cout << "DataCTAMsg::getBitStream: time: " << time << endl;
	return bitstreamVector;
}
