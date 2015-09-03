/*
 * HistDescriptor.h
 *
 *  Created on: 09/giu/2015
 *      Author: luca
 */

#ifndef SRC_MULTIMEDIA_HISTDESCRIPTOR_H_
#define SRC_MULTIMEDIA_HISTDESCRIPTOR_H_

#include <opencv2/core/core.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <vector>

namespace cv {

class CV_EXPORTS HistDescriptor: public DescriptorExtractor {

public:
	HistDescriptor (uchar binshift_, ushort maxvalue_){
		_binshift = binshift_;
		_maxvalue = maxvalue_;
	}

	void setBinshift(uchar binshift_) {
		_binshift = binshift_;
	}
	void setMaxvalue(uchar maxvalue_) {
		_maxvalue = maxvalue_;
	}

protected:

	void computeImpl(const Mat& image, std::vector<KeyPoint>& keypoints,
			Mat& descriptors) const;

	int descriptorSize() const {
		return ((_maxvalue-1)>>_binshift)+1;
	}

	int descriptorType() const {
		return CV_16UC1;
	}

private:
	uchar _binshift;
	ushort _maxvalue;

};

}
#endif /* SRC_MULTIMEDIA_HISTDESCRIPTOR_H_ */
