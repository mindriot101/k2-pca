CXX=g++

all: extract

extract: src/extract.cpp
	$(CXX) -std=c++11 $< -o $@ $(shell pkg-config --cflags --libs cfitsio) $(shell pkg-config --cflags tclap) -Wall -Wextra -g -O2

clean:
	@rm extract
