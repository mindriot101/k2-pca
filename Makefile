CXX=g++
OUTDIR ?= bin

all: $(OUTDIR)/extract

$(OUTDIR)/extract: src/extract.cpp $(OUTDIR)
	$(CXX) -std=c++11 $< -o $@ $(shell pkg-config --cflags --libs cfitsio) $(shell pkg-config --cflags tclap) -Wall -Wextra -g -O2

$(OUTDIR):
	mkdir -p $(OUTDIR)

clean:
	@rm $(OUTDIR)/extract
