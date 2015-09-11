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

#include <iostream>
#include <iomanip>
#include <cmath>
#include <utility>
#include <string>
#include <algorithm>

#include "Assert.h"
#include "Types.h"
#include "Timer.h"
#include "Conversion.h"

using namespace std;

namespace Tools {

class FancyProgressBar {

public:


	// Create a new instance of this fancy progress bar.
	FancyProgressBar(const Types::SizeType n, const string m = "", const bool v = true, ostream &o = cerr) :
		os(o)		
	{
		Initialize(n, m, v);
	}


	// Reinitialize the progress bar.
	inline void Initialize(const Types::SizeType n, const string m = "", const bool v = true) {
		finished = false;
		numIterations = n;
		message = m.empty() ? "" : m + ": ";
		verbose = v;
		currentIteration = 0;
		nextUpdate = 0;
		maxNumBars = screenColumns - 10 - message.length();
		bar = "";
		timer.Start();
		if (n > 0 && verbose)
			Draw();
	}

	// Redraws the progress bar.
	inline void Draw() {
		// Create new bar.
		stringstream ss;
		ss << message 
			<< GetPromille() << "% "
			<< "[" << GetBars() << (currentIteration < numIterations ? ">" : "") << GetHoles() << "] "
			<< GetETA();

		// Clear previous bar (if necessary).
		if (bar.length() > ss.str().length())
			os << '\r' << string(bar.length(), ' ');

		bar = ss.str();

		// Draw bar.
		os << '\r' << bar << flush;
	}

	// Iterate the progress bar.
	inline void Iterate() { IterateTo(currentIteration + 1); }

	// Iterate to a certain iteration.
	inline void IterateTo(const Types::SizeType newIteration) {
		// If we are finished, skip everything.
		if (finished)
			return;

		// If we are at the end, mark us as finished.
		if (newIteration >= numIterations) {
			Finish();
			return;
		}

		// Set current iteration.
		currentIteration = newIteration;

		// If nothing will change on the screen, return.
		if (numIterations == 0 || currentIteration < nextUpdate)
			return;

		// Draw the progress bar.
		if (verbose)
			Draw();

		// Next update is at next promille point.
		nextUpdate = Types::SizeType(floor((0.001 + (double(currentIteration) / double(numIterations))) * double(numIterations)));
	}

	// Support operator++.
	void operator++() {	Iterate(); }


	// Finish the progress bar.
	inline void Finish() {
		if (finished)
			return;
		currentIteration = numIterations;
		if (verbose) {
			Draw();
			os << endl;
		}
		finished = true;
	}

protected:

	// Obtains the current percent of progress.
	inline string GetPromille() const {
		stringstream ss;
		ss << fixed << setprecision(1) << (double(currentIteration) * 100.0 / double(numIterations));
		const string promille = ss.str();
		return string(5 - promille.length(), ' ') + promille;
	}

	// Obtains a string of bars, for a given percentage.
	inline string GetBars() const {
		return string(Types::SizeType(floor(double(currentIteration) * double(maxNumBars) / double(numIterations))), '=');
	}

	// Obtains a string of holes.
	inline string GetHoles() const {
		return string(max(Types::SizeType(1), maxNumBars - Types::SizeType(floor(double(currentIteration) * double(maxNumBars) / double(numIterations)))) - 1, ' ');
	}

	// Obtains the ETA.
	inline string GetETA() const {
		// The number of seconds elapsed so far.
		double elapsedSeconds = timer.LiveElapsedSeconds();

		// If we just started, don't show any ETA.
		if (currentIteration == 0)
			return "---";

		// If we are at the end, just print the time that was required.
		if (currentIteration == numIterations)
			return "done (" + Tools::SecondsToString(elapsedSeconds) + ").";


		// The number of seconds extrapolated until 100%
		double remainingSeconds = elapsedSeconds * double(numIterations - currentIteration) / double(currentIteration);

		return Tools::SecondsToString(remainingSeconds) + " left.";
	}

private:

	static const Types::SizeType screenColumns = 60;

	// This is the start time.
	Platform::Timer timer;

	// Indicates whether we are finished.
	bool finished;

	// This is the bar.
	string bar;

	// The number of iterations.
	Types::SizeType numIterations;

	// This is a message that preceeds the progress bar.
	string message;

	// The ostream to use for printing.
	ostream &os;

	// Indicates whether the progress bar should be used at all.
	bool verbose;

	// This depicts the index of the progress bar.
	Types::SizeType currentIteration;
	

	// This indicates when the next update should happen (at which iteration).
	Types::SizeType nextUpdate;

	// The number of bars to draw at 100%.
	Types::SizeType maxNumBars;
};

}