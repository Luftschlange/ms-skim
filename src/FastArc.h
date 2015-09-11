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

#include <cstdint>
#include <iostream>
using namespace std;

#include "Assert.h"
#include "Types.h"

namespace DataStructures {
namespace Graphs {

class FastArc {
public:

	// Expose types.
	typedef uint32_t VertexIdType;

	// Default constructor.
	FastArc() : data(0u) {}

	// Get methods.
	inline VertexIdType OtherVertexId() const { return data & 0x3FFFFFFFu; }
	inline bool Forward() const { return (data & 0x40000000) == 0x40000000; }
	inline bool Backward() const { return (data & 0x80000000) == 0x80000000; }
	inline bool HasDirection(const Types::Direction direction) const { if (direction == Types::FORWARD_DIRECTION) return Forward(); else return Backward(); }
	//inline bool Valid() const { return Forward() || Backward(); }
	inline bool Valid() const { return (data & 0xC0000000u) != 0; }

	// Set methods.
	inline void SetOtherVertexId(const VertexIdType v) { data &= 0xC0000000u; data |= (v & 0x3FFFFFFFu); }
	inline void SetForwardFlag() { data |= 0x40000000; }
	inline void SetBackwardFlag() { data |= 0x80000000; }
	inline void Invalidate() { data &= 0x3FFFFFFFu; }

	// Consistency check.
	int GetErrors(const bool verbose = true) {
		int numErrors = 0;
		if (!Forward() && !Backward()) {
			if (verbose) cout << "ERROR: This arc has neither the forward nor the backward flag set. It is useless." << endl;
			++numErrors;
		}
		return numErrors;
	}


	// Compare two arcs for sorting them.
	inline bool operator<(const FastArc &other) const {
		Assert(Forward() || Backward());
		Assert(other.Forward() || other.Backward());
		int a = 0;
		int b = 0;
		if (Forward() && Backward()) a = 1;
		if (!Forward() && Backward()) a = 2;
		if (other.Forward() && other.Backward()) b = 1;
		if (!other.Forward() && other.Backward()) b = 2;
		if (a == b)
			return OtherVertexId() < other.OtherVertexId();
		else
			return a < b;
	}

	// Compare two arcs for equality of their metadata.
	inline bool EqualData(const FastArc& other) const {
		return true;
	}

protected:

	// The data encoded by this arc.
	VertexIdType data;
};

}
}