#-------------------------------------------------
# Dependencies:
# - opencv 2.4.9+
# - boos
# - eigen 3
# ------------------------------------------------

QT       += core gui xml

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

TARGET = greeneyes-gui
TEMPLATE = app

DEFINES += GUI

INCLUDEPATH += /usr/local/include/ \
            /usr/include/ \
            ../vsn/src \
            ../thirdparty

LIBS += \
     -L/usr/local/lib \
     -L/usr/lib \
     -L../../thirdparty/lib-host \
     -lopencv_core \
     -lopencv_imgproc \
     -lopencv_features2d \
     -lopencv_calib3d \
     -lopencv_highgui \
     -lopencv_ml\
     -lboost_system \
     -lboost_thread \
     -lboost_serialization \
     -lboost_chrono \
     -lboost_iostreams \
     -lboost_filesystem \
     -lagast \
     -lac_extended \
     -ltinyxml2

SOURCES += \
    ../vsn/src/Network/Tcp/Server.cpp \
    ../vsn/src/Network/Tcp/Session.cpp \
    ../vsn/src/Network/NetworkNode.cpp \
    ../vsn/src/Messages/AckMsg.cpp \
    ../vsn/src/Messages/CoopInfoMsg.cpp \
    ../vsn/src/Messages/DataATCMsg.cpp \
    ../vsn/src/Messages/DataCTAMsg.cpp \
    ../vsn/src/Messages/Header.cpp \
    ../vsn/src/Messages/Message.cpp \
    ../vsn/src/Messages/StartATCMsg.cpp \
    ../vsn/src/Messages/StartCTAMsg.cpp \
    ../vsn/src/Multimedia/CodecParams.cpp \
    ../vsn/src/Network/MessageParser.cpp \
    ../vsn/src/TestbedTypes.cpp \
    ../vsn/src/Messages/NodeInfoMsg.cpp \
    ../vsn/src/Multimedia/VisualFeatureDecoding.cpp \
    ../vsn/src/Multimedia/VisualFeatureEncoding.cpp \
    ../vsn/src/Multimedia/VisualFeatureExtraction.cpp \
    ../vsn/src/Multimedia/HistDescriptor.cpp \
    main.cpp\
    GuiProcessingSystem.cpp \
    GuiNetworkSystem.cpp \
    CameraView.cpp \
    CameraSettings.cpp \
    Camera.cpp \
    BriskWithLookup.cpp \
    InverseBrisk.cpp \
    IoThread.cpp \
    Link.cpp \
    MainWindow.cpp \
    NetworkItem.cpp \
    NetworkTopology.cpp \
    ObjectTracking.cpp \
    ParseNetworkConfigurationXml.cpp \
    PerformanceManager.cpp \
    Plot.cpp \
    QCustomPlot.cpp \
    Sink.cpp \
    PKLot.cpp \
    PKLotClassifier.cpp

HEADERS  += \
    GuiNetworkSystem.h \
    GuiProcessingSystem.h \
    Camera.h \
    CameraSettings.h \
    CameraView.h \
    Global.h \
    InverseBrisk.h \
    IoThread.h \
    Link.h \
    MainWindow.h \
    NetworkItem.h \
    NetworkTopology.h \
    ObjectTracking.h \
    ParseNetworkConfigurationXml.h \
    PerformanceManager.h \
    Plot.h \
    QCustomPlot.h \
    Sink.h \
    Brisk.h \
    SortLikeMatlab.h \
    PKLot.h \
    PKLotClassifier.h

FORMS    += mainwindow.ui \
    camerasettings.ui \
    cameraview.ui \
    networktopology.ui \
    plot.ui

DISTFILES +=

CONFIG += c++11
