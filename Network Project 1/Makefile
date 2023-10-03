# Developers	 : Drew L Mortenson, Christopher M Claire
# Class info	 : 2238-EECE-446-01-2992
# Semester		 : Fall 2023
# Functionality  : This is a make file for the related p-counter program


EXE = p-counter
CFLAGS = -Wall
CXXFLAGS = -Wall
LDLIBS =
CC = gcc
CXX = g++

.PHONY: all
all: $(EXE)

p-counter: p-counter.cpp
	$(CXX) $(CXXFLAGS) p-counter.cpp $(LDLIBS) -o p-counter

.PHONY: clean
clean:
	rm -f $(EXE)
