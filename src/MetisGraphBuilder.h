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
void BuildMetisGraph(const string inFilename, graphType &outGraph, const bool ignoreSelfLoops, const bool transpose, const bool directed, const bool buildIncomingArcs, const bool removeParallelArcs, const bool verbose) {
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
	Types::SizeType numArcs = 0;
	vertexIdType fromVertexId = 0;
	typedef pair<vertexIdType, vertexIdType> arcType;
	vector<arcType> arcs;

	// Read line-by-line.
	while (!inStream.Finished()) {
		inStream.ExtractLine(line);
		++lineNumber;
		bar.IterateTo(inStream.NumBytesRead());

		// Skip lines that begin with a %-symbol.
		if (line[0] == '%') continue;

		// The header contains the number of vertices and arcs.
		if (!headerParsed) {
			// Skip empty lines.
			if (line.empty()) continue;

			// Parse the header.
			Tools::DynamicSplitInline(line, tokens, ' ');

			// We need at least two tokens for the file format to be correct.
			Assert(tokens.size() >= 2);

			// The first token indicates the number of vertices.
			numVertices = Tools::LexicalCast<vertexIdType>(tokens[0]);

			headerParsed = true;
		}
		else {
			if (!line.empty()) {
				// Parse head vertices.
				Tools::DynamicSplitInline(line, tokens, ' ');

				// Go over the tokens, and add an arc for each (that is non-empty).
				for (size_t i = 0; i < tokens.size(); ++i) {
					if (strlen(tokens[i]) == 0) continue;
					typename graphType::VertexIdType toVertexId = Tools::LexicalCast<typename graphType::VertexIdType>(tokens[i]);
					--toVertexId; // In the text file vertex ids are one-based.
					Assert(fromVertexId < numVertices);
					Assert(toVertexId < numVertices);
					if (ignoreSelfLoops && fromVertexId == toVertexId)
						continue;
					if (transpose) {
						if (directed || toVertexId <= fromVertexId)
							arcs.push_back(arcType(toVertexId, fromVertexId));
					}
					else {
						if (directed || fromVertexId <= toVertexId)
							arcs.push_back(arcType(fromVertexId, toVertexId));
					}
				}
			}

			// The tail vertices are actually consecutive and zero-based.
			++fromVertexId;
		}
	}
	bar.Finish();

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
void StreamMetisGraph(const string inFilename, const string outFilename, const bool ignoreSelfLoops, const bool undirected, const bool transpose, const bool verbose) {
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
	typename graphType::VertexIdType fromVertexId = 0;

	// Read line-by-line.
	while (!inStream.Finished()) {
		inStream.ExtractLine(line);
		++lineNumber;
		bar.IterateTo(inStream.NumBytesRead());

		// Skip lines that begin with a %-symbol.
		if (line[0] == '%') continue;

		// The header contains the number of vertices and arcs.
		if (!headerParsed) {
			// Skip empty lines.
			if (line.empty()) continue;

			// Parse the header.
			Tools::DynamicSplitInline(line, tokens, ' ');

			// We need at least two tokens for the file format to be correct.
			Assert(tokens.size() >= 2);

			// The first token indicates the number of vertices.
			numVertices = Tools::LexicalCast<Types::SizeType>(tokens[0]);
			outStream.SetNumVertices(numVertices);

			headerParsed = true;
		} else {
			if (!line.empty()) {
				// Parse head vertices.
				Tools::DynamicSplitInline(line, tokens, ' ');

				// Go over the tokens, and add an arc for each (that is non-empty).
				for (size_t i = 0; i < tokens.size(); ++i) {
					if (strlen(tokens[i]) == 0) continue;
					typename graphType::VertexIdType toVertexId = Tools::LexicalCast<typename graphType::VertexIdType>(tokens[i]);
					--toVertexId; // In the text file vertex ids are one-based.
					Assert(fromVertexId < numVertices);
					Assert(toVertexId < numVertices);
					if (ignoreSelfLoops && fromVertexId == toVertexId)
						continue;
					if (transpose) {
						if (!undirected || toVertexId <= fromVertexId)
							outStream.AddArc(toVertexId, fromVertexId, typename graphType::ArcMetaDataType(1));
					}
					else {
						if (!undirected || fromVertexId <= toVertexId)
							outStream.AddArc(fromVertexId, toVertexId, typename graphType::ArcMetaDataType(1));
					}
				}
			}

			// The tail vertices are actually consecutive and zero-based.
			++fromVertexId;
		}
	}
	bar.Finish();

	// Close graph stream.
	outStream.Close();

	// Dump statistics.
	if (verbose) outStream.DumpStatistics(cout);
}





}