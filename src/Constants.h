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


namespace Constants {

// The nicest number of the world.
const double Pi = 3.14159265359;

// The default size for read buffers for files.
const int DefaultFileBufferSize = 1024*64; // 64 kiB

// This is a list of prime numbers with the following properties:
// 1. Each number is about twice the size of the previous.
// 2. Each number is as far as possible from the nearest two powers of two.
// 3. The number at index i is the prime between 2^i and 2^(i+1).
const uint32_t Primes[] = {
	2, // 2^0 - 2^1
	3, // 2^1 - 2^2
	7, // 2^2 - 2^3
	13, // 2^3 - 2^4
	23, // 2^4 - 2^6
	53, // 2^5 - 2^6
	97, // 2^6 - 2^7
	193, // 2^7 - 2^8
	389, // 2^8 - 2^9
	769, // 2^9 - 2^10
	1543, // 2^10 - 2^11
	3079, // 2^11 - 2^12
	6151, // 2^12 - 2^13
	12289, // 2^13 - 2^14
	24593, // 2^14 - 2^15
	49157, // 2^15 - 2^16
	98317, // 2^16 - 2^17
	196613, // 2^17 - 2^18
	393241, // 2^18 - 2^19
	786433, // 2^19 - 2^20
	1572869, // 2^20 - 2^21
	3145739, // 2^21 - 2^22
	6291469, // 2^22 - 2^23
	12582917, // 2^23 - 2^24
	25165843, // 2^24 - 2^25
	50331653, // 2^25 - 2^26
	100663319, // 2^26 - 2^27
	201326611, // 2^27 - 2^28
	402653189, // 2^28 - 2^29
	805306457, // 2^29 - 2^30
	1610612741 // 2^30 - 2^31
};

}