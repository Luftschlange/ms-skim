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
#include <sstream>
#include <vector>

using namespace std;


namespace Tools {

// Splits a string using a delimiting character.
// This is somewhat slow but convenient.
inline vector<string> Split(const string &s, const char delim = '\t') {
	vector<string> elements;
    stringstream ss(s);
    string item;
    while(getline(ss, item, delim)) {
        elements.push_back(item);
    }
	return elements;
}


// Use this function directly only if you keep managing the vector "elements" yourself.
// Otherwise, see below.
// Returns the number of elements found.
inline int StaticSplitInline(string &s, const int numElements, vector<const char*> &elements, const char delim = '\t') {
	// If the string is empty, do nothing.
	if (s.empty()) return 0;

	// Push first element.
	elements[0] = s.c_str();
	int index = 1;

	// Iterate over the string.
	const string::size_type size = s.size();
	for (int i = 0; i < size; ++i) {
		if (s[i] == delim) {
			s[i] = '\0';
			if (i < size - 1) {
				elements[index] = s.c_str() + i + 1;
				++index;
			}
			if (index >= numElements) break;
		}
	}

	return index;
}

// "Splits" a string by substituting the delimiting character by \0 and
// returning a vector of char* to the beginning of each substring.
// You MUST supply the number of expected elements, and it has to be exact.
// This is pretty fast.
inline vector<const char*> StaticSplitInline(string &s, const int numElements, const char delim = '\t') {
	vector<const char*> elements(numElements, nullptr);
	StaticSplitInline(s, numElements, elements, delim);
	return elements;
}


// This is a dynamic in-line split function.
// It returns an arrach of char pointers and replaces the delimieter with
// the \0 character. It's faster than the "normal" split, but
// it uses dynamic memory allocation.
inline void DynamicSplitInline(string &s, vector<const char*> &elements, const char delim = '\t') {
	// Clear the output vector.
	elements.clear();

	// If the string is empty, do nothing.
	if (s.empty()) return;

	// Push first element.
	elements.push_back(s.c_str());
	int index = 1;

	// Iterate over the string and add more.
	const string::size_type size = s.size();
	for (int i = 0; i < size; ++i) {
		if (s[i] == delim) {
			s[i] = '\0';
			if (i < size - 1)
				elements.push_back(s.c_str() + i + 1);
		}
	}
}

// This is the split function that corresponds to the one above,
// but allocates a new vector of char-pointers.
inline vector<char const*> DynamicSplitInline(string &s, const char delim = '\t') {
	vector<const char*> elements;
	DynamicSplitInline(s, elements, delim);
	return elements;
}

}