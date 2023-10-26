# Developers	 : Drew L Mortenson, Christopher M Claire
# Class info	 : 2238-EECE-446-01-2992
# Semester		 : Fall 2023
# Functionality  : This is a make file for the related P2P 'peer' program


EXE = peer
CFLAGS = -Wall
CXXFLAGS = -Wall
LDLIBS =
CC = gcc
CXX = g++

.PHONY: all
all: $(EXE)

peer: peer.cpp
	$(CXX) $(CXXFLAGS) peer.cpp $(LDLIBS) -o peer

.PHONY: clean
clean:
	rm -f $(EXE)
