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

#include <string>
#include <vector>
#include <algorithm>
using namespace std;

#include "Assert.h"
#include "Types.h"
#include "Split.h"
#include "Conversion.h"

namespace Tools {

// Extracts a certain range from a string.
// Not too fast, but works. :-)
inline vector<Types::IndexType> ExtractRange(const string range) {
	vector<Types::IndexType> indices;
	vector<string> ranges = Tools::Split(range, ',');
	for (const string currentRange : ranges) {
		vector<string> limits = Tools::Split(currentRange, '-');
		Assert(!limits.empty());
		Assert(limits.size() <= 2);
		if (limits.size() == 1) {
			indices.push_back(Tools::LexicalCast<Types::IndexType>(limits[0]));
		}
		else {
			const Types::IndexType lowerLimit = Tools::LexicalCast<Types::IndexType>(limits[0]);
			const Types::IndexType upperLimit = Tools::LexicalCast<Types::IndexType>(limits[1]);
			Assert(lowerLimit <= upperLimit);
			for (Types::IndexType currentIndex = lowerLimit; currentIndex <= upperLimit; ++currentIndex)
				indices.push_back(currentIndex);
		}
	}

	// Sort and uniquify.
	sort(indices.begin(), indices.end());
	indices.resize(distance(indices.begin(), unique(indices.begin(), indices.end())));

	return indices;
}


}