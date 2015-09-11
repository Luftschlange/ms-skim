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

using namespace std;

#include "Assert.h"
#include "Types.h"


#include "FileStream.h"

namespace IO {

template<typename arcIdType>
class FastCompatibleGraphStream {

public:

	// Expose typedefs.
	typedef arcIdType ArcIdType;
	typedef uint32_t VertexIdType;

	// This is entities in the graph streaming file.
	enum EntityType : uint8_t {
		VERTEX_META_DATA = 0,
		ARC_META_DATA = 1,
		ARC = 2
	};

	// Create a new instance of the class.
	FastCompatibleGraphStream(const string fn) : filename(fn + ".gr") {
		file.OpenForReading(filename);
		Assert(file.IsOpen());

		// Extract the header from the file.
		header.Read(file);
		Assert(header.MagicNumber == FileHeaderType::CorrectMagicNumber);
	}



	// Reset.
	inline void Reset() {
		file.Reset();
		file.SeekFromBeginning(sizeof(FileHeaderType));
	}

	// Finished?
	inline bool Finished() {
		return file.Finished();
	}

	// Number of bytes read.
	inline Types::SizeType NumBytesRead() const {
		return file.NumBytesRead();
	}

	// Number of vertices.
	inline Types::SizeType NumVertices() const { return header.NumVertices; }
	inline Types::SizeType NumArcs() const { return header.NumArcs; }
	inline bool IsDirected() const { return header.IsDirected; }

	// Get next entity type.
	inline EntityType GetNextEntityType() {
		Assert(!file.Finished());
		return IO::ReadEntity<EntityType>(file);
	}

	// Get next arc.
	inline pair<VertexIdType, VertexIdType> GetNextArc() {
		while (!file.Finished()) {
			const EntityType currentEntity = GetNextEntityType();

			switch (currentEntity) {
			case ARC: {
						  const VertexIdType fromVertexId = IO::ReadEntity<VertexIdType>(file);
						  const VertexIdType toVertexId = IO::ReadEntity<VertexIdType>(file);
						  file.Ignore(header.ArcMetaDataSize);
						  return make_pair(fromVertexId, toVertexId);
			}
			case VERTEX_META_DATA:
				file.Ignore(header.VertexMetaDataSize);
				break;
			default:
				cerr << "ERROR: Input file is corrupt. Next Entity has undefined header: " << currentEntity << "." << endl;
				Assert(false);
			}
		}
		Assert(false);
		return make_pair(0, 0);
	}


	// Close the stream. Writes the header (to the beginning).
	inline void Close() {
		if (!file.IsOpen()) return;
		file.Close();
	}


private:

	// The file header
	struct FileHeaderType {
		static const uint32_t CorrectMagicNumber = 0x12341234;
		uint32_t MagicNumber;
		bool IsDirected;
		Types::SizeType NumVertices;
		Types::SizeType NumArcs;
		Types::SizeType GraphMetaDataSize;
		Types::SizeType VertexMetaDataSize;
		Types::SizeType ArcMetaDataSize;
		FileHeaderType() : MagicNumber(CorrectMagicNumber), IsDirected(true), NumVertices(0), NumArcs(0), GraphMetaDataSize(0), VertexMetaDataSize(0), ArcMetaDataSize(0) {}

		FileHeaderType(IO::FileStream &is) { Read(is); }

		// Get the sum of all entries
		Types::SizeType SumItems() const {
			return NumVertices + NumArcs;
		}

		void Read(IO::FileStream &is) {
			Assert(is.IsOpen());
			is.Read(reinterpret_cast<char*>(this), sizeof(*this));
		}
	} header;

	// The file's associated name.
	string filename;

	// The file stream to write/read.
	FileStream file;
};


}