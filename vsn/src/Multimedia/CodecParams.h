#ifndef SRC_MULTIMEDIA_CODECPARAMS_H_
#define SRC_MULTIMEDIA_CODECPARAMS_H_

#include <iostream>
#include <fstream>

#ifdef GUI
#include <Brisk.h>
#else
#include <Multimedia/Briskola.h>
#endif

#define AC_PRECISION 1000

using namespace std;

// PROBABILITY MODEL FOR BRISK DESCRIPTOR
// Important: uses a singleton class, in order to load the model only the first time the object is created
// --------------------------------------------------------------------------------------------------------
class BRISK_pModel {
private:

	double P0[1776];
	double P0c0[1776][1776];
	double P0c1[1776][1776];
	int idxs[BRISK_LENGTH_BITS];

	static BRISK_pModel* instance_ptr;

	void loadFiles();

public:

	BRISK_pModel();
	~BRISK_pModel() {
	}
	;
	static BRISK_pModel* get_instance();
	double getP0(int id);
	double getP0c0(int id1, int id2);
	double getP0c1(int id1, int id2);

};

#endif
