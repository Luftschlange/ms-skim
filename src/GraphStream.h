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


#include "Assert.h"
#include "Types.h"

#include "FileStream.h"

using namespace std;

namespace IO {

template<typename graphType>
class GraphStream {

private:

	// Forward decleration of internal arc type.
	struct internalArcType;

public:

	// This is entities in the graph streaming file.
	enum EntityType : uint8_t {
		VERTEX_META_DATA = 0,
		ARC_META_DATA = 1,
		ARC = 2
	};


	// Expose typedefs.
	typedef graphType GraphType;

	// Create a new instance of the class.
	GraphStream() :
		filename(""), 
		numVertices(0),
		numArcs(0),
		isDirected(false),
		mode(UNKNOWN_MODE)
	{
	}


	// Create a new stream to a file.
	inline void New(const string fn) {
		filename = fn + ".gr";
		numVertices = 0;
		numArcs = 0;
		
		Close();

		file.OpenNewForWriting(filename);
		Assert(file.IsOpen());
		mode = WRITE_MODE;

		// Write a dummy header.
		typename GraphType::FileHeader dummyHeader;
		IO::WriteEntity(file, dummyHeader);
	}


	// Open a new stream for reading.
	inline typename GraphType::FileHeader Open(const string fn) {
		Close();

		filename = fn + ".gr";

		file.OpenForReading(filename);
		Assert(file.IsOpen());
		mode = READ_MODE;

		// Extract the header from the file.
		typename GraphType::FileHeader header;
		header.Read(file);
		Assert(header.MagicNumber == GraphType::FileHeader::CorrectMagicNumber);
		
		return header;
	}


	// Reset.
	inline void Reset() {
		file.Reset();
		file.SeekFromBeginning(sizeof(typename GraphType::FileHeader));
	}

	// Finished?
	inline bool Finished() {
		Assert(mode == READ_MODE);
		return file.Finished();
	}

	// Number of bytes read.
	inline Types::SizeType NumBytesRead() const {
		Assert(mode == READ_MODE);
		return file.NumBytesRead();
	}


	// Get next entity type.
	inline EntityType GetNextEntityType() {
		Assert(mode == READ_MODE);
		Assert(!file.Finished());
		return IO::ReadEntity<EntityType>(file);
	}

	// Get next arc.
	inline internalArcType GetNextArc() {
		Assert(mode == READ_MODE);
		while(!file.Finished()) {
			const EntityType currentEntity = GetNextEntityType();

			switch(currentEntity) {
			case ARC:
				return IO::ReadEntity<internalArcType>(file);
			case VERTEX_META_DATA:
				file.Ignore(sizeof(typename GraphType::VertexMetaDataType));
				break;
			default:
				cerr << "ERROR: Input file is corrupt. Next Entity has undefined header: " << currentEntity << "." << endl;
				Assert(false);
			}
		}
		Assert(false);
		return internalArcType();
	}


	// Get next vertex meta data.
	inline typename GraphType::VertexMetaDataType GetNextVertexMetaData() {
		Assert(mode == READ_MODE);
		while (!file.Finished()) {
			const EntityType currentEntity = GetNextEntityType();

			switch (currentEntity) {
			case ARC:
				file.Ignore(sizeof(internalArcType));
				break;
			case VERTEX_META_DATA:
				return IO::ReadEntity<typename GraphType::VertexMetaDataType>(file);
			default:
				cerr << "ERROR: Input file is corrupt. Next Entity has undefined header: " << currentEntity << "." << endl;
				Assert(false);
			}
		}
		Assert(false);
		return GraphType::VertexMetaDataType();
	}

	// Read from the stream and interpret as arc.
	inline internalArcType GetArc() { Assert(mode == READ_MODE); return IO::ReadEntity<internalArcType>(file);	}

	// Read from the stream and interpret as vertex meta data.
	inline typename GraphType::VertexMetaDataType GetVertexMetaData() { Assert(mode == READ_MODE); return IO::ReadEntity<typename GraphType::VertexMetaDataType>(file); }

	// Adds a single vertex to the stream. Writes its meta data to the stream.
	inline void AddVertexMetaData(const typename GraphType::VertexMetaDataType &metaData) {
		Assert(mode == WRITE_MODE);

		// Write the vertex's data to the stream.
		if (sizeof(metaData) > 0) {
			EntityType entityType = VERTEX_META_DATA;
			IO::WriteEntity(file, entityType);
			IO::WriteEntity(file, metaData);
		}

		++numVertices;
	}


	// Add an arc to the stream.
	inline void AddArc(const typename GraphType::VertexIdType fromVertexId, const typename GraphType::VertexIdType toVertexId, const typename GraphType::ArcMetaDataType &metaData) {
		Assert(mode == WRITE_MODE);

		// Create temporary internal arc that is written to the stream.
		internalArcType internalArc(fromVertexId, toVertexId, metaData);
		const EntityType entityType = ARC;
		IO::WriteEntity(file, entityType);
		IO::WriteEntity(file, internalArc);

		++numArcs;
	}


	// Sets the number of vertices to a given value.
	// Only use this if you did not use AddVertex
	// while creating the stream.
	inline void SetNumVertices(const Types::SizeType num) { numVertices = num; }

	// Sets whether the graph is considered as directed (true) or undirected (false).
	inline void SetDirectedness(const bool d) {	isDirected = d;	}


	// Close the stream. Writes the header (to the beginning).
	inline void Close() { 
		if (!file.IsOpen()) return;

		if (mode == WRITE_MODE) {
			file.SeekFromBeginning(0);
			typename GraphType::FileHeader header(isDirected, numVertices, numArcs);
			header.Write(file);
		}
		file.Close();
	}


	// Dump statistics.
	inline void DumpStatistics(ostream &os) {
		if (mode == WRITE_MODE) {
			cout << "The graph contained " << numVertices << " vertices and " << numArcs << " arcs (" << fixed << setprecision(2) <<  file.NumBytesWritten()/1024.0/1024.0 << " MiB)." << endl;
		}
	}
	

private:

	// This is an internal arc representation.
	struct internalArcType {
		typename GraphType::VertexIdType FromVertexId;
		typename GraphType::VertexIdType ToVertexId;
		typename GraphType::ArcMetaDataType MetaData;

		internalArcType() : FromVertexId(0), ToVertexId(0), MetaData(typename GraphType::ArcMetaDataType()) {}
		internalArcType(const typename GraphType::VertexIdType f, const typename GraphType::VertexIdType t, const typename GraphType::ArcMetaDataType &m) : FromVertexId(f), ToVertexId(t), MetaData(m) {}
	};

	// Modes supported by the stream.
	enum modeType {
		UNKNOWN_MODE,
		READ_MODE,
		WRITE_MODE
	};

	// The file's associated name.
	string filename;

	// The file stream to write/read.
	FileStream file;

	// Number of vertices in the stream.
	Types::SizeType numVertices;

	// Number of arcs in the stream.
	Types::SizeType numArcs;

	// This indicates whether the graph is directed (true) or not (false).
	bool isDirected;

	// The mode in which this instance is currently operating.
	modeType mode;
};


}