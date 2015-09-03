#include <Network/OffloadingManager.h>
#include <Network/NodeNetworkSystem.h>
#include <Network/NetworkNode.h>
#include <Network/NetworkNode.h>
#include <Network/Tcp/Session.h>
#include <Messages/DataATCMsg.h>
#include <Messages/DataCTAMsg.h>
#include <Messages/StartATCMsg.h>
#include <Multimedia/VisualFeatureDecoding.h>
#include <vector>
#include <opencv2/highgui/highgui.hpp>

using namespace std;
using namespace cv;

OffloadingManager* OffloadingManager::_instance = NULL;

OffloadingManager::OffloadingManager(NodeProcessingSystem* nps,
		NodeNetworkSystem* nns) :
		_waitPeriod(1000), io(), _timer(io), work(io) {
	_cooperatorsToUse = 0;
	_receivedCooperators = 0;
	_nodeProcessingSystem = nps;

	_nodeNetworkSystem = nns;
	startTimer();
	_frameId = 0;
	_decoder = VisualFeatureDecoding::getInstance();
}

OffloadingManager* OffloadingManager::getInstance(NodeProcessingSystem* nps,
		NodeNetworkSystem* nns) {
	if (_instance == NULL) {
		_instance = new OffloadingManager(nps, nns);
	}
	return _instance;
}

bool bandwidthComp(Cooperator i, Cooperator j) {

	return (i.bandwidth > j.bandwidth);
}

int OffloadingManager::getNumAvailableCoop() {
	return cooperators.size();
}

/**
 * Add a cooperator to cooperators[] array
 */
void OffloadingManager::addCooperator(NetworkNode* cooperator) {

	/* If the cooperator was already in the cooperators[] array remove it */
	removeCooperator(cooperator);

	boost::mutex::scoped_lock lock(_coopArrayMutex);

	Cooperator temp_coop;
	temp_coop.node = cooperator;
	cooperators.push_back(temp_coop);

}

void OffloadingManager::removeCooperator(NetworkNode* cooperator) {

	boost::mutex::scoped_lock lock(_coopArrayMutex);

	for (uint i = 0; i < cooperators.size(); i++) {
		Cooperator temp_coop = cooperators[i];
		if (temp_coop.node == cooperator) {
			cooperators.erase(cooperators.begin() + i);
		}
	}


}

Mat OffloadingManager::computeLoads(Mat& image) {
	Mat myLoad;
	double camera_load;

	double v = cooperators[0].CPUspeed;
	double ovl = 15 / 100.0;

	double c_2, c_3, c_4;
	double s_0, s_1, s_2, s_3
	//,s_4
			;

	switch (_cooperatorsToUse) {
	case 1:
		camera_load = (0.5 - ovl / 2);
		cooperators[0].load = (ovl / 2 + 0.5);

		s_0 = camera_load;
		s_1 = camera_load + cooperators[0].load;

		cout << "image size: " << image.rows << "x" << image.cols << endl;
		cout << "a0: " << camera_load << ". cols from " << 0 << " to "
				<< ceil((camera_load + ovl) * image.cols) << endl;
		cout << "a1: " << cooperators[0].load << ". cols from "
				<< ceil(camera_load * image.cols) << " to " << image.cols
				<< endl;

		myLoad = Mat(image, Range(0, image.rows),
				Range(0, ceil((camera_load + ovl) * image.cols))).clone();
		cooperators[0].image_slice = Mat(image, Range(0, image.rows),
				Range(ceil(camera_load * image.cols), image.cols)).clone();
		cooperators[0].col_offset = ceil(camera_load * image.cols);
		break;

	case 2:
		c_2 = cooperators[1].bandwidth;
		camera_load =
				-(OFFLOADING_F * c_2 * ovl - OFFLOADING_F * c_2
						+ OFFLOADING_F * ovl * v + OFFLOADING_H * ovl * v)
						/ (3 * OFFLOADING_F * c_2 + OFFLOADING_F * v
								+ OFFLOADING_H * v);
		cooperators[0].load =
				(OFFLOADING_F * c_2 + OFFLOADING_F * v + OFFLOADING_H * v
						- OFFLOADING_F * c_2 * ovl + OFFLOADING_F * ovl * v
						+ OFFLOADING_H * ovl * v)
						/ (3 * OFFLOADING_F * c_2 + OFFLOADING_F * v
								+ OFFLOADING_H * v);
		cooperators[1].load =
				(OFFLOADING_F * c_2 + 2 * OFFLOADING_F * c_2 * ovl)
						/ (3 * OFFLOADING_F * c_2 + OFFLOADING_F * v
								+ OFFLOADING_H * v);

		s_0 = camera_load;
		s_1 = s_0 + cooperators[0].load;
		s_2 = s_1 + cooperators[1].load;

		cout << "image size: " << image.rows << "x" << image.cols << endl;
		cout << "a0: " << camera_load << ". cols from " << 0 << " to "
				<< ceil((s_0 + ovl) * image.cols) << endl;
		cout << "a1: " << cooperators[0].load << ". cols from "
				<< ceil(s_0 * image.cols) << " to "
				<< ceil((s_1 + ovl) * image.cols) << endl;
		cout << "a2: " << cooperators[1].load << ". cols from "
				<< ceil(s_1 * image.cols) << " to " << image.cols << endl;

		myLoad = Mat(image, Range(0, image.rows),
				Range(0, ceil((s_0 + ovl) * image.cols))).clone();
		cooperators[0].image_slice =
				Mat(image, Range(0, image.rows),
						Range(ceil(s_0 * image.cols),
								ceil((s_1 + ovl) * image.cols))).clone();
		cooperators[0].col_offset = ceil(s_0 * image.cols);
		cooperators[1].image_slice = Mat(image, Range(0, image.rows),
				Range(ceil(s_1 * image.cols), image.cols)).clone();
		cooperators[1].col_offset = ceil(s_1 * image.cols);

		break;

	case 3:
		c_2 = cooperators[1].bandwidth;
		c_3 = cooperators[2].bandwidth;
		camera_load = -(pow(OFFLOADING_F, 2) * ovl * pow(v, 2)
				+ pow(OFFLOADING_H, 2) * ovl * pow(v, 2)
				- pow(OFFLOADING_F, 2) * c_2 * c_3
				+ 2 * OFFLOADING_F * OFFLOADING_H * ovl * pow(v, 2)
				+ pow(OFFLOADING_F, 2) * c_2 * c_3 * ovl
				+ 2 * pow(OFFLOADING_F, 2) * c_2 * ovl * v
				+ pow(OFFLOADING_F, 2) * c_3 * ovl * v
				+ 2 * OFFLOADING_F * OFFLOADING_H * c_2 * ovl * v
				+ OFFLOADING_F * OFFLOADING_H * c_3 * ovl * v)
				/ (pow(OFFLOADING_F, 2) * pow(v, 2)
						+ pow(OFFLOADING_H, 2) * pow(v, 2)
						+ 2 * OFFLOADING_F * OFFLOADING_H * pow(v, 2)
						+ 4 * pow(OFFLOADING_F, 2) * c_2 * c_3
						+ 2 * pow(OFFLOADING_F, 2) * c_2 * v
						+ pow(OFFLOADING_F, 2) * c_3 * v
						+ 2 * OFFLOADING_F * OFFLOADING_H * c_2 * v
						+ OFFLOADING_F * OFFLOADING_H * c_3 * v);
		cooperators[0].load = (pow(OFFLOADING_F, 2) * pow(v, 2)
				+ pow(OFFLOADING_H, 2) * pow(v, 2)
				+ 2 * pow(OFFLOADING_F, 2) * ovl * pow(v, 2)
				+ 2 * pow(OFFLOADING_H, 2) * ovl * pow(v, 2)
				+ 2 * OFFLOADING_F * OFFLOADING_H * pow(v, 2)
				+ pow(OFFLOADING_F, 2) * c_2 * c_3
				+ pow(OFFLOADING_F, 2) * c_2 * v
				+ pow(OFFLOADING_F, 2) * c_3 * v
				+ 4 * OFFLOADING_F * OFFLOADING_H * ovl * pow(v, 2)
				- pow(OFFLOADING_F, 2) * c_2 * c_3 * ovl
				+ pow(OFFLOADING_F, 2) * c_2 * ovl * v
				+ 2 * pow(OFFLOADING_F, 2) * c_3 * ovl * v
				+ OFFLOADING_F * OFFLOADING_H * c_2 * v
				+ OFFLOADING_F * OFFLOADING_H * c_3 * v
				+ OFFLOADING_F * OFFLOADING_H * c_2 * ovl * v
				+ 2 * OFFLOADING_F * OFFLOADING_H * c_3 * ovl * v)
				/ (pow(OFFLOADING_F, 2) * pow(v, 2)
						+ pow(OFFLOADING_H, 2) * pow(v, 2)
						+ 2 * OFFLOADING_F * OFFLOADING_H * pow(v, 2)
						+ 4 * pow(OFFLOADING_F, 2) * c_2 * c_3
						+ 2 * pow(OFFLOADING_F, 2) * c_2 * v
						+ pow(OFFLOADING_F, 2) * c_3 * v
						+ 2 * OFFLOADING_F * OFFLOADING_H * c_2 * v
						+ OFFLOADING_F * OFFLOADING_H * c_3 * v);
		cooperators[1].load = -(pow(OFFLOADING_F, 2) * ovl * pow(v, 2)
				+ pow(OFFLOADING_H, 2) * ovl * pow(v, 2)
				- pow(OFFLOADING_F, 2) * c_2 * c_3
				- pow(OFFLOADING_F, 2) * c_2 * v
				+ 2 * OFFLOADING_F * OFFLOADING_H * ovl * pow(v, 2)
				+ pow(OFFLOADING_F, 2) * c_2 * c_3 * ovl
				- pow(OFFLOADING_F, 2) * c_2 * ovl * v
				+ pow(OFFLOADING_F, 2) * c_3 * ovl * v
				- OFFLOADING_F * OFFLOADING_H * c_2 * v
				- OFFLOADING_F * OFFLOADING_H * c_2 * ovl * v
				+ OFFLOADING_F * OFFLOADING_H * c_3 * ovl * v)
				/ (pow(OFFLOADING_F, 2) * pow(v, 2)
						+ pow(OFFLOADING_H, 2) * pow(v, 2)
						+ 2 * OFFLOADING_F * OFFLOADING_H * pow(v, 2)
						+ 4 * pow(OFFLOADING_F, 2) * c_2 * c_3
						+ 2 * pow(OFFLOADING_F, 2) * c_2 * v
						+ pow(OFFLOADING_F, 2) * c_3 * v
						+ 2 * OFFLOADING_F * OFFLOADING_H * c_2 * v
						+ OFFLOADING_F * OFFLOADING_H * c_3 * v);
		cooperators[2].load = (pow(OFFLOADING_F, 2) * c_2 * c_3
				+ 3 * pow(OFFLOADING_F, 2) * c_2 * c_3 * ovl)
				/ (pow(OFFLOADING_F, 2) * pow(v, 2)
						+ pow(OFFLOADING_H, 2) * pow(v, 2)
						+ 2 * OFFLOADING_F * OFFLOADING_H * pow(v, 2)
						+ 4 * pow(OFFLOADING_F, 2) * c_2 * c_3
						+ 2 * pow(OFFLOADING_F, 2) * c_2 * v
						+ pow(OFFLOADING_F, 2) * c_3 * v
						+ 2 * OFFLOADING_F * OFFLOADING_H * c_2 * v
						+ OFFLOADING_F * OFFLOADING_H * c_3 * v);

		s_0 = camera_load;
		s_1 = s_0 + cooperators[0].load;
		s_2 = s_1 + cooperators[1].load;
		s_3 = s_2 + cooperators[2].load;

		cout << "image size: " << image.rows << "x" << image.cols << endl;
		cout << "a0: " << camera_load << ". cols from " << 0 << " to "
				<< ceil((s_0 + ovl) * image.cols) << endl;
		cout << "a1: " << cooperators[0].load << ". cols from "
				<< ceil(s_0 * image.cols) << " to "
				<< ceil((s_1 + ovl) * image.cols) << endl;
		cout << "a2: " << cooperators[1].load << ". cols from "
				<< ceil(s_1 * image.cols) << " to "
				<< ceil((s_2 + ovl) * image.cols) << endl;
		cout << "a3: " << cooperators[2].load << ". cols from "
				<< ceil(s_2 * image.cols) << " to " << image.cols << endl;

		myLoad = Mat(image, Range(0, image.rows),
				Range(0, ceil((s_0 + ovl) * image.cols))).clone();
		cooperators[0].image_slice =
				Mat(image, Range(0, image.rows),
						Range(ceil(s_0 * image.cols),
								ceil((s_1 + ovl) * image.cols))).clone();
		cooperators[0].col_offset = ceil(s_0 * image.cols);
		cooperators[1].image_slice =
				Mat(image, Range(0, image.rows),
						Range(ceil(s_1 * image.cols),
								ceil((s_2 + ovl) * image.cols))).clone();
		cooperators[1].col_offset = ceil(s_1 * image.cols);
		cooperators[2].image_slice = Mat(image, Range(0, image.rows),
				Range(ceil(s_2 * image.cols), image.cols)).clone();
		cooperators[2].col_offset = ceil(s_2 * image.cols);
		break;

	case 4:
		c_2 = cooperators[1].bandwidth;
		c_3 = cooperators[2].bandwidth;
		c_4 = cooperators[3].bandwidth;

		camera_load = -(pow(OFFLOADING_F, 3) * ovl * pow(v, 3)
				+ pow(OFFLOADING_H, 3) * ovl * pow(v, 3)
				- pow(OFFLOADING_F, 3) * c_2 * c_3 * c_4
				+ 3 * OFFLOADING_F * pow(OFFLOADING_H, 2) * ovl * pow(v, 3)
				+ 3 * pow(OFFLOADING_F, 2) * OFFLOADING_H * ovl * pow(v, 3)
				+ 2 * pow(OFFLOADING_F, 3) * c_2 * ovl * pow(v, 2)
				+ pow(OFFLOADING_F, 3) * c_3 * ovl * pow(v, 2)
				+ pow(OFFLOADING_F, 3) * c_4 * ovl * pow(v, 2)
				+ pow(OFFLOADING_F, 3) * c_2 * c_3 * c_4 * ovl
				+ 3 * pow(OFFLOADING_F, 3) * c_2 * c_3 * ovl * v
				+ 2 * pow(OFFLOADING_F, 3) * c_2 * c_4 * ovl * v
				+ pow(OFFLOADING_F, 3) * c_3 * c_4 * ovl * v
				+ 2 * OFFLOADING_F * pow(OFFLOADING_H, 2) * c_2 * ovl
						* pow(v, 2)
				+ 4 * pow(OFFLOADING_F, 2) * OFFLOADING_H * c_2 * ovl
						* pow(v, 2)
				+ OFFLOADING_F * pow(OFFLOADING_H, 2) * c_3 * ovl * pow(v, 2)
				+ 2 * pow(OFFLOADING_F, 2) * OFFLOADING_H * c_3 * ovl
						* pow(v, 2)
				+ OFFLOADING_F * pow(OFFLOADING_H, 2) * c_4 * ovl * pow(v, 2)
				+ 2 * pow(OFFLOADING_F, 2) * OFFLOADING_H * c_4 * ovl
						* pow(v, 2)
				+ 3 * pow(OFFLOADING_F, 2) * OFFLOADING_H * c_2 * c_3 * ovl * v
				+ 2 * pow(OFFLOADING_F, 2) * OFFLOADING_H * c_2 * c_4 * ovl * v
				+ pow(OFFLOADING_F, 2) * OFFLOADING_H * c_3 * c_4 * ovl * v)
				/ (pow(OFFLOADING_F, 3) * pow(v, 3)
						+ pow(OFFLOADING_H, 3) * pow(v, 3)
						+ 3 * OFFLOADING_F * pow(OFFLOADING_H, 2) * pow(v, 3)
						+ 3 * pow(OFFLOADING_F, 2) * OFFLOADING_H * pow(v, 3)
						+ 2 * pow(OFFLOADING_F, 3) * c_2 * pow(v, 2)
						+ pow(OFFLOADING_F, 3) * c_3 * pow(v, 2)
						+ pow(OFFLOADING_F, 3) * c_4 * pow(v, 2)
						+ 5 * pow(OFFLOADING_F, 3) * c_2 * c_3 * c_4
						+ 3 * pow(OFFLOADING_F, 3) * c_2 * c_3 * v
						+ 2 * pow(OFFLOADING_F, 3) * c_2 * c_4 * v
						+ pow(OFFLOADING_F, 3) * c_3 * c_4 * v
						+ 2 * OFFLOADING_F * pow(OFFLOADING_H, 2) * c_2
								* pow(v, 2)
						+ 4 * pow(OFFLOADING_F, 2) * OFFLOADING_H * c_2
								* pow(v, 2)
						+ OFFLOADING_F * pow(OFFLOADING_H, 2) * c_3 * pow(v, 2)
						+ 2 * pow(OFFLOADING_F, 2) * OFFLOADING_H * c_3
								* pow(v, 2)
						+ OFFLOADING_F * pow(OFFLOADING_H, 2) * c_4 * pow(v, 2)
						+ 2 * pow(OFFLOADING_F, 2) * OFFLOADING_H * c_4
								* pow(v, 2)
						+ 3 * pow(OFFLOADING_F, 2) * OFFLOADING_H * c_2 * c_3
								* v
						+ 2 * pow(OFFLOADING_F, 2) * OFFLOADING_H * c_2 * c_4
								* v
						+ pow(OFFLOADING_F, 2) * OFFLOADING_H * c_3 * c_4 * v);
		cooperators[0].load = (pow(OFFLOADING_F, 3) * pow(v, 3)
				+ pow(OFFLOADING_H, 3) * pow(v, 3)
				+ 3 * OFFLOADING_F * pow(OFFLOADING_H, 2) * pow(v, 3)
				+ 3 * pow(OFFLOADING_F, 2) * OFFLOADING_H * pow(v, 3)
				+ pow(OFFLOADING_F, 3) * c_2 * pow(v, 2)
				+ pow(OFFLOADING_F, 3) * c_3 * pow(v, 2)
				+ pow(OFFLOADING_F, 3) * c_4 * pow(v, 2)
				+ 3 * pow(OFFLOADING_F, 3) * ovl * pow(v, 3)
				+ 3 * pow(OFFLOADING_H, 3) * ovl * pow(v, 3)
				+ pow(OFFLOADING_F, 3) * c_2 * c_3 * c_4
				+ pow(OFFLOADING_F, 3) * c_2 * c_3 * v
				+ pow(OFFLOADING_F, 3) * c_2 * c_4 * v
				+ pow(OFFLOADING_F, 3) * c_3 * c_4 * v
				+ OFFLOADING_F * pow(OFFLOADING_H, 2) * c_2 * pow(v, 2)
				+ 2 * pow(OFFLOADING_F, 2) * OFFLOADING_H * c_2 * pow(v, 2)
				+ OFFLOADING_F * pow(OFFLOADING_H, 2) * c_3 * pow(v, 2)
				+ 2 * pow(OFFLOADING_F, 2) * OFFLOADING_H * c_3 * pow(v, 2)
				+ OFFLOADING_F * pow(OFFLOADING_H, 2) * c_4 * pow(v, 2)
				+ 2 * pow(OFFLOADING_F, 2) * OFFLOADING_H * c_4 * pow(v, 2)
				+ 9 * OFFLOADING_F * pow(OFFLOADING_H, 2) * ovl * pow(v, 3)
				+ 9 * pow(OFFLOADING_F, 2) * OFFLOADING_H * ovl * pow(v, 3)
				+ 2 * pow(OFFLOADING_F, 3) * c_2 * ovl * pow(v, 2)
				+ 3 * pow(OFFLOADING_F, 3) * c_3 * ovl * pow(v, 2)
				+ 3 * pow(OFFLOADING_F, 3) * c_4 * ovl * pow(v, 2)
				+ pow(OFFLOADING_F, 2) * OFFLOADING_H * c_2 * c_3 * v
				+ pow(OFFLOADING_F, 2) * OFFLOADING_H * c_2 * c_4 * v
				+ pow(OFFLOADING_F, 2) * OFFLOADING_H * c_3 * c_4 * v
				- pow(OFFLOADING_F, 3) * c_2 * c_3 * c_4 * ovl
				+ pow(OFFLOADING_F, 3) * c_2 * c_3 * ovl * v
				+ 2 * pow(OFFLOADING_F, 3) * c_2 * c_4 * ovl * v
				+ 3 * pow(OFFLOADING_F, 3) * c_3 * c_4 * ovl * v
				+ 2 * OFFLOADING_F * pow(OFFLOADING_H, 2) * c_2 * ovl
						* pow(v, 2)
				+ 4 * pow(OFFLOADING_F, 2) * OFFLOADING_H * c_2 * ovl
						* pow(v, 2)
				+ 3 * OFFLOADING_F * pow(OFFLOADING_H, 2) * c_3 * ovl
						* pow(v, 2)
				+ 6 * pow(OFFLOADING_F, 2) * OFFLOADING_H * c_3 * ovl
						* pow(v, 2)
				+ 3 * OFFLOADING_F * pow(OFFLOADING_H, 2) * c_4 * ovl
						* pow(v, 2)
				+ 6 * pow(OFFLOADING_F, 2) * OFFLOADING_H * c_4 * ovl
						* pow(v, 2)
				+ pow(OFFLOADING_F, 2) * OFFLOADING_H * c_2 * c_3 * ovl * v
				+ 2 * pow(OFFLOADING_F, 2) * OFFLOADING_H * c_2 * c_4 * ovl * v
				+ 3 * pow(OFFLOADING_F, 2) * OFFLOADING_H * c_3 * c_4 * ovl * v)
				/ (pow(OFFLOADING_F, 3) * pow(v, 3)
						+ pow(OFFLOADING_H, 3) * pow(v, 3)
						+ 3 * OFFLOADING_F * pow(OFFLOADING_H, 2) * pow(v, 3)
						+ 3 * pow(OFFLOADING_F, 2) * OFFLOADING_H * pow(v, 3)
						+ 2 * pow(OFFLOADING_F, 3) * c_2 * pow(v, 2)
						+ pow(OFFLOADING_F, 3) * c_3 * pow(v, 2)
						+ pow(OFFLOADING_F, 3) * c_4 * pow(v, 2)
						+ 5 * pow(OFFLOADING_F, 3) * c_2 * c_3 * c_4
						+ 3 * pow(OFFLOADING_F, 3) * c_2 * c_3 * v
						+ 2 * pow(OFFLOADING_F, 3) * c_2 * c_4 * v
						+ pow(OFFLOADING_F, 3) * c_3 * c_4 * v
						+ 2 * OFFLOADING_F * pow(OFFLOADING_H, 2) * c_2
								* pow(v, 2)
						+ 4 * pow(OFFLOADING_F, 2) * OFFLOADING_H * c_2
								* pow(v, 2)
						+ OFFLOADING_F * pow(OFFLOADING_H, 2) * c_3 * pow(v, 2)
						+ 2 * pow(OFFLOADING_F, 2) * OFFLOADING_H * c_3
								* pow(v, 2)
						+ OFFLOADING_F * pow(OFFLOADING_H, 2) * c_4 * pow(v, 2)
						+ 2 * pow(OFFLOADING_F, 2) * OFFLOADING_H * c_4
								* pow(v, 2)
						+ 3 * pow(OFFLOADING_F, 2) * OFFLOADING_H * c_2 * c_3
								* v
						+ 2 * pow(OFFLOADING_F, 2) * OFFLOADING_H * c_2 * c_4
								* v
						+ pow(OFFLOADING_F, 2) * OFFLOADING_H * c_3 * c_4 * v);
		cooperators[1].load = (pow(OFFLOADING_F, 3) * c_2 * pow(v, 2)
				- pow(OFFLOADING_F, 3) * ovl * pow(v, 3)
				- pow(OFFLOADING_H, 3) * ovl * pow(v, 3)
				+ pow(OFFLOADING_F, 3) * c_2 * c_3 * c_4
				+ pow(OFFLOADING_F, 3) * c_2 * c_3 * v
				+ pow(OFFLOADING_F, 3) * c_2 * c_4 * v
				+ OFFLOADING_F * pow(OFFLOADING_H, 2) * c_2 * pow(v, 2)
				+ 2 * pow(OFFLOADING_F, 2) * OFFLOADING_H * c_2 * pow(v, 2)
				- 3 * OFFLOADING_F * pow(OFFLOADING_H, 2) * ovl * pow(v, 3)
				- 3 * pow(OFFLOADING_F, 2) * OFFLOADING_H * ovl * pow(v, 3)
				+ 2 * pow(OFFLOADING_F, 3) * c_2 * ovl * pow(v, 2)
				- pow(OFFLOADING_F, 3) * c_3 * ovl * pow(v, 2)
				- pow(OFFLOADING_F, 3) * c_4 * ovl * pow(v, 2)
				+ pow(OFFLOADING_F, 2) * OFFLOADING_H * c_2 * c_3 * v
				+ pow(OFFLOADING_F, 2) * OFFLOADING_H * c_2 * c_4 * v
				- pow(OFFLOADING_F, 3) * c_2 * c_3 * c_4 * ovl
				+ pow(OFFLOADING_F, 3) * c_2 * c_3 * ovl * v
				+ 2 * pow(OFFLOADING_F, 3) * c_2 * c_4 * ovl * v
				- pow(OFFLOADING_F, 3) * c_3 * c_4 * ovl * v
				+ 2 * OFFLOADING_F * pow(OFFLOADING_H, 2) * c_2 * ovl
						* pow(v, 2)
				+ 4 * pow(OFFLOADING_F, 2) * OFFLOADING_H * c_2 * ovl
						* pow(v, 2)
				- OFFLOADING_F * pow(OFFLOADING_H, 2) * c_3 * ovl * pow(v, 2)
				- 2 * pow(OFFLOADING_F, 2) * OFFLOADING_H * c_3 * ovl
						* pow(v, 2)
				- OFFLOADING_F * pow(OFFLOADING_H, 2) * c_4 * ovl * pow(v, 2)
				- 2 * pow(OFFLOADING_F, 2) * OFFLOADING_H * c_4 * ovl
						* pow(v, 2)
				+ pow(OFFLOADING_F, 2) * OFFLOADING_H * c_2 * c_3 * ovl * v
				+ 2 * pow(OFFLOADING_F, 2) * OFFLOADING_H * c_2 * c_4 * ovl * v
				- pow(OFFLOADING_F, 2) * OFFLOADING_H * c_3 * c_4 * ovl * v)
				/ (pow(OFFLOADING_F, 3) * pow(v, 3)
						+ pow(OFFLOADING_H, 3) * pow(v, 3)
						+ 3 * OFFLOADING_F * pow(OFFLOADING_H, 2) * pow(v, 3)
						+ 3 * pow(OFFLOADING_F, 2) * OFFLOADING_H * pow(v, 3)
						+ 2 * pow(OFFLOADING_F, 3) * c_2 * pow(v, 2)
						+ pow(OFFLOADING_F, 3) * c_3 * pow(v, 2)
						+ pow(OFFLOADING_F, 3) * c_4 * pow(v, 2)
						+ 5 * pow(OFFLOADING_F, 3) * c_2 * c_3 * c_4
						+ 3 * pow(OFFLOADING_F, 3) * c_2 * c_3 * v
						+ 2 * pow(OFFLOADING_F, 3) * c_2 * c_4 * v
						+ pow(OFFLOADING_F, 3) * c_3 * c_4 * v
						+ 2 * OFFLOADING_F * pow(OFFLOADING_H, 2) * c_2
								* pow(v, 2)
						+ 4 * pow(OFFLOADING_F, 2) * OFFLOADING_H * c_2
								* pow(v, 2)
						+ OFFLOADING_F * pow(OFFLOADING_H, 2) * c_3 * pow(v, 2)
						+ 2 * pow(OFFLOADING_F, 2) * OFFLOADING_H * c_3
								* pow(v, 2)
						+ OFFLOADING_F * pow(OFFLOADING_H, 2) * c_4 * pow(v, 2)
						+ 2 * pow(OFFLOADING_F, 2) * OFFLOADING_H * c_4
								* pow(v, 2)
						+ 3 * pow(OFFLOADING_F, 2) * OFFLOADING_H * c_2 * c_3
								* v
						+ 2 * pow(OFFLOADING_F, 2) * OFFLOADING_H * c_2 * c_4
								* v
						+ pow(OFFLOADING_F, 2) * OFFLOADING_H * c_3 * c_4 * v);
		cooperators[2].load = -(pow(OFFLOADING_F, 3) * ovl * pow(v, 3)
				+ pow(OFFLOADING_H, 3) * ovl * pow(v, 3)
				- pow(OFFLOADING_F, 3) * c_2 * c_3 * c_4
				- pow(OFFLOADING_F, 3) * c_2 * c_3 * v
				+ 3 * OFFLOADING_F * pow(OFFLOADING_H, 2) * ovl * pow(v, 3)
				+ 3 * pow(OFFLOADING_F, 2) * OFFLOADING_H * ovl * pow(v, 3)
				+ 2 * pow(OFFLOADING_F, 3) * c_2 * ovl * pow(v, 2)
				+ pow(OFFLOADING_F, 3) * c_3 * ovl * pow(v, 2)
				+ pow(OFFLOADING_F, 3) * c_4 * ovl * pow(v, 2)
				- pow(OFFLOADING_F, 2) * OFFLOADING_H * c_2 * c_3 * v
				+ pow(OFFLOADING_F, 3) * c_2 * c_3 * c_4 * ovl
				- pow(OFFLOADING_F, 3) * c_2 * c_3 * ovl * v
				+ 2 * pow(OFFLOADING_F, 3) * c_2 * c_4 * ovl * v
				+ pow(OFFLOADING_F, 3) * c_3 * c_4 * ovl * v
				+ 2 * OFFLOADING_F * pow(OFFLOADING_H, 2) * c_2 * ovl
						* pow(v, 2)
				+ 4 * pow(OFFLOADING_F, 2) * OFFLOADING_H * c_2 * ovl
						* pow(v, 2)
				+ OFFLOADING_F * pow(OFFLOADING_H, 2) * c_3 * ovl * pow(v, 2)
				+ 2 * pow(OFFLOADING_F, 2) * OFFLOADING_H * c_3 * ovl
						* pow(v, 2)
				+ OFFLOADING_F * pow(OFFLOADING_H, 2) * c_4 * ovl * pow(v, 2)
				+ 2 * pow(OFFLOADING_F, 2) * OFFLOADING_H * c_4 * ovl
						* pow(v, 2)
				- pow(OFFLOADING_F, 2) * OFFLOADING_H * c_2 * c_3 * ovl * v
				+ 2 * pow(OFFLOADING_F, 2) * OFFLOADING_H * c_2 * c_4 * ovl * v
				+ pow(OFFLOADING_F, 2) * OFFLOADING_H * c_3 * c_4 * ovl * v)
				/ (pow(OFFLOADING_F, 3) * pow(v, 3)
						+ pow(OFFLOADING_H, 3) * pow(v, 3)
						+ 3 * OFFLOADING_F * pow(OFFLOADING_H, 2) * pow(v, 3)
						+ 3 * pow(OFFLOADING_F, 2) * OFFLOADING_H * pow(v, 3)
						+ 2 * pow(OFFLOADING_F, 3) * c_2 * pow(v, 2)
						+ pow(OFFLOADING_F, 3) * c_3 * pow(v, 2)
						+ pow(OFFLOADING_F, 3) * c_4 * pow(v, 2)
						+ 5 * pow(OFFLOADING_F, 3) * c_2 * c_3 * c_4
						+ 3 * pow(OFFLOADING_F, 3) * c_2 * c_3 * v
						+ 2 * pow(OFFLOADING_F, 3) * c_2 * c_4 * v
						+ pow(OFFLOADING_F, 3) * c_3 * c_4 * v
						+ 2 * OFFLOADING_F * pow(OFFLOADING_H, 2) * c_2
								* pow(v, 2)
						+ 4 * pow(OFFLOADING_F, 2) * OFFLOADING_H * c_2
								* pow(v, 2)
						+ OFFLOADING_F * pow(OFFLOADING_H, 2) * c_3 * pow(v, 2)
						+ 2 * pow(OFFLOADING_F, 2) * OFFLOADING_H * c_3
								* pow(v, 2)
						+ OFFLOADING_F * pow(OFFLOADING_H, 2) * c_4 * pow(v, 2)
						+ 2 * pow(OFFLOADING_F, 2) * OFFLOADING_H * c_4
								* pow(v, 2)
						+ 3 * pow(OFFLOADING_F, 2) * OFFLOADING_H * c_2 * c_3
								* v
						+ 2 * pow(OFFLOADING_F, 2) * OFFLOADING_H * c_2 * c_4
								* v
						+ pow(OFFLOADING_F, 2) * OFFLOADING_H * c_3 * c_4 * v);
		cooperators[3].load = (pow(OFFLOADING_F, 3) * c_2 * c_3 * c_4
				+ 4 * pow(OFFLOADING_F, 3) * c_2 * c_3 * c_4 * ovl)
				/ (pow(OFFLOADING_F, 3) * pow(v, 3)
						+ pow(OFFLOADING_H, 3) * pow(v, 3)
						+ 3 * OFFLOADING_F * pow(OFFLOADING_H, 2) * pow(v, 3)
						+ 3 * pow(OFFLOADING_F, 2) * OFFLOADING_H * pow(v, 3)
						+ 2 * pow(OFFLOADING_F, 3) * c_2 * pow(v, 2)
						+ pow(OFFLOADING_F, 3) * c_3 * pow(v, 2)
						+ pow(OFFLOADING_F, 3) * c_4 * pow(v, 2)
						+ 5 * pow(OFFLOADING_F, 3) * c_2 * c_3 * c_4
						+ 3 * pow(OFFLOADING_F, 3) * c_2 * c_3 * v
						+ 2 * pow(OFFLOADING_F, 3) * c_2 * c_4 * v
						+ pow(OFFLOADING_F, 3) * c_3 * c_4 * v
						+ 2 * OFFLOADING_F * pow(OFFLOADING_H, 2) * c_2
								* pow(v, 2)
						+ 4 * pow(OFFLOADING_F, 2) * OFFLOADING_H * c_2
								* pow(v, 2)
						+ OFFLOADING_F * pow(OFFLOADING_H, 2) * c_3 * pow(v, 2)
						+ 2 * pow(OFFLOADING_F, 2) * OFFLOADING_H * c_3
								* pow(v, 2)
						+ OFFLOADING_F * pow(OFFLOADING_H, 2) * c_4 * pow(v, 2)
						+ 2 * pow(OFFLOADING_F, 2) * OFFLOADING_H * c_4
								* pow(v, 2)
						+ 3 * pow(OFFLOADING_F, 2) * OFFLOADING_H * c_2 * c_3
								* v
						+ 2 * pow(OFFLOADING_F, 2) * OFFLOADING_H * c_2 * c_4
								* v
						+ pow(OFFLOADING_F, 2) * OFFLOADING_H * c_3 * c_4 * v);

		s_0 = camera_load;
		s_1 = s_0 + cooperators[0].load;
		s_2 = s_1 + cooperators[1].load;
		s_3 = s_2 + cooperators[2].load;
		//s_4 = s_3 + cooperatorList[3].load;

		cout << "image size: " << image.rows << "x" << image.cols << endl;
		cout << "a0: " << camera_load << ". cols from " << 0 << " to "
				<< ceil((s_0 + ovl) * image.cols) << endl;
		cout << "a1: " << cooperators[0].load << ". cols from "
				<< ceil(s_0 * image.cols) << " to "
				<< ceil((s_1 + ovl) * image.cols) << endl;
		cout << "a2: " << cooperators[1].load << ". cols from "
				<< ceil(s_1 * image.cols) << " to "
				<< ceil((s_2 + ovl) * image.cols) << endl;
		cout << "a3: " << cooperators[2].load << ". cols from "
				<< ceil(s_2 * image.cols) << " to "
				<< ceil((s_3 + ovl) * image.cols) << endl;
		cout << "a4: " << cooperators[3].load << ". cols from "
				<< ceil(s_3 * image.cols) << " to " << image.cols << endl;

		myLoad = Mat(image, Range(0, image.rows),
				Range(0, ceil((s_0 + ovl) * image.cols))).clone();
		cooperators[0].image_slice =
				Mat(image, Range(0, image.rows),
						Range(ceil(s_0 * image.cols),
								ceil((s_1 + ovl) * image.cols))).clone();
		cooperators[0].col_offset = ceil(s_0 * image.cols);
		cooperators[1].image_slice =
				Mat(image, Range(0, image.rows),
						Range(ceil(s_1 * image.cols),
								ceil((s_2 + ovl) * image.cols))).clone();
		cooperators[1].col_offset = ceil(s_1 * image.cols);
		cooperators[2].image_slice =
				Mat(image, Range(0, image.rows),
						Range(ceil(s_2 * image.cols),
								ceil((s_3 + ovl) * image.cols))).clone();
		cooperators[2].col_offset = ceil(s_2 * image.cols);
		cooperators[3].image_slice = Mat(image, Range(0, image.rows),
				Range(ceil(s_3 * image.cols), image.cols)).clone();
		cooperators[3].col_offset = ceil(s_3 * image.cols);
		break;
	}

	return myLoad;
}

void OffloadingManager::transmitStartATC(StartATCMsg* const msg) {
	boost::mutex::scoped_lock lock(_coopArrayMutex);
	_lastOpMode = msg->getOperativeMode();
	for (unsigned char i = 0; i < _cooperatorsToUse; i++) {
		StartATCMsg* coopMsg = new StartATCMsg(NetworkNode::getMyself(),
				cooperators[i].node, LINKTYPE_TCP, msg->getBitStream());
		_nodeNetworkSystem->sendMessage(coopMsg);
	}

}

void OffloadingManager::transmitLoads() {

	boost::mutex::scoped_lock lock(_coopArrayMutex);

	_startIncomingThread();

	vector<uchar> bitstream;
	vector<int> param = vector<int>(2);
	param[0] = CV_IMWRITE_JPEG_QUALITY;
	param[1] = 70;

	for (int i = 0; i < _cooperatorsToUse; i++) {
		double enc_time = getTickCount();
		imencode(".jpg", cooperators[i].image_slice, bitstream, param);
		enc_time = (getTickCount() - enc_time) / getTickFrequency();
		if (_DEBUG > 1)
			cout
				<< "Offloading manager::transmitLoads: DATC: imencode time: "
				<< enc_time << " - bitstream size: " << bitstream.size() << "byte" << endl;


		ushort top_left_xCoordinate = cooperators[i].col_offset;
		ushort top_left_yCoordinate = 0;

		double time = getTickCount();
		DataCTAMsg* coopMsg = new DataCTAMsg(NetworkNode::getMyself(),
				cooperators[i].node, LINKTYPE_TCP, _frameId++, 0,
				top_left_xCoordinate, top_left_yCoordinate, bitstream.size(),
				enc_time, 0, bitstream,_lastOpMode);

		if (_DEBUG > 1)
			cout
				<< "OffloadingManager::transmitLoads: DATC: message preparation time: "
				<< (cv::getCPUTickCount()-time)/cv::getTickFrequency() << endl;

		_nodeNetworkSystem->sendMessage(coopMsg);
	}
}

void OffloadingManager::_probeLinks() {

	boost::mutex::scoped_lock lock(_coopArrayMutex);

	for (uint i = 0; i < cooperators.size(); i++) {
		//DUMMY BANDWIDTH - REPLACE WITH ACTUAL PROBING
		cooperators[i].bandwidth = 24000000;
		//cooperators[i].CPUspeed = 1480000;
		cooperators[i].CPUspeed = 1480000*6;
	}

	/*char command[60];
	 for(int i=0;i<cooperatorList.size();i++){
	 strcpy(command,"iperf -c");
	 strcat(command,inet_ntoa(cooperatorList[i].client.sin_addr));
	 strcat(command," -t 5 -p 8000  >/home/ubuntu/iperfLog.txt");
	 cout << "Starting bandwidth test: " << inet_ntoa(cooperatorList[i].client.sin_addr) << endl;
	 system(command);

	 //Parsing file to get bandwidth
	 double bandwidth=getBandwidth(false);
	 if(bandwidth==0)
	 return -1;
	 else{
	 cooperatorList[i].bandwidth = bandwidth*1000000;
	 cooperatorList[i].CPUspeed = 1480000;
	 }
	 }

	 return 0;*/
}

void OffloadingManager::_sortCooperators() {
	boost::mutex::scoped_lock lock(_coopArrayMutex);
	std::sort(cooperators.begin(), cooperators.end(), bandwidthComp);
}

void OffloadingManager::createOffloadingTask(int num_cooperators) {
	_receivedCooperators = 0;
	_cooperatorsToUse = num_cooperators;
	features_buffer.release();
	keypoint_buffer.clear();

	//here we should start a timer that will check if data is received from all cooperators
	//if it expires, it should notify the node_manager anyway to prevent deadlocks.
	_timer.expires_from_now(boost::posix_time::seconds(_timerExpiryTime));
	_timer.async_wait(
			boost::bind(&OffloadingManager::timerExpired, this,
					boost::asio::placeholders::error));
}

void OffloadingManager::startTimer() {
	r_thread = boost::thread(&OffloadingManager::runThread, this);
}
void OffloadingManager::runThread() {
	io.run();
	cout << "out of io service" << endl;
}

void OffloadingManager::addKeypointsAndFeatures(vector<KeyPoint>& kpts,
		Mat& features, Session* session, double detTime, double descTime,
		double kencTime, double fencTime) {

	boost::mutex::scoped_lock lock(_coopArrayMutex);

	features_buffer.push_back(features);

	cout << "Adding " << kpts.size() << "keypoints to the buffer" << endl;

	if (session) {
		for (unsigned char i = 0; i < cooperators.size(); i++) {
			if (session == cooperators[i].node->getSession()) {
				//add time measurementes
				cooperators[i].detTime = detTime;
				cooperators[i].descTime = descTime;
				cooperators[i].kencTime = kencTime;
				cooperators[i].fencTime = fencTime;
				//compensate for slicing if keypoints come from a cooperator
				for (unsigned short j = 0; j < kpts.size(); j++) {
					kpts[j].pt.x = kpts[j].pt.x + cooperators[i].col_offset;
					keypoint_buffer.push_back(kpts[j]);
				}
				break;
			}
		}
	} else {
		//add time measurements
		camDetTime = detTime;
		camDescTime = descTime;
		camkEncTime = kencTime;
		camfEncTime = fencTime;

		for (unsigned short j = 0; j < kpts.size(); j++) {
			//TODO ugly
			keypoint_buffer.push_back(kpts[j]);
		}
	}

	_receivedCooperators++;
	if (_receivedCooperators == _cooperatorsToUse + 1) {
		//data received from all cooperators: stop timer
		_timer.cancel();
		//node_manager->notifyOffloadingCompleted(keypoint_buffer,features_buffer,camDetTime,camDescTime);
	}

}

void OffloadingManager::timerExpired(const boost::system::error_code& error) {
	if (error != boost::asio::error::operation_aborted) {
		cout << "Offloading timer expired" << endl;
	} else {
		cout << "Data received, canceling timer" << endl;
	}
	_waitingForCoopsResp = false;
	_offloadingQueue.setActive(false);
	_coopRespWaitCondVar.notify_one();
}

/* Returns a vector of ids of active cooperators */
vector<unsigned char> OffloadingManager::getCooperatorsIds() const {

	boost::mutex::scoped_lock lock(_coopArrayMutex);

	vector<unsigned char> coopsIds;
	for (unsigned char idx = 0; idx < cooperators.size(); ++idx) {
		coopsIds.push_back(cooperators[idx].node->getId());
	}
	return coopsIds;
}

cv::Mat OffloadingManager::splitLoads(const cv::Mat& wholeImage,
		const unsigned char numCoops) {

	_probeLinks();
	_sortCooperators();

	boost::mutex::scoped_lock lock(_coopArrayMutex);

	_cooperatorsToUse = min((unsigned char) numCoops,
			(unsigned char) cooperators.size());

	cv::Mat cameraSlice;

	/**
	 * Load for camera [0] and cooperators [1...Ncoops]
	 */
	vector<float> sliceLoads(_cooperatorsToUse + 1);

	/**
	 * Overlap
	 */
	const float ovl = 15 / 100.0;

	/**
	 * Should be estimated. Processing capability of the cooperator. Will be estimated some day at cooperators connection
	 */
	const float v = cooperators[0].CPUspeed;

	switch (_cooperatorsToUse) {
	case 0: {
		cameraSlice = wholeImage.clone();
		break;
	}
	case 1: {

		sliceLoads[0] = (0.5 - ovl / 2);
		sliceLoads[1] = sliceLoads[0] + (ovl / 2 + 0.5);

		if (_DEBUG) {
			cout << "image size: " << wholeImage.rows << "x" << wholeImage.cols
					<< endl;
			cout << "a0: " << sliceLoads[0] << ". cols from " << 0 << " to "
					<< ceil((sliceLoads[0] + ovl) * wholeImage.cols) << endl;
			cout << "a1: " << sliceLoads[1] << ". cols from "
					<< ceil(sliceLoads[0] * wholeImage.cols) << " to "
					<< wholeImage.cols << endl;
		}
		cameraSlice =
				Mat(wholeImage, Range(0, wholeImage.rows),
						Range(0, ceil((sliceLoads[0] + ovl) * wholeImage.cols))).clone();
		cooperators[0].image_slice =
				Mat(wholeImage, Range(0, wholeImage.rows),
						Range(ceil(sliceLoads[0] * wholeImage.cols),
								wholeImage.cols)).clone();
		cooperators[0].col_offset = ceil(sliceLoads[0] * wholeImage.cols);
		break;
	}
	case 2: {
		const float c_2 = cooperators[1].bandwidth;

		sliceLoads[0] =
				-(OFFLOADING_F * c_2 * ovl - OFFLOADING_F * c_2
						+ OFFLOADING_F * ovl * v + OFFLOADING_H * ovl * v)
						/ (3 * OFFLOADING_F * c_2 + OFFLOADING_F * v
								+ OFFLOADING_H * v);
		sliceLoads[1] =
				(OFFLOADING_F * c_2 + OFFLOADING_F * v + OFFLOADING_H * v
						- OFFLOADING_F * c_2 * ovl + OFFLOADING_F * ovl * v
						+ OFFLOADING_H * ovl * v)
						/ (3 * OFFLOADING_F * c_2 + OFFLOADING_F * v
								+ OFFLOADING_H * v);
		sliceLoads[2] =
				(OFFLOADING_F * c_2 + 2 * OFFLOADING_F * c_2 * ovl)
						/ (3 * OFFLOADING_F * c_2 + OFFLOADING_F * v
								+ OFFLOADING_H * v);

		sliceLoads[1] = sliceLoads[0] + sliceLoads[1];
		sliceLoads[2] = sliceLoads[1] + sliceLoads[2];

		if (_DEBUG) {
			cout << "image size: " << wholeImage.rows << "x" << wholeImage.cols
					<< endl;
			cout << "a0: " << sliceLoads[0] << ". cols from " << 0 << " to "
					<< ceil((sliceLoads[0] + ovl) * wholeImage.cols) << endl;
			cout << "a1: " << sliceLoads[1] << ". cols from "
					<< ceil(sliceLoads[1] * wholeImage.cols) << " to "
					<< ceil((sliceLoads[1] + ovl) * wholeImage.cols) << endl;
			cout << "a2: " << sliceLoads[2] << ". cols from "
					<< ceil(sliceLoads[2] * wholeImage.cols) << " to "
					<< wholeImage.cols << endl;
		}

		cameraSlice =
				Mat(wholeImage, Range(0, wholeImage.rows),
						Range(0, ceil((sliceLoads[0] + ovl) * wholeImage.cols))).clone();
		cooperators[0].image_slice = Mat(wholeImage, Range(0, wholeImage.rows),
				Range(ceil(sliceLoads[0] * wholeImage.cols),
						ceil((sliceLoads[1] + ovl) * wholeImage.cols))).clone();
		cooperators[1].image_slice =
				Mat(wholeImage, Range(0, wholeImage.rows),
						Range(ceil(sliceLoads[1] * wholeImage.cols),
								wholeImage.cols)).clone();
		cooperators[0].col_offset = ceil(sliceLoads[0] * wholeImage.cols);
		cooperators[1].col_offset = ceil(sliceLoads[1] * wholeImage.cols);
		break;
	}

	}

	return cameraSlice;
}

/*
 * Delete message
 */
void OffloadingManager::_incomingThread() {
	if (_DEBUG)
		cout << "OffloadingManager::_incomingThread" << endl;

	_offloadingQueue.setActive(true);
	_offloadingQueue.flush();
	_waitingForCoopsResp = true;
	features_buffer.release();
	keypoint_buffer.clear();
	_receivedCooperators = 0;

	_timer.expires_from_now(boost::posix_time::seconds(_timerExpiryTime));
	_timer.async_wait(
			boost::bind(&OffloadingManager::timerExpired, this,
					boost::asio::placeholders::error));

	DataATCMsg* msg;

	while (_waitingForCoopsResp) {
		_offloadingQueue.wait_and_pop(msg);
		if (!_waitingForCoopsResp) {
			break;
		}

		vector<cv::KeyPoint> kpts;
		cv::Mat features;

		_decoder->dummy_decodeKeyPoints((*msg->getKeypointsData()), kpts);
		_decoder->dummy_decodeBinaryDescriptors(DESCRIPTORTYPE_BRISK,
				*(msg->getFeaturesData()), features);

		features_buffer.push_back(features);

		cout << "Adding " << kpts.size() << " keypoints to the buffer" << endl;

		float detTime = msg->getDetTime();
		float descTime = msg->getDescTime();
		float kencTime = msg->getKptsEncodingTime();
		float fencTime = msg->getFeatEncodingTime();

		if (msg->getSrc()->getType() == NODETYPE_COOPERATOR) {

			boost::mutex::scoped_lock lock(_coopArrayMutex);

			for (unsigned char i = 0; i < cooperators.size(); i++) {
				if (msg->getSrc() == cooperators[i].node) {
					//add time measurementes

					cooperators[i].detTime = detTime;
					cooperators[i].descTime = descTime;
					cooperators[i].kencTime = kencTime;
					cooperators[i].fencTime = fencTime;
					//compensate for slicing if keypoints come from a cooperator
					for (unsigned short j = 0; j < kpts.size(); j++) {
						kpts[j].pt.x = kpts[j].pt.x + cooperators[i].col_offset;
						keypoint_buffer.push_back(kpts[j]);
					}
					break;
				}
			}
		} else {
			//add time measurements
			camDetTime = detTime;
			camDescTime = descTime;
			camkEncTime = kencTime;
			camfEncTime = fencTime;

			for (unsigned short j = 0; j < kpts.size(); j++) {
				keypoint_buffer.push_back(kpts[j]);
			}
		}

		delete msg;

		if (_receivedCooperators++ == _cooperatorsToUse) {
			_timer.cancel();
		}

	}

}
