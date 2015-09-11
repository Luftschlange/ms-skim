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

#pragma unmanaged

#include "Assert.h"
#include "Types.h"

namespace Platform {

#if defined(_WIN32) || defined(__CYGWIN__)
#include <Windows.h>
#elif __linux__
#include <sched.h>
#endif

// Pin a thread to a cpu
inline void PinThread(const int threadId) {
#if defined(_WIN32) || defined(__CYGWIN__)
	const int cpuId = threadId;
	SetThreadAffinityMask(GetCurrentThread(), DWORD_PTR(1 << cpuId));
#elif __linux__
	cpu_set_t mask;
	CPU_ZERO(&mask);
	CPU_SET(threadId, mask);
	sched_setaffinity(0, sizeof(mask), &mask);
#elif __APPLE__
#endif
}

// Unpin a thread from a cpu
inline void UnpinThread() {
#if defined(_WIN32) || defined(__CYGWIN__)
	SetThreadAffinityMask(GetCurrentThread(), DWORD_PTR(0xFFFFFFFF));
#elif __linux
#elif __APPLE
#endif
}

// Returns the number of numa nodes.
inline uint32_t GetNumberOfNumaNodes() {
#if defined(_WIN32) || defined(__CYGWIN__)
	ULONG highestNodeNumber(0);
	if (!GetNumaHighestNodeNumber(&highestNodeNumber))
		cerr << "GetNumaHighestNodeNumber failed: " << GetLastError() << endl;
	return static_cast<uint32_t>(highestNodeNumber + 1);
#elif __linux__
#elif __APPLE__
#endif
}

// Returns for a given processor id its numa node id.
inline uint8_t GetNumaNodeFromProcessorId(const uint32_t processorId) {
#if defined(_WIN32) || defined(__CYGWIN__)
	UCHAR nodeNumber(0);
	if (!GetNumaProcessorNode(processorId, &nodeNumber))
		cerr << "GetNumaProcessorNode failed: " << GetLastError() << endl;
	return static_cast<uint8_t>(nodeNumber);
#else
	return 0;
#endif
}

inline uint64_t GetAffinityMaskForNumaNode(const uint32_t numaNode) {
#if defined(_WIN32) || defined(__CYGWIN__)
	Assert(numaNode < GetNumberOfNumaNodes());
	ULONGLONG nodeMask = 0;
	GetNumaNodeProcessorMask(static_cast<Platform::UCHAR>(numaNode), &nodeMask);
	return nodeMask;
#else
	return uint64_t(0)-1;
#endif
}

// Pin a thread to a certain numa node.
inline void PinThreadToNumaNode(const uint32_t numaNode) {
#if defined(_WIN32) || defined(__CYGWIN__)
	SetThreadAffinityMask(GetCurrentThread(), GetAffinityMaskForNumaNode(numaNode));
#elif __linux
#elif __APPLE
#endif
}


// Pin a process to a certain numa node.
inline void PinProcessToNumaNode(const uint32_t numaNode) {
#if defined(_WIN32) || defined(__CYGWIN__)
	Assert(numaNode < GetNumberOfNumaNodes());
	ULONGLONG nodeMask = 0;
	GetNumaNodeProcessorMask(static_cast<Platform::UCHAR>(numaNode), &nodeMask);
	SetProcessAffinityMask(GetCurrentProcess(), GetAffinityMaskForNumaNode(numaNode));
#elif __linux
#elif __APPLE
#endif
}



#undef max
#undef min

}