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

#include <vector>
#include <limits>

using namespace std;

#include "Assert.h"
#include "Types.h"

namespace DataStructures {
namespace Container {

template<typename keyType>
class FastSet {

public:

	// Expose typedefs.
	typedef keyType KeyType;

	// Construct the map.
	FastSet() {}

	// Construct the map with a predefined number of key-elements.
	FastSet(const Types::SizeType numElements) : isContained(numElements, false) {}

	// Resize the set. It has to grow.
	inline void Resize(const Types::SizeType numElements) {
		Assert(numElements >= Size());
		isContained.resize(numElements, false);
	}

	// Get the size of the set, i.e., the number of keys that are contained.
	inline Types::SizeType Size() const {
		return containedKeys.size();
	}

	// Test whether this set is empty.
	inline bool IsEmpty() const {
		return containedKeys.empty();
	}

	// Access a key by index.
	inline keyType KeyByIndex(const Types::SizeType index) const {
		Assert(index >= 0);
		Assert(index < containedKeys.size());
		return containedKeys[index];
	}

	// Test whether a certain key is contained in the set.
	inline bool IsContained(const keyType &key) const {
		Assert(key >= 0);
		Assert(key < isContained.size());
		return isContained[key];
	}


	// Insert an element into the set.
	inline void Insert(const keyType &key) {
		Assert(key >= 0);
		Assert(key < isContained.size());

		// If the keyType is too big, resize the set.
		//if (key >= isContained.size())
			//isContained.resize(key + 1, false);
		
		// If the element is not contained, add it.
		if (!IsContained(key)) {
			containedKeys.push_back(key);
		}

		// Set the value for key.
		isContained[key] = true;
	}

	// Insert from another fast set.
	inline void Insert(const FastSet<keyType> &other) {
		for (Types::IndexType i = 0; i < other.Size(); ++i)
			Insert(other.KeyByIndex(i));
	}

	// Delete an item by index.
	inline keyType DeleteByIndex(const Types::IndexType index) {
		Assert(index >= 0);
		Assert(index < containedKeys.size());
		Assert(isContained[containedKeys[index]]);
		isContained[containedKeys[index]] = false;
		keyType key = containedKeys[index];
		swap(containedKeys.back(), containedKeys[index]);
		containedKeys.pop_back();
		return key;
	}

	// Deletes from the back.
	inline keyType DeleteBack() {
		Assert(containedKeys.size() > 0);
		keyType key = containedKeys.back();
		isContained[key] = false;
		containedKeys.pop_back();
		return key;
	}


	// Clear this set.
	inline void Clear() {
		for (keyType key : containedKeys) {
			isContained[key] = false;
		}
		containedKeys.clear();
	}

	// Access the contained keys (read only).
	inline const vector<keyType> &ContainedKeys() const {
		return containedKeys;
	}

private:

	// This vector maps keys to a bool value indicating whether the key is in the set.
	vector<bool> isContained;

	// This vector is dynamic, and it is a collection of the
	// keys that are contained in the set. This is required
	// to support the delete operation.
	vector<keyType> containedKeys;
};

}
}