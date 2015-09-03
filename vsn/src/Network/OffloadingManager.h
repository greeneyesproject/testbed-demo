/*
 * OffloadingManager.h
 *
 *  Created on: 19/set/2014
 *      Author: greeneyes
 */

#ifndef SRC_NETWORK_OFFLOADINGMANAGER_H_
#define SRC_NETWORK_OFFLOADINGMANAGER_H_

#include <Network/Cooperator.h>
#include <Network/Queue.h>

#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <opencv2/features2d/features2d.hpp>

#include <vector>

class NodeNetworkSystem;
class NodeProcessingSystem;
class Connection;
class StartATCMsg;
class DataATCMsg;
class VisualFeatureDecoding;
class Session;
class Message;
class NetworkNode;

#define OFFLOADING_F (float)1514
#define OFFLOADING_H (float)66

class OffloadingManager {
public:
	OffloadingManager(NodeProcessingSystem* nps, NodeNetworkSystem* nns);

	static OffloadingManager* getInstance(NodeProcessingSystem* nps,
			NodeNetworkSystem* nns);

	static OffloadingManager* getInstance() {
		return _instance;
	}

	//add keypoints and features from cooperators
	void addKeypointsAndFeatures(std::vector<cv::KeyPoint>& kpts,
			cv::Mat& features, Session* session, double detTime,
			double descTime, double kencTime, double fencTime);

	//reset variables and keep track of progresses
	void createOffloadingTask(int num_cooperators);

	void addCooperator(NetworkNode* c);
	void removeCooperator(NetworkNode* c);
	cv::Mat computeLoads(cv::Mat& image);
	void transmitLoads();
	void transmitStartATC(StartATCMsg* const msg);


	void timerExpired(const boost::system::error_code& error);
	void startTimer();
	void runThread();
	int getNumAvailableCoop();

	void queueATCMsg(DataATCMsg* msg) {
		_offloadingQueue.push(msg);
	}

	void waitForCooperators() {
		boost::mutex::scoped_lock lock(_coopRespWaitMutex);
		while (_waitingForCoopsResp) {
			_coopRespWaitCondVar.wait_for(lock, _waitPeriod);
		}
	}

	std::vector<cv::KeyPoint> getFullKeypoints() const {
		return keypoint_buffer;
	}
	cv::Mat getFullFeatures() const {
		return features_buffer;
	}

	std::vector<unsigned char> getCooperatorsIds() const;

	cv::Mat splitLoads(const cv::Mat& wholeImage, const unsigned char numCoops);

private:

	const static unsigned char _DEBUG = 2;

	const static unsigned short _timerExpiryTime = 2; //[s]

	boost::chrono::milliseconds _waitPeriod;

	static OffloadingManager* _instance;

	bool _waitingForCoopsResp;

	mutable boost::mutex _coopArrayMutex;

	mutable boost::mutex _coopRespWaitMutex;
	boost::condition_variable _coopRespWaitCondVar;

	unsigned char _cooperatorsToUse;
	unsigned char _receivedCooperators;
	std::vector<Cooperator> cooperators;

	unsigned char _frameId;

	void _probeLinks();

	void _sortCooperators();

	void _startIncomingThread() {
		boost::thread(&OffloadingManager::_incomingThread, this);
	}

	void _incomingThread();

	NodeProcessingSystem* _nodeProcessingSystem;
	NodeNetworkSystem* _nodeNetworkSystem;

	VisualFeatureDecoding* _decoder;

	//used to store keypoints and features from cooperators
	std::vector<cv::KeyPoint> keypoint_buffer;
	cv::Mat features_buffer;
	boost::thread r_thread;
	double camDetTime, camDescTime, camkEncTime, camfEncTime;

	boost::asio::io_service io;
	//deadline timer for receiving data from all cooperators
	boost::asio::deadline_timer _timer;
	boost::asio::io_service::work work;

	Queue<DataATCMsg> _offloadingQueue;

	OperativeMode _lastOpMode;

};

#endif /* OFFLOADINGMANAGER_H_ */
