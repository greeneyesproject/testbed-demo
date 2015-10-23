#include "InverseBrisk.h"
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <opencv2/opencv.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/legacy/legacy.hpp>
#include <eigen3/Eigen/Dense>
#include <eigen3/Eigen/Sparse>
#include <math.h>
using namespace std;
using namespace cv;
using namespace Eigen;

typedef SparseMatrix<float> SpMat;
typedef Triplet<float> T;

inverse_BRISK* inverse_BRISK::_instance = NULL;

inverse_BRISK* inverse_BRISK::getinverse_BRISK(VisualFeatureExtraction* extractor, int imWidth, int imHeight){
    if (_instance==NULL){
        _instance = new inverse_BRISK(extractor,imWidth,imHeight);
    }
    return _instance;
}

bool kpts_sort_by_size(const KeyPoint_idx &a, const KeyPoint_idx &b)
{
    return (a.keypoint.size > b.keypoint.size);
}



std::vector<Camera *> *inverse_BRISK::cameras() const
{
    return cameras_;
}

void inverse_BRISK::setCameras(std::vector<Camera *> *cameras)
{
    cameras_ = cameras;
}

int inverse_BRISK::nCameras() const
{
    return nCameras_;
}

void inverse_BRISK::setNCameras(int nCameras)
{
    nCameras_ = nCameras;
}
inverse_BRISK::inverse_BRISK(VisualFeatureExtraction* briskExtractor_, int imWidth, int imHeight){
    extractor = briskExtractor_;
    matcher   = new BruteForceMatcher< Hamming > ();
    erosionKernel = (Mat_<uchar>(3,3) << 0, 1, 0, 1, 0, 1, 0, 1, 0);
    imW = imWidth;
    imH = imHeight;

    _running.resize(Camera::getCameras()->size(),false);
}

int inverse_BRISK::build_database(string path){

    bool first_image = true;

    // Get the name of the objects
    vector<string> objects = vector<string>();
    if( getDir(path , objects) != 0){
        cout << "Error in reading the folder!" << endl;
        return -1;
    }

    db_patches.clear();
    db_keypoints.clear();

    for(unsigned int i=0; i<objects.size(); i++){

        // For each object, read the database images
        vector<string> db_files = vector<string>();
        // add slash if necessary
        if (!path.empty() && *path.rbegin() != '/')
            path += '/';

        if( getDir(path + objects[i] , db_files) != 0){
            cout << path + objects[i] << endl;
            cout << "Error in reading image database path!" << endl;
            // return -1;
            continue;
        }

        for(unsigned int j=0; j<db_files.size(); j++){

            string img_name = path + objects[i] + '/' + db_files[j];
            //cout << img_name << endl;

            // Open the current image
            Mat img = imread(img_name);
            if( img.empty() ){
                cout << "Error opening the image" << endl;
                return -1;
            }

            // Convert to gray
            Mat imgGray;
            cvtColor(img,imgGray,CV_BGR2GRAY);

            // Extract keypoints and descriptors
            vector<KeyPoint> tmp_kpts;
            Mat tmp_desc;
            extractor->extractKeypointsFeatures(imgGray, tmp_kpts, tmp_desc);

            // Add keypoints to the database
            for(unsigned int k=0; k<tmp_kpts.size();k++){
                db_keypoints.push_back(tmp_kpts[k]);
            }

            // Add descriptors to the database
            if( first_image == true ){
                db_descriptors = tmp_desc.clone();
                first_image = false;
            }
            else{
                vconcat(db_descriptors, tmp_desc.clone(), db_descriptors);
            }

            // Add the patches to the database
            for(unsigned int k=0; k<tmp_kpts.size(); k++){
                int x = tmp_kpts[k].pt.x;
                int y = tmp_kpts[k].pt.y;
                int d = floor(tmp_kpts[k].size / 2.0);
                Mat patch = Mat(img, Range(y-d, y+d+1), Range(x-d, x+d+1));
                db_patches.push_back(patch.clone());
            }

        }
    }


}

int inverse_BRISK::save_database(){

    // Save Descriptors
    string fn_descriptors  = "db_descriptors.xml";
    FileStorage fs_descriptors(fn_descriptors, FileStorage::WRITE);
    fs_descriptors << "brisk_descriptors" << db_descriptors;
    fs_descriptors.release();
    cout << "Saved descriptors: " << db_descriptors.rows << " x " << db_descriptors.cols << endl;


    // Save Keypoints (after converting them in a suitable representation)
    vector<float> float_kpts;
    for(unsigned int k=0; k<db_keypoints.size(); k++){
        float_kpts.push_back(db_keypoints[k].pt.x);
        float_kpts.push_back(db_keypoints[k].pt.y);
        float_kpts.push_back(db_keypoints[k].size);
        float_kpts.push_back(db_keypoints[k].angle);
    }
    string fn_keypoints    = "db_keypoints.xml";
    FileStorage fs_keypoints(fn_keypoints, FileStorage::WRITE);
    fs_keypoints << "brisk_keypoints" << Mat(float_kpts);
    fs_keypoints.release();
    cout << "Saved keypoints: " << float_kpts.size()/4.0 << endl;


    // Save patches
    string fn_patches    = "db_patches.xml";
    FileStorage fs_patches(fn_patches, FileStorage::WRITE);
    fs_patches << "patches" << db_patches;
    fs_patches.release();
    cout << "Saved patches: " << db_patches.size() << endl;

    return 0;

}

int inverse_BRISK::load_database(){

    // Load Descriptors
    string fn_descriptors  = "db_descriptors.xml";
    FileStorage fs_descriptors(fn_descriptors, FileStorage::READ);
    fs_descriptors["brisk_descriptors"] >> db_descriptors;
    fs_descriptors.release();
    cout << "Loaded descriptors: " << db_descriptors.rows << " x " << db_descriptors.cols << endl;


    // Load Keypoints
    db_keypoints.clear();
    string fn_keypoints    = "db_keypoints.xml";
    FileStorage fs_keypoints(fn_keypoints, FileStorage::READ);
    Mat float_kpts;
    fs_keypoints["brisk_keypoints"] >> float_kpts;
    fs_keypoints.release();
    int num_el = float_kpts.rows;
    for(int k=0; k<num_el; k=k+4){
        float x      = float_kpts.at<float>(0,k);
        float y      = float_kpts.at<float>(0,k+1);
        float size   = float_kpts.at<float>(0,k+2);
        float angle  = float_kpts.at<float>(0,k+3);
        db_keypoints.push_back(KeyPoint(x,y,size,angle));
    }
    cout << "Loaded keypoints: " << db_keypoints.size() << endl;


    // Load patches
    string fn_patches    = "db_patches.xml";
    FileStorage fs_patches(fn_patches, FileStorage::READ);
    fs_patches["patches"] >> db_patches;
    fs_patches.release();
    cout << "Loaded patches: " << db_patches.size() << endl;

    return 0;
}

int inverse_BRISK::get_features(string inputImg, Mat &descriptors, vector<KeyPoint> &keypoints){

    // Open the current image
    Mat img = imread(inputImg);
    if( img.empty() ){
        cout << "Error opening the image" << endl;
        return -1;
    }

    // Convert to gray
    Mat imgGray;
    cvtColor(img,imgGray,CV_BGR2GRAY);

    // Extract keypoints and descriptors
    keypoints.clear();
    extractor->extractKeypointsFeatures(imgGray, keypoints, descriptors);

    return 0;
}

Mat inverse_BRISK::get_patch(int id){

    if( (id>=0) & (id<(int)db_patches.size()) ){
        return db_patches[id];
    }

    return Mat();

}


Mat inverse_BRISK::invert_BRISK(Mat descriptors, vector<KeyPoint> keypoints, int dist_threshold){

    double compTime = (double)cv::getTickCount();
    vector<KeyPoint_idx> kpts_idx;
    for(unsigned int k=0; k<keypoints.size(); k++){
        kpts_idx.push_back(KeyPoint_idx(keypoints[k],k));
    }


    // Sort the keypoints by size
    sort(kpts_idx.begin(),kpts_idx.end(),kpts_sort_by_size);

    // Extract sorted keypoints and descriptors
    vector<KeyPoint> sort_kpts;
    Mat              sort_desc = Mat::zeros(descriptors.size(),descriptors.type());

    for(unsigned int k=0; k<keypoints.size(); k++){
        sort_kpts.push_back(kpts_idx[k].keypoint);
        int idx = kpts_idx[k].index;
        descriptors.row(idx).copyTo(sort_desc.row(k));
    }

    // Compute the matches
    vector<vector<DMatch> > all_matches;
    matcher->knnMatch(sort_desc, db_descriptors, all_matches, 1);

    // Define the image to be reconstructed
    Mat rec_image = Mat(imH,imW, CV_8UC3, Scalar(0,0,0));

    // Define the global mask
    Mat glob_mask = Mat(imH,imW, CV_8UC1, Scalar(0));

    // For all the matches....
    int num_matches = (int)all_matches.size();
    cout << "num_matches: " << num_matches << endl;
    compTime = ((double)cv::getTickCount() - compTime)/cv::getTickFrequency();
    cout << "Time for matching: " << compTime << " sec" << endl;


    compTime = (double)cv::getTickCount();
    for(int i=0; i<num_matches; i++){

        if(all_matches[i][0].distance <= dist_threshold){

            // get the current patch
            int patch_id = all_matches[i][0].trainIdx;

            Mat cur_patch = get_patch(patch_id);


            // scale and rotate the current patch
            float scale = sort_kpts[i].size / db_keypoints[patch_id].size;
            float theta = db_keypoints[patch_id].angle - sort_kpts[i].angle;
            Mat srcPatch, srcMask, contour;
            scale_rot_patch(cur_patch, srcPatch, srcMask, contour, theta, scale);

            // superimpose the patch
            int r = round(sort_kpts[i].pt.y);
            int c = round(sort_kpts[i].pt.x);
            int d = (srcPatch.rows-1)/2;

            // select the destination region and its mask
            cout << "inv brisk: ok1" << "r = " << r << ", c = " << c << ", d = " << d << endl;
            Mat dstPatch = rec_image.rowRange(r-d,r+d+1).colRange(c-d,c+d+1);
            Mat dstMask  = glob_mask.rowRange(r-d,r+d+1).colRange(c-d,c+d+1);
             cout << "inv brisk: ok2" << endl;

            poisson_stitch(srcPatch,srcMask,dstPatch,dstMask);

            // Update the global mask
            srcMask.copyTo(dstMask,srcMask);

        }


    }


    compTime = ((double)cv::getTickCount() - compTime)/cv::getTickFrequency();
    cout << "Time for drawing: " << compTime << " sec" << endl;

    return rec_image;

}

// PRIVATE METHODS

int inverse_BRISK::scale_rot_patch(Mat inPatch, Mat &outPatch, Mat &mask, Mat &contour, float theta, float scale){

    int ctr = (inPatch.rows-1)/2;

    int in_ps = inPatch.rows;
    int out_ps = ceil(scale * sqrt(2) * (float)(inPatch.rows + 1));
    if (out_ps%2 == 0){
        out_ps++;
    }
    int d = out_ps - in_ps;
    Mat H;
    H = getRotationMatrix2D(Point2f(ctr,ctr), theta, scale);
    H.row(0).col(2) += d/2;
    H.row(1).col(2) += d/2;

    outPatch = Mat(Size(out_ps, out_ps), inPatch.type());
    warpAffine(inPatch, outPatch, H, outPatch.size());

    // Find the mask of the patch
    vector<Point> verteces;
    int in_rho = (inPatch.rows - 5)/2;
    int out_rho = (out_ps - 1)/2;
    for(int i=0; i<4; i++){
        float phi = theta*M_PI/180.0 + i*M_PI/2.0;
        int r = round( scale*in_rho*(cos(phi)-sin(phi)) + out_rho);
        int c = round( scale*in_rho*(cos(phi)+sin(phi)) + out_rho);
        verteces.push_back(Point(c,r));
    }
    vector<Point> ROI_Poly;
    approxPolyDP(verteces, ROI_Poly, 1.0, true);

    mask = Mat::zeros(outPatch.size(),CV_8UC1);
    fillConvexPoly(mask,&ROI_Poly[0],ROI_Poly.size(),1);

    // Find the contour of the patch
    vector< vector<Point> > contour_vec;
    contour_vec.push_back(verteces);
    contour = Mat::zeros(outPatch.size(), CV_8UC1);
    drawContours(contour,contour_vec,0,Scalar(1));

    return 0;

}


void inverse_BRISK::poisson_stitch(Mat srcPatch, Mat srcMask, Mat dst, Mat dstMask){

    int mode = 1;

    // Compute the overlap of srcPatch and dst
    Mat overlap = dstMask.mul(srcMask);
    int ovlSize = sum(overlap).val[0];

    // Some initializations
    int problemSize = 0;
    Mat intMask;
    Mat bounds = Mat::zeros(srcPatch.size(),srcPatch.type());

    if( ovlSize>0 && mode !=0 ){

        // Get the contour of the srcPatch
        Mat intSrcMask;
        erode(srcMask,intSrcMask,erosionKernel);
        Mat srcContour = srcMask-intSrcMask;

        // Set the mask to work on
        intMask = intSrcMask;

        // Compute the boundary conditions
        Mat boundOverlap = dstMask.mul(srcContour);
        srcPatch.copyTo(bounds,srcContour);
        dst.copyTo(bounds,boundOverlap);

        problemSize = sum(intMask).val[0];

    }


    if(mode!=0 && problemSize>0 && problemSize<2500){ // if they overlap, perform Poisson blending


        // build an indexed map of the intMask
        Mat idxMap = Mat::zeros(intMask.size(),CV_32S);
        int index = 1;
        vector<Point> pts;
        for(int r=0; r<intMask.rows; r++){
            for(int c=0; c<intMask.cols; c++){
                if(intMask.at<uchar>(r,c) == 1){
                    idxMap.at<int>(r,c) = index;
                    pts.push_back(Point(c,r));
                    index++;
                }
            }
        }

        // build the system matrix M and the vector of coefficients b
        index--;
        int L = index;

        int numChannels = srcPatch.channels();
        int numCols     = srcPatch.cols;


        SpMat     M_sp(L,L);       // the sparse matrix
        vector<T> M_coeff;         // coefficients of the sparse matrix
        MatrixXf b(L,numChannels); // coefficients vector

        for(index = 0; index<L; index++){

            // get the current point
            Point curPt = pts[index];

            // get the 4-neighborhood points
            Point nl = Point(curPt.x-1,curPt.y); // left
            int i_nl = idxMap.at<int>(nl);
            Point nr = Point(curPt.x+1,curPt.y); // right
            int i_nr = idxMap.at<int>(nr);
            Point nb = Point(curPt.x,curPt.y-1); // bottom
            int i_nb = idxMap.at<int>(nb);
            Point nt = Point(curPt.x,curPt.y+1); // top
            int i_nt = idxMap.at<int>(nt);

            int row = curPt.y;
            int col = curPt.x;

            // compute the Laplacian of srcPatch at curPt
            for(int ch=0; ch<numChannels; ch++){

                b(index,ch) =  (float)4*srcPatch.at<Vec3b>(row,col)[ch]
                              -(float)srcPatch.at<Vec3b>(row-1,col)[ch]
                              -(float)srcPatch.at<Vec3b>(row+1,col)[ch]
                              -(float)srcPatch.at<Vec3b>(row,col-1)[ch]
                              -(float)srcPatch.at<Vec3b>(row,col+1)[ch];
            }



            // set the diagonal elements
            M_coeff.push_back(Triplet<float>(index,index,4.0));

            // set the neighbor elements
            if( i_nl > 0 ){ // left
                M_coeff.push_back(Triplet<float>(index,i_nl-1,-1.0));
            }
            else{
                for(int ch=0; ch<numChannels; ch++){
                    b(index,ch) += bounds.data[row*numChannels*numCols + (col-1)*numChannels + ch];
                }
            }

            if( i_nr > 0 ){ // right
                M_coeff.push_back(Triplet<float>(index,i_nr-1,-1.0));
            }
            else{
                for(int ch=0; ch<numChannels; ch++){
                    b(index,ch) += bounds.data[row*numChannels*numCols + (col+1)*numChannels + ch];
                }
            }

            if( i_nb > 0 ){ // bottom
                M_coeff.push_back(Triplet<float>(index,i_nb-1,-1.0));
            }
            else{
                for(int ch=0; ch<numChannels; ch++){
                    b(index,ch) += bounds.data[(row-1)*numChannels*numCols + col*numChannels + ch];
                }
            }

            if( i_nt > 0 ){ // top
                M_coeff.push_back(Triplet<float>(index,i_nt-1,-1.0));
            }
            else{
                for(int ch=0; ch<numChannels; ch++){
                    b(index,ch) += bounds.data[(row+1)*numChannels*numCols + col*numChannels + ch];
                }
            }


        }

        // fill the sparse matrix
        M_sp.setFromTriplets(M_coeff.begin(),M_coeff.end());

        //double compTime = (double)cv::getTickCount();
        SimplicialCholesky<SpMat> chol(M_sp);
        MatrixXf sol = chol.solve(b);
        //compTime = ((double)cv::getTickCount() - compTime)/cv::getTickFrequency();
        //cout << "RISOLTO (sparse) in " << compTime << " secondi" << endl;

        // fill the dst patch with the interpolated values
        for(index=0; index<L; index++ ){
            int col = pts[index].x;
            int row = pts[index].y;
            for(int ch=0; ch<numChannels; ch++){
                float aux = sol(index,ch);
                if(aux<0)
                    aux = 0;
                else if (aux>255){
                    aux = 255;
                }
                dst.at<Vec3b>(row,col)[ch] = (uchar)aux;
            }
        }

    }
    else{ // else copy the srcPatch as it is
        srcPatch.copyTo(dst,srcMask);
    }

    return;

}

void inverse_BRISK::invertBRISKOnCamSlot(int camId){

    Camera * cur_cam = (*(cameras()))[camId];
    if (!_running.at(camId)){
        _running.at(camId) = true;
        cv::Mat inverse = invert_BRISK(cur_cam->getGoodDescriptors(), cur_cam->getGoodKeypoints());
        if (cur_cam->getShowReconstruction()){
            cur_cam->setATCRecFrame(inverse);
        }
        _running.at(camId) = false;
    }
}

void inverse_BRISK::connectTasks(int nCam){

    for (int i = 0; i < nCam; i++){
        Camera * a = (*cameras_)[i];

        std::cout << "connecting inverse BRISK task" << std::endl << std::endl;
        connect(a, SIGNAL(reconstructFrameSignal(int)), this, SLOT(invertBRISKOnCamSlot(int)));
    }

}
