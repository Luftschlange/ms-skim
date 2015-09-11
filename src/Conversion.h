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

#include <sstream>
#include <string>
#include <cstdlib>

#include "Assert.h"
#include "Types.h"

using namespace std;

namespace Tools {


// Provide a lexical cast between arbitrary types.
// Works via a string stream.
template<typename returnType, typename valueType>
inline returnType LexicalCast(const valueType value) {
	stringstream stream;
	returnType ret;
	stream << value;
	stream >> ret;
	return ret;
}

// Provide a lexical cast between some types
// with faster methods.
template<>
inline string LexicalCast(const string value) {
	return value;
}

// Provide a lexical cast to int.
template<>
inline int LexicalCast(const string &value) {
	return atoi(value.c_str());
}


// Provide a lexical cast from const char to signed int.
// This is an order of magnitude faster than atoi.
// Note: Does not perform ANY input checks, and WILL provide
// unpredictable results on malformed input.
template<>
inline signed int LexicalCast(const char* value) {
    signed int x = 0;
	bool negative = false;
	if (*value == '-') {
		negative = true;
		++value;
	}
    while (*value != '\0') {
		Assert(*value >= '0');
		Assert(*value <= '9');
        x = (x*10) + (*value - '0');
        ++value;
    }
	if (negative) {
		x = -x;
	}
    return x;
}

// Provide a lexical cast from const char to unsigned int.
// This is an order of magnitude faster than atoi.
// Note: Does not perform ANY input checks, and WILL provide
// unpredictable results on malformed input.
template<>
inline unsigned int LexicalCast(const char* value) {
    unsigned int x = 0;
    while (*value != '\0') {
		Assert(*value >= '0');
		Assert(*value <= '9');
        x = (x*10) + (*value - '0');
        ++value;
    }
    return x;
}

// Provide a lexical cast from const char to unsigned int64.
// This is an order of magnitude faster than atoi.
// Note: Does not perform ANY input checks, and WILL provide
// unpredictable results on malformed input.
template<>
inline uint64_t LexicalCast(const char* value) {
	unsigned int x = 0;
	while (*value != '\0') {
		Assert(*value >= '0');
		Assert(*value <= '9');
		x = (x * 10) + (*value - '0');
		++value;
	}
	return x;
}


// Convert a hex string to unsigned int.
inline uint32_t LexicalCast(const char* value, const int length) {
	Assert(length <= sizeof(uint32_t) * 2);
	unsigned int x = 0;
    for (int i = 0; i < length; ++i) {
		if (value[i] >= '0' && value[i] <= '9')
			x = (x*16) + (value[i] - '0');
		else if (value[i] >= 'a' && value[i] <= 'f')
			x = (x*16) + (value[i] - 'a' + 10);
		else if (value[i] >= 'A' && value[i] <= 'F')
			x = (x*16) + (value[i] - 'A' + 10);
		else
			Assert(false);
    }
	return x;
}


// Converts a duration (in seconds) to a string.
template<typename T>
string SecondsToString(const T duration) {
	stringstream ss;

	Types::SizeType min = Types::SizeType(duration) / 60;
	Types::SizeType sec = Types::SizeType(duration) % 60;
	if (min == 0) {
		if (sec == 0)
			ss << "< 1 sec";
		else
			ss << sec << " sec";
	}
	else if (min < 1) {
		ss << "< 1 m";
	}
	else if (min < 60) {
		ss << min << " m " << sec << " s";
	}
	else {
		Types::SizeType hour = min / 60;
		min = min % 60;
		ss << hour << " hr " << min << " m " << sec << " s";
	}
	return ss.str();
}

// Converts a duration (in milliseconds) to a string.
template<typename T>
string MillisecondsToString(const T duration) {
	T seconds = duration / 1000.0;
	if (seconds < T(1.0))
		return Tools::LexicalCast<string>(seconds) + " ms";
	else 
		return SecondsToString(seconds);
}

}
