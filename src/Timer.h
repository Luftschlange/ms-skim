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


namespace Platform {

#if defined(_WIN32)
#include <windows.h>
#else
#include <sys/time.h>
#endif


class Timer {

#if defined(_WIN32)
	typedef struct {
		LARGE_INTEGER start;
		LARGE_INTEGER stop;
	} StopWatch;
#else
	struct StopWatch {
		double start = 0;
		double stop = 0;
	};
#endif


private:
	StopWatch timer;

#if defined(_WIN32)
	LARGE_INTEGER frequency;
	inline double LIToSecs(LARGE_INTEGER & L) const {
		return ((double)L.QuadPart / (double)frequency.QuadPart);
	}
#endif

public:

	Timer() {
#if defined(_WIN32)
		timer.start.QuadPart = 0;
		timer.stop.QuadPart=0; 
		QueryPerformanceFrequency(&frequency);
#endif
	}

	inline void Start() {
#if defined(_WIN32)
		QueryPerformanceCounter(&timer.start);
#else
		timer.start = GetTimestampMilliseconds();
#endif
	}

	inline void Stop() {
#if defined(_WIN32)
		QueryPerformanceCounter(&timer.stop);
#else
		timer.stop = GetTimestampMilliseconds();
#endif
	}

	inline double ElapsedSeconds() {
#if defined(_WIN32)
		LARGE_INTEGER time;
		time.QuadPart = timer.stop.QuadPart - timer.start.QuadPart;
		return LIToSecs(time);
#else
		return (timer.stop - timer.start) / 1000.0;
#endif
	}

	inline double LiveElapsedSeconds() const {
#if defined(_WIN32)
		LARGE_INTEGER now, diff;
		QueryPerformanceCounter(&now);
		diff.QuadPart = now.QuadPart - timer.start.QuadPart;
		return LIToSecs(diff);
#else
		return (GetTimestampMilliseconds() - timer.start) / 1000.0;
#endif
	}

	inline double LiveElapsedMilliseconds() const {
#if defined(_WIN32)
		return LiveElapsedSeconds() * 1000.0;
#else
		return GetTimestampMilliseconds() - timer.start;
#endif
	}

	inline double ElapsedMilliseconds() {
#if defined(_WIN32)
		return ElapsedSeconds() * 1000.0;
#else
		return timer.stop - timer.start;
#endif
	}


#ifndef _WIN32
	static inline double GetTimestampMilliseconds() {
		timeval tp;
		gettimeofday(&tp, NULL);
		return static_cast<double>(tp.tv_sec)*1000.0 + static_cast<double>(tp.tv_usec) / 1000.0;
	}
#endif
};

#if defined(_WIN32)
#undef max
#undef min
#endif

}
