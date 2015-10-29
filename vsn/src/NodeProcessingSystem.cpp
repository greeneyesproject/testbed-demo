/*
 * NodeProcessingSystem.cpp
 *
 *  Created on: 27/feb/2015
 *      Author: luca
 */

#include <TestbedTypes.h>
#include <NodeProcessingSystem.h>
#include <Network/NodeNetworkSystem.h>
#include <Network/NetworkNode.h>
#include <Messages/Message.h>
#include <Messages/StartATCMsg.h>
#include <Messages/StartCTAMsg.h>
#include <Messages/StopMsg.h>
#include <Messages/DataATCMsg.h>
#include <Messages/DataCTAMsg.h>
#include <Messages/Header.h>
#include <Messages/CoopInfoMsg.h>
#include <Messages/CoopInfoReqMsg.h>
#include <Multimedia/ImageAcquisition.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

NodeProcessingSystem* NodeProcessingSystem::_instance = NULL;

NodeProcessingSystem* NodeProcessingSystem::getInstance(NodeNetworkSystem* nns_,
		const NodeType nodeType, const CameraParameters& cameraParameters,
		const bool oneShot, BlackLib::BlackGPIO** gpios) {
	if (_instance == NULL) {
		_instance = new NodeProcessingSystem(nns_, nodeType, cameraParameters,
				oneShot, gpios);
	}
	return _instance;

}

NodeProcessingSystem::NodeProcessingSystem(NodeNetworkSystem* nns_,
		const NodeType nodeType, const CameraParameters& cameraParameters,
		const bool oneShot, BlackLib::BlackGPIO** gpios) :
		_processingQueue(_waitPeriod, 1), _serviceQueue(_waitPeriod) {

	_gpios = gpios;
	_oneShot = oneShot;

	_processingThread = new boost::thread(
			&NodeProcessingSystem::_processingThreadHandler, this);
	_serviceThread = new boost::thread(
			&NodeProcessingSystem::_serviceThreadHandler, this);

	_nodeType = nodeType;

	_nodeNetworkSystem = nns_;
	nns_->setNodeProcessingSystem(this);
	_offloadingManager = OffloadingManager::getInstance();

	_frameId = 0;

	switch (_nodeType) {
	case NODETYPE_SINK:
		_initSink();
		break;
	case NODETYPE_CAMERA:
		_initCamera(cameraParameters);
		break;
	case NODETYPE_COOPERATOR:
		_initCooperator();
		break;
	default:
		if (_DEBUG)
			cerr
					<< "NodeProcessingSystem::NodeProcessingSystem: unexpected node type: "
					<< _nodeType << endl;
		break;
	}
}

/* --------------------- QUEUES MANAGEMENT METHODS ----------------------- */

/**
 * Determine whether to add the message to the processing queue or to the service queue
 */
void NodeProcessingSystem::queueMessage(Message* msg) {
	if (_DEBUG)
		cout << "NodeProcessingSystem::queueMessage" << endl;

	switch (_nodeType) {
	case NODETYPE_SINK:
		switch (msg->getType()) {
		case MESSAGETYPE_START_CTA:
		case MESSAGETYPE_START_ATC:
		case MESSAGETYPE_DATA_CTA:
		case MESSAGETYPE_DATA_ATC:
		case MESSAGETYPE_COOP_INFO_REQ:
		case MESSAGETYPE_COOP_INFO:
		case MESSAGETYPE_STOP:
			_serviceQueue.push(msg);
			break;
		default:
			if (_DEBUG)
				cerr
						<< "NodeProcessingSystem::queueMessage: unexpected message type: "
						<< msg->getTypeStr() << " for a node of type: "
						<< _nodeType << endl;
			break;
		}
		break;
	case NODETYPE_CAMERA:
		switch (msg->getType()) {
		case MESSAGETYPE_START_CTA:
		case MESSAGETYPE_START_ATC:
			_processingQueue.push(msg);
			break;
		case MESSAGETYPE_COOP_INFO_REQ:
		case MESSAGETYPE_STOP:
			_serviceQueue.push(msg);
			break;
		case MESSAGETYPE_DATA_ATC:
			if (_DEBUG)
				cout
						<< "NodeProcessingSystem::queueMessage: queuing a DATA_ATC_MESSAGE to offloading manager "
						<< endl;
			_offloadingManager->queueATCMsg((DataATCMsg*) msg);
			if (_DEBUG)
				cout
						<< "NodeProcessingSystem::queueMessage: queuing a DATA_ATC_MESSAGE to offloading manager: done "
						<< endl;

			break;
		default:
			if (_DEBUG)
				cerr
						<< "NodeProcessingSystem::queueMessage: unexpected message type: "
						<< msg->getTypeStr() << " for a node of type: "
						<< _nodeType << endl;
			break;
		}
		break;
	case NODETYPE_COOPERATOR:
		switch (msg->getType()) {
		case MESSAGETYPE_START_ATC:
			_serviceQueue.push(msg);
			break;
		case MESSAGETYPE_DATA_CTA:
			_processingQueue.push(msg);
			break;
		case MESSAGETYPE_STOP:
			_serviceQueue.push(msg);
			break;
		default:
			if (_DEBUG)
				cerr
						<< "NodeProcessingSystem::queueMessage: unexpected message type: "
						<< msg->getTypeStr() << " for a node of type: "
						<< _nodeType << endl;
			break;
		}
		break;
	default:
		if (_DEBUG)
			cerr << "NodeProcessingSystem::queueMessage: unexpected node type: "
					<< _nodeType << endl;
		break;
	}

}

void NodeProcessingSystem::_processingThreadHandler() {

	if (_DEBUG)
		cout << "NodeProcessingSystem::_processingThreadHandler" << endl;

	Message* msg;
	while (1) {
		_processingQueue.wait_and_front(msg);

		switch (_nodeType) {
		case NODETYPE_CAMERA:
			_cameraProcessing(msg);
			break;
		case NODETYPE_COOPERATOR:
			_cooperatorProcessing(msg);
			break;
		default:
			if (_DEBUG)
				cerr
						<< "NodeProcessingSystem::_processingThreadHandler: unexpected node type: "
						<< _nodeType << endl;
		}

		/* Pop at the end of all the operations to free the queue
		 * as late as possible and guarantee that only one operation is in the queue */
		_processingQueue.pop();

	}

}

void NodeProcessingSystem::_serviceThreadHandler() {

	if (_DEBUG)
		cout << "NodeProcessingSystem::_serviceThreadHandler" << endl;

	Message* msg = NULL;
	while (1) {
		_serviceQueue.wait_and_pop(msg);

		switch (_nodeType) {
		case NODETYPE_SINK:
			_sinkService(msg);
			break;
		case NODETYPE_CAMERA:
			_cameraService(msg);
			break;
		case NODETYPE_COOPERATOR:
			_cooperatorService(msg);
			break;
		default:
			if (_DEBUG)
				cerr
						<< "NodeProcessingSystem::_serviceThreadHandler: unexpected node type: "
						<< _nodeType << endl;
		}

	}

}

void NodeProcessingSystem::_cameraProcessing(Message* msg) {

	switch (msg->getType()) {
	case MESSAGETYPE_START_CTA: {
		StartCTAMsg* message = (StartCTAMsg*) msg;

		if (message->getLinkType() == LINKTYPE_TCP) {
			float tcTime = _nodeNetworkSystem->setWifiBandwidth(
					message->getWifiBandwidth());
			if (tcTime && _DEBUG > 1)
				cout << "NodeProcessingSystem::_cameraProcessing: tcTime: "
						<< tcTime << endl;
		}

		if (_DEBUG)
			cout << "NodeProcessingSystem::_cameraProcessing: acquiring image "
					<< endl;

		_gpios[1]->setValue(BlackLib::high); //Set acquisition pin

		cv::Mat image;
		float acquisitionTime = _acquireImage(image,
				message->getOperativeMode(), message->getImageSource());

		if (_DEBUG > 1)
			cout << "NodeProcessingSystem::_cameraProcessing: acquisitionTime: "
					<< acquisitionTime << endl;

		switch (message->getOperativeMode()) {
		case OPERATIVEMODE_OBJECT:
			/* Convert to grayscale */
			cv::cvtColor(image, image, CV_BGR2GRAY);
			break;
		case OPERATIVEMODE_PKLOT:
			/* Send RGB image */
			break;
		}

		_gpios[1]->setValue(BlackLib::low);  //Reset acquisition pin

		int64 firstStartTick;

		unsigned char numSlices = message->getNumSlices();
		unsigned char qf = message->getQualityFactor();
		for (unsigned char sliceIdx = 0; sliceIdx < numSlices; ++sliceIdx) {
			if (_DEBUG)
				cout
						<< "NodeProcessingSystem::_cameraProcessing: processing slice "
						<< (int) sliceIdx << endl;

			_gpios[4]->setValue(BlackLib::high); //Set coding pin

			cv::Mat slice;
			cv::Size topLeftCorner = _imageSlice(image, slice, sliceIdx,
					numSlices);

			if (_DEBUG)
				cout
						<< "NodeProcessingSystem::_cameraProcessing: encoding slice "
						<< endl;
			Bitstream encodedSliceBitstream;
			float encodingTime = _jpegEncode(slice, encodedSliceBitstream, qf);

			if (_DEBUG)
				cout
						<< "NodeProcessingSystem::_cameraProcessing: encoding time: "
						<< encodingTime << endl;

			_gpios[4]->setValue(BlackLib::low);  //Reset coding pin

			/* Written by NetworkSystem */
			const float transmissionTime = 0;
			if (_DEBUG)
				cout
						<< "NodeProcessingSystem::_cameraProcessing: sending slice "
						<< endl;

			if (sliceIdx == 0){
				firstStartTick = cv::getTickCount();
			}

			DataCTAMsg* reply = new DataCTAMsg(NetworkNode::getMyself(),
					NetworkNode::getSink(), msg->getLinkType(), _frameId,
					sliceIdx, numSlices , topLeftCorner.width, topLeftCorner.height,
					encodedSliceBitstream.size(), encodingTime,
					transmissionTime, encodedSliceBitstream,
					message->getOperativeMode(),firstStartTick);

			_nodeNetworkSystem->sendMessage(reply);

		}

		++_frameId; //Expected to be the same for all the slices of a single shot

		if (_oneShot) {
			NodeNetworkSystem::stopOnSendEnd();
		}

		break;
	}
	case MESSAGETYPE_START_ATC: {
		StartATCMsg* const message = (StartATCMsg*) msg;

		if (message->getLinkType() == LINKTYPE_TCP) {
			float tcTime = _nodeNetworkSystem->setWifiBandwidth(
					message->getWifiBandwidth());
			if (tcTime && _DEBUG > 1)
				cout << "NodeProcessingSystem::_cameraProcessing: tcTime: "
						<< tcTime << endl;
		}

		_gpios[1]->setValue(BlackLib::high); //Set acquisition pin
		cv::Mat image;
		float acquisitionTime = _acquireImage(image,
				message->getOperativeMode(), message->getImageSource());
		if (_DEBUG > 1)
			cout << "NodeProcessingSystem::_cameraProcessing: acquisitionTime: "
					<< acquisitionTime << endl;

		/* When NBS activated need to cut the acquired image */
		cv::Size topLeft = message->getTopLeft();
		cv::Size bottomRight = message->getBottomRight();

		image = image.rowRange(topLeft.height,bottomRight.height).colRange(topLeft.width,bottomRight.width);

		_gpios[1]->setValue(BlackLib::low);  //Reset acquisition pin

		cv::Size wholeImageSize = image.size();
		vector<cv::KeyPoint> keypoints;
		cv::Mat features;

		float detectionTime = 0;
		float descriptionTime = 0;
		unsigned short totFeatures = 0;

		if (message->getNumCooperators() > 0
				&& (message->getNumCooperators()
						<= _offloadingManager->getNumAvailableCoop())) {
			/* --- DATC --- */

			double time = cv::getCPUTickCount();
			cv::cvtColor(image, image, CV_BGR2GRAY);
			if (_DEBUG > 1)
				cout
						<< "NodeProcessingSystem::_cameraProcessing: DATC: cvtColor time: "
						<< (cv::getCPUTickCount() - time)
								/ cv::getTickFrequency() << endl;

			/* Split loads. Returns camera load */
			time = cv::getCPUTickCount();
			image = _offloadingManager->splitLoads(image,
					message->getNumCooperators());
			if (_DEBUG > 1)
				cout
						<< "NodeProcessingSystem::_cameraProcessing: DATC: splitLoads time: "
						<< (cv::getCPUTickCount() - time)
								/ cv::getTickFrequency() << endl;

			if (_DEBUG)
				cout
						<< "NodeProcessingSystem::_cameraProcessing: DATC: self image: "
						<< image.size() << endl;

			/* Send StartDATCMsg to cooperators */
			time = cv::getCPUTickCount();
			_offloadingManager->transmitStartATC(message);
			if (_DEBUG > 1)
				cout
						<< "NodeProcessingSystem::_cameraProcessing: DATC: transmitStartATC time: "
						<< (cv::getCPUTickCount() - time)
								/ cv::getTickFrequency() << endl;

			/* Transmit Loads */
			time = cv::getCPUTickCount();
			_offloadingManager->transmitLoads();
			if (_DEBUG > 1)
				cout
						<< "NodeProcessingSystem::_cameraProcessing: DATC: transmitLoads time: "
						<< (cv::getCPUTickCount() - time)
								/ cv::getTickFrequency() << endl;

			/* Compute keypoints on camera load */
			detectionTime = _briskExtractKeypoints(image, keypoints,
					message->getDetectorThreshold());

			if (_DEBUG > 1)
				cout
						<< "NodeProcessingSystem::_cameraProcessing: DATC: _briskExtractKeypoints time: "
						<< detectionTime << endl;

			if (_DEBUG > 1)
				cout
						<< "NodeProcessingSystem::_cameraProcessing: DATC: detected keypoints: "
						<< keypoints.size() << endl;

			unsigned short maxNumFeat = message->getMaxNumFeat();
			descriptionTime = _briskExtractFeatures(image, keypoints, features,
					maxNumFeat);

			if (_DEBUG > 1)
				cout
						<< "NodeProcessingSystem::_cameraProcessing: DATC: _briskExtractFeatures time: "
						<< descriptionTime << endl;

			if (_DEBUG > 1)
				cout
						<< "NodeProcessingSystem::_cameraProcessing: DATC: preserved keypoints: "
						<< keypoints.size() << endl;

			totFeatures = features.rows;

			if (_DEBUG > 1)
				cout
						<< "NodeProcessingSystem::_cameraProcessing: DATC: extracted features: "
						<< totFeatures << endl;

			Bitstream kptsBistream;
			Bitstream featBitstream;

			time = cv::getCPUTickCount();
			codingParams parms(false);
			_encoder->encodeKeyPoints(parms, keypoints, kptsBistream,
					cv::Size(), false);
			_encoder->encodeDescriptors(parms, DESCRIPTORTYPE_BRISK, features,
					featBitstream);

			/* Send data to offloading manager */
			DataATCMsg *cameraContribute = new DataATCMsg(
					NetworkNode::getMyself(), NetworkNode::getMyself(),
					LINKTYPE_UNDEF, _frameId, 0, 1, detectionTime,
					descriptionTime, 0, 0, 0, keypoints.size(),
					keypoints.size(), wholeImageSize.width,
					wholeImageSize.height, featBitstream, kptsBistream,
					message->getOperativeMode(),0);

			if (_DEBUG > 1)
				cout
						<< "NodeProcessingSystem::_cameraProcessing: DATC: create message for offloading manager time: "
						<< (cv::getCPUTickCount() - time)
								/ cv::getTickFrequency() << endl;

			if (_DEBUG)
				cout
						<< "NodeProcessingSystem::_cameraProcessing: queuing a DATA_ATC_MESSAGE to offloading manager "
						<< endl;

			time = cv::getCPUTickCount();
			_offloadingManager->queueATCMsg(cameraContribute);
			if (_DEBUG > 1)
				cout
						<< "NodeProcessingSystem::_cameraProcessing: DATC: queuing message to offloading manager time: "
						<< (cv::getCPUTickCount() - time)
								/ cv::getTickFrequency() << endl;

			/* Waiting for cooperator response */
			time = cv::getCPUTickCount();
			_offloadingManager->waitForCooperators();
			if (_DEBUG > 1)
				cout
						<< "NodeProcessingSystem::_cameraProcessing: DATC: waiting for cooperators time: "
						<< (cv::getCPUTickCount() - time)
								/ cv::getTickFrequency() << endl;

			keypoints = _offloadingManager->getFullKeypoints();
			features = _offloadingManager->getFullFeatures();

			if (_DEBUG) {
				cout
						<< "NodeProcessingSystem::_cameraProcessing: DATC: total keypoints: "
						<< keypoints.size() << endl;
				cout
						<< "NodeProcessingSystem::_cameraProcessing: DATC: total features: "
						<< features.rows << endl;
			}

			time = cv::getCPUTickCount();
			_briskExtractor->cutFeatures(keypoints, features, maxNumFeat);
			if (_DEBUG > 1)
				cout
						<< "NodeProcessingSystem::_cameraProcessing: DATC: cutting features time: "
						<< (cv::getCPUTickCount() - time)
								/ cv::getTickFrequency() << endl;

		} else {
			/* No cooperators */

			unsigned short maxNumFeat = message->getMaxNumFeat();

			/* Depending on the operative mode extract BRISK features or HUE HIST features */
			switch (message->getOperativeMode()) {
			case OPERATIVEMODE_OBJECT:

				cv::cvtColor(image, image, CV_BGR2GRAY);

				_gpios[2]->setValue(BlackLib::high);  //Set detection pin

				detectionTime = _briskExtractKeypoints(image, keypoints,
						message->getDetectorThreshold());

				_gpios[2]->setValue(BlackLib::low);  //Reset detection pin

				if (_DEBUG > 1)
					cout
							<< "NodeProcessingSystem::_cameraProcessing: ATC: extracted keypoints: "
							<< keypoints.size() << endl;

				/*
				 cout << "image.rows: " << image.rows
				 << " image.cols: " << image.cols << " image.type: "
				 << image.type() << endl;
				 */

				/*
				 cv::imshow("pre brisk image",image);
				 cv::waitKey(1);
				 */

				_gpios[3]->setValue(BlackLib::high);  //Set extraction pin

				descriptionTime = _briskExtractFeatures(image, keypoints,
						features, maxNumFeat);

				_gpios[3]->setValue(BlackLib::low);  //Reset extraction pin

				break;
			case OPERATIVEMODE_PKLOT:

				detectionTime = 0;
				_decoder->decodeKeyPoints(message->getKeypoints(), keypoints,
						message->getBottomRight(), true);

				if (_DEBUG > 1)
					cout
							<< "NodeProcessingSystem::_cameraProcessing: ATC: extracted keypoints: "
							<< keypoints.size() << endl;

				_gpios[3]->setValue(BlackLib::high);  //Set extraction pin

				cv::Mat components[3];
				cv::Mat imageHsv;
				cv::cvtColor(image, imageHsv, CV_BGR2HSV);
				cv::split(imageHsv, components);
				cv::Mat hue = components[0];
				cout << "hue: type: " << hue.type() << " channels: "
						<< hue.channels() << endl;

				descriptionTime = _histExtractFeatures(hue, keypoints, features,
						message->getBinShift());
				_gpios[3]->setValue(BlackLib::low);  //Reset extraction pin

				//cout << "features: " << features.row(0) << endl;
				cout
						<< "NodeProcessingSystem::_cameraProcessing: PKLot features extraction time: "
						<< descriptionTime << endl;

				break;
			}

			totFeatures = keypoints.size();

			if (_DEBUG > 1)
				cout
						<< "NodeProcessingSystem::_cameraProcessing: ATC: extracted features: "
						<< totFeatures << endl;
		}

		switch (message->getOperativeMode()) {
		case OPERATIVEMODE_OBJECT:
			_sendATC(keypoints, features, message->getSrc(),
					message->getLinkType(), wholeImageSize,
					message->getNumFeatPerBlock(),
					message->getKeypointsCoding(), detectionTime,
					descriptionTime, DESCRIPTORTYPE_BRISK,
					codingParams(message->getFeaturesCoding()),
					OPERATIVEMODE_OBJECT);
			break;
		case OPERATIVEMODE_PKLOT:
			_sendATC(keypoints, features, message->getSrc(),
					message->getLinkType(), wholeImageSize,
					message->getNumFeatPerBlock(),
					message->getKeypointsCoding(), detectionTime,
					descriptionTime, DESCRIPTORTYPE_HIST,
					HIST_codingParams(message->getFeaturesCoding(),
							message->getValShift()), OPERATIVEMODE_PKLOT);
			break;
		}

		if (_oneShot) {
			NodeNetworkSystem::stopOnSendEnd();
		}

		break;
	}
	default:
		if (_DEBUG)
			cerr
					<< "NodeProcessingSystem::_cameraProcessing: unexpected message type: "
					<< msg->getType() << endl;
	}

	delete msg;
}

void NodeProcessingSystem::_cooperatorProcessing(Message* msg) {

	switch (msg->getType()) {

	case MESSAGETYPE_DATA_CTA: {
		DataCTAMsg* message = (DataCTAMsg*) msg;

		//TODO Send ack to the camera to estimate the transfer bandwidth

		/* Decode the image */
		double time = cv::getCPUTickCount();
		cv::Mat image = cv::imdecode(*(message->getData()),
				CV_LOAD_IMAGE_GRAYSCALE);
		if (_DEBUG > 1) {
			cout
					<< "NodeProcessingSystem::_cooperatorProcessing: imdecode time: "
					<< (cv::getCPUTickCount() - time) / cv::getTickFrequency()
					<< endl;
		}

		if (_DEBUG) {
			cout << "NodeProcessingSystem::_cooperatorProcessing: image size "
					<< image.size() << endl;
		}

		vector<cv::KeyPoint> keypoints;
		float detectionTime = _briskExtractKeypoints(image, keypoints,
				_datcParams.detectionThreshold);

		if (_DEBUG > 1)
			cout
					<< "NodeProcessingSystem::_cooperatorProcessing: DATC: extracted keypoints: "
					<< keypoints.size() << endl;

		unsigned short maxNumFeat = _datcParams.maxNumFeatures;
		cv::Mat features;
		float descriptionTime = _briskExtractFeatures(image, keypoints,
				features, maxNumFeat);

		unsigned short totFeatures = keypoints.size();

		if (_DEBUG > 1)
			cout
					<< "NodeProcessingSystem::_cooperatorProcessing: DATC: extracted features: "
					<< totFeatures << endl;

		_sendATC(keypoints, features, message->getSrc(), message->getLinkType(),
				image.size(), totFeatures, false, detectionTime,
				descriptionTime, DESCRIPTORTYPE_BRISK, codingParams(false),
				message->getOperativeMode());

		break;
	}

	default:
		if (_DEBUG)
			cerr
					<< "NodeProcessingSystem::_cooperatorProcessing: unexpected message type: "
					<< msg->getType() << endl;
		break;
	}

	delete msg;

}

/**
 * Forward to the gui all the packets whose destination is the sink.
 * If the sender is the gui there is something wrong.
 */
void NodeProcessingSystem::_sinkService(Message* msg) {

	if (msg->getDst() == NetworkNode::getSink()) {
		if (msg->getSrc() == NetworkNode::getGui()) {
			/* A message from gui to sink is not meaningful up to now */
			if (_DEBUG) {
				cerr
						<< "NodeProcessingSystem::_sinkService: unexpected message from GUI to SINK: "
						<< endl;
				return;
			}
		} else {
			/* Forward packets from cameras to the gui */
			msg->setDst(NetworkNode::getGui());
			msg->setLinkType(LINKTYPE_TCP);
		}
	}

	if (msg->getSrc() == NetworkNode::getGui()) {
		/* Masquerade the gui when sending messages to the cameras. */
		msg->setSrc(NetworkNode::getSink());
	}

	_nodeNetworkSystem->sendMessage(msg);

}
void NodeProcessingSystem::_cameraService(Message* msg) {

	switch (msg->getType()) {
	case MESSAGETYPE_COOP_INFO_REQ: {
		CoopInfoMsg* replyMsg = new CoopInfoMsg(NetworkNode::getMyself(),
				NetworkNode::getSink(), msg->getLinkType(),
				_offloadingManager->getCooperatorsIds());
		_nodeNetworkSystem->sendMessage(replyMsg);
		break;
	}
	case MESSAGETYPE_STOP: {
		_nodeNetworkSystem->flushOutgoingMessages();
		break;
	}
	default: {
		if (_DEBUG)
			cerr
					<< "NodeProcessingSystem::_cameraService: unexpected message type: "
					<< msg->getType() << endl;
	}
	}

	delete msg;

}

void NodeProcessingSystem::_cooperatorService(Message* msg) {

	if (_DEBUG)
		cout << "NodeProcessingSystem::_cooperatorService" << endl;

	switch (msg->getType()) {
	case MESSAGETYPE_START_ATC: {
		StartATCMsg* message = (StartATCMsg*) msg;

		/**
		 * Save the DATC parameters for the specific camera
		 */
		_datcParams.detectionThreshold = message->getDetectorThreshold();
		_datcParams.maxNumFeatures = message->getMaxNumFeat();
		break;
	}
	case MESSAGETYPE_STOP: {
		//TODO clean processing queue from messages coming form the camera that forwards the stop;
		break;
	}

	default:
		if (_DEBUG)
			cerr
					<< "NodeProcessingSystem::_cooperatorService: unexpected message type: "
					<< msg->getType() << endl;
	}

	delete msg;
}

/* --------------------- PROCESSING METHODS ----------------------- */

float NodeProcessingSystem::_acquireImage(cv::Mat& image,
		const OperativeMode opMode_, const ImageSource imgSource_) {
	int64 time = cv::getTickCount();

	switch (imgSource_) {
	case IMAGESOURCE_LIVE: {
		_imageAcquisition->takeLiveFrame(image);
		break;
	}
	case IMAGESOURCE_REC: {
		switch (opMode_) {
		case OPERATIVEMODE_OBJECT: {
			_imageAcquisition->takeObjFrame(image);
			break;
		}
		case OPERATIVEMODE_PKLOT: {
			_imageAcquisition->takePklotFrame(image);
			break;
		}

		default:
			cerr
					<< "NodeProcessingSystem::_acquireImage: opMode_ case not managed:"
					<< opMode_ << endl;
		}
		break;
	}
	default:
		cerr
				<< "NodeProcessingSystem::_acquireImage: imgSource_ case not managed:"
				<< imgSource_ << endl;
	}

	if (_DEBUG > 1) {
		cout << "NodeProcessingSystem::_acquireImage: image.rows: "
				<< image.rows << endl;
		cout << "NodeProcessingSystem::_acquireImage: image.cols: "
				<< image.cols << endl;
		cout << "NodeProcessingSystem::_acquireImage: image.channels(): "
				<< image.channels() << endl;
		cout << "NodeProcessingSystem::_acquireImage: image.type(): "
				<< image.type() << endl;
	}
	return (double)(cv::getTickCount() - time) / cv::getTickFrequency();
}

cv::Size NodeProcessingSystem::_imageSlice(const cv::Mat& src, cv::Mat& dst,
		unsigned char sliceIdx, unsigned char numSlices) const {

	unsigned short rowsPerSlice = ceil((float) src.rows / (float) numSlices);
	unsigned short firstRow = sliceIdx * rowsPerSlice;
	unsigned short lastRow = firstRow + rowsPerSlice;
	lastRow = lastRow > src.rows ? src.rows : lastRow;

	cv::Size topLeftCorner(0, firstRow);

	dst = src.rowRange(firstRow, lastRow);

	return topLeftCorner;
}

float NodeProcessingSystem::_jpegEncode(const cv::Mat& image,
		Bitstream& bitstream, unsigned char qf) const {
	int64 time = cv::getTickCount();
	vector<int> param = vector<int>(2);
	param[0] = CV_IMWRITE_JPEG_QUALITY;
	param[1] = qf;
	cv::imencode(".jpg", image, bitstream, param);
	return (double)(cv::getTickCount() - time) / cv::getTickFrequency();
}

float NodeProcessingSystem::_briskExtractKeypoints(const cv::Mat& image,
		std::vector<cv::KeyPoint>& keypoints, float detectorThreshold) const {
	int64 time = cv::getTickCount();
	const detParams& parms = BRISK_detParams(detectorThreshold);
	_briskExtractor->setDetectorParameters(DETECTORTYPE_BRISK, parms);
	_briskExtractor->extractKeypoints(image, keypoints);
	return (double)(cv::getTickCount() - time) / cv::getTickFrequency();
}

float NodeProcessingSystem::_briskExtractFeatures(const cv::Mat& image,
		std::vector<cv::KeyPoint>& keypoints, cv::Mat& features,
		unsigned short maxFeatures) const {
	int64 time = cv::getTickCount();
	_briskExtractor->extractFeatures(image, keypoints, features);
	_briskExtractor->cutFeatures(keypoints, features, maxFeatures);
	return (double)(cv::getTickCount() - time) / cv::getTickFrequency();
}

float NodeProcessingSystem::_histExtractFeatures(const cv::Mat& image,
		std::vector<cv::KeyPoint>& keypoints, cv::Mat& features,
		const uchar binshift) const {
	int64 time = cv::getTickCount();

	const descParams& parms = HIST_descParams(binshift, 180);
	_histExtractor->setDescriptorParameters(DESCRIPTORTYPE_HIST, parms);
	_histExtractor->extractFeatures(image, keypoints, features);

	return (double)(cv::getTickCount() - time) / cv::getTickFrequency();
}

float NodeProcessingSystem::_encodeKeypoints(
		const std::vector<cv::KeyPoint>& keypoints, Bitstream& bitstream,
		const cv::Size imageSize, bool encodeAngles, bool encode) const {
	int64 time = cv::getTickCount();

	if (_DEBUG)
		cout << "NodeProcessingSystem::_encodeKeypoints: encoding " << endl;
	_encoder->encodeKeyPoints(codingParams(encode), keypoints, bitstream,
			imageSize, encodeAngles);

	return (double)(cv::getTickCount() - time) / cv::getTickFrequency();
}

float NodeProcessingSystem::_encodeFeatures(const cv::Mat& features,
		Bitstream& bitstream, const DescriptorType encoderType,
		const codingParams &parms) const {
	int64 time = cv::getTickCount();

	if (_DEBUG)
		cout << "NodeProcessingSystem::_encodeFeatures: encoding " << endl;

	if (_DEBUG > 1)
		cout << "NodeProcessingSystem::_encodeFeatures: features: "
				<< features.row(0).colRange(0, min(20, features.cols)) << endl;

	_encoder->encodeDescriptors(parms, encoderType, features, bitstream);

	if (_DEBUG > 1) {
		cout << "NodeProcessingSystem::_encodeFeatures: bitstream: ";
		for (int i = 0; i < min(20, (int) bitstream.size()); ++i) {
			cout << (int) bitstream[i] << " ";
		}
		cout << endl;
	}

	return (double)(cv::getTickCount() - time) / cv::getTickFrequency();
}

void NodeProcessingSystem::_sendATC(const std::vector<cv::KeyPoint>& keypoints,
		const cv::Mat& features, NetworkNode* dst, const LinkType linkType,
		const cv::Size imageSize, const unsigned short featuresPerBlock,
		const bool keypointsCoding, const float detectionTime,
		const float descriptionTime, const DescriptorType descType,
		const codingParams& codingParms, const OperativeMode opMode) {

	const unsigned short totFeatures = features.rows;

	int64 firstStartTick;

	if (totFeatures) {
		unsigned char numBlocks = ceil(
				(float) totFeatures / (float) (featuresPerBlock));

		for (unsigned char blockIdx = 0; blockIdx < numBlocks; ++blockIdx) {

			/* 0 - N indices */
			unsigned short beginIdx = blockIdx * featuresPerBlock;
			unsigned short endIdx = beginIdx + featuresPerBlock;
			endIdx = endIdx > totFeatures ? totFeatures : endIdx;

			_gpios[4]->setValue(BlackLib::high); //set coding pin

			vector<cv::KeyPoint> keypointsPerBlock(keypoints.begin() + beginIdx,
					keypoints.begin() + endIdx);
			unsigned short numKeypointsPerBlock = keypointsPerBlock.size();
			Bitstream keypointsBitstreamPerBlock;
			const bool encodeAngles = true;
			float keypointEncodingTime = _encodeKeypoints(keypointsPerBlock,
					keypointsBitstreamPerBlock, imageSize, encodeAngles,
					keypointsCoding);

			cv::Mat featuresPerBlock = features.rowRange(beginIdx, endIdx);
			unsigned short numFeaturesPerBlock = featuresPerBlock.rows;
			Bitstream featuresBitstreamPerBlock;
			float featuresEncodingTime = _encodeFeatures(featuresPerBlock,
					featuresBitstreamPerBlock, descType, codingParms);

			_gpios[4]->setValue(BlackLib::low); //reset coding pin

			const float transmissionTime = 0;

			if (_DEBUG > 1)
				cout
						<< "NodeProcessingSystem::_sendATC: ATC: keypoints in block "
						<< (int) blockIdx << ": " << numKeypointsPerBlock
						<< endl;

			if (_DEBUG > 1)
				cout
						<< "NodeProcessingSystem::_sendATC: ATC: features in block "
						<< (int) blockIdx << ": " << numFeaturesPerBlock
						<< endl;
			if (blockIdx == 0){
				firstStartTick = cv::getTickCount();
			}
			DataATCMsg *reply = new DataATCMsg(NetworkNode::getMyself(), dst,
					linkType, _frameId, blockIdx, numBlocks, detectionTime,
					descriptionTime, keypointEncodingTime, featuresEncodingTime,
					transmissionTime, numFeaturesPerBlock, numKeypointsPerBlock,
					imageSize.width, imageSize.height,
					featuresBitstreamPerBlock, keypointsBitstreamPerBlock,
					opMode,firstStartTick);

			_nodeNetworkSystem->sendMessage(reply);
		}

		++_frameId; // expected to be the same for all the subblocks of features in a single shot

	} else {
		const unsigned short blockIdx = 0;
		const unsigned short numBlocks = 1;
		const float keypointEncodingTime = 0;
		const float featuresEncodingTime = 0;
		const float transmissionTime = 0;
		const unsigned short numFeaturesPerBlock = 0;
		const unsigned short numKeypointsPerBlock = 0;
		const Bitstream featuresBitstreamPerBlock;
		const Bitstream keypointsBitstreamPerBlock;
		DataATCMsg *reply = new DataATCMsg(NetworkNode::getMyself(), dst,
				linkType, _frameId++, blockIdx, numBlocks, detectionTime,
				descriptionTime, keypointEncodingTime, featuresEncodingTime,
				transmissionTime, numFeaturesPerBlock, numKeypointsPerBlock,
				imageSize.width, imageSize.height, featuresBitstreamPerBlock,
				keypointsBitstreamPerBlock, opMode,0);

		_nodeNetworkSystem->sendMessage(reply);

	}

}

/* --------------------- INITIALIZATION METHODS ----------------------- */
void NodeProcessingSystem::_initCamera(
		const CameraParameters& cameraParameters) {

	/* Initialize camera */
	_imageAcquisition = ImageAcquisition::getInstance(cameraParameters);
	/* Initialize extractor, encoder */
	const BRISK_detParams briskDetPrms(60, 4);
	const BRISK_descParams briskDescPrms;
	_briskExtractor = new VisualFeatureExtraction(DETECTORTYPE_BRISK,
			DESCRIPTORTYPE_BRISK, briskDetPrms, briskDescPrms);

	const detParams detParms;
	const HIST_descParams histDescPrms;
	_histExtractor = new VisualFeatureExtraction(DETECTORTYPE_NONE,
			DESCRIPTORTYPE_HIST, detParms, histDescPrms);

	_encoder = VisualFeatureEncoding::getInstance();

	_decoder = VisualFeatureDecoding::getInstance();

	_offloadingManager = OffloadingManager::getInstance(this,
			_nodeNetworkSystem);

	_nodeNetworkSystem->prepareWifiBandwidthControl(
			NetworkNode::getSink()->getIpAddrString(),
			NetworkNode::getSink()->getIpPort());

}

void NodeProcessingSystem::_initSink() {
	/* Nothing to do */
}

void NodeProcessingSystem::_initCooperator() {

	/* Initialize extractor, encoder */
	BRISK_detParams briskDetPrms;
	BRISK_descParams briskDescPrms;
	_briskExtractor = new VisualFeatureExtraction(DETECTORTYPE_BRISK,
			DESCRIPTORTYPE_BRISK, briskDetPrms, briskDescPrms);

	detParams detParms;
	HIST_descParams histDescPrms;
	_histExtractor = new VisualFeatureExtraction(DETECTORTYPE_NONE,
			DESCRIPTORTYPE_HIST, detParms, histDescPrms);

	_encoder = VisualFeatureEncoding::getInstance();

}
