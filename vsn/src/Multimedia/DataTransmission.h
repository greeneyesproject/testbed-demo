/*
 * DataTransmission.h
 */
#ifndef DATATRANSMISSION_H_
#define DATATRANSMISSION_H_

#include <iostream>
#include <stdio.h>

#include <opencv2/core/core.hpp>

#include <arpa/inet.h>
#include <sys/socket.h>

#include <unistd.h>

#include <Network/Telosb/serialsource.h>
#include <math.h>
#include <string>

#define MAX_RADIO_PACKET_LENGTH 122
#define MAX_RADIO_PAYLOAD_LENGTH 114
#define RADIO_HEADER_LENGTH 8
#define APP_HEADER_LENGTH 10

using namespace std;

// STRUCTURE OF THE RADIO PACKET:
// serial_packet[ 0]  -> 0x00 (reserved)
// serial_packet[ 1]  -> 0x00 (reserved)
// serial_packet[ 2]  -> 0x00 (reserved)
// serial_packet[ 3]  -> 0x00 (reserved)
// serial_packet[ 4]  -> 0x00 (reserved)
// serial_packet[ 5]  -> total length of the payload
// serial_packet[ 6]  -> 0x00 (reserved)
// serial_packet[ 7]  -> 0x47 (reserved)
// serial_packet[ 8]  -> RADIO_SRC (MS byte)
// serial_packet[ 9]  -> RADIO_SRC (LS byte)
// serial_packet[10]  -> RADIO_DST (MS byte)
// serial_packet[11]  -> RADIO_DST (LS byte)
// serial_packet[12]  -> PACKET_ID
// serial_packet[13]  -> NUMBER OF FRAMES (LS Byte)
// serial_packet[14]  -> NUMBER OF FRAMES (MS Byte)
// serial_packet[15]  -> FRAME NUMBER (LS Byte)
// serial_packet[16]  -> FRAME NUMBER (MS Byte)
// serial_packet[17]  -> Packet Info (Type + imgID)
// serial_packet[18]  -> First byte of the frame
// .....
// serial_packet[ N]  -> Last byte of the frame

enum packetType {
	IMG_INFO_PKT = 0x0F,  // IMAGE INFO PACKET
	CTA_IMG_PKT = 0x01,  // CTA IMAGE PACKET
	ATC_IMG_PKT = 0x02,  // ATC IMAGE PACKET
	ACK_REC_INFO_PKT = 0x03,  // ACK: RECEIVED INFO PACKET
	ACK_REC_IMG_PKT = 0x04,  // ACK: RECEIVED ALL IMAGE DATA
	ACK_REC_IMG_PKT_ACT = 0x05, // ACK: RECEIVED ALL IMAGE DATA with ACTIONS REQUEST
	REQ_ACK_REC_INFO = 0x06,  // REQUEST OF ACK for ACK_REC_INFO_PKT
	REQ_ACK_REC_IMG = 0x07,  // REQUEST OF ACK for ACK_REC_IMG_PKT
	ERR_PKT = 0x08,  // ERROR PACKET
	CMD_PKT = 0x09,  // COMMAND PACKET
	NO_OBJ_PKT = 0x0A,  // NO OBJECTS PRESENT (or no features extracted)
	UNKNOWN_PKT = -1     // UNKNOWN PACKET
};

enum commandType {
	START,                // START
	STOP,			   	  // STOP
	NO_CMD = -1           // no command received (timeout expired)
};

enum ackType {
	ACK_REC_INFO, ACK_REC_IMG, UNKNOWN_ACK = -1
};

// Data read from INFO packets
struct imgInfo {
	int imageID;     // image identifier
	int w;           // width
	int h;           // height
	bool atc_cta;    // atc (true) / cta (false) paradigm
	bool compressed; // features are compressed (true) / non-compressed (false)
	int nFeats;      // number of features (valid only if atc=true)
	string descName; // descriptor
};

// Data read from data packets
struct pktInfo {
	bool atc_cta;    // atc (true) / cta (false) paradigm
	int imageID;     // image identifier
	int nFeats;      // number of features in the packet
};

// Possible actions for img processing
struct actions {
	bool atc;
	bool cta;
	bool do_compression;
	int det_threshold;
	int max_features;
	int jpeg_qf;
	int tx_delay;
	bool stop_program;
};

class DataTransmission {

	uchar *packet;  // received packet
	bool dealloc_packet;
	char *message; // message to be sent
	int msg_length; // length of the message (in bytes)
	uchar RADIO_packetID;
	uchar recv_RADIO_packetID;

	int create_INFO_Packet();
	int create_ATC_Packet(int imgID, int nFeats, vector<uchar> kpts_bitstream,
			vector<uchar> feats_bitstream);
	int create_CTA_Packet(int imgID, vector<uchar> jpeg_bitstream);

	int sendPacketRADIO(serial_source src, int radio_dst, packetType pt,
			int imgID);

	uchar packet_info; //PACKET_TYPE + IMAGE_ID

public:

	int sendPacketUDP(string server_addr, int port, int imgID, int nFeats,
			vector<uchar> kpts_bitstream, vector<uchar> feats_bitstream,
			char* rec_stream);

	serial_source openSerialRADIO(const char *serial_device, int baud_rate,
			int non_blocking);
	int closeSerialRADIO(serial_source src);

	int sendImgInfoRADIO(serial_source src, int radio_dst, int imgID,
			bool atc_cta, bool compressed = true, string descName = "BRISK",
			int nFeats = 0, int imWidth = 640, int imHeight = 480);

	int sendNoObjPktRADIO(serial_source src, int radio_dst);

	int sendFeaturesRADIO(serial_source src, int radio_dst, int imgID,
			int nFeats, vector<uchar> kpts_bitstream,
			vector<uchar> feats_bitstream);
	int sendJpegRADIO(serial_source src, int radio_dst, int imgID,
			vector<uchar> jpeg_bitstream);
	int sendACK_RADIO(serial_source src, int radio_dst, ackType at, int imgID,
			actions *acts = NULL);
	int sendCMD_RADIO(serial_source src, int radio_dst, commandType ct,
			actions *acts = NULL);
	int reqACK_RADIO(serial_source src, int radio_dst, ackType at, int imgID);

	bool waitACK_RADIO(serial_source src, ackType at, int imgID,
			double timeout = 0, actions *acts = NULL);
	commandType waitCMD_RADIO(serial_source src, actions *acts, double timeout =
			0);

	int receivePacketRADIO(serial_source src, uchar **packet, int *packetLength,
			double timeout = 0);
	int receiveDataPacketRADIO(serial_source src, uchar **packet,
			int *packetLength, double timeout = 0);

	int closeSerialRADIO();

	packetType parseMessage(uchar *packet);

	int getImgInfo(uchar *packet, imgInfo &info);
	int getPacketInfo(uchar *packet, pktInfo &info);
	int getEncodedKeypoints(uchar *packet, int packetLength, imgInfo &info,
			vector<uchar> &enc_keypoints);
	int getEncodedFeatures(uchar *packet, int packetLength, imgInfo &info,
			vector<uchar> &enc_features);
	int getEncodedJPEG(uchar *packet, int packetLength,
			vector<uchar> &enc_image);
	uchar getPacketID();
	uchar getRecvPacketID();
};

#endif /* DATATRANSMISSION_H_ */
