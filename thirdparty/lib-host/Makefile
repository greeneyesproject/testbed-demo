CXX=g++

all:
	$(CXX) $(CFLAGS) -I ../agast/include -I /usr/local/include -c ../agast/src/*.cc 
	$(CXX) $(CFLAGS) -I ../coder/include -c ../coder/src/*.cpp
	$(CXX) $(CFLAGS) -I ../tinyxml/include -c ../tinyxml/src/*.cpp
	ar rcs libac_extended.a ac_extended.o 
	ar rcs libtinyxml2.a tinyxml2.o 
	ar rcs libagast.a agast5_8_nms.o agast5_8.o agast7_12d_nms.o agast7_12d.o agast7_12s_nms.o agast7_12s.o AstDetector.o nonMaximumSuppression.o oast9_16_nms.o oast9_16.o 

