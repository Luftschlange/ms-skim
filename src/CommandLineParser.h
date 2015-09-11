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
#include <map>

#include "Types.h"
#include "Conversion.h"

using namespace std;

namespace Tools {

// This is a command line parser that uses a simple map to store the
// command line arguments.
class CommandLineParser {

public:

	// Initialize the command line parser, and build the map.
	CommandLineParser(int argc, char** argv) : executable(argv[0]) {
		// Iterate over the arguments.
		for (int i = 1; i < argc;) {
			string currentArgument(argv[i]);
		
			// If the argument starts with a '-', it is a key, not a value.
			if (currentArgument[0] == '-') {
				// See if we are the last argument.
				if (i == (argc-1)) {
					arguments[currentArgument.substr(1)] = "1";
					++i;
					continue;
				}

				// Obtain next argument.
				string nextArgument(argv[i+1]);

				// If the next argument also starts with a dash, the current
				// one is just a switch.
				if (nextArgument[0] == '-') {
					arguments[currentArgument.substr(1)] = "1";
					++i;
					continue;
				}

				// Store value (next arguement) for current argument.
				arguments[currentArgument.substr(1)] = nextArgument;
				i += 2;
			}
		}
	}


	// Return the name of the executable.
	inline string ExecutableName() const { return executable; }


	// Test whether a certain argument has been set at the command line.
	inline bool IsSet(const string argument) const {
		return arguments.find(argument) != arguments.end();
	}


	// Obtain the value of an argument. Need to look it up in the map.
	// Also, converts it to some target type.
	template<typename returnType>
	inline returnType Value(const string argument, const returnType defaultValue = returnType()) const {
		if (!IsSet(argument)) return defaultValue;
		return Tools::LexicalCast<returnType>(arguments.at(argument));
	}


	// Set the value for a certain argument.
	// This is used to inject arguments which were not specified by the user.
	inline void SetValue(const string argument, const string value) {
		arguments[argument] = value;
	}

private:

	// This is the name of the executable (argv[0]).
	const string executable;

	// This map stores the arguments.
	map<string, string> arguments;
};

}