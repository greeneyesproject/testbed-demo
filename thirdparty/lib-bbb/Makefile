CXX=arm-linux-gnueabihf-g++
AR=arm-linux-gnueabihf-ar

all:
	$(CXX) $(CFLAGS) -I ../agast/include -I /opt/opencv-arm/include -c ../agast/src/*.cc
	$(CXX) $(CFLAGS) -I ../coder/include -c ../coder/src/*.cpp
	$(CXX) $(CFLAGS) -I ../tinyxml/include -c ../tinyxml/src/*.cpp
	$(AR) rcs libac_extended.a ac_extended.o 
	$(AR) rcs libtinyxml2.a tinyxml2.o 
	$(AR) rcs libagast.a agast5_8_nms.o agast5_8.o agast7_12d_nms.o agast7_12d.o agast7_12s_nms.o agast7_12s.o AstDetector.o nonMaximumSuppression.o oast9_16_nms.o oast9_16.o 

clean:
	rm *.o *.a
