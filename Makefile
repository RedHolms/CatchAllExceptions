CXX = g++
CXXFLAGS =

all: CAE_test

test.o: test.cpp
	$(CXX) -c $(CXXFLAGS) $< -o $@

CAE.o: CAE_unix.cpp
	$(CXX) -c $(CXXFLAGS) $< -o $@

CAE_test: test.o CAE.o
	$(CXX) test.o CAE.o -o CAE_test