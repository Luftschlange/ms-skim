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
#include <iostream>
#include <unordered_map>
#include <algorithm>

#ifndef _WIN32
#include <cstdlib>
#endif

using namespace std;

namespace Platform {

#if defined(_WIN32) || defined(__CYGWIN__)
#include <Windows.h>
#else
	typedef int DWORD;
#endif


	// This class manages shared memory between processes.
	// This is currently only supported for Windows.
	// If not compiling under Windows, each time shared memory is requested,
	// it will always create new memory by just calling malloc. Fun.
	class SharedMemoryManager {

		// Shared memory file handle type.
#if defined(_WIN32) || defined(__CYGWIN__)
		struct FileType {
			HANDLE Handle;
			LPCTSTR Pointer;
			Types::SizeType NumBytes;
			FileType() : Handle(NULL), Pointer(NULL), NumBytes(0) {}
			FileType(const HANDLE h, const LPCTSTR p, const Types::SizeType n) : Handle(h), Pointer(p), NumBytes(n) {}
		};
#else
		struct FileType {
			void* Pointer;
			Types::SizeType NumBytes;
			FileType() : Pointer(nullptr), NumBytes(0) {}
			FileType(void* p, const Types::SizeType n) : Pointer(p), NumBytes(n) {}
		};
#endif

	public:


		// Create new shared memory, and return a pointer to it.
#if defined(_WIN32) || defined(__CYGWIN__)
		static inline const void* CreateSharedMemoryFile(Types::SizeType numBytes, const string name, const bool verbose, const DWORD preferredNumaNode = NUMA_NO_PREFERRED_NODE) {
			if (verbose) {
				if (preferredNumaNode == NUMA_NO_PREFERRED_NODE)
					cout << "Creating memory mapped file '" << name << "'... " << flush;
				else
					cout << "Creating memory mapped file '" << name << "' on NUMA node " << preferredNumaNode << "... " << flush;
			}

			// If the file already exists in memory, just return its pointer.
			if (openFiles.find(name) != openFiles.end()) {
				if (verbose) cout << "exists (" << openFiles[name].NumBytes << " Bytes)." << endl;
				return openFiles[name].Pointer;
			}

			// The actual size of the file we need to create is the requested size
			// plus one Types::SizeType to store the length of the file in the beginning.
			Types::SizeType actualSize = numBytes + sizeof(Types::SizeType);

			// Attempt to create a new file mapping.
			HANDLE mappedFileHandle = CreateFileMappingNuma(
				INVALID_HANDLE_VALUE,    // use paging file
				NULL,                    // default security
				PAGE_READWRITE,          // read/write access
				static_cast<DWORD>(actualSize >> sizeof(DWORD)* 8),          // maximum object size (high-order DWORD)
				static_cast<DWORD>(actualSize),                // maximum object size (low-order DWORD)
				static_cast<const TCHAR*>(name.c_str()), // name.
				preferredNumaNode); // The preferred numa node where the file mapping shall be created.

			// If the creation failed, spit it out.
			if (mappedFileHandle == NULL) {
				if (verbose) cout << "Could not create file mapping object: " << GetLastError() << endl;
				return nullptr;
			}

			// Map the file into memory.
			LPCTSTR buffer(NULL);
			buffer = static_cast<LPTSTR>(MapViewOfFileExNuma(mappedFileHandle, FILE_MAP_ALL_ACCESS, 0, 0, 0, NULL, preferredNumaNode));

			// If the mapping failed, abort.
			if (buffer == NULL) {
				if (verbose) cout << "Could not map view of file: " << GetLastError() << endl;
				CloseHandle(mappedFileHandle);
				return nullptr;
			}

			// Write to the mapping the size (numBytes).
			*(Types::SizeType*)buffer = numBytes;

			// Compute the actual buffer where it begins.
			LPCTSTR beginBuffer = (LPCTSTR)((const Types::SizeType*)(buffer)+1);

			if (verbose) cout << "done (" << numBytes << " Bytes, Base address: 0x" << hex << (void*)beginBuffer << ")." << dec << endl;

			openFiles[name] = FileType(mappedFileHandle, beginBuffer, numBytes);
			return beginBuffer;
		}
#else	
		static inline const void* CreateSharedMemoryFile(Types::SizeType numBytes, const string name, const bool verbose, const DWORD preferredNumaNode = -1 /* ignored parameter */) {
			if (verbose) {
				cout << "Creating memory mapped file '" << name << "'... " << flush;
			}

			// If the file already exists in memory, just return its pointer.
			if (openFiles.find(name) != openFiles.end()) {
				if (verbose) cout << "exists (" << openFiles[name].NumBytes << " Bytes)." << endl;
				return openFiles[name].Pointer;
			}

			// Create a new buffer with the requested number of bytes.
			void* beginBuffer = ::operator new(numBytes);
			memset(beginBuffer, 0, numBytes);
			if (verbose) cout << "done (" << numBytes << " Bytes, Base address: 0x" << hex << (void*)beginBuffer << ")." << dec << endl;

			openFiles[name] = FileType(beginBuffer, numBytes);
			return beginBuffer;
		}

#endif


		// Open a shared memory file, and return pointer to it.
#if defined(_WIN32) || defined(__CYGWIN__)
		static inline const void* OpenSharedMemoryFile(const string name, const bool verbose, const DWORD preferredNumaNode = NUMA_NO_PREFERRED_NODE) {
			if (verbose) cout << "Attaching to memory mapped file '" << name << "'... " << flush;

			// If the file is already mapped, just return pointer to it.
			if (openFiles.find(name) != openFiles.end()) {
				if (verbose) cout << "exists (" << openFiles[name].NumBytes << " Bytes)." << endl;
				return openFiles[name].Pointer;
			}

			// Attempt to open an existing file mapping.
			HANDLE mappedFileHandle = OpenFileMapping(
				FILE_MAP_ALL_ACCESS,   // read/write access
				FALSE,                 // do not inherit the name
				static_cast<const TCHAR*>(name.c_str())); // name of mapping object

			// If the file could not be opened, return with a null pointer.
			if (mappedFileHandle == NULL) {
				if (verbose) cout << "Could not open file mapping object: " << GetLastError() << endl;
				return nullptr;
			}

			// Map the file into memory.
			LPCTSTR buffer(NULL);
			buffer = static_cast<LPTSTR>(MapViewOfFileExNuma(mappedFileHandle, FILE_MAP_ALL_ACCESS, 0, 0, 0, NULL, preferredNumaNode));

			// If this failed, return a null pointer.
			if (buffer == NULL) {
				if (verbose) cout << "Could not map view of file: " << GetLastError() << endl;
				CloseHandle(mappedFileHandle);
				return nullptr;
			}

			// Read size of the file.
			const Types::SizeType numBytes = *(Types::SizeType*)buffer;

			// Compute the real start address of the buffer.
			LPCTSTR beginBuffer = (LPCTSTR)((const Types::SizeType*)(buffer)+1);

			if (verbose) cout << "OK (" << numBytes << " Bytes, Base address: 0x" << hex << (void*)beginBuffer << ")." << dec << endl;

			// Add this opened file to the list of open files.
			openFiles[name] = FileType(mappedFileHandle, beginBuffer, numBytes);

			return beginBuffer;
		}
#else
		static inline const void* OpenSharedMemoryFile(const string name, const bool verbose, const DWORD preferredNumaNode = -1 /* ignored parameter */) {
			if (verbose) cout << "Attaching to memory mapped file '" << name << "'... " << flush;

			// If the file is already mapped, just return pointer to it.
			if (openFiles.find(name) != openFiles.end()) {
				if (verbose) cout << "exists (" << openFiles[name].NumBytes << " Bytes)." << endl;
				return openFiles[name].Pointer;
			} else {
				if (verbose) cout << "Could not map view of file." << endl;
				return nullptr;
			}
		}
#endif
	


		// Close a shared memory file (by string).
#if defined(_WIN32) || defined(__CYGWIN__)
		static inline void CloseSharedMemoryFile(const string name) {
			if (openFiles.find(name) == openFiles.end()) return;

			// Call the thing on the handle.
			CloseSharedMemory(openFiles[name]);

			// Delete from open files.
			openFiles.erase(name);
		}
#else
		static inline void CloseSharedMemoryFile(const string name) {
			if (openFiles.find(name) == openFiles.end()) return;

			// Free up memory.
			::operator delete(openFiles[name].Pointer);

			// Delete from table.
			openFiles.erase(name);
		}
#endif

		// Get the size of a memory mapped file.
		static inline Types::SizeType GetSharedMemoryFileSize(const string name) {
			if (openFiles.find(name) == openFiles.end()) return 0;

			return openFiles[name].NumBytes;
		}

		// Test if shared memory file is mapped already.
		static inline bool Mapped(const string name) {
			return openFiles.find(name) != openFiles.end();
		}


		// Test for existance.
#if defined(_WIN32) || defined(__CYGWIN__)
		static inline bool Exists(const string name) {
			// If the file is already mapped in memory, return true.
			if (Mapped(name)) return true;

			// Try to open the file and see if it succeeds.
			HANDLE mappedFileHandle = OpenFileMapping(
				FILE_MAP_ALL_ACCESS,   // read/write access
				FALSE,                 // do not inherit the name
				static_cast<const TCHAR*>(name.c_str())); // name of mapping object

			if (mappedFileHandle != NULL) {
				CloseHandle(mappedFileHandle);
				return true;
			}
			else	
				return false;
		}
#else
		static inline bool Exists(const string name) {
			return Mapped(name);
		}
#endif


		// Get an identifier which can be used as a name for a shared memory file
		// from an existing filename.
		// In case something fails, it will just return the input.
		static inline string GetIdentifierFromFilename(const string filename) {
			if (filename.empty())
				return filename;

#if defined(_WIN32) || defined(__CYGWIN__)
			// Obtain a path of up to 4kb length.
			char buffer[4096];
			const DWORD returnValue = GetFullPathName(filename.c_str(), 4096, buffer, NULL);

			// If this failed, just return the input.
			if (returnValue == 0)
				return filename;
#else
			const string buffer = filename;
#endif


			// Otherwise, return the full path.
			// But do some string transformations before.
			string fullpath(buffer);
			replace(fullpath.begin(), fullpath.end(), '\\', '/');
			fullpath.erase(remove_if(fullpath.begin(), fullpath.end(), [](const char c) {return c == ':'; }), fullpath.end());
			transform(fullpath.begin(), fullpath.end(), fullpath.begin(), ::tolower);
			return fullpath;
		}



	protected:


		// Close a shared memory file by file.
#if defined(_WIN32) || defined(__CYGWIN__)
		static inline void CloseSharedMemory(FileType &file) {
			Assert(file.Pointer != NULL);
			UnmapViewOfFile(file.Pointer);
			file.Pointer = NULL;
			Assert(file.Handle != NULL);
			CloseHandle(file.Handle);
			file.Handle = NULL;
		}
#endif


	private:

		// This is a collection of mapped file handles, indexed by name.
		static unordered_map<string, FileType> openFiles;

	};

	unordered_map<string, SharedMemoryManager::FileType> SharedMemoryManager::openFiles;

#if defined(_WIN32) || defined(__CYGWIN__)
	// Fix WinAPI.
#undef min
#undef max
#endif


}