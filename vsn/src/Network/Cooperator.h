/*
 * Cooperator.h
 *
 *  Created on: 04/feb/2015
 *      Author: luca
 */

#ifndef SRC_NETWORK_COOPERATOR_H_
#define SRC_NETWORK_COOPERATOR_H_

#include <opencv2/core/core.hpp>

class Connection;
class NetworkNode;

typedef struct Cooperator {
	NetworkNode* node;

	double bandwidth;
	double CPUspeed;

	double load;
	cv::Mat image_slice;
	int col_offset;

	double detTime;
	double descTime;
	double kencTime;
	double fencTime;
} Cooperator;

#endif /* SRC_RADIOSYSTEM_COOPERATOR_H_ */
