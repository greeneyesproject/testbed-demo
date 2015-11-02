#ifndef GLOBAL_H
#define GLOBAL_H

typedef unsigned char uchar;

// Please define your project path
#define PROJECT_PATH "./"

// Please define image path for BRISK2image reconstruction
#define DB_IMAGE_PATH "./db/obj-reconstruction/"

// Please define object_DB path for object recognition
#define DB_PATH "./db/obj-recognition/"

// Please define bounding box file path
#define BBOX_PATH DB_PATH

// DO NOT TOUCH
/* Parameters for BeagleBoneBlack */
#define CPU_POWER_BBB 2 //Full CPU Power
#define TX_POWER_TELOS_BBB 1.625 //TX + IDLE
#define TX_POWER_WIFI_BBB (1.122+1.5) //tx + IDLE

#define WIFI_BW_DEFAULT 30 //kbps. 30kbps gives the same application framerate as telosb

#define MAX_FEAT 200
#define MIN_FEAT 10
#define MAX_QF 100
#define MIN_QF 1
#define MAX_DETTH 120
#define MIN_DETTH 20
#define MIN_BINSHIFT 0
#define MAX_BINSHIFT 6
#define MIN_VALSHIFT 0
#define MAX_VALSHIFT 10

#define CAMERA_IM_PATH PROJECT_PATH "Network_images/camera.jpeg"
#define CAMERA_IM_PATH_SELECTED PROJECT_PATH "Network_images/camera.jpeg"
#define RELAY_IM_PATH PROJECT_PATH "Network_images/telosb.jpeg"
#define RELAY_IM_PATH_SELECTED PROJECT_PATH "Network_images/telosb.jpeg"
#define SINK_IM_PATH PROJECT_PATH "Network_images/sink.png"
#define SINK_IM_PATH_SELECTED PROJECT_PATH "Network_images/sink_selected_new.png"
#define COOP_IM_PATH PROJECT_PATH "Network_images/beaglebone.jpeg"
#define COOP_IM_PATH_SELECTED PROJECT_PATH "Network_images/beaglebone.jpeg"
#define COOP_IM_PATH_AVAILABLE PROJECT_PATH "Network_images/beaglebone.jpeg"

#define XML_NETWORK_PATH PROJECT_PATH "../vsn/network_config.xml"

#define NETWORK_TOPOLOGY_ROWS 8
#define NETWORK_TOPOLOGY_COLS 8

#define FRAME_W 640
#define FRAME_H 480

#define NBS_OVL 0.15

#define GUI_LISTENING_TCP_PORT 1234

/* Parking Lot */
#define PKLOT_XML_PATH "./pklot/camera"
#define PKLOT_SVM_PATH_PREFIX "./pklot/svm_"
#define PKLOT_SVM_PATH_SUFFIX ".xml"
#define PKLOT_BKG_COLOR cv::Scalar::all(100)
#define PKLOT_LINES_COLOR cv::Scalar::all(220)
#define PKLOT_LINES_THICKNESS 2
#define PKLOT_FREE_COLOR cv::Scalar(0,255,0)
#define PKLOT_OCCUPIED_COLOR cv::Scalar(0,0,255)
#define PKLOT_CTA_ALPHA 0.5
#define PKLOT_KEYPOINT_SCALE_FACTOR 0.5
#define PKLOT_MEDIAN_FILTER_LENGTH 1

enum cta_op_mode {CTAONESHOT = 0, CTASTREAMING = 1};
enum atc_op_mode {ATCONESHOT = 0, ATCSTREAMING = 1};
enum det_type {BRISKDET = 0, SIFTDET = 1};
enum desc_type {BRISKDESC = 0, SIFTDESC= 1};
enum atc_coding_info {FULL = 0, DESCONLY = 1};

enum plot_var {OP_MODE = 0, FRAMERATE = 1, DET_TH = 2, MAX_F = 3, ENCODE_KP = 4, ENTROPY = 5, COOP = 6, QF = 7, ENERGY = 8, PLOT_VAR_SIZE = 9};


#define PATH_BRISK_FILES PROJECT_PATH "../vsn/util/brisk"

#define BUFFER_SIZE 500000

#define MIN_MATCHES_TO_BE_RECOGNIZED 5

#endif // GLOBAL_H

