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

#include <array>
#include <tuple>
#include <vector>
#include <set>
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <unordered_set>
#include <random>
#include <climits>
#include <algorithm>
using namespace std;

#include "FastStaticGraphs.h"
#include "Macros.h"
#include "Timer.h"
#include "FastSet.h"
#include "Permutations.h"
#include "RangeExtraction.h"

namespace std {
	template<>
	struct hash<pair<uint32_t, uint64_t> > {
	public:
		size_t operator()(pair<uint32_t, uint64_t> x) const throw() {
			// Pack pair into uint64_t.
			uint64_t input(x.second ^ x.first);
			size_t h = hash<uint64_t>()(input);
			return h;
		}
	};
}

namespace Algorithms{
namespace InfluenceMaximization {

class FastRSInfluenceOracle {
public:

	// Type definitions.
	typedef DataStructures::Graphs::FastUnweightedGraph GraphType;
	typedef GraphType::ArcIdType ArcIdType;
	static const uint32_t NullVertex = UINT_MAX;

	enum ModelType { WEIGHTED, BINARY, TRIVALENCY };
	enum SeedMethodType { UNIFORM, NEIGHBORHOOD };

	// Default construct the algorithm.
	FastRSInfluenceOracle(GraphType &g, const uint32_t s, const bool v) :
		graph(g),
		randomSeed(s),
		resolution(3000000),
		indeg(graph.NumVertices(), 0),
		binprob(resolution / 10),
		triprob{ { resolution / 10, resolution / 100, resolution / 1000 } },
		searchSpace(graph.NumVertices()),
		levels(graph.NumVertices(), UINT32_MAX),
		twisty(s),
		sketchSize(0),
		verbose(v)
	{
		cout << "Computing in-degrees... " << flush;
		// Compute the degrees.
		FORALL_ARCS(graph, vertexId, arc) {
			if (!arc->Forward()) continue;
			++indeg[arc->OtherVertexId()];
		}
		cout << "done." << endl;
	}


	// Set the binary probability.
	inline void SetBinaryProbability(const double prob) {
		binprob = uint32_t(prob * double(resolution));
	}
	
	// This runs a specific query, once the preprocessing is established.
	// It returns the estimated influence of the vertex set S.
	double RunSpecificQuery(const vector<uint32_t> &S, const uint16_t k, const uint16_t l) {
		return Estimator(S, k, l);
	}
	

	// This runs random queries of varying size ranges.
	// This assumes that sketches have already been computed.
	template<ModelType modelType>
	void Run(const string seedSizeRange, const SeedMethodType method, const uint16_t numQueries, const uint16_t k, const uint16_t l, const uint16_t lEval, const string statsFilename) {
		vector<Types::IndexType> seedSetSizes = Tools::ExtractRange(seedSizeRange);
		
		// Set up random number generator.
		vector<uint32_t> S;
		uniform_int_distribution<uint32_t> dist(0, static_cast<uint32_t>(method == UNIFORM ? graph.NumVertices() : graph.NumArcs())-1);

		// Initiate some statistics.
		stringstream stats;
		if (!statsFilename.empty())
			stats << "NumberOfVertices = " << graph.NumVertices() << endl
			<< "NumberOfArcs = " << graph.NumArcs() << endl
			<< "PreprocessingElapsedMilliseconds = " << preprocessingElapsedMilliseconds << endl
			<< "NumberOfQueries = " << numQueries << endl
			<< "SeedGenerator = " << method << endl
			<< "SeedSizeRange = " << seedSizeRange << endl
			<< "TotalSketchesSize = " << sketchSize << endl
			<< "TotalSketchesBytes = " << sketchSize * sizeof(uint64_t) << endl
			<< "NumberOfSeedSetSizes = " << seedSetSizes.size() << endl;

		// Iterate all ranges.
		Platform::Timer timer;
		for (Types::IndexType seedSetSizeIndex = 0; seedSetSizeIndex < seedSetSizes.size(); ++seedSetSizeIndex) {
			const Types::IndexType N = seedSetSizes[seedSetSizeIndex];
			//Z.reserve(N*k);
			sourceZ.reserve(N*k + 1); destZ.reserve(N*k + 1);
			sourceI.reserve(N + 1); destI.reserve(N + 1);
			cout << "Running " << numQueries << " queries with seed set size " << N << "... " << flush;
			
			if (!statsFilename.empty())
				stats << seedSetSizeIndex << "_SeedSetSize = " << N << endl;

			double averageError(0), averageEstimatedInfluence(0), averageExactInfluence(0), averageEstimatorElapsedMilliseconds(0), averageExactElapsedMilliseconds(0);
			for (uint32_t q = 0; q < numQueries; ++q) {
				// Compute N random seed vertices.
				S.clear();
				GenerateSeetSet(S, N, method, dist);
				Assert(S.size() == N);
				
				// Run estimator and exact algorithm.
				timer.Start();
				const double estimatedInfluence = Estimator(S, k, l);
				const double estimatorElapsedMilliseconds = timer.LiveElapsedMilliseconds();
				timer.Start();
				const double exactInfluence = ComputeInfluence<modelType>(S, lEval);
				const double exactElapsedMilliseconds = timer.LiveElapsedMilliseconds();
				const double error = abs(estimatedInfluence - exactInfluence) / exactInfluence;
				//cout << "est=" << estimatedInfluence << ", ex=" << exactInfluence << ", err=" << error << endl;
				averageError += error;
				averageEstimatedInfluence += estimatedInfluence;
				averageExactInfluence += exactInfluence;
				averageEstimatorElapsedMilliseconds += estimatorElapsedMilliseconds;
				averageExactElapsedMilliseconds += exactElapsedMilliseconds;

				// Write statistics, if a stats filename is given.
				if (!statsFilename.empty()) {
					stats << seedSetSizeIndex << "_" << q << "_VertexIds = ";
					for (uint32_t i = 0; i < N; ++i) {
						if (i > 0)
							stats << ",";
						stats << S[i];
					}
					stats << endl
						<<  seedSetSizeIndex << "_" << q << "_EstimatedInfluence = " << estimatedInfluence << endl
						<< seedSetSizeIndex << "_" << q << "_ExactInfluence = " << exactInfluence << endl
						<< seedSetSizeIndex << "_" << q << "_Error = " << error << endl
						<< seedSetSizeIndex << "_" << q << "_EstimatorElapsedMilliseconds = " << estimatorElapsedMilliseconds << endl
						<< seedSetSizeIndex << "_" << q << "_ExactElapsedMilliseconds = " << exactElapsedMilliseconds << endl;
				}
			}
			averageError /= double(numQueries);
			averageEstimatedInfluence /= double(numQueries);
			averageExactInfluence /= double(numQueries);
			averageEstimatorElapsedMilliseconds /= double(numQueries);
			averageExactElapsedMilliseconds /= double(numQueries);
			cout << "done (est=" << averageEstimatedInfluence << ", ex=" << averageExactInfluence << ", err=" << averageError << ", test=" << setprecision(5) << averageEstimatorElapsedMilliseconds << "ms, tex=" << averageExactElapsedMilliseconds << "ms)." << endl;
			if (!statsFilename.empty())
				stats << seedSetSizeIndex << "_AverageEstimatedInfluence = " << averageEstimatedInfluence << endl
				<< seedSetSizeIndex << "_AverageExactInfluence = " << averageExactInfluence << endl
				<< seedSetSizeIndex << "_AverageError = " << averageError << endl
				<< seedSetSizeIndex << "_AverageEstimatorElapsedMilliseconds = " << averageEstimatorElapsedMilliseconds << endl
				<< seedSetSizeIndex << "_AverageExactElapsedMilliseconds = " << averageExactElapsedMilliseconds << endl;
		}

		if (!statsFilename.empty()) {
			cout << "Attempting to write statistics to " << statsFilename << "... " << flush;
			ofstream file(statsFilename);
			if (file.is_open()) {
				file << stats.str();
				file.close();
				cout << "done." << endl;
			}
		}
	}


	// This is the estimator: S is the seed set of vertex ids. Based on sorting a vector.
	vector<pair<uint64_t, uint64_t>> sourceZ, destZ;
	vector<size_t> sourceI, destI;
	double Estimator(const vector<uint32_t> &S, const uint16_t k, const uint16_t l) {
		sourceI.clear();
		sourceZ.clear();
		const uint64_t sentinelRank = graph.NumVertices()*l;
		// Collect rank and taus.
		for (const uint32_t s : S) {
			const vector<uint64_t> &sketch = sketches[s];
			const size_t num = sketch.size() == k ? sketch.size() - 1 : sketch.size();
			const uint64_t tau = sketch.size() == k ? sketch.back() : (graph.NumVertices()*l);
			sourceI.push_back(sourceZ.size());
			for (size_t i = 0; i < num; ++i) {
				sourceZ.push_back(make_pair(sketch[i], tau));
			}
			sourceZ.push_back(make_pair(sentinelRank, 0));
		}
		sourceI.push_back(sourceZ.size()); // sentinel

		// Merge while there are things to merge.
		Assert(sourceI.size() >= 2);
		while (sourceI.size() > 2) {
			//cout << " " << sourceZ.size() << flush;
			const size_t num = sourceI.size() - 1;
			for (uint64_t i = 0; i < num; i += 2) {
				destI.push_back(destZ.size());
				const auto beginZ = sourceZ.begin();
				auto first1 = beginZ + sourceI[i];
				auto last1 = beginZ + sourceI[i + 1];

				// In case only one more chunk is left, copy it to destZ.
				if ((i + 1) == num) {
					destZ.insert(destZ.end(), first1, last1);
					continue;
				}

				// Otherwise merge the i'th with the i+1st chunk into destZ.
				Assert((i + 2) < num);
				auto first2 = last1;
				auto last2 = beginZ + sourceI[i + 2];
				while (true) {
					if (first1->first < first2->first) { destZ.push_back(*first1); ++first1; }
					else if (first2->first < first1->first) { destZ.push_back(*first2); ++first2; }
					else if (first1->second > first2->second) { destZ.push_back(*first1); ++first1; ++first2; }
					else { 
						destZ.push_back(*first2); 
						if (first1->first == sentinelRank)
							break;
						++first1; ++first2;
					}
				}
			}
			destI.push_back(destZ.size()); // sentinel

			sourceI.swap(destI);
			sourceZ.swap(destZ);
			destI.clear();
			destZ.clear();
		}

		// Accumulate estimate.
		Assert(!sourceZ.empty());
		sourceZ.pop_back();
		double estimate = 0;
		for (const pair<uint64_t, uint64_t> &z : sourceZ) {
			estimate += 1.0 / double(z.second);
		}
		return estimate*graph.NumVertices();
	}


	// This precomputes the sketches.
	template<ModelType modelType>
	void RunPreprocessing(const uint16_t k, const uint16_t l) {
		// Allocate data structures.
		cout << "Allocating data structures... " << flush;
		sketches.resize(graph.NumVertices()); // the sketches.
		vector<vector<uint64_t>> localSketches(graph.NumVertices()); // These are the temporary sketches (per instances).
		vector<uint64_t> permutation;
		DataStructures::Container::FastSet<uint32_t> &S = searchSpace; // The search space of the bfs.
		Tools::GenerateRandomPermutation(permutation, static_cast<uint64_t>(graph.NumVertices()*l), randomSeed);
		cout << "done." << endl;

		// Group vertex/instance pairs by instance.
		vector<vector<pair<uint64_t, uint32_t>>> instanceRanks(l);
		cout << "Grouping ranks by instance... " << flush;
		for (uint64_t r = 0; r < permutation.size(); ++r) {
			const uint16_t i = uint16_t(permutation[r] / graph.NumVertices());
			Assert(i < l);
			const uint32_t u = uint32_t(permutation[r] % graph.NumVertices());
			Assert(u < graph.NumVertices());
			instanceRanks[i].push_back(pair<uint64_t, uint32_t>(r, u));
		}
		vector<uint64_t>().swap(permutation);
		cout << "done." << endl;


		// Compute combined bottom-k rank sketches over all l instances.
		cout << "Attempting to compute combined bottom-k reachablility sketches... " << flush;
		Platform::Timer timer; timer.Start();
		for (uint16_t i = 0; i < l; ++i) {
			if (verbose) cout << " " << i << flush;
			Assert(instanceRanks[i].size() == graph.NumVertices());
			for (uint32_t j = 0; j < uint32_t(graph.NumVertices()); ++j) {
				const uint64_t rank = instanceRanks[i][j].first;
				const uint32_t sourceVertexId = instanceRanks[i][j].second;

				// Run a BFS from source vertex in instance i.
				S.Clear();
				S.Insert(sourceVertexId);
				uint32_t ind = 0;
				while (ind < S.Size()) {
					uint32_t u = S.KeyByIndex(ind++);
					vector<uint64_t> &Y = localSketches[u];

					// Prune if the sketch at u exceeds size k.
					if (Y.size() >= k)
						continue;

					// Insert rank into sketch of u.
					Y.push_back(rank);

					// Arc expansion.
					FORALL_INCIDENT_ARCS_BACKWARD(graph, u, a) {
						if (!a->Backward()) break;
						const uint32_t v = a->OtherVertexId();
						if (Contained<modelType>(v, u, i, l) && !S.IsContained(v))
							S.Insert(v);
					}
				}
			}
			if (verbose) cout << "m" << flush;

			// Merge local sketches into the global sketches.
			vector<uint64_t> Z;
			sketchSize = 0;
			FORALL_VERTICES(graph, u) {
				vector<uint64_t> &X = sketches[u];
				vector<uint64_t> &Y = localSketches[u];
				Z.resize(X.size()+Y.size(), 0);
				Z.resize(set_union(X.begin(), X.end(), Y.begin(), Y.end(), Z.begin()) - Z.begin()); // merge X and Y, erasing duplicates.
				if (Z.size() > k) Z.resize(k); // trim.
				sketchSize += Z.size();
				X.swap(Z); // copy new values from Z to X.
				Y.clear(); // erase local sketch to make room for next instance.
			}
			if (verbose) cout << "d" << flush;
		}
		preprocessingElapsedMilliseconds = timer.LiveElapsedMilliseconds();
		cout << endl << "Finished in " << Tools::MillisecondsToString(preprocessingElapsedMilliseconds) << endl;
	}


	// This computes exact influence.
	template<ModelType modelType>
	double ComputeInfluence(const vector<uint32_t> &S, const uint16_t l) {
		uint64_t size = 0;
		for (uint16_t i = 0; i < l; ++i) {
			// Run a BFS from source vertex in instance i.
			searchSpace.Clear();
			for (const uint32_t s : S) 
				searchSpace.Insert(s);
			uint32_t ind = 0;
			while (ind < searchSpace.Size()) {
				uint32_t u = searchSpace.KeyByIndex(ind++);
				++size;
				// arc expansion.
				FORALL_INCIDENT_ARCS(graph, u, a) {
					if (!a->Forward()) break;
					const uint32_t v = a->OtherVertexId();
					if (Contained<modelType>(u, v, i, l) && !searchSpace.IsContained(v))
						searchSpace.Insert(v);
				}
			}
		}

		return double(size) / double(l);
	}


	// Generates a random seed set according to various methods.
	template<typename distType>
	inline void GenerateSeetSet(vector<uint32_t> &S, const uint64_t N, const SeedMethodType t, distType &dist) {
		if (t == UNIFORM) {
			for (uint32_t i = 0; i < N; ++i)
				S.push_back(dist(twisty));
		}
		if (t == NEIGHBORHOOD) {
			while (S.size() < N) {
				// First, sample an arc.
				ArcIdType arcId(0);
				do {
					arcId = dist(twisty);
				} while (!graph.Arcs()[arcId].Backward());
				const uint32_t sourceVertexId = graph.Arcs()[arcId].OtherVertexId();

				// Run a BFS.
				searchSpace.Clear();
				searchSpace.Insert(sourceVertexId);
				levels[sourceVertexId] = 0;
				uint32_t cur = 0;
				uint32_t finalLevel = UINT32_MAX;
				while (cur < searchSpace.Size()) {
					const uint32_t u = searchSpace.KeyByIndex(cur++);
					if (levels[u] > finalLevel)
						break;
					if (cur >= (N-S.size()))
						finalLevel = levels[u];
					if (levels[u] == finalLevel)
						continue;
					FORALL_INCIDENT_ARCS(graph, u, arc) {
						if (!arc->Forward()) continue;
						const uint32_t v = arc->OtherVertexId();
						if (searchSpace.IsContained(v)) continue;
						levels[v] = levels[u] + 1;
						searchSpace.Insert(v);
					}
				}

				// Reset level information.
				for (size_t i = 0; i < searchSpace.Size(); ++i)
					levels[searchSpace.KeyByIndex(i)] = UINT32_MAX;

				// Remove stuff beyond the current level from the search space.
				Assert(cur > 0);
				while (searchSpace.Size() > cur)
					searchSpace.DeleteBack();

				// Now sample from the search space of the BFS.
				while (S.size() < N && !searchSpace.IsEmpty()) {
					const uint32_t randomIndex = fullDist(twisty) % searchSpace.Size();
					S.push_back(searchSpace.DeleteByIndex(randomIndex));
				}
			}
		}
	}

protected:

	// Returns true if the (forward) arc from u to v is contained in instance i.
	template<ModelType modelType>
	inline bool Contained(const uint32_t u, const uint32_t v, const uint16_t i, const uint16_t l) {
		assert(false);
		return false;
	}

	// A tailored Murmur hash 3 function for pair of vertices and instance.
	inline uint32_t Murmur3Hash(const uint32_t u, const uint32_t v, const uint16_t i, const uint16_t l) const {
		// Seed with our seed value.
		uint32_t h = (randomSeed << 16) + l;

		// Declare magic constants c1 and c2.
		const uint32_t c1 = 0xcc9e2d51;
		const uint32_t c2 = 0x1b873593;

		// Hash the first vertex.
		uint32_t k = u;
		k *= c1;
		k = _rotl(k, 15);
		k *= c2;
		h ^= k;
		h = _rotl(h, 13);
		h = h * 5 + 0xe6546b64;

		// Hash the second vertex.
		k = v;
		k *= c1;
		k = _rotl(k, 15);
		k *= c2;
		h ^= k;
		h = _rotl(h, 13);
		h = h * 5 + 0xe6546b64;

		// Hash the instance.
		k = static_cast<uint32_t>(i);
		k *= c1;
		k = _rotl(k, 15);
		k *= c2;
		h ^= k;

		// Mix the result.
		h ^= 10; // length of input in bytes.
		h ^= h >> 16;
		h *= 0x85ebca6b;
		h ^= h >> 13;
		h *= 0xc2b2ae35;
		h ^= h >> 16;

		return h;
	}

private:

	// The graph we are working on.
	GraphType &graph;

	// This is the random seed.
	const uint32_t randomSeed;

	// The resolution for integer probabilities using the hash function.
	const uint32_t resolution;
	
	// The degrees of each vertex.
	vector<ArcIdType> indeg;

	// The binary probability.
	uint32_t binprob;

	// The trivalency probabilities.
	const array<uint32_t, 3> triprob;

	// These are the sketches for each vertex.
	vector<vector<uint64_t>> sketches;

	// This holds search spaces for BFSes.
	DataStructures::Container::FastSet<uint32_t> searchSpace;

	// These are BFS levels.
	vector<uint32_t> levels;

	// A random number generator.
	mt19937 twisty;

	// A full distribution.
	uniform_int_distribution<uint32_t> fullDist;

	// Timing.
	double preprocessingElapsedMilliseconds;

	// Statistics.
	uint64_t sketchSize;

	// Verbosity.
	const bool verbose;

};


template<>
inline bool FastRSInfluenceOracle::Contained<FastRSInfluenceOracle::WEIGHTED>(const uint32_t u, const uint32_t v, const uint16_t i, const uint16_t l) {
	uint32_t prob = min(resolution, resolution / indeg[v]);
	return (Murmur3Hash(u, v, i, l) % resolution) < prob;
}
template<>
inline bool FastRSInfluenceOracle::Contained<FastRSInfluenceOracle::BINARY>(const uint32_t u, const uint32_t v, const uint16_t i, const uint16_t l) {
	return (Murmur3Hash(u, v, i, l) % resolution) < binprob;
}
template<>
inline bool FastRSInfluenceOracle::Contained<FastRSInfluenceOracle::TRIVALENCY>(const uint32_t u, const uint32_t v, const uint16_t i, const uint16_t l) {
	const uint32_t index = (Murmur3Hash(u, v, i, l) % triprob.size());
	return (Murmur3Hash(u, v, i, l) % resolution) < triprob[i];
}

}
}