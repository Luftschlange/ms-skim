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

#include <fstream>
#include <cstring>

#include "Assert.h"
#include "Types.h"
#include "Constants.h"

using namespace std;

namespace IO {

class FileStream {

public:

	// Construct this object.
	FileStream(const int bc = Constants::DefaultFileBufferSize) : // Leave the default buffer size, please. 
		buffer(nullptr), 
		bufferCapacity(bc), 
		bufferSize(0), 
		bufferIndex(0), 
		bytesRead(0), 
		bytesWritten(0),
		readToEnd(true),
		previousOperation(NO_OPERATION)
	{
		buffer = new char[bufferCapacity];
		Assert(buffer != nullptr);
	}


	// Opens a file.
	inline void OpenForReading(const string filename) { Open(filename, ios::binary | ios::in); }
	inline void OpenNewForWriting(const string filename) { Open(filename, ios::binary | ios::out | ios::trunc); }
	inline void OpenForReadingWriting(const string filename) { Open(filename, ios::binary | ios::in | ios::out); }
	inline void Open(const string filename, const ios_base::openmode mode) {
		// Try to open the file.
		file.open(filename.c_str(), mode);

		// Abort if the file is not open.
		if (!file.is_open()) return;

		// Not finished reading.
		readToEnd = false;

		// Number of bytes read is zero.
		bytesRead = 0;
		bytesWritten = 0;

		// Set buffer size to 0 (empty)
		bufferSize = 0;
		bufferIndex = 0;

		previousOperation = NO_OPERATION;

		return;
	}


	// Close the file.
	inline void Close() {
		if (!file.is_open()) return;

		// If the previous operation was a write, flush the buffer.
		if (previousOperation == WRITE_OPERATION)
			Flush();

		file.close();
		bufferIndex = 0;
		bufferSize = 0;
		bytesRead = 0;
		readToEnd = true;
	}


	// Reset the file stream. Seeks to the beginning.
	inline void Reset() {
		Assert(file.is_open());
		file.clear();
		file.seekg(0, ios::beg); 
		readToEnd = false;
		bufferIndex = 0;
		bufferSize = 0;
		bytesRead = 0;
		previousOperation = NO_OPERATION;
	}


	// Check if file is open.
	inline bool IsOpen() {
		return file.is_open();
	}

	// If the object gets destroyed, close the file.
	~FileStream() {
		Close();
		Assert(buffer != nullptr);
		delete[] buffer;
	}


	// Return the number of bytes read from the file.
	inline streamsize NumBytesRead() const {
		return bytesRead;
	}

	// Return the number of bytes written.
	inline streamsize NumBytesWritten() const {
		return bytesWritten;
	}

	// Test if the end of the file has been reached.
	inline bool Finished() const {
		return readToEnd;
	}


	// Seek in the file.
	inline void SeekFromEnd(streampos position) {
		// If the previous operation was write, we first need to flush buffers.
		if (previousOperation == WRITE_OPERATION)
			Flush();

		// Seek to position in input/output buffer of fstream.
		file.seekg(-position, ios::end);

		// Reset buffer.
		bufferSize = 0;
		bufferIndex = bufferSize;
		previousOperation = NO_OPERATION;
	}


	// Seek in the file.
	inline void SeekFromBeginning(streampos position) {
		// If the previous operation was write, we first need to flush buffers.
		if (previousOperation == WRITE_OPERATION)
			Flush();

		if (file.eof() && file.is_open())
			file.clear();

		// Seek to position in input/output buffer of fstream.
		file.seekp(position, ios::beg);
		file.seekg(position, ios::beg);

		// Reset buffer.
		bufferSize = 0;
		bufferIndex = bufferSize;
		previousOperation = NO_OPERATION;
	}

	// Extract a line from the file.
	inline void ExtractLine(string &line) {
		PrepareRead();
		line.clear();

		for (int fromIndex = bufferIndex; ; ++bufferIndex) {
			
			// If file pointer is such that new data should be read, do so.
			// Also dump whatever we have to the string.
			if (ReadRequired()) {

				// Append the remainder of the buffer.
				if (fromIndex < bufferSize)
					line.append(buffer + fromIndex, bufferSize - fromIndex);

				// If the file is eof. Return whatever is in the string, and stop.
				if (file.eof()) {
					readToEnd = true;
					return;
				}

				Read();
				fromIndex = 0;
			}

			// If we have a newline character, append stuff up to here to the
			// string, and return it.
			if (buffer[bufferIndex] == '\n') {
				if (fromIndex < bufferSize)
					line.append(buffer + fromIndex, bufferIndex - fromIndex);

				// Pop off carriage return, if it exists.
				if (!line.empty() && line[line.size() - 1] == '\r')
					line.pop_back();

				++bufferIndex;
				fromIndex = bufferIndex;
				return;
			}
		}

	}


	// Extract a single character from the stream.
	inline char ExtractCharacter() {
		PrepareRead();

		// See if we need to read in stuff.
		if (ReadRequired()) {
			if (file.eof()) {
				readToEnd = true;
				return '\0'; // Uhm...
			}

			Read();
		}

		return buffer[bufferIndex++];
	}



	// Reads a chunk of binary data.
	inline void Read(char *targetBuffer, streamsize numBytes) {
		PrepareRead();
		streamsize remainingBytesToExtract = numBytes;

		while (remainingBytesToExtract > 0) {
			// If we can safely extract everything from the internal buffer do so, and quit.
			if (bufferSize - bufferIndex >= remainingBytesToExtract) {
				std::memcpy(targetBuffer, buffer+bufferIndex, static_cast<size_t>(remainingBytesToExtract));
				bufferIndex += static_cast<int>(remainingBytesToExtract);

				UpdateFinished();
				break;
			// If we have to wrap around, extract all the stuff up to the end, and re-try.
			} else {
				const int bytesToExtract = bufferSize - bufferIndex;
				if (bytesToExtract > 0) {
					std::memcpy(targetBuffer, buffer+bufferIndex, bytesToExtract);
					targetBuffer += bytesToExtract;
					remainingBytesToExtract -= bytesToExtract;
				}

				// Try to read, but if it failed, we cannot read more, and return whatever we have read.
				if (file.eof()) {
					readToEnd = true;
					return;
				}
				
				Read();
			}
		}
	}


	// Reads a chunk of binary data, but does not copy it anywhere.
	inline void Ignore(streamsize numBytes) {
		PrepareRead();
		streamsize remainingBytesToExtract = numBytes;

		while (remainingBytesToExtract > 0) {
			// If we can safely extract everything from the internal buffer do so, and quit.
			if (bufferSize - bufferIndex >= remainingBytesToExtract) {
				bufferIndex += static_cast<int>(remainingBytesToExtract);
				UpdateFinished();
				break;
			// If we have to wrap around, extract all the stuff up to the end, and re-try.
			} else {
				const int bytesToExtract = bufferSize - bufferIndex;
				if (bytesToExtract > 0)
					remainingBytesToExtract -= bytesToExtract;

				// Try to read, but if it failed, we cannot read more, and return whatever we have read.
				if (file.eof()) {
					readToEnd = true;
					return;
				}
				
				Read();
			}
		}
	}

	// Writes a chunk of binary data.
	inline void Write(const char* sourceBuffer, streamsize numBytes) {
		PrepareWrite();
		streamsize remainingBytesToCopy = numBytes;

		while (remainingBytesToCopy > 0) {
			if (bufferCapacity - bufferSize >= remainingBytesToCopy) {
				memcpy(buffer+bufferSize, sourceBuffer, static_cast<size_t>(remainingBytesToCopy));
				bufferSize += static_cast<int>(remainingBytesToCopy);
				break;
			} else {
				const int bytesToCopy = bufferCapacity - bufferSize;
				memcpy(buffer+bufferSize, sourceBuffer, bytesToCopy);
				bufferSize += static_cast<int>(bytesToCopy);
				sourceBuffer += bytesToCopy;
				remainingBytesToCopy -= bytesToCopy;

				// Try to flush to disk.
				Flush();
			}
		}

	}

	// Writes a string to the file.
	inline void WriteString(const string str) {
		Write(str.c_str(), str.length());
	}


private:

	// Test if a read is required.
	inline bool ReadRequired() const {
		return (bufferIndex < 0 || bufferIndex >= bufferSize);
	}


	// Updates the eof flag.
	inline void UpdateFinished() {
		if (previousOperation != READ_OPERATION) return;
		//cout << "Read operation done; pos = " << file.tellg() << "; bufferIndex = " << bufferIndex << "; bufferSize = " << bufferSize << "; file.eof() = " << file.eof() << endl;
		if (bufferSize == bufferIndex && file.peek() == EOF)
			readToEnd = true;
	}

	// Read data, and set file pointer to zero.
	inline bool Read() {
		bufferIndex = 0;
		file.read(buffer, bufferCapacity);
		bufferSize = static_cast<int>(file.gcount());
		bytesRead += bufferSize;
		return bufferSize > 0;
	}

	// Flush the buffer to disk.
	inline void Flush() {
		Assert(bufferSize >= 0);
		Assert(bufferSize <= bufferCapacity);
		file.write(buffer, bufferSize);
		bytesWritten += bufferSize;
		bufferIndex = 0;
		bufferSize = 0;
	}

	// Prepare a read operation. Tests whether the last operation
	// was a write, in which case it first flushes the buffer.
	inline void PrepareRead() {
		if (previousOperation == WRITE_OPERATION) {
			Flush(); // Flush buffer to disk.
			bufferIndex = bufferCapacity; // Set the buffer index past the end to induce a new read request.
		}
		if (previousOperation == NO_OPERATION) {
			bufferIndex = bufferCapacity;
		}
		previousOperation = READ_OPERATION;
	}

	// Prepare a write operation. Tests whether the last operation
	// has been a read, in which case it prepares an empty buffer.
	inline void PrepareWrite() {
		if (previousOperation == READ_OPERATION) {
			bufferIndex = 0;
			bufferSize = 0;
		}
		previousOperation = WRITE_OPERATION;
	}


private:

	enum OperationType {
		NO_OPERATION,
		READ_OPERATION,
		WRITE_OPERATION
	};

	// This is the file we want to stream from/to.
	fstream file;

	// The buffer itself.
	char *buffer;

	// The size of the input buffer.
	const int bufferCapacity;

	// The current size of the buffer.
	int bufferSize;

	// A pointer into the buffer array where we are currently.
	int bufferIndex;

	// The number of globally read bytes. This is just a statistics counter.
	Types::SizeType bytesRead, bytesWritten;

	// Indicates whether reading has finished.
	bool readToEnd;

	// Indicates the previous operation on the disk.
	OperationType previousOperation;
};

}