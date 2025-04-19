CXX = g++

CXXFLAGS = -Wall -g

SRCS = Server.cpp Client.cpp

$(CXX) $(CXXFLAGS) -o Server Server.cpp
$(CXX) $(CXXFLAGS) -o Client Client.cpp
