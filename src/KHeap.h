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
// K-ary heap implementation. Based on the one from mapLabs.
// KeyType is a type for keys: more important entries have smaller keys.
// ValueType: this has to be an integral type which refers to the actual values of the items (e.g. vertex ids)
// Arity of the heap is 2^{logK}.

#include <limits>
#include <vector>
using namespace std;

#include "Assert.h"
#include "Types.h"

namespace DataStructures {
namespace Container {


template<typename keyType, typename elementType, int logK> class KHeap {
public:

	// Expose typedefs.
	typedef keyType KeyType;
	typedef elementType ElementType;
	typedef elementType PositionType;

	// Define empty position
	static const PositionType EmptyPosition;

	// Standard constructor for the heap.
	KHeap() : arity(1 << logK) {}

	// The heap contains ids from 0 to n-1.
	KHeap(const Types::SizeType numberOfValues) : arity(1 << logK) {
		heap.reserve(numberOfValues);
		positions.resize(numberOfValues, EmptyPosition);
	}


	// Resize the heap.
	// Returns true if it actually did a resize, false otherwise.
	inline bool Resize(const Types::SizeType newSize) {
		heap.reserve(newSize);
		positions.resize(newSize, EmptyPosition);
		return true;
	}

	// Clears the heap.
	inline void Clear() {
		for (Types::IndexType i = 0; i < Size(); i++) {
			positions[heap[i].Element] = EmptyPosition;
		}
		heap.clear();
	}


	// Get the memory footprint of the data structure.
	inline int64_t GetMemoryFootprint() const {
		return Capacity() * (sizeof(HeapEntryType) + sizeof(PositionType));
	}


	// Return the capacity of the heap.
	inline Types::SizeType Capacity() const { return positions.size(); }

	// Returns the size (number of elements) in the heap.
	inline Types::SizeType Size() const { return heap.size(); }

	// Checks if the heap is empty.
	inline bool Empty() const { return Size() == 0;	}


	// Returns the element with minimum id.
	inline elementType DeleteMin() {
		Assert(!heap.empty());

		const elementType element = heap[0].Element;

		// If the heap is just getting empty, clear it.
		if (heap.size() == 1) {
			positions[element] = EmptyPosition;
			heap.pop_back();
			Assert(heap.empty());
		// Else swap with the last element, and sift down.
		} else {
			Swap(0, static_cast<PositionType>(heap.size() - 1));
			heap.pop_back();
			positions[element] = EmptyPosition;
			SiftDown(0);
		}

		return element;
	}

	// Delete minimum element, but also return its key.
	inline elementType DeleteMin(keyType &key) {
		Assert(!heap.empty());
		key = heap[0].Key;
		return DeleteMin();
	}


	// Delete an arbitrary element from the heap.
	inline void Delete(const elementType element) {
		Assert(!heap.empty());
		Assert(element >= 0);
		Assert(element < positions.size());

		elementType position = positions[element];
		Assert(position != EmptyPosition);

		// If there is only element in the heap, just delete it and we're done.
		if (heap.size() == 1) {
			positions[element] = EmptyPosition;
			heap.pop_back();
			Assert(heap.empty());
		}
		else {
			Swap(position, heap.size() - 1);
			positions[element] = EmptyPosition;
			heap.pop_back();

			PositionType parentPosition = (position - 1) >> logK;

			// Determine if we sift down or up.
			if (position == 0 || heap[parentPosition].Key < heap[position].Key)
				SiftDown(position);
			else
				SiftUp(position);
		}		
	}


	// Determine the minimum key in the heap.
	inline keyType MinKey() const {
		Assert(!heap.empty());
		return heap[0].Key;
	}

	// Get the min element.
	inline elementType MinElement() const {
		Assert(!heap.empty());
		return heap[0].Element;
	}


	// Test if some element is in the heap.
	inline bool Contains(const elementType element) const {
		Assert(element >= 0);
		Assert(element < positions.size());
		return (positions[element] != EmptyPosition);
	}

	// Retrieve the key of an element.
	inline keyType GetKey(const elementType element) const {
		Assert(element >= 0);
		Assert(element < positions.size());
		Assert(Contains(element));
		return heap[positions[element]].Key;
	}


	// Updates an element in the heap. If it is already contained,
	// just update its key, otherwise insert it.
	inline void Update(elementType element, keyType key) {
		if (positions[element] == EmptyPosition) {
			heap.push_back(HeapEntryType(element, key));
			positions[element] = static_cast<PositionType>(heap.size()) - 1;
			SiftUp(static_cast<PositionType>(heap.size() - 1));
		} else {
			PositionType position = positions[element];
			if (heap[position].Key >= key) {
				heap[position].Key = key;
				SiftUp(position);
			}
			else {
				heap[position].Key = key;
				SiftDown(position);
			}
		}
	}

private:

	// Move an element which is in position pos upwards.
	inline void SiftUp(PositionType position) {
		int64_t numSifts = 0;
		while (position) {
			const PositionType parentPosition = (position - 1) >> logK;
			if (heap[position].Key < heap[parentPosition].Key) {
				Swap(position, parentPosition);
				position = parentPosition;
				++numSifts;
			} else
				break;
		}
		if (numSifts >= 100) cout << "!!!" << numSifts << "!!!" << flush;
	}

	/*
	  Move an element which is in position pos downwards.
	*/
	inline void SiftDown(PositionType position) {
		const size_t heapLength = heap.size();
		while (true) {
			PositionType bestPosition = position;
			if (logK == 1) {
				PositionType childPosition = (position << 1) | 1;
				if (childPosition < heapLength) {
					if (heap[childPosition].Key < heap[bestPosition].Key) bestPosition = childPosition;
					++childPosition;
					if (childPosition < heapLength) {
						if (heap[childPosition].Key < heap[bestPosition].Key) bestPosition = childPosition;
					}
				}
			} else if (logK == 2) {
				PositionType childPosition = (position << 2) | 1;
				if (childPosition < heapLength) {
					if (heap[childPosition].Key < heap[bestPosition].Key) bestPosition = childPosition;
					++childPosition;
					if (childPosition < heapLength) {
						if (heap[childPosition].Key < heap[bestPosition].Key) bestPosition = childPosition;
						++childPosition;
						if (childPosition < heapLength) {
							if (heap[childPosition].Key < heap[bestPosition].Key) bestPosition = childPosition;
							++childPosition;
							if (childPosition < heapLength) {
								if (heap[childPosition].Key < heap[bestPosition].Key) bestPosition = childPosition;
							}
						}
					}
				}
			} else {
				for (int i = 0; i < arity; i += 2) { // partial loop unrolling for better performance
					PositionType childPosition = ((position << logK) | i) + 1;
					if (childPosition >= heapLength) break;
					if (heap[childPosition].Key < heap[bestPosition].Key) bestPosition = childPosition;
					++childPosition;
					if (childPosition >= heapLength) break;
					if (heap[childPosition].Key < heap[bestPosition].Key) bestPosition = childPosition;
				}
			}
			if (bestPosition == position) break;
			Swap(bestPosition, position);
			position = bestPosition;
		}
	}

	// Swaps heap entries at positions a and b.
	inline void Swap(const PositionType a, const PositionType b) {
		HeapEntryType entry = heap[a];
		heap[a] = heap[b];
		heap[b] = entry;
		positions[heap[a].Element] = a;
		positions[heap[b].Element] = b;
	}

private:

	// This is an entry of the heap.
	struct HeapEntryType {
		elementType Element;
		keyType Key;
		HeapEntryType(const elementType e, const keyType k) : Element(e), Key(k) {}
	};


	// The arity of the heap.
	int arity;

	// A heap of elements.
	vector<HeapEntryType> heap;

	// For each element its position in the heap.
	vector<PositionType> positions;

};

// Isn't C++ just beautiful?
template<typename keyType, typename elementType, int logK>
const elementType KHeap<keyType, elementType, logK>::EmptyPosition = numeric_limits<elementType>::max();

}
}
