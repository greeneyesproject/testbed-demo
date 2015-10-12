/**
 * Dependencies:
 * - opencv
 * - boost
 * - tclap
 * - tinyxml2
 */
#include <NodeProcessingSystem.h>
#include <Network/NetworkNode.h>
#include <tinyxml/include/tinyxml2.h>
#include <opencv2/core/core.hpp>
#include <tclap/CmdLine.h>
#include <boost/asio.hpp>
#include <Network/NodeNetworkSystem.h>
#include <TestbedTypes.h>
#include <Messages/StartATCMsg.h>
#include <Messages/StartCTAMsg.h>
#include <stdio.h>
#include <iostream>
#include <signal.h>

#include <gpio/BlackGPIO.h>

#define _DEBUG 1

using namespace std;
using namespace boost::asio::ip;

int main(int argc, char ** argv) {

	float startupTime = cv::getTickCount();

	BlackLib::BlackGPIO* gpios[6];
	/* Loading pin */
	gpios[0] = new BlackLib::BlackGPIO(BlackLib::GPIO_80, BlackLib::output,
			BlackLib::FastMode);
	/* Acquisition pin */
	gpios[1] = new BlackLib::BlackGPIO(BlackLib::GPIO_79, BlackLib::output,
			BlackLib::FastMode);
	/* Detection pin */
	gpios[2] = new BlackLib::BlackGPIO(BlackLib::GPIO_77, BlackLib::output,
			BlackLib::FastMode);
	/* Extraction pin */
	gpios[3] = new BlackLib::BlackGPIO(BlackLib::GPIO_75, BlackLib::output,
			BlackLib::FastMode);
	/* Coding pin */
	gpios[4] = new BlackLib::BlackGPIO(BlackLib::GPIO_73, BlackLib::output,
			BlackLib::FastMode);
	/* Transmission pin */
	gpios[5] = new BlackLib::BlackGPIO(BlackLib::GPIO_71, BlackLib::output,
			BlackLib::FastMode);

	gpios[0]->setValue(BlackLib::high); //Set loading pin
	gpios[1]->setValue(BlackLib::low);  //Reset acquisition pin
	gpios[2]->setValue(BlackLib::low);  //Reset detection pin
	gpios[3]->setValue(BlackLib::low);  //Reset extraction pin
	gpios[4]->setValue(BlackLib::low);  //Reset coding pin
	gpios[5]->setValue(BlackLib::low);  //Reset transmission pin

	/**
	 * Command line arguments parser
	 */
	NodeType nodeType = NODETYPE_UNDEF;
	string telosDevPath, networkConfigPath;

	vector<string> remoteIps;
	vector<uint16_t> remotePorts;
	vector<uint16_t> localPorts;

	uint8_t nodeId = 0, cameraId = 0;
	bool cameraFlip = false;
	bool nodeIdFound = false;

	bool oneShot = false;
	string mode = "cta";
	int ctaQf = 100;
	int ctaSlices = 1;
	int atcDetTh = 60;
	int atcNumfeat = 40;
	int atcBinShift = 0;
	int atcValShift = 0;

	try {

		TCLAP::CmdLine cmd("GreenEyes Testbed", ' ', "2.0");

		vector<string> nodeTypeArgValues;
		nodeTypeArgValues.push_back("sink");
		nodeTypeArgValues.push_back("camera");
		nodeTypeArgValues.push_back("cooperator");
		TCLAP::ValuesConstraint<string>* nodeTypeArgValuesAllowed =
				new TCLAP::ValuesConstraint<string>(nodeTypeArgValues);

		/* Positional arguments */
		TCLAP::UnlabeledValueArg<string> nodeTypeArg("type", "Node type", true,
				"null", "string", nodeTypeArgValuesAllowed);
		cmd.add(nodeTypeArg);

		TCLAP::UnlabeledValueArg<int> nodeIdArg("id", "Node id", true, -1,
				"int");
		cmd.add(nodeIdArg);

		/* Non positional arguments */
		TCLAP::ValueArg<string> networkConfigArg("", "network-config",
				"Network configuration path", false, "network_config.xml",
				"string");
		cmd.add(networkConfigArg);

		TCLAP::ValueArg<string> telosDevPathArg("", "telos",
				"Telos device path", false, "null", "string");
		cmd.add(telosDevPathArg);

		TCLAP::ValueArg<int> cameraIdArg("", "camera", "Camera id", false, 0,
				"int");
		cmd.add(cameraIdArg);

		TCLAP::SwitchArg cameraFlipArg("", "camera-flip",
				"Camera horizontal flip", false);
		cmd.add(cameraFlipArg);

		/* One-shot non positional arguments */
		TCLAP::SwitchArg oneShotArg("", "oneshot", "Enable One-shot mode",
				false);
		cmd.add(oneShotArg);

		vector<string> oneshotModeArgValues;
		oneshotModeArgValues.push_back("atc-object");
		oneshotModeArgValues.push_back("atc-pklot");
		oneshotModeArgValues.push_back("cta");
		TCLAP::ValuesConstraint<string>* oneshotModeArgValuesAllowed =
				new TCLAP::ValuesConstraint<string>(oneshotModeArgValues);
		TCLAP::ValueArg<string> modeTypeArg("", "mode", "One-shot mode", false,
				"null", oneshotModeArgValuesAllowed);
		cmd.add(modeTypeArg);

		TCLAP::ValueArg<int> ctaQfArg("", "qf", "CTA quality factor", false, 20,
				"int");
		cmd.add(ctaQfArg);

		TCLAP::ValueArg<int> ctaSlicesArg("", "slices", "CTA slices", false, 5,
				"int");
		cmd.add(ctaSlicesArg);

		TCLAP::ValueArg<int> atcThArg("", "det-th", "ATC detection threshold",
				false, 60, "int");
		cmd.add(atcThArg);

		TCLAP::ValueArg<int> atcFeatArg("", "num-feat",
				"ATC number of features", false, 20, "int");
		cmd.add(atcFeatArg);

		TCLAP::ValueArg<int> atcBinShiftArg("", "bin-shift",
				"ATC PKLot bin shift", false, 0, "int");
		cmd.add(atcBinShiftArg);

		TCLAP::ValueArg<int> atcValShiftArg("", "val-shift",
				"ATC PKLot val shift", false, 0, "int");
		cmd.add(atcValShiftArg);

		/* Parse the argv array. */
		cmd.parse(argc, argv);

		if (nodeTypeArg.getValue() == "sink") {
			nodeType = NODETYPE_SINK;
		} else if (nodeTypeArg.getValue() == "camera") {
			nodeType = NODETYPE_CAMERA;
		} else {
			nodeType = NODETYPE_COOPERATOR;
		}
		nodeId = nodeIdArg.getValue();
		cameraId = cameraIdArg.getValue();
		telosDevPath = telosDevPathArg.getValue();
		networkConfigPath = networkConfigArg.getValue();
		cameraFlip = cameraFlipArg.getValue();

		oneShot = oneShotArg.getValue();
		mode = modeTypeArg.getValue();
		ctaQf = ctaQfArg.getValue();
		ctaSlices = ctaSlicesArg.getValue();
		atcDetTh = atcThArg.getValue();
		atcNumfeat = atcFeatArg.getValue();
		atcBinShift = atcBinShiftArg.getValue();
		atcValShift = atcValShiftArg.getValue();

	} catch (TCLAP::ArgException &e) {
		cerr << "Parser error: " << e.error() << " for arg " << e.argId()
				<< endl;
	}

	/*
	 * Network config parser
	 */
	tinyxml2::XMLDocument* xmlDoc = new tinyxml2::XMLDocument();
	xmlDoc->LoadFile(networkConfigPath.c_str());
	if (xmlDoc->Error()) {
		cerr << "Main: xmlDoc error: " << xmlDoc->GetErrorStr1() << endl;
		exit(-1);
	}
	tinyxml2::XMLElement* xmlNetwork = xmlDoc->FirstChildElement("network");

	/*
	 * Parse network items to find the identity of this node and the connection parameters to the server(s)
	 */
	tinyxml2::XMLElement* xmlItem = xmlNetwork->FirstChildElement("item");
	while (xmlItem) {
		const char* itemEnabled = xmlItem->Attribute("enabled");
		if (itemEnabled == NULL || !strcmp(itemEnabled, "1")) {
			/* Item enabled or not specified */

			const char* itemClassStr = xmlItem->Attribute("class");
			string itemClass(itemClassStr);

			const char* itemIdStr = xmlItem->Attribute("id");
			uint8_t itemId = itemIdStr ? atoi(itemIdStr) : 0;

			const char* itemAmAddrStr = xmlItem->Attribute("am_address");
			uint16_t itemAmAddr = itemAmAddrStr ? atoi(itemAmAddrStr) : 0;

			const char* itemIpAddr = xmlItem->Attribute("ip_address");
			const char* itemIpPortServerStr = xmlItem->Attribute(
					"ip_port_server");
			uint16_t itemIpPortServer =
					itemIpPortServerStr ? atoi(itemIpPortServerStr) : -1;

			if (itemClass == "gui") {
				/* The gui is relevant only for the SINK node */
				if (nodeType == NODETYPE_SINK) {
					NetworkNode::setGui(itemIpAddr, itemIpPortServer);
					remoteIps.push_back(itemIpAddr);
					remotePorts.push_back(itemIpPortServer);
				}
			} else if (itemClass == "sink") {
				/* The sink node is relevant only for SINK and CAMERA nodes */
				switch (nodeType) {
				case NODETYPE_SINK:
					NetworkNode::setMyself(
							NetworkNode::setSink(itemId, itemAmAddr, itemIpAddr,
									itemIpPortServer));
					nodeIdFound = true;
					break;
				case NODETYPE_CAMERA:
					NetworkNode::setSink(itemId, itemAmAddr, itemIpAddr,
							itemIpPortServer);
					remoteIps.push_back(itemIpAddr);
					remotePorts.push_back(itemIpPortServer);
					break;
				default:
					break;
				}
			} else if (itemClass == "camera") {
				/* The camera node is relevant only for SINK, CAMERA with the same id and for COOPERATORs */
				switch (nodeType) {
				case NODETYPE_SINK:
					NetworkNode::addCamera(itemId, itemAmAddr, itemIpAddr,
							itemIpPortServer);
					break;
				case NODETYPE_CAMERA:
					if (nodeId == itemId) {
						NetworkNode::setMyself(
								NetworkNode::addCamera(itemId, itemAmAddr,
										itemIpAddr, itemIpPortServer));
						nodeIdFound = true;
					}
					break;
				case NODETYPE_COOPERATOR:
					NetworkNode::addCamera(itemId, itemAmAddr, itemIpAddr,
							itemIpPortServer);
					/* Not adding the camera port here since not all the cooperators connect to all the cameras. Links are parsed later on */
					break;
				default:
					break;
				}
			} else if (itemClass == "cooperator") {
				/* The cooperator node is only relevant for the COOPERATOR with the same id */
				switch (nodeType) {
				case NODETYPE_COOPERATOR:
					if (nodeId == itemId) {
						NetworkNode::setMyself(
								NetworkNode::addCooperator(itemId));
						nodeIdFound = true;
					}
					break;
				default:
					break;
				}
			}
			xmlItem = xmlItem->NextSiblingElement("item");
		}
	}

	/*
	 * If this is a COOPERATOR node parse the links to find to which cameras connect
	 */
	if (nodeType == NODETYPE_COOPERATOR) {
		tinyxml2::XMLElement* xmlLink = xmlNetwork->FirstChildElement("link");
		while (xmlLink) {
			const char* itemEnabled = xmlLink->Attribute("enabled");
			if (itemEnabled == NULL || !strcmp(itemEnabled, "1")) {
				/* Item enabled or not specified */

				const char* sourceIdStr = xmlLink->Attribute("source_id");
				uint8_t sourceId = sourceIdStr ? atoi(sourceIdStr) : 0;
				const char* destIdStr = xmlLink->Attribute("dest_id");
				uint8_t destId = destIdStr ? atoi(destIdStr) : 0;

				if (sourceId == nodeId) {
					NetworkNode* camera = NetworkNode::getCameraById(destId);
					if (camera) {
						remoteIps.push_back(camera->getIpAddrString());
						remotePorts.push_back(camera->getIpPort());
					} else {
						cerr << "Main: Camera with id " << destId
								<< " not found " << endl;
						exit(-1);
					}
				}
			}
			xmlLink = xmlLink->NextSiblingElement("link");
		}
	}

	delete (xmlDoc);
	xmlDoc = NULL;

	if (!nodeIdFound) {
		cerr << "Node id not found." << endl;
		exit(1);
	}

	NetworkNode::printNetwork();

	/* Create the io_service */
	boost::asio::io_service io_service;

	/* Video stream */
	stringstream ss;
	if (oneShot) {
		ss << "oneshot/oneshot_%02d.jpg";
	} else {
		ss << "pklot/camera" << (int) nodeId << "_%04d.jpg";
	}

	/* Create the NodeNetworkSystem. Needs to be instantiated before the NodeProcessingSystem */
	NodeNetworkSystem* nodeNetworkSystem = NodeNetworkSystem::getInstance(
			io_service, remoteIps,
			remotePorts, telosDevPath, gpios);

	/* Create the NodeProcessingSystem */
	NodeProcessingSystem* nodeProcessingSystem =
			NodeProcessingSystem::getInstance(nodeNetworkSystem,nodeType,
					CameraParameters(cameraId, ss.str(), cv::Size(640, 480),
							cameraFlip), oneShot, gpios);

	gpios[0]->setValue(BlackLib::low);  //Reset loading pin

	startupTime = (cv::getTickCount() - startupTime) / cv::getTickFrequency();
	cout << "Startup time: " << startupTime << "s" << endl;

	/* OneShot mode */
	if (oneShot) {

		cout << "One-shot Mode" << endl;
		Message* fakeStart;
		LinkType fakeStartLinkType;
		if (telosDevPath != "null") {
			fakeStartLinkType = LINKTYPE_TELOS;
		} else {
			fakeStartLinkType = LINKTYPE_TCP;
		}
		if (mode == "atc-object") {
			Bitstream emptyBitstream;
			fakeStart = (Message*) new StartATCMsg(NetworkNode::getSink(),
					NetworkNode::getMyself(), fakeStartLinkType,
					DETECTORTYPE_BRISK, atcDetTh, //Detection Threshold
					DESCRIPTORTYPE_BRISK, 256, //Description Length
					atcNumfeat, //Number of features
					true, //Rotation invariance
					true, //Keypoints encoding
					true, //Features encoding
					true, //Keypoints transfer
					true, //Scale transfer
					true, //Orientation transfer
					atcNumfeat, //Number of features per block
					cv::Size(0,0), //topLeft
					cv::Size(640,480), //bottomRight
					0, //binShift
					0, //valShift
					0, //numcoops
					OPERATIVEMODE_OBJECT,
					emptyBitstream, //kpts bitstream
					80 //wifi bw
					);
		}
		else if (mode == "atc-pklot") {

			cv::Mat atcOneShotKeypoints = cv::imread("pklot/camera_12_kptsBitstream.bmp",cv::IMREAD_UNCHANGED);
			Bitstream atcOneShotKeypointsBitstream(atcOneShotKeypoints.datastart,atcOneShotKeypoints.dataend);
			fakeStart = (Message*) new StartATCMsg(NetworkNode::getSink(),
					NetworkNode::getMyself(), fakeStartLinkType,
					DETECTORTYPE_NONE, 0, //Detection Threshold (ignore)
					DESCRIPTORTYPE_HIST, 0, //Description Length (ignored)
					0, //Number of features (ignored)
					true, //Rotation invariance
					true, //Keypoints encoding
					true, //Features encoding
					true, //Keypoints transfer
					true, //Scale transfer
					true, //Orientation transfer
					200, //Number of features per block
					cv::Size(0, 0), //top left
					cv::Size(640, 480), //bottom right
					atcBinShift,
					atcValShift,
					0, //Num coops
					OPERATIVEMODE_PKLOT,
					atcOneShotKeypointsBitstream,
					80 //kpbs wifibw
					);
		}else {
			fakeStart = (Message*) new StartCTAMsg(NetworkNode::getSink(),
					NetworkNode::getMyself(), fakeStartLinkType, ctaQf, // Quality factor
					cv::Size(640, 480), //Frame size
					ctaSlices, // Number of slices
					OPERATIVEMODE_OBJECT,
					80 //kpbs wifibw
					);
		}
		nodeProcessingSystem->queueMessage(fakeStart);
	}

	signal(SIGINT, NodeNetworkSystem::shutdown);
	signal(SIGQUIT, NodeNetworkSystem::shutdown);
	//signal(SIGABRT, NodeNetworkSystem::shutdown);
	//signal(SIGTERM, NodeNetworkSystem::shutdown);
	//signal(SIGKILL, NodeNetworkSystem::shutdown);

	/* Start the io_service */
	try {
		io_service.run();
	} catch (exception &e) {
		cerr << "Main: " << e.what() << endl;
	}

	return 0;
}
