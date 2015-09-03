/*
 * HistDescriptor.cpp
 *
 *  Created on: 09/giu/2015
 *      Author: luca
 */

#include "HistDescriptor.h"
#include <iostream>

using namespace std;
using namespace cv;

void HistDescriptor::computeImpl(const Mat& image, vector<KeyPoint>& keypoints,
		Mat& features) const {

	features = Mat::zeros(keypoints.size(), descriptorSize(), descriptorType());

	if (!features.isContinuous()) {
		features = features.clone();
	}

	cout << "HistDescriptor::computeImpl: features.rows: " << features.rows
			<< " features.cols: " << features.cols << " features.type: "
			<< features.type() << endl;

	for (ushort kptIdx = 0; kptIdx < keypoints.size(); ++kptIdx) {
		KeyPoint kpt = keypoints.at(kptIdx);
		short lowy = kpt.pt.y - kpt.size;
		short highy = kpt.pt.y + kpt.size + 1;
		short lowx = kpt.pt.x - kpt.size;
		short highx = kpt.pt.x + kpt.size + 1;

		lowy = lowy < 0 ? 0 : lowy;
		lowx = lowx < 0 ? 0 : lowx;
		highy = highy > image.rows ? image.rows : highy;
		highx = highx > image.cols ? image.cols : highx;

		//TODO serialize to avoid nested for loops
		for (ushort y = lowy; y < highy; ++y) {
			for (ushort x = lowx; x < highx; ++x) {
				uchar val = image.at<uchar>(y, x);
				features.at<uint16_t>(kptIdx, (val >> _binshift))++;}
			}
		}
	}
