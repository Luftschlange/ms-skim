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
#include <vector>
#include <fstream>
#include <algorithm>
#include <string>

#include "Types.h"
#include "FastVertex.h"
#include "FastArc.h"

#include "EntityIO.h"
#include "FileSize.h"
#include "FileStream.h"
//#include "StaticGraph.h"
#include "FastCompatibleGraphStream.h"

#include "FancyProgressBar.h"
#include "SharedMemoryManager.h"

using namespace std;


namespace DataStructures {
namespace Graphs {


template<typename arcIdType>
class FastStaticGraph {

public:

	// Expose typedefs.
	typedef uint32_t VertexIdType;
	typedef arcIdType ArcIdType;
	typedef FastStaticGraph<arcIdType> ThisType;

	// Declare typedefs.
	typedef FastVertex<ArcIdType> VertexType;
	typedef FastArc ArcType;

	// The header type.
	struct HeaderType {
		Types::SizeType NumVertices = 0;
		Types::SizeType NumArcs = 0;
		bool IsDirected = false;
	};

	// Construct an empty graph.
	FastStaticGraph() : header(nullptr), vertices(nullptr), arcs(nullptr) {}

	// Construct the graph by immediately reading it.
	FastStaticGraph(const string filename, const bool buildIncomingArcs, const bool verbose, const Platform::DWORD preferredNumaNode = Platform::DWORD() - 1) :
		header(nullptr),
		vertices(nullptr),
		arcs(nullptr)
	{
		Read(filename, buildIncomingArcs, verbose, preferredNumaNode);
	}


	// Destructor, free up the shared memory stuff.
	~FastStaticGraph() {
		Detach();
	}



	// Get extents.
	inline Types::SizeType NumVertices() const { return header->NumVertices; }
	inline Types::SizeType NumArcs() const { return header->NumArcs; }
	inline Types::SizeType NumArcs(const VertexIdType vertexId) { return vertices[vertexId + 1].FirstArcId() - vertices[vertexId].FirstArcId(); }

	inline bool Empty() const { return NumVertices() == 0; }

	// Test if the graph is directed.
	inline bool IsDirected() const { return header->IsDirected; }

	// Get direct access to stuff.
	inline const VertexType* const Vertices() const { return vertices; }
	inline VertexType* const Vertices() { return vertices; }
	inline const ArcType* const Arcs() const { return arcs; }
	inline ArcType* const Arcs() { return arcs; }

	// Access to stuff by id.
	inline const VertexType &Vertex(const VertexIdType vertexId) const {
		Assert(vertexId < NumVertices());
		return vertices[vertexId];
	}

	// Get access to items.
	inline VertexIdType GetFirstVertexId() const { return 0; }
	inline ArcType *GetFirstArc(const VertexIdType vertexId) {
		Assert(vertexId < NumVertices());
		return arcs + vertices[vertexId].FirstArcId();
	}
	inline ArcType* GetLastArc(const VertexIdType vertexId) {
		Assert(vertexId < NumVertices());
		return arcs + vertices[vertexId + 1].FirstArcId() - 1;
	}


	// Map between pointers and ids.
	inline const ArcIdType GetArcId(const ArcType *const arc) const { return static_cast<ArcIdType>(arc - arcs); }

	// Get the identifier of this static graph.
	inline string GetIdentifier() const { return identifier; }


	// Allocates (shared) memory for a graph with n vertices and m arcs.
	// The identifier is used to share the graph between different processes.
	template<typename containerType>
	inline void BuildFromArcList(const string id, const VertexIdType numVertices, const containerType &inArcs, const bool directed, const bool buildIncomingArcs, const bool verbose, const Platform::DWORD preferredNumaNode = Platform::DWORD() - 1) {
		// Unload whatever we have in memory.
		Detach();

		// Create identifiers.
		identifier = "fgraph/" + id + "/" + (buildIncomingArcs ? "bi" : "uni");
		identifierHeader = identifier + "/header";
		identifierVertices = identifier + "/vertices";
		identifierArcs = identifier + "/arcs";

		// If the graph is already loaded in memory, just attach to it.
		if (Platform::SharedMemoryManager::Exists(identifierHeader)) {
			if (verbose) cout << "*** The graph '" << identifier << "' is in memory already, attaching." << endl;
			Attach(verbose, preferredNumaNode);
		}
		else {
			ReadFromArcList(numVertices, inArcs, directed, buildIncomingArcs, verbose, preferredNumaNode);
		}

		// Perform a consistency check.
		int numErrors = GetErrors(verbose);
		Assert(numErrors == 0);

		// Dump statistics.
		if (verbose) DumpStatistics(cout);
		if (verbose) cout << endl;
	}


	// Read the graph fully into memory from disk.
	inline void Read(const string filename, const bool buildIncomingArcs, const bool verbose, const Platform::DWORD preferredNumaNode = Platform::DWORD() - 1) {
		// Unload whatever we have in memory.
		Detach();

		// Create identifiers from filename.
		const string fullpath = Platform::SharedMemoryManager::GetIdentifierFromFilename(filename);
		identifier = "fgraph/" + fullpath + "/" + (buildIncomingArcs ? "bi" : "uni");
		identifierHeader = identifier + "/header";
		identifierVertices = identifier + "/vertices";
		identifierArcs = identifier + "/arcs";

		// If the graph is already loaded in memory, just attach to it.
		if (Platform::SharedMemoryManager::Exists(identifierHeader)) {
			if (verbose) cout << "*** The graph '" << identifier << "' is in memory already, attaching." << endl;
			Attach(verbose, preferredNumaNode);
		}
		else {
			if (verbose) cout << "*** The graph '" << identifier << "' is not found in memory, attempting to read from '" << filename << ".gr'." << endl;
			ReadFromDisk(filename, buildIncomingArcs, verbose, preferredNumaNode);
		}

		// Perform a consistency check.
		int numErrors = GetErrors(verbose);
		Assert(numErrors == 0);

		// Dump statistics.
		if (verbose) DumpStatistics(cout);
		if (verbose) cout << endl;
	}


	// Dump statistics.
	void DumpStatistics(ostream &os) const {
		os << "Graph statistics: "
			<< NumVertices() << " vertices, "
			<< NumArcs() << " arcs, "
			<< fixed << setprecision(2) << (MemoryFootprint() / 1024.0 / 1024.0) << " MiB." << endl;
	}


	// Return the memory footprint of this graph.
	inline uint64_t MemoryFootprint() const {
		return sizeof(HeaderType) + (header->NumVertices + 1)*sizeof(VertexType)+(header->NumArcs + 1)*sizeof(ArcType);
	}


	// Check for consistency.
	int GetErrors(const bool verbose = true) {
		int numErrors(0);

		if (verbose) cout << "Performing a consistency check on Graph... " << flush;

		// Check vertices.
		bool reachedEnd(false);
		for (VertexIdType vertexId = 0; vertexId < NumVertices(); ++vertexId) {
			// FirstArcId has to be in range
			if (vertices[vertexId].FirstArcId() < 0 || vertices[vertexId].FirstArcId() >= (header->NumArcs + 1)) {
				if (verbose) cout << "ERROR: Vertex " << vertexId << "'s FirstArcId is out of range: " << vertices[vertexId].FirstArcId() << "." << endl;
				++numErrors;
			}
			// FirstArcIds have to be monotonically increasing.
			if (vertexId > 0 && vertices[vertexId].FirstArcId() < vertices[vertexId - 1].FirstArcId()) {
				if (verbose) cout << "ERROR: Vertex " << vertexId << "'s FirstArcId is smaller than that of vertex " << vertexId - 1 << ": " << vertices[vertexId].FirstArcId() << " < " << vertices[vertexId - 1].FirstArcId() << "." << endl;
				++numErrors;
			}
			numErrors += vertices[vertexId].GetErrors(verbose);
		}

		// The last vertex's FirstArcId has to point to the last arc of the graph.
		if (vertices[header->NumVertices].FirstArcId() != header->NumArcs) {
			if (verbose) cout << "ERROR: The last (dummy) vertex's FirstArcId does not point to the last (dummy) arc, but to " << vertices[header->NumVertices].FirstArcId() << "." << endl;
		}

		// Check the arcs.
		for (ArcIdType arcId = 0; arcId < NumArcs(); ++arcId) {
			// Check whether other vertex id exists.
			if (arcs[arcId].OtherVertexId() < 0 || arcs[arcId].OtherVertexId() >= header->NumVertices) {
				if (verbose) cout << "ERROR: Arc " << arcId << "'s OtherVertexId is out of range: " << arcs[arcId].OtherVertexId() << "." << endl;
				++numErrors;
			}
			numErrors += arcs[arcId].GetErrors(verbose);
		}

		// Check for self-loops.
		int numSelfLoops(0);
		for (VertexIdType vertexId = 0; vertexId < NumVertices(); ++vertexId) {
			for (ArcType *arc = GetFirstArc(vertexId); arc <= GetLastArc(vertexId); ++arc) {
				if (arc->OtherVertexId() == vertexId) {
					++numSelfLoops;
				}
			}
		}
		if (numSelfLoops > 0) {
			if (verbose) cout << "WARNING: " << numSelfLoops << " self-loops found." << endl;
		}

		if (verbose) cout << numErrors << " errors found." << endl;
		return numErrors;
	}


public:

	// Sort the arcs for a certain vertex u.
	inline void SortArcs(const VertexIdType u) {
		Assert(u >= 0);
		Assert(u < NumVertices());
		sort(arcs + vertices[u].FirstArcId(), arcs + vertices[u + 1].FirstArcId());
	}

protected:

	// Attach the graph to shared memory.
	inline void Attach(const bool verbose, const Platform::DWORD preferredNumaNode) {
		// Get the header.
		header = (HeaderType*)Platform::SharedMemoryManager::OpenSharedMemoryFile(identifierHeader, verbose, preferredNumaNode);
		Assert(header != nullptr);
		//Assert(header->MagicNumber == FileHeader::CorrectMagicNumber);
		//Assert(header->VertexMetaDataSize == sizeof(vertexMetaDataType));
		//Assert(header->ArcMetaDataSize == sizeof(arcMetaDataType));

		// Get the vertices.
		vertices = (VertexType*)Platform::SharedMemoryManager::OpenSharedMemoryFile(identifierVertices, verbose, preferredNumaNode);
		Assert(vertices != nullptr);
		Assert((header->NumVertices + 1)*sizeof(VertexType) == Platform::SharedMemoryManager::GetSharedMemoryFileSize(identifierVertices));

		// Get the arcs.
		arcs = (ArcType*)Platform::SharedMemoryManager::OpenSharedMemoryFile(identifierArcs, verbose, preferredNumaNode);
		Assert(arcs != nullptr);
		Assert((header->NumArcs + 1)*sizeof(ArcType) == Platform::SharedMemoryManager::GetSharedMemoryFileSize(identifierArcs));
	}


	// Detach whatever is loaded.
	inline void Detach() {
		if (header != nullptr) {
			Platform::SharedMemoryManager::CloseSharedMemoryFile(identifierHeader);
			header = nullptr;
		}
		if (vertices != nullptr) {
			Platform::SharedMemoryManager::CloseSharedMemoryFile(identifierVertices);
			vertices = nullptr;
		}
		if (arcs != nullptr) {
			Platform::SharedMemoryManager::CloseSharedMemoryFile(identifierArcs);
			arcs = nullptr;
		}
	}


	// Read the graph from a vector<pair> which contains the arcs.
	template<typename containerType>
	inline void ReadFromArcList(const VertexIdType numVertices, const containerType &inArcs, const bool isDirected, const bool buildIncomingArcs, const bool verbose, const Platform::DWORD preferredNumaNode) {
		// Allocate memory for the header.
		if (header != nullptr) {
			Platform::SharedMemoryManager::CloseSharedMemoryFile(identifierHeader);
			header = nullptr;
		}
		header = (HeaderType*)Platform::SharedMemoryManager::CreateSharedMemoryFile(sizeof(HeaderType), identifierHeader, verbose, preferredNumaNode);
		Assert(header != nullptr);

		// Determine extents of the graph.
		Types::SizeType numArcs(inArcs.size());

		// Print some stats about the graph.
		if (verbose) cout << "Awaiting " << numVertices << " vertices and " << numArcs << " arcs." << endl;
		if (verbose) cout << "Incoming arcs will be built: " << (buildIncomingArcs ? "YES" : "NO") << "." << endl;
		if (verbose) cout << "The graph will be: " << (isDirected ? "directed" : "undirected") << "." << endl;
		if (verbose) cout << "Total number of arc entities in data structure: " << ((buildIncomingArcs || !isDirected) ? (numArcs * 2) : numArcs) << "." << endl << endl;

		// Allocate storage for the vertices.
		if (vertices != nullptr) {
			Platform::SharedMemoryManager::CloseSharedMemoryFile(identifierVertices);
			vertices = nullptr;
		}
		vertices = (VertexType*)Platform::SharedMemoryManager::CreateSharedMemoryFile((numVertices + 1)*sizeof(VertexType), identifierVertices, verbose, preferredNumaNode);
		Assert(vertices != nullptr);

		// Allocate storage for the arcs.
		const double memoryConsumptionArcs = (sizeof(ArcType)* numArcs) / 1024.0 / 1024.0 * (buildIncomingArcs || !isDirected ? 2 : 1);
		numArcs *= (buildIncomingArcs || isDirected) ? 2 : 1;
		if (arcs != nullptr) {
			Platform::SharedMemoryManager::CloseSharedMemoryFile(identifierArcs);
			arcs = nullptr;
		}
		arcs = (ArcType*)Platform::SharedMemoryManager::CreateSharedMemoryFile((numArcs + 1)*sizeof(ArcType), identifierArcs, verbose, preferredNumaNode);
		Assert(arcs != nullptr);

		// Compute vertex degrees.
		if (verbose) cout << "Computing vertex degrees:" << endl;
		Tools::FancyProgressBar bar(inArcs.size(), "", verbose);
		for (const auto &inArc : inArcs) {
			VertexIdType fromVertexId(inArc.first), toVertexId(inArc.second);
			Assert(fromVertexId >= 0 && fromVertexId < numVertices);
			Assert(toVertexId >= 0 && toVertexId < numVertices);

			// If the graph is undirected, the fromVertexId has to be smaller than toVertexId
			Assert(isDirected || fromVertexId < toVertexId);

			// Increase out-degree at source vertex.
			vertices[fromVertexId].SetFirstArcId(vertices[fromVertexId].FirstArcId() + 1);

			// If we build incoming arcs or the graph is undirected, also increase in-degree at target vertex.
			if (buildIncomingArcs || !isDirected)
				vertices[toVertexId].SetFirstArcId(vertices[toVertexId].FirstArcId() + 1);
		}
		bar.Finish();

		// Rewire pointers.
		if (verbose) cout << "Wiring first arc id pointers:" << endl;
		vector<arcIdType> firstFreeArcId(numVertices + 1, 0);
		bar.Initialize(numVertices, "", verbose);
		arcIdType currentFirstArc = 0;
		for (VertexIdType u = 0; u < numVertices; ++u) {
			const arcIdType previousFirstArc = currentFirstArc;
			currentFirstArc += vertices[u].FirstArcId();
			vertices[u].SetFirstArcId(previousFirstArc);
			firstFreeArcId[u] = previousFirstArc;
			++bar;
		}
		firstFreeArcId[numVertices] = currentFirstArc;
		bar.Finish();
		Assert(currentFirstArc == numArcs);

		// Add arcs.
		if (verbose) cout << "Adding arcs to the graph:" << endl;
		bar.Initialize(inArcs.size(), "", verbose);
		for (const auto &inArc : inArcs) {
			VertexIdType fromVertexId(inArc.first), toVertexId(inArc.second);

			// Add outgoing arc.
			Assert(firstFreeArcId[fromVertexId] < firstFreeArcId[fromVertexId + 1]);
			arcs[firstFreeArcId[fromVertexId]].SetOtherVertexId(toVertexId);
			arcs[firstFreeArcId[fromVertexId]].SetForwardFlag();
			if (!isDirected && buildIncomingArcs)
				arcs[firstFreeArcId[fromVertexId]].SetBackwardFlag();
			++firstFreeArcId[fromVertexId];

			// If we shall add incoming arcs, do that as well.
			if (buildIncomingArcs || !isDirected) {
				Assert(firstFreeArcId[toVertexId] < firstFreeArcId[toVertexId + 1]);
				arcs[firstFreeArcId[toVertexId]].SetOtherVertexId(fromVertexId);
				if (!isDirected)
					arcs[firstFreeArcId[toVertexId]].SetForwardFlag();
				if (buildIncomingArcs)
					arcs[firstFreeArcId[toVertexId]].SetBackwardFlag();
				++firstFreeArcId[toVertexId];
			}

			// Iteratre progress bar.
			++bar;
		}
		bar.Finish();

		// Finalize construction.
		arcs[numArcs].SetOtherVertexId(static_cast<VertexIdType>(numVertices));
		vertices[numVertices].SetFirstArcId(static_cast<ArcIdType>(numArcs));

		// Copy extents of the graph to the header.
		header->NumVertices = numVertices;
		header->NumArcs = numArcs;
		header->IsDirected = isDirected;

		// Sort the arcs if the graph is directed and incoming arcs should have been build.
		if (buildIncomingArcs && isDirected) {
			if (verbose) cout << "Sorting the arcs at each vertex:" << endl;
			bar.Initialize(NumVertices(), "", verbose);
			for (VertexIdType u = 0; u < NumVertices(); ++u) {
				sort(arcs + vertices[u].FirstArcId(), arcs + vertices[u + 1].FirstArcId());
				++bar;
			}
			bar.Finish();
		}
	}


	// Read the graph from disk. Uses the standard .gr format. I.e., this is binary compatible.
	inline void ReadFromDisk(const string filename, const bool buildIncomingArcs, const bool verbose, const Platform::DWORD preferredNumaNode) {
		const Types::SizeType fileSize = IO::FileSize(filename + ".gr");
		IO::FastCompatibleGraphStream<arcIdType> stream(filename);

		// Allocate memory for the header.
		if (header != nullptr) {
			Platform::SharedMemoryManager::CloseSharedMemoryFile(identifierHeader);
			header = nullptr;
		}
		header = (HeaderType*)Platform::SharedMemoryManager::CreateSharedMemoryFile(sizeof(HeaderType), identifierHeader, verbose, preferredNumaNode);
		Assert(header != nullptr);

		// Determine extents of the graph.
		Types::SizeType numVertices(stream.NumVertices()), numArcs(stream.NumArcs());
		bool isDirected(stream.IsDirected());

		// Print some stats about the graph.
		if (verbose) cout << "Size of file: " << fixed << setprecision(1) << (fileSize / 1024.0 / 1024.0) << " MiB." << endl;
		if (verbose) cout << "Awaiting " << numVertices << " vertices and " << numArcs << " arcs." << endl;
		if (verbose) cout << "Incoming arcs will be built: " << (buildIncomingArcs ? "YES" : "NO") << "." << endl;
		if (verbose) cout << "The graph will be: " << (isDirected ? "directed" : "undirected") << "." << endl;
		if (verbose) cout << "Total number of arc entities in data structure: " << ((buildIncomingArcs || !isDirected) ? (numArcs * 2) : numArcs) << "." << endl << endl;

		// Allocate storage for the vertices.
		if (vertices != nullptr) {
			Platform::SharedMemoryManager::CloseSharedMemoryFile(identifierVertices);
			vertices = nullptr;
		}
		vertices = (VertexType*)Platform::SharedMemoryManager::CreateSharedMemoryFile((numVertices + 1)*sizeof(VertexType), identifierVertices, verbose, preferredNumaNode);
		Assert(vertices != nullptr);

		// Allocate storage for the arcs.
		const double memoryConsumptionArcs = (sizeof(ArcType)* numArcs) / 1024.0 / 1024.0 * (buildIncomingArcs || !isDirected ? 2 : 1);
		numArcs *= (buildIncomingArcs || !stream.IsDirected()) ? 2 : 1;
		if (arcs != nullptr) {
			Platform::SharedMemoryManager::CloseSharedMemoryFile(identifierArcs);
			arcs = nullptr;
		}
		arcs = (ArcType*)Platform::SharedMemoryManager::CreateSharedMemoryFile((numArcs + 1)*sizeof(ArcType), identifierArcs, verbose, preferredNumaNode);
		Assert(arcs != nullptr);

		// Compute vertex degrees.
		if (verbose) cout << "Computing vertex degrees:" << endl;
		Tools::FancyProgressBar bar(fileSize, "", verbose);
		Types::SizeType numArcsRead = 0;
		while (!stream.Finished() && numArcsRead < stream.NumArcs()) {
			VertexIdType fromVertexId(0), toVertexId(0);
			tie(fromVertexId, toVertexId) = stream.GetNextArc();
			Assert(fromVertexId >= 0 && fromVertexId < numVertices);
			Assert(toVertexId >= 0 && toVertexId < numVertices);

			// If the graph is undirected, the fromVertexId has to be smaller than toVertexId
			Assert(isDirected || fromVertexId < toVertexId);

			// Increase out-degree at source vertex.
			vertices[fromVertexId].SetFirstArcId(vertices[fromVertexId].FirstArcId() + 1);

			// If we build incoming arcs or the graph is undirected, also increase in-degree at target vertex.
			if (buildIncomingArcs || !isDirected)
				vertices[toVertexId].SetFirstArcId(vertices[toVertexId].FirstArcId() + 1);

			++numArcsRead;

			// Read type.
			bar.IterateTo(stream.NumBytesRead());
		}
		bar.Finish();
		if (verbose) cout << numArcsRead << " arcs parsed." << endl;

		// Rewire pointers.
		if (verbose) cout << "Wiring first arc id pointers:" << endl;
		vector<arcIdType> firstFreeArcId(numVertices + 1, 0);
		bar.Initialize(numVertices, "", verbose);
		arcIdType currentFirstArc = 0;
		for (VertexIdType u = 0; u < numVertices; ++u) {
			const arcIdType previousFirstArc = currentFirstArc;
			currentFirstArc += vertices[u].FirstArcId();
			vertices[u].SetFirstArcId(previousFirstArc);
			firstFreeArcId[u] = previousFirstArc;
			++bar;
		}
		firstFreeArcId[numVertices] = currentFirstArc;
		bar.Finish();
		Assert(currentFirstArc == numArcs);

		// Add arcs.
		stream.Reset();
		if (verbose) cout << "Adding arcs to the graph:" << endl;
		bar.Initialize(fileSize, "", verbose);
		numArcsRead = 0;
		while (!stream.Finished() && numArcsRead < stream.NumArcs()) {
			VertexIdType fromVertexId(0), toVertexId(0);
			tie(fromVertexId, toVertexId) = stream.GetNextArc();

			// Add outgoing arc.
			Assert(firstFreeArcId[fromVertexId] < firstFreeArcId[fromVertexId + 1]);
			arcs[firstFreeArcId[fromVertexId]].SetOtherVertexId(toVertexId);
			arcs[firstFreeArcId[fromVertexId]].SetForwardFlag();
			if (!isDirected && buildIncomingArcs)
				arcs[firstFreeArcId[fromVertexId]].SetBackwardFlag();
			++firstFreeArcId[fromVertexId];

			// If we shall add incoming arcs, do that as well.
			if (buildIncomingArcs || !isDirected) {
				Assert(firstFreeArcId[toVertexId] < firstFreeArcId[toVertexId + 1]);
				arcs[firstFreeArcId[toVertexId]].SetOtherVertexId(fromVertexId);
				if (!isDirected)
					arcs[firstFreeArcId[toVertexId]].SetForwardFlag();
				if (buildIncomingArcs)
					arcs[firstFreeArcId[toVertexId]].SetBackwardFlag();
				++firstFreeArcId[toVertexId];
			}

			// Iteratre progress bar.
			++numArcsRead;
			bar.IterateTo(stream.NumBytesRead());
		}
		bar.Finish();
		if (verbose) cout << numArcsRead << " arcs parsed." << endl;

		// Finalize construction.
		arcs[numArcs].SetOtherVertexId(static_cast<VertexIdType>(numVertices));
		vertices[numVertices].SetFirstArcId(static_cast<ArcIdType>(numArcs));

		// Copy extents of the graph to the header.
		header->NumVertices = numVertices;
		header->NumArcs = numArcs;
		header->IsDirected = isDirected;

		// Sort the arcs if the graph is directed and incoming arcs should have been build.
		if (buildIncomingArcs && isDirected) {
			if (verbose) cout << "Sorting the arcs at each vertex:" << endl;
			bar.Initialize(NumVertices(), "", verbose);
			for (VertexIdType u = 0; u < NumVertices(); ++u) {
				sort(arcs + vertices[u].FirstArcId(), arcs + vertices[u + 1].FirstArcId());
				++bar;
			}
			bar.Finish();
		}
	}


private:

	// These are the identifiers of the graph (as strings).
	string identifier, identifierHeader, identifierVertices, identifierArcs;

	// This holds the number of vertices and arcs, and whether the graph is directed.
	HeaderType *header;

	// This is an array of vertices.
	VertexType* vertices;

	// This is an array of arcs.
	ArcType* arcs;
};

}
}