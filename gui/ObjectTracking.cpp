#include "ObjectTracking.h"
#include "SortLikeMatlab.h"
#include <QDebug>

using namespace cv;
using namespace std;

ObjectTracking* ObjectTracking::_instance = NULL;

ObjectTracking* ObjectTracking::getObjectTracking(string dbPath){
    if (_instance==NULL){
        _instance = new ObjectTracking(dbPath);
    }
    return _instance;
}

ObjectTracking::ObjectTracking(string path):
    _db_path(path)
{

    qDebug() << "reading feature db";
    cv::FileStorage f;

    String obj;

    for (int cur = 0;; cur++){

        std::stringstream sstm;
        sstm << db_path() << cur << ".xml";
        //cout << sstm.str() << endl;
        f.open(sstm.str(), FileStorage::READ);

        if (!f.isOpened()&&cur==0){
            cout << "database not found" << endl;
            break;
        }
        if (!f.isOpened()){
            break;
        }
        vector< KeyPoint > cur_DB_kpts;
        Mat cur_DB_desc;
        Mat kpts_mat;
        Mat cur_BB;
        int obj_index;
        f["desc"] >> cur_DB_desc;
        f["kpts"] >> kpts_mat;
        f["obj"] >> obj;
        f["obj_index"] >> obj_index;
        f["BB"] >> cur_BB;
        for (int i = 0; i < kpts_mat.rows; i++){

            float x = kpts_mat.at<float>(i, 0);
            float y = kpts_mat.at<float>(i, 1);
            float angle = kpts_mat.at<float>(i, 2);
            int class_id = (int) kpts_mat.at<float>(i, 3);
            int octave = (int) kpts_mat.at<float>(i, 4);
            float response = kpts_mat.at<float>(i, 5);
            float size = kpts_mat.at<float>(i, 6);

            cur_DB_kpts.push_back(KeyPoint(x, y, size, angle, response, octave, class_id));
        }
        _dbKpts.push_back(cur_DB_kpts);
        _dbDesc.push_back(cur_DB_desc);
        _dbObj.push_back(obj);
        _dbClass.push_back(obj_index);

        std::vector< cv::Point2f > tmp_vec_points;
        cv::Point2f ponto;

        tmp_vec_points.clear();
        ponto.x = cur_BB.at<float>(0, 0);
        ponto.y = cur_BB.at<float>(0, 1);
        tmp_vec_points.push_back(ponto);
        ponto.x = cur_BB.at<float>(0, 2);
        ponto.y = cur_BB.at<float>(0, 3);
        tmp_vec_points.push_back(ponto);
        ponto.x = cur_BB.at<float>(0, 4);
        ponto.y = cur_BB.at<float>(0, 5);
        tmp_vec_points.push_back(ponto);
        ponto.x = cur_BB.at<float>(0, 6);
        ponto.y = cur_BB.at<float>(0, 7);
        tmp_vec_points.push_back(ponto);

        _Db_bounding_box.push_back(tmp_vec_points);
    }
    f.release();

    //    // load bounding boxes for tracking
    //    std::vector< cv::Point2f > tmp_vec_points;
    //    cv::Point2f ponto;

    //    stringstream ss;
    //    ss << BBOX_PATH << "Obj_Database_bounding_box.txt";
    //    ifstream text_in(ss.str().c_str());

    //    for( int img=0; img<_dbObj.size(); img++)
    //    {
    //        tmp_vec_points.clear();
    //        text_in >> ponto.x;
    //        text_in >> ponto.y;
    //        tmp_vec_points.push_back(ponto);
    //        text_in >> ponto.x;
    //        text_in >> ponto.y;
    //        tmp_vec_points.push_back(ponto);
    //        text_in >> ponto.x;
    //        text_in >> ponto.y;
    //        tmp_vec_points.push_back(ponto);
    //        text_in >> ponto.x;
    //        text_in >> ponto.y;
    //        tmp_vec_points.push_back(ponto);

    //        _Db_bounding_box.push_back(tmp_vec_points);
    //    }
    cout<<"BoundingBOX "<<_Db_bounding_box.size()<<endl;
    // bounding boxes loaded

}

string ObjectTracking::db_path() const
{
    return _db_path;
}

void ObjectTracking::setDb_path(const string &db_path)
{
    _db_path = db_path;
}


int ObjectTracking::trackObject(int camIdx, int opMode){

    Camera * cur_cam = (*(cameras()))[camIdx];

    if ((((OperativeMode)opMode) != OPERATIVEMODE_OBJECT) || (cur_cam->getGoodDescriptors().cols != 32)){
        return 0;
    }

    CameraView * cur_camview = (*(camViews()))[camIdx];

    _B_Box_old = _B_Box;
    bool hom_ok = false;

    std::vector<std::vector<cv::DMatch> >::iterator it_all_matches;
    std::vector<std::vector<cv::DMatch> > all_matches;
    std::vector<DMatch>::iterator it_filt_matches;
    std::vector<DMatch> filt_matches;
    std::vector<cv::Point2f> q_pts, db_pts;
    std::vector<uchar> mask;
    std::vector<uchar>::iterator it_mask;
    Mat descr_out, H_out;
    vector<KeyPoint> keyp_out;
    int best_img=0, best_score=0, inliers;
    //some test redefinitions are To be replaced
    int MIN_MATCHES = 8;
    float RT_threshold= 1.0f;
    double RANSAC_THRESHOLD = 10.0f;
    double RADIUS_THRES = 0.21;
    Ptr<DescriptorMatcher> descr_matcher = DescriptorMatcher::create("BruteForce-Hamming");

    std::cout << "camera " << camIdx << " Tracking object..." << std::endl;

    String obj;


    //***NBS***//
    // do nothing if the other cam is not ready
    if(cur_cam->getNbs() && !cur_cam->getBargainCamera()->getNbsRecReady()){
        return -1;
    }
    //***NBS***//

    cv::Mat query_desc = cur_cam->getGoodDescriptors();
    vector<KeyPoint> query_kpts = cur_cam->getGoodKeypoints();

    //***NBS***//
    //append features of bargaining cam
    if(cur_cam->getNbs()){
        query_desc.push_back(cur_cam->getBargainCamera()->getGoodDescriptors());
        vector<KeyPoint> query_kpts2 = cur_cam->getBargainCamera()->getGoodKeypoints();
        for(int i=0;i<query_kpts2.size();i++){
            query_kpts.push_back(query_kpts2[i]);
        }
    }
    //***NBS***//

    descr_out.release();
    keyp_out.clear();

    vector<vector<DMatch> > matches_for_BB;


    vector<unsigned int> ranked_list;
    if (cur_cam->getRecognitionEnabled()||cur_cam->getTrackingEnabled()){
        // perform matching
        for( size_t img = 0; img < _dbDesc.size(); img++ )
        {
            //            descr_matcher->knnMatch(query_desc, _dbDesc[img], all_matches, 2);

            descr_matcher->radiusMatch(query_desc, _dbDesc[img], all_matches, /*RADIUS_THRES * BRISK_LENGTH_BITS*/45);

            // find correspondences by NNDR (Nearest Neighbor Distance Ratio)
            //            for ( it_all_matches = all_matches.begin(); it_all_matches < all_matches.end(); it_all_matches++ )
            //            {
            //                if ( it_all_matches->size() != 2 ) continue;

            //                if ( it_all_matches->at(0).distance <= RT_threshold*it_all_matches->at(1).distance )
            //                    filt_matches.push_back(it_all_matches->at(0));
            //            }

            for (size_t i = 0; i < all_matches.size(); i++){
                //for (int j = 0; j < all_matches[i].size(); j++){
                if (all_matches[i].size() > 0)
                    filt_matches.push_back(all_matches[i][0]);
                //}
            }

            int inliers_no_ransac = filt_matches.size();

            // with homography
            // get the 'good' points from the filtered matches
            for(it_filt_matches = filt_matches.begin(); it_filt_matches < filt_matches.end(); it_filt_matches++ )
            {
                q_pts.push_back(query_kpts.at(it_filt_matches->queryIdx).pt);
                db_pts.push_back(_dbKpts[img].at(it_filt_matches->trainIdx).pt);
            }

            Mat mask;

            if ( filt_matches.size() >= MIN_MATCHES )
            {
                // find homography with RANSAC
                H_out = findHomography(q_pts, db_pts, CV_RANSAC, RANSAC_THRESHOLD, mask);
                // filter query's
                inliers = sum(mask)[0];
            }
            else
                inliers = filt_matches.size()/2;




            ranked_list.push_back(inliers_no_ransac);

            /*if( inliers > best_score )
            {
                best_img = img;
                best_score = inliers;
            }*/

            vector<DMatch> good_BB_matches;
            for (size_t i = 0; i < filt_matches.size(); i++){
                //if (mask.at<uchar>(i, 0) > 0){
                    good_BB_matches.push_back(filt_matches[i]);
                //}
            }
            matches_for_BB.push_back(good_BB_matches);

            // clean data
            all_matches.clear();
            filt_matches.clear();
            q_pts.clear();
            db_pts.clear();
            all_matches.reserve(query_kpts.size());
            filt_matches.reserve(query_kpts.size());
            q_pts.reserve(query_kpts.size());
            db_pts.reserve(query_kpts.size());
        }


        ////////////////////////////////////////////

        // sort the ranking list
        vector<size_t> sorted_idxs;
        sorted_idxs.clear();
        sort_like_matlab(ranked_list,ranked_list,sorted_idxs,1);
        string rec_object;
        cout << "SORTED IDXS" << endl;
        for(size_t i=0;i<ranked_list.size();i++){
            cout << sorted_idxs[i] << " " << ranked_list[i] << endl;
        }

        // group the object of the same class
        vector<int> classScore (_dbDesc.size(),0);
        for(unsigned int i=0; i<ranked_list.size(); i++){
            classScore[_dbClass[sorted_idxs[i]]] += ranked_list[i];
        }

        vector<size_t> sorted_idxs_class;

        // obtain the final ranking

        sort_like_matlab( classScore, classScore, sorted_idxs_class, 1);

        if( classScore[0]>4 ){
            hom_ok = true;
            size_t i;
            for (i = 0; i < _dbClass.size(); i++){
                if (_dbClass[i] == sorted_idxs_class[0])
                    break;
            }
            rec_object = _dbObj[i];
            cout << "OBJECT: " << _dbObj[i] << endl;
            for( i=0; i<classScore.size(); i++){
                cout << "CLASS " << sorted_idxs_class[i] << " , rank = " << classScore[i] << endl;
            }
        }
        else{
            rec_object = "not recognized";
        }
        emit cur_cam->recognitionCompletedSignal("Object: ",QString(rec_object.c_str()));

        ////////////////////////////////////////////////////////////////////////
        // NEW BOUNDING BOX

        if (hom_ok && cur_cam->getTrackingEnabled()){
            size_t i;
            for ( i = 0; i < sorted_idxs.size(); i++){
                if (_dbClass[sorted_idxs[i]] == sorted_idxs_class[0])
                    break;
            }

            vector<DMatch> good_m = matches_for_BB[sorted_idxs[i]];
            vector<Point2f> point_cloud;

            /*for (int m = 0; m < good_m.size(); m++){
                point_cloud.push_back(query_kpts[good_m[m].queryIdx].pt);
            }*/
            //all keypoints
            for(size_t m=0;m<query_kpts.size();m++){
                point_cloud.push_back(query_kpts[m].pt);
            }

            vector<Point2f> BB;
            if (point_cloud.size()>2){
                Rect bb_rect = boundingRect(point_cloud);

                BB.push_back(bb_rect.tl());
                BB.push_back(Point(bb_rect.br().x, bb_rect.tl().y));
                BB.push_back(bb_rect.br());
                BB.push_back(Point(bb_rect.tl().x, bb_rect.br().y));
            }
            else{
                BB.push_back(Point(0, 0));
                BB.push_back(Point(0, 0));
                BB.push_back(Point(0, 0));
                BB.push_back(Point(0, 0));
            }

            setB_Box(BB);
        }

    }

    Mat img_track;

    //store bounding box
    _cameras->at(camIdx)->setBoundingBoxes(getB_Box());

    if (cur_cam->getTrackingEnabled()){
        if (cur_cam->getCurrentMode()==CAMERA_MODE_CTA){ // CTA
            img_track = _cameras->at(camIdx)->getCTAFrameClean();
            if (hom_ok){
                vector<Point2f> BB = this->_B_Box;
                cv::line(img_track, BB.at(0), BB.at(1), Scalar(0, 255, 0), 2);
                cv::line(img_track, BB.at(1), BB.at(2), Scalar(0, 255, 0), 2);
                cv::line(img_track, BB.at(2), BB.at(3), Scalar(0, 255, 0), 2);
                cv::line(img_track, BB.at(3), BB.at(0), Scalar(0, 255, 0), 2);
            }
            cur_cam->setCTASlice(img_track, cv::Point(0, 0), cv::Point(img_track.cols, img_track.rows));

            // draw line
            qDebug() << "drawing bounding box on top of CTA frame";
        }
        else {

            if (cur_cam->getShowReconstruction()){
                img_track = _cameras->at(camIdx)->getATCRecFrameClean();
                if (hom_ok){
                    vector<Point2f> BB = this->_B_Box;
                    cv::line(img_track, BB.at(0), BB.at(1), Scalar(0, 255, 0), 2);
                    cv::line(img_track, BB.at(1), BB.at(2), Scalar(0, 255, 0), 2);
                    cv::line(img_track, BB.at(2), BB.at(3), Scalar(0, 255, 0), 2);
                    cv::line(img_track, BB.at(3), BB.at(0), Scalar(0, 255, 0), 2);
                }
                cur_cam->setATCRecFrame(img_track);
                qDebug() << "drawing bounding box on top of reconstructed frame";
            }

            else{
                img_track = _cameras->at(camIdx)->getATCFrameClean();
                if (hom_ok){
                    vector<Point2f> BB = this->_B_Box;
                    cv::line(img_track, BB.at(0), BB.at(1), Scalar(0, 255, 0), 2);
                    cv::line(img_track, BB.at(1), BB.at(2), Scalar(0, 255, 0), 2);
                    cv::line(img_track, BB.at(2), BB.at(3), Scalar(0, 255, 0), 2);
                    cv::line(img_track, BB.at(3), BB.at(0), Scalar(0, 255, 0), 2);
                }
                cur_cam->showATCFrame(img_track);
            }
            qDebug() << "drawing bounding box on top of ATC frame";
        }

    }




    ////////

    //    if( best_score > 0 )
    //    {
    //        // Repeat for the best match
    //        descr_matcher->knnMatch(query_desc, _dbDesc[best_img], all_matches, 2);

    //        // find correspondences by NNDR (Nearest Neighbor Distance Ratio)
    //        for ( it_all_matches = all_matches.begin(); it_all_matches < all_matches.end(); it_all_matches++ )
    //        {
    //            if ( it_all_matches->size() != 2 ) continue;

    //            if ( it_all_matches->at(0).distance <= RT_threshold*it_all_matches->at(1).distance )
    //                filt_matches.push_back(it_all_matches->at(0));
    //        }

    //        // get the 'good' points from the filtered matches
    //        for(it_filt_matches = filt_matches.begin(); it_filt_matches < filt_matches.end(); it_filt_matches++ )
    //        {
    //            q_pts.push_back(query_kpts.at(it_filt_matches->queryIdx).pt);
    //            db_pts.push_back(_dbKpts[best_img].at(it_filt_matches->trainIdx).pt);
    //        }
    //        if ( filt_matches.size() >= MIN_MATCHES )
    //        {

    //            hom_ok = true;
    //            // find homography with RANSAC
    //            H_out = findHomography(q_pts, db_pts, RANSAC, RANSAC_THRESHOLD, mask);
    //            // filter query's
    //            for(it_mask = mask.begin(), it_filt_matches = filt_matches.begin(); it_mask < mask.end(); it_mask++, it_filt_matches++)
    //            {
    //                if(*it_mask == 1)
    //                {
    //                    descr_out.push_back(query_desc.row(it_filt_matches->queryIdx));
    //                    keyp_out.push_back(query_kpts.at(it_filt_matches->queryIdx));
    //                }
    //            }

    //        }
    //        else
    //        {
    //            for(it_filt_matches = filt_matches.begin(); it_filt_matches < filt_matches.end(); it_filt_matches++)
    //            {
    //                descr_out.push_back(query_desc.row(it_filt_matches->queryIdx));
    //                keyp_out.push_back(query_kpts.at(it_filt_matches->queryIdx));
    //            }
    //            H_out.release();
    //        }

    //        Camera * cur_cam = _cameras->at(camId);
    //        Mat img_track;

    //        vector<Point2f> bBox_old = _B_Box_old;
    //        // vector<Point2f> bBox_new = _B_Box;

    //        Point2f centerOfGravity_old = Point2f(0, 0);
    //        for (int i = 0; i < bBox_old.size(); i++){
    //            centerOfGravity_old.x += bBox_old[i].x / (float) bBox_old.size();
    //            centerOfGravity_old.y += bBox_old[i].y / (float) bBox_old.size();
    //        }

    //        int arrow_l = 5;

    //        Point2f centerOfGravity_new = Point2f(0, 0);



    //        if (cur_cam->getTrackingEnabled()){
    //            if (cur_cam->currentMode()==CAMERA_CTA){ // CTA
    //                img_track = _cameras->at(camId)->CTAFrameClean();
    //                if (hom_ok){
    //                    draw_bounding_box(_Db_bounding_box, img_track, H_out, best_img);

    //                    vector<Point2f> bBox_new = getB_Box();

    //                    for (int i = 0; i < bBox_new.size(); i++){
    //                        centerOfGravity_new.x += bBox_new[i].x / (float) bBox_new.size();
    //                        centerOfGravity_new.y += bBox_new[i].y / (float) bBox_new.size();
    //                    }

    //                    cv::line(img_track, centerOfGravity_old, centerOfGravity_new, CV_RGB(0, 0, 0));
    //                }
    //                else{
    //                    vector<Point2f> temp;
    //                    setB_Box(temp);
    //                }
    //                cur_cam->setCTASlice(img_track, cv::Point(0, 0), cv::Point(img_track.cols, img_track.rows));

    //                // draw line
    //                qDebug() << "drawing bounding box on top of CTA frame";
    //            }
    //            else {

    //                if (cur_cam->showR()){
    //                    img_track = _cameras->at(camId)->ATCRecFrameClean();
    //                    if (hom_ok){
    //                        draw_bounding_box(_Db_bounding_box, img_track, H_out, best_img);

    //                        vector<Point2f> bBox_new = getB_Box();

    //                        for (int i = 0; i < bBox_new.size(); i++){
    //                            centerOfGravity_new.x += bBox_new[i].x / (float) bBox_new.size();
    //                            centerOfGravity_new.y += bBox_new[i].y / (float) bBox_new.size();
    //                        }

    //                        cv::line(img_track, centerOfGravity_old, centerOfGravity_new, CV_RGB(0, 0, 0));
    //                    }
    //                    else{
    //                        vector<Point2f> temp;
    //                        setB_Box(temp);
    //                    }
    //                    cur_cam->setATCRecFrame(img_track);
    //                    qDebug() << "drawing bounding box on top of reconstructed frame";
    //                }

    //                else{
    //                    img_track = _cameras->at(camId)->ATCFrameClean();
    //                    if (hom_ok){
    //                        draw_bounding_box(_Db_bounding_box, img_track, H_out, best_img);

    //                        vector<Point2f> bBox_new = getB_Box();

    //                        for (int i = 0; i < bBox_new.size(); i++){
    //                            centerOfGravity_new.x += bBox_new[i].x / (float) bBox_new.size();
    //                            centerOfGravity_new.y += bBox_new[i].y / (float) bBox_new.size();
    //                        }

    //                        cv::line(img_track, centerOfGravity_old, centerOfGravity_new, CV_RGB(0, 0, 0));
    //                    }
    //                    else{
    //                        vector<Point2f> temp;
    //                        setB_Box(temp);
    //                    }
    //                    cur_cam->setATCBlock(img_track);
    //                }
    //                qDebug() << "drawing bounding box on top of ATC frame";
    //            }

    //        }

    //        //store bounding box
    //        _cameras->at(camId)->setBBox(getB_Box());
    //        if (hom_ok){
    //            vector<Point2f> temp;
    //            temp.push_back(centerOfGravity_old);
    //            temp.push_back(centerOfGravity_new);
    //            cur_cam->setTrack_points(temp);
    //        }
    //        else{
    //            vector<Point2f> temp;
    //            temp.push_back(Point2f(0, 0));
    //            temp.push_back(Point2f(0, 0));
    //            cur_cam->setTrack_points(temp);
    //        }


    //        if (!cur_cam->recognition()){
    //            cur_camview->updateObject("n.d.");
    //        }
    //        /*else{
    //            cur_camview->updateObject(_dbObj.at(best_img));
    //        }
    //        cout<<"OBJECT "<< best_img<<" - "<<_dbObj.at(best_img)<<endl;*/
    //        return best_img;
    //    }
    //cur_camview->updateObject("n.d.");
    return -1;
}

void ObjectTracking::draw_bounding_box( vector<vector<Point2f > > Db_bounding_box, Mat& img_to_show, Mat& H, int best_img )
{
    if (H.empty())
    {}
    else
    {
        vector<Point2f> pts_w(Db_bounding_box.at(best_img).size());
        Mat m_pts_w(pts_w);
        perspectiveTransform(Mat(Db_bounding_box.at(best_img)), m_pts_w, H.inv());
        cv::line(img_to_show, pts_w.at(0), pts_w.at(1), Scalar(0, 255, 0), 2);
        cv::line(img_to_show, pts_w.at(1), pts_w.at(2), Scalar(0, 255, 0), 2);
        cv::line(img_to_show, pts_w.at(2), pts_w.at(3), Scalar(0, 255, 0), 2);
        cv::line(img_to_show, pts_w.at(3), pts_w.at(0), Scalar(0, 255, 0), 2);

        cout<<"Bounding Box ok"<<endl;
        setB_Box(pts_w);
    }
}

std::vector<Camera *> *ObjectTracking::cameras() const
{
    return _cameras;
}

void ObjectTracking::setCameras(std::vector<Camera *> *cameras)
{
    _cameras = cameras;
}
std::vector<CameraView *> *ObjectTracking::camViews() const
{
    return _camViews;
}

void ObjectTracking::setCamViews(std::vector<CameraView *> &camViews)
{
    _camViews = &camViews;
}

void ObjectTracking::setDb_bounding_box(vector<vector<Point2f> > Db_bounding_box){
    _Db_bounding_box = Db_bounding_box;
}
vector<vector<Point2f> > ObjectTracking::getDb_bounding_box() const{
    return _Db_bounding_box;
}
void ObjectTracking::setH_out(cv::Mat H_out){
    _H_out = H_out;
}
cv::Mat ObjectTracking::getH_out() const{
    return _H_out;
}

void ObjectTracking::setBest_img(int best_img){
    _best_img = best_img;
}
int ObjectTracking::getBest_ing() const{
    return _best_img;
}
void ObjectTracking::setB_Box(vector<Point2f> B_Box){
    _B_Box = B_Box;
}
vector<Point2f> ObjectTracking::getB_Box() const{
    return _B_Box;
}

void ObjectTracking::connectTasks(Camera* camera){

    cout << "connecting object tracking task" << endl;
    connect(camera, SIGNAL(frameCompletedSignal(int,int)), this, SLOT(trackObject(int,int)));

}
