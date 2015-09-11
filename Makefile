CXX = g++
CXXFLAGS = -std=c++11 -O3 -fopenmp -DNDEBUG -m64
BIN = ./bin
SRC = ./src

.phony: RunSKIM RunInfluenceOracle

all: RunSKIM RunInfluenceOracle

RunSKIM:
	$(CXX) $(CXXFLAGS) -o $(BIN)/RunSKIM $(SRC)/RunSKIM.cpp

RunInfluenceOracle:
	$(CXX) $(CXXFLAGS) -o $(BIN)/RunInfluenceOracle $(SRC)/RunInfluenceOracle.cpp
