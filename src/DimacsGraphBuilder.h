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
#include <iostream>
#include <algorithm>

#include "SharedMemoryManager.h"
#include "FancyProgressBar.h"
#include "FileSize.h"
#include "GraphStream.h"
#include "Conversion.h"
#include "Split.h"

using namespace std;

namespace RawData {


// Build a metis graph directly into a fast unweighted graph.
template<typename graphType>
void BuildDimacsGraph(const string inFilename, graphType &outGraph, const bool ignoreSelfLoops, const bool transpose, const bool directed, const bool buildIncomingArcs, const bool removeParallelArcs, const bool verbose) {
	typedef typename graphType::VertexIdType vertexIdType;

	// Get the file size of the input filestream.
	Types::SizeType fileSize = IO::FileSize(inFilename);

	// Open the input file as a stream.
	IO::FileStream inStream;
	inStream.OpenForReading(inFilename);

	// Create timer and progress bar.
	if (verbose) cout << "Streaming from " << inFilename << " (" << (fileSize / 1024.0 / 1024.0) << " MiB): " << endl;
	Tools::FancyProgressBar bar(fileSize, "", verbose);

	// Read the file line-by-line.
	string line;
	uint64_t lineNumber = 0;
	bool headerParsed = false;
	vector<const char*> tokens;
	vertexIdType numVertices = 0;
	Types::SizeType numSelfLoopsIgnored = 0;
	Types::SizeType expectedNumArcs = 0;
	typedef pair<vertexIdType, vertexIdType> arcType;
	vector<arcType> arcs;

	// Read line-by-line.
	while (!inStream.Finished()) {
		inStream.ExtractLine(line);
		++lineNumber;
		bar.IterateTo(inStream.NumBytesRead());

		// Skip empty lines.
		if (line.empty()) continue;

		// Skip lines that begin with a %-symbol.
		if (line[0] == '%') continue;
		if (line[0] == 'c') continue;

		// The header contains the number of vertices and arcs.
		if (!headerParsed) {
			// Parse the header.
			Tools::DynamicSplitInline(line, tokens, ' ');

			// We need at least two tokens for the file format to be correct.
			Assert(tokens.size() >= 4);

			// The third token indicates the number of vertices.
			numVertices = Tools::LexicalCast<vertexIdType>(tokens[2]);

			// Store the expected number of arcs.
			expectedNumArcs = Tools::LexicalCast<Types::SizeType>(tokens[3]);

			headerParsed = true;
		}
		else {
			// Parse tail and head vertices.
			Tools::DynamicSplitInline(line, tokens, ' ');

			// The line should have at least four tokens.
			Assert(tokens.size() >= 4);

			// The first token must be an "a".
			Assert(tokens[0][0] == 'a' && tokens[0][1] == '\0');

			vertexIdType fromVertexId = Tools::LexicalCast<vertexIdType>(tokens[1]);
			vertexIdType toVertexId = Tools::LexicalCast<vertexIdType>(tokens[2]);
			--fromVertexId; // In the text file vertex ids are one-based.
			--toVertexId;

			if (transpose)
				swap(fromVertexId, toVertexId);

			Assert(fromVertexId >= 0);
			Assert(toVertexId >= 0);
			Assert(fromVertexId < numVertices);
			Assert(toVertexId < numVertices);

			if (ignoreSelfLoops && fromVertexId == toVertexId) {
				++numSelfLoopsIgnored;
				continue;
			}

			if (!directed && fromVertexId > toVertexId)
				continue;

			arcs.push_back(arcType(fromVertexId, toVertexId));
		}
	}

	if (verbose) cout << arcs.size() << " of " << expectedNumArcs << " expected arcs parsed; " << numSelfLoopsIgnored << " selfloops ignored." << endl;

	// Remove parallel arcs?
	if (removeParallelArcs) {
		if (verbose) cout << "Removing parallel arcs... " << flush;
		sort(arcs.begin(), arcs.end(), [](const arcType &a, const arcType &b) {
			if (a.first == b.first)
				return a.second < b.second;
			else
				return a.first < b.first;
		});
		arcs.erase(unique(arcs.begin(), arcs.end()), arcs.end());
		if (verbose) cout << "done." << endl;
	}

	if (verbose) cout << endl;

	// Determine identifier.
	const string identifier = Platform::SharedMemoryManager::GetIdentifierFromFilename(inFilename);

	// Build the graph.
	outGraph.BuildFromArcList(identifier, numVertices, arcs, directed, buildIncomingArcs, verbose);
}


// Stream a graph.
template<typename graphType>
void StreamDimacsGraph(const string inFilename, const string outFilename, const bool ignoreSelfLoops, const bool undirected, const bool transpose, const bool verbose) {
	// Get the file size of the input filestream.
	Types::SizeType fileSize = IO::FileSize(inFilename);

	// Define the type of the out stream.
	typedef IO::GraphStream<graphType> graphStreamType;

	// Open the input file as a stream.
	IO::FileStream inStream;
	inStream.OpenForReading(inFilename);

	// Create a new graph stream.
	graphStreamType outStream;
	outStream.New(outFilename);
	outStream.SetDirectedness(!undirected);

	// Create timer and progress bar.
	cout << "Streaming from " << inFilename << " (" << (fileSize / 1024.0 / 1024.0) << " MiB): " << endl;
	Tools::FancyProgressBar bar(fileSize, "", verbose);

	// Read the file line-by-line.
	string line;
	uint64_t lineNumber = 0;
	bool headerParsed = false;
	vector<const char*> tokens;
	Types::SizeType numVertices = 0;
	Types::SizeType numArcs = 0;
	Types::SizeType numSelfLoopsIgnored = 0;
	Types::SizeType expectedNumArcs = 0;

	// Read line-by-line.
	while (!inStream.Finished()) {
		inStream.ExtractLine(line);
		++lineNumber;
		bar.IterateTo(inStream.NumBytesRead());

		// Skip empty lines.
		if (line.empty()) continue;

		// Skip lines that begin with a %-symbol.
		if (line[0] == '%') continue;
		if (line[0] == 'c') continue;

		// The header contains the number of vertices and arcs.
		if (!headerParsed) {
			// Parse the header.
			Tools::DynamicSplitInline(line, tokens, ' ');

			// We need at least two tokens for the file format to be correct.
			Assert(tokens.size() >= 4);

			// The third token indicates the number of vertices.
			numVertices = Tools::LexicalCast<Types::SizeType>(tokens[2]);
			outStream.SetNumVertices(numVertices);

			// Store the expected number of arcs.
			expectedNumArcs = Tools::LexicalCast<Types::SizeType>(tokens[3]);

			headerParsed = true;
		}
		else {
			// Parse tail and head vertices.
			Tools::DynamicSplitInline(line, tokens, ' ');

			// The line should have at least four tokens.
			Assert(tokens.size() >= 4);

			// The first token must be an "a".
			Assert(tokens[0][0] == 'a' && tokens[0][1] == '\0');

			typename graphType::VertexIdType fromVertexId = Tools::LexicalCast<typename graphType::VertexIdType>(tokens[1]);
			typename graphType::VertexIdType toVertexId = Tools::LexicalCast<typename graphType::VertexIdType>(tokens[2]);
			--fromVertexId; // In the text file vertex ids are one-based.
			--toVertexId;

			if (transpose)
				swap(fromVertexId, toVertexId);

			typename graphType::ArcMetaDataType::WeightType weight = Tools::LexicalCast<typename graphType::ArcMetaDataType::WeightType>(tokens[3]);
			//weight /= 100;
			//weight += 1;
			Assert(fromVertexId >= 0);
			Assert(toVertexId >= 0);
			Assert(fromVertexId < numVertices);
			Assert(toVertexId < numVertices);

			if (ignoreSelfLoops && fromVertexId == toVertexId) {
				++numSelfLoopsIgnored;
				continue;
			}

			if (undirected && fromVertexId > toVertexId)
				continue;

			outStream.AddArc(fromVertexId, toVertexId, weight);
			++numArcs;
		}
	}

	if (verbose) cout << numArcs << " of " << expectedNumArcs << " expected arcs parsed; " << numSelfLoopsIgnored << " selfloops ignored." << endl;

	// Close graph stream.
	outStream.Close();

	// Dump statistics.
	if (verbose) outStream.DumpStatistics(cout);
}





}