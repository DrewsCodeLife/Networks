# Developers	 : Drew L Mortenson, Christopher M Claire
# Class info	 : 2238-EECE-446-01-2992
# Semester		 : Fall 2023
# Functionality  : This is a make file for the related P2P 'p4_registry' program


EXE = p4_registry
CFLAGS = -Wall
CXXFLAGS = -Wall
LDLIBS =
CC = gcc
CXX = g++

.PHONY: all
all: $(EXE)

p4_registry: program4.cpp
	$(CXX) $(CXXFLAGS) program4.cpp $(LDLIBS) -o p4_registry

.PHONY: clean
clean:
	rm -f $(EXE)
