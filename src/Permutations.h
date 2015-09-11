/*
Algorithm for Influence Estimation and Maximization

Copyright (c) Microsoft Corporation

All rights reserved.

MIT License

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the ""Software""), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED *AS IS*, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#pragma once

#include <random>
#include <algorithm>
using namespace std;

#include "Assert.h"

namespace Tools {


// Tests whether an array of length n is a permutation of [0..n-1].
template<typename containerType>
bool IsPermutation(const containerType &permutation) {
	vector<bool> appears(permutation.size(), false);
	for (typename containerType::value_type item : permutation) {
		if (item >= permutation.size()) return false;
		if (item < static_cast<typename containerType::value_type>(0)) return false;
		if (appears[item]) return false;
		appears[item] = true;
	}
	return true;
}


// Generate an identity permutation.
template<typename valueType>
void GenerateIdentityPermutation(vector<valueType> &permutation, valueType numElements) {
	permutation.resize(numElements);
	for (valueType i = 0; i < permutation.size(); ++i) {
		permutation[i] = i;
	}
}

// Generates for a permutation its inverse permutation.
template<typename valueType>
void GenerateInversePermutation(const vector<valueType> &inputPermutation, vector<valueType> &outputPermutation) {
	outputPermutation.clear();
	for (valueType i = 0; i < inputPermutation.size(); ++i)  {
		if (outputPermutation.size() <= inputPermutation[i])
			outputPermutation.resize(inputPermutation[i] + 1);
		outputPermutation[inputPermutation[i]] = i;
	}
}

// Generate a uniformly distributed random permutation.
template<typename valueType>
void GenerateRandomPermutation(vector<valueType> &permutation, valueType numElements, int seed) {
	// If the permutation consists of 0 elements, do nothing.
	if (numElements <= 0) {
		permutation.clear();
		return;
	}

	// Initialize random number generator.
	mt19937_64 gen(seed);

	GenerateIdentityPermutation(permutation, numElements);
	shuffle(permutation.begin(), permutation.end(), gen);
}

// Generate a random permutation according to weights.
template<typename valueType>
void GenerateWeightedRandomPermutation(vector<valueType> &permutation, const vector<valueType> &weights, int seed) {
	// If the permutation consists of 0 elements, do nothing.
	if (weights.empty()) {
		permutation.clear();
		return;
	}

	// Initialize a random number generator.
	mt19937_64 gen(seed);

	// Generating an identity permutation where each element i occurs weights[i] times.
	bool hasZeroWeights = false;
	vector<valueType> blownUpPermutation;
	for (valueType i = 0; i < weights.size(); ++i) {
		if (weights[i] == 0)
			hasZeroWeights = true;
		for (valueType j = 0; j < weights[i]; ++j)
			blownUpPermutation.push_back(i);
	}

	// Shuffle this set.
	shuffle(blownUpPermutation.begin(), blownUpPermutation.end(), gen);

	// Remember which elements have been selected.
	vector<bool> selected(weights.size(), false);

	// Generate the permutation.
	permutation.clear();
	for (const valueType element : blownUpPermutation) {
		if (!selected[element]) {
			selected[element] = true;
			permutation.push_back(element);
		}
	}
	// Add zero-weight vertices to the end.
	for (valueType i = 0; i < weights.size(); ++i) {
		if (weights[i] == 0)
			permutation.push_back(i);
	}
	Assert(permutation.size() == weights.size());
}

}