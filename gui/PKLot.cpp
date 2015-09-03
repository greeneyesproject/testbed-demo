#include "PKLot.h"
#include <QDomElement>
#include <QDomNodeList>
#include <QFile>
#include <iostream>
#include <Global.h>

using namespace std;

PKLot::PKLot(int id, cv::RotatedRect& rotatedRect, std::vector<cv::Point> rotatedContour){
    _id = id;
    _rotatedRect=rotatedRect;
    _rotatedContour = rotatedContour;

}

int PKLot::getKpt(cv::KeyPoint& kpt){
    kpt.pt = _rotatedRect.center;
    kpt.size = min(_rotatedRect.boundingRect().height,_rotatedRect.boundingRect().width)*PKLOT_KEYPOINT_SCALE_FACTOR;
    return 0;
}

void PKLot::drawLines(cv::Mat &img){
    cv::line(img,_rotatedContour.at(0),_rotatedContour.at(1),PKLOT_LINES_COLOR,PKLOT_LINES_THICKNESS);
    cv::line(img,_rotatedContour.at(1),_rotatedContour.at(2),PKLOT_LINES_COLOR,PKLOT_LINES_THICKNESS);
    cv::line(img,_rotatedContour.at(2),_rotatedContour.at(3),PKLOT_LINES_COLOR,PKLOT_LINES_THICKNESS);
    cv::line(img,_rotatedContour.at(3),_rotatedContour.at(0),PKLOT_LINES_COLOR,PKLOT_LINES_THICKNESS);
}

void PKLot::fill(cv::Mat& img){
    cv::Scalar color = _occupied ? PKLOT_OCCUPIED_COLOR : PKLOT_FREE_COLOR;
    cv::fillConvexPoly(img,_rotatedContour,color);
}

int PKLot::parse(string filename,vector<PKLot>& parkingLots){

    //The QDomDocument class represents an XML document.
    QDomDocument xmlBOM;

    // Load xml file as raw data
    QFile f(filename.c_str());
    if (!f.open(QIODevice::ReadOnly )){
        // Error while loading file
        std::cerr << "ParseNetworkConfigurationXML: Error while loading " << filename << std::endl;
        return 1;
    }

    // Set data into the QDomDocument before processing
    xmlBOM.setContent(&f);
    f.close();

    QDomNodeList spaceNodeList = xmlBOM.elementsByTagName("space");

    for (int spaceNodeIdx = 0; spaceNodeIdx < spaceNodeList.length(); spaceNodeIdx++){
        QDomNode spaceNode = spaceNodeList.item(spaceNodeIdx);
        int spaceId = spaceNode.attributes().namedItem("id").nodeValue().toInt();

        QDomElement rotatedRectElement = spaceNode.firstChildElement("rotatedRect");
        QDomElement centerElement = rotatedRectElement.firstChildElement("center");
        cv::Point center = cv::Point(
                    centerElement.attribute("x").toUInt(),
                    centerElement.attribute("y").toUInt()
                    );
        QDomElement sizeElement = rotatedRectElement.firstChildElement("size");
        cv::Size size = cv::Size(
                    sizeElement.attribute("w").toUInt(),
                    sizeElement.attribute("h").toUInt()
                    );
        QDomElement angleElement = rotatedRectElement.firstChildElement("angle");
        float angle = angleElement.attribute("d").toFloat();

        cv::RotatedRect rotatedRect = cv::RotatedRect(center,size,angle);

        QDomElement rotatedContourElement = spaceNode.firstChildElement("contour");
        QDomNodeList rotatedContourPointList = rotatedContourElement.elementsByTagName("point");
        std::vector<cv::Point> rotatedContour;
        for (int pointIdx = 0; pointIdx < 4; pointIdx++){
            QDomNode pointNode = rotatedContourPointList.item(pointIdx);
            rotatedContour.push_back(cv::Point(
                                         pointNode.attributes().namedItem("x").nodeValue().toInt(),
                                         pointNode.attributes().namedItem("y").nodeValue().toInt()
                                         ));
        }

        parkingLots.push_back(PKLot(spaceId,rotatedRect,rotatedContour));

    }

    return 0;
}
