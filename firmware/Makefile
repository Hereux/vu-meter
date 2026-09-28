# Bauen und Testen der DSP-Kette ohne PlatformIO -- nur g++ noetig.
#
#   make test    Abnahmetest gegen die Referenzwerte aus dem Python-Modell
#   make bench   Durchsatzmessung
#   make clean

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra
BUILD    := build
CORE     := src/dsp.cpp src/spectrum.cpp src/meter.cpp

.PHONY: all test bench clean

all: test

$(BUILD):
	@mkdir -p $(BUILD)

$(BUILD)/test_dsp: $(BUILD) test/test_dsp/test_dsp.cpp $(CORE) test/reference_vectors.h
	$(CXX) $(CXXFLAGS) -o $@ test/test_dsp/test_dsp.cpp $(CORE) -lm

$(BUILD)/bench_dsp: $(BUILD) test/bench/bench_dsp.cpp $(CORE)
	$(CXX) $(CXXFLAGS) -o $@ test/bench/bench_dsp.cpp $(CORE) -lm

test: $(BUILD)/test_dsp
	@./$(BUILD)/test_dsp

bench: $(BUILD)/bench_dsp
	@./$(BUILD)/bench_dsp

clean:
	@rm -rf $(BUILD)
