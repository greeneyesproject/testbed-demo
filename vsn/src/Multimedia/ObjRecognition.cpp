#include <Multimedia/CodecParams.h>
#include <Multimedia/ObjRecognition.h>

objRecognition::objRecognition(VisualFeatureExtraction* featExtract,
		bool isBinary, const int _nDBImages) {

	m_pFeatExtract = featExtract;

	if (isBinary) {
		m_pMatcher = new BruteForceMatcher<Hamming>();
		feature_type = 1;
	} else {
		m_pMatcher = new BruteForceMatcher<L2<float> >();
		feature_type = 0;
	}

	// just define the capacity
	m_DbKeyp.reserve(_nDBImages);
	m_DbDescr.reserve(_nDBImages);
	m_nDBImages = 0;

	m_bitrate = -1;

}
;

void objRecognition::dCreateAddToDB(const std::string& dbImgPath) {
	std::vector<KeyPoint> keyp;
	Mat c_img, db_img, descr;

	// read db image
	c_img = imread(dbImgPath, 1);
	if (c_img.empty()) {
		std::cerr << "Could not load image " << dbImgPath << std::endl;
		exit(0);

	}
	// convert color
	cvtColor(c_img, db_img, COLOR_BGR2GRAY);

	// detect keypoints and create descriptor
	//m_pFeatExtract.extractFeatures(db_img,&keyp,&descr);
	m_pFeatExtract->extractFeatures(db_img, keyp, descr);

	// sort the descriptors
	//sortDescriptor(keyp, descr);

	// add keypoints+descriptor to DB
	m_DbKeyp.push_back(keyp);
	m_DbDescr.push_back(descr);

	m_nDBImages++;
}

vector<unsigned int> objRecognition::rankedQueryDB(vector<KeyPoint> &kpts,
		Mat &features, std::vector<vector<float> > &distances, bool do_bin_NNDR,
		int NND_bin_thr, float NNDR_ratio, bool do_ransac,
		int ransac_min_matches, float ransac_threshold) {

	distances.clear();

	std::vector<float> aux_dist;
	aux_dist.clear();

	std::vector<std::vector<cv::DMatch> >::iterator it_all_matches;
	std::vector<std::vector<cv::DMatch> > all_matches;
	std::vector<DMatch>::iterator it_fmatches;
	std::vector<DMatch> filt_matches1;
	std::vector<cv::Point2f> q_pts, db_pts;
	std::vector<uchar>::iterator it_mask;
	std::vector<uchar> mask;
	unsigned int inliers, outliers;
	std::vector<unsigned int> score;
	Mat q_img, H;
	//int nDeletions;

	q_keyp = kpts;
	q_descr = features;

	// allocate memory
	all_matches.reserve(q_keyp.size());
	filt_matches1.reserve(q_keyp.size());
	q_pts.reserve(q_keyp.size());
	db_pts.reserve(q_keyp.size());
	score.reserve(m_nDBImages);

	// look for all the images ...
	for (unsigned int nImg = 0; nImg < m_nDBImages; nImg++) {

		// clean data
		all_matches.clear();
		filt_matches1.clear();
		q_pts.clear();
		db_pts.clear();

		m_pMatcher->knnMatch(q_descr, m_DbDescr[nImg], all_matches, 2);

		// find correspondences
		for (it_all_matches = all_matches.begin();
				it_all_matches < all_matches.end(); it_all_matches++) {
			if (it_all_matches->size() != 2)
				continue;

			if (feature_type == 1 && do_bin_NNDR == 0) {
				if (it_all_matches->at(0).distance <= NND_bin_thr) {
					filt_matches1.push_back(it_all_matches->at(0));
				}
			} else {
				if (it_all_matches->at(0).distance
						<= NNDR_ratio * it_all_matches->at(1).distance) {
					filt_matches1.push_back(it_all_matches->at(0));
				}
			}
		}

		if (do_ransac) {
			// get the 'good' points from the filtered matches
			for (it_fmatches = filt_matches1.begin();
					it_fmatches < filt_matches1.end(); it_fmatches++) {
				q_pts.push_back(q_keyp.at(it_fmatches->queryIdx).pt);
				db_pts.push_back(m_DbKeyp[nImg].at(it_fmatches->trainIdx).pt);
			}
			if (filt_matches1.size() >= ransac_min_matches) {
				// find homography with RANSAC
				H = findHomography(q_pts, db_pts, RANSAC, ransac_threshold,
						mask);
				// filter query's
				for (inliers = 0, outliers = 0, it_mask = mask.begin();
						it_mask < mask.end(); it_mask++) {
					*it_mask ? ++inliers : ++outliers;
				}
			} else {
				inliers = 0;
			}
		} else {
			inliers = filt_matches1.size();
		}

		score.push_back(inliers);

		aux_dist.clear();
		for (unsigned int j = 0; j < inliers; j++) {
			aux_dist.push_back(filt_matches1[j].distance);
		}

		distances.push_back(aux_dist);

	}

	//vector<unsigned int>::iterator max = max_element(score.begin(),score.end());
	return score;
}

