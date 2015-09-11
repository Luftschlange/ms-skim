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
#include <vector>
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <random>
#include <climits>
#include <omp.h>
using namespace std;

#include "FastStaticGraphs.h"
#include "Macros.h"
#include "FastSet.h"
#include "HashPair.h"
#include "Timer.h"
#include "KHeap.h"

namespace Algorithms{
namespace InfluenceMaximization {


class SKIM {
public:

	// Type definitions.
	typedef DataStructures::Graphs::FastUnweightedGraph GraphType;
	typedef GraphType::ArcIdType ArcIdType;
	static const uint32_t NullVertex = UINT_MAX;

	enum ModelType { WEIGHTED, BINARY, TRIVALENCY };

	// A seed vertex and associated data.
	struct SeedType {
		uint32_t VertexId = NullVertex;
		double EstimatedInfluence = 0.0;
		double ExactInfluence = 0.0;
		double BuildSketchesElapsedMilliseconds = 0;
		double ComputeInfluenceElapsedMilliseconds = 0;
	};

	// Default constructor.
	SKIM(GraphType &g, const uint32_t s, const bool v) :
		verbose(v),
		randomSeed(s),
		graph(g),
		resolution(3000000),
		indeg(graph.NumVertices(), 0),
		binprob(resolution / 10),
		triprob{ { resolution / 10, resolution / 100, resolution / 1000 } }//,
		//dis(0, resolution-1),
		//gen(s)
	{
		if (verbose) cout << "Computing in-degrees... " << flush;
		// Compute the degrees.
		FORALL_ARCS(graph, vertexId, arc) {
			if (!arc->Forward()) continue;
			++indeg[arc->OtherVertexId()];
		}
		if (verbose) cout << "done." << endl;
	}

	// Set the binary probability.
	inline void SetBinaryProbability(const double prob) {
		binprob = uint32_t(prob * double(resolution));
	}

	// Run.
	template<ModelType modelType>
	inline void Run(uint32_t N, const uint16_t k, const uint16_t l, const uint16_t lEval, const int32_t numt, const string statsFilename = "", const string coverageFilename = "") {
		// Set N to number of vertices, if it's zero.
		if (N == 0) N = static_cast<uint32_t>(graph.NumVertices());
		/*
		Initialize the algorithm.
		*/
		if (verbose) cout << "Setting up data structures... " << flush;
		// Some datastructures that are necessary for the algorithm.
		const uint64_t nl = graph.NumVertices()*l;
		vector<SeedType> seedSet; // this will hold the seed vertices.
		vector<uint32_t> permutation; // this is a permutation of the vertices to draw ranks from.
		unordered_map< pair<uint32_t, uint16_t>, vector<uint32_t> > invSketches; // these are the "inverse sketches" (search spaces).
		vector<uint16_t> sketchSizes(graph.NumVertices(), 0); // these are the sizes of the real sketches.
		vector<vector<bool>> covered(l); // this indicates whether a vertex/instance pair has been covered (influenced).
		vector<vector<bool>> processed(l); // this indicates whether a vertex/instance pair has been processed (sketches built from it).
		vector<DataStructures::Container::FastSet<uint32_t>> searchSpaces(numt); // this is for maintaining search spaces of BFSes; one per thread.
		DataStructures::Container::FastSet<uint32_t> &S0 = searchSpaces[0];
		vector<vector<pair<uint32_t, uint16_t>>> updateQueues(numt);
		vector<vector<uint32_t>> buck;
		vector<uint32_t> buckind;
		uint16_t buckp(0);
		mt19937_64 rnd(randomSeed); // Random number generator.
		uniform_int_distribution<uint16_t> distr(0, l - 1);
		uint64_t rank(0); // this is the current rank value.
		Platform::Timer timer, globalTimer;
		double estinf(0), exinf(0), exinfloc(0), sketchms(0), infms(0);
		bool runParallel(numt > 1), saturated(false);
		uint32_t numperm(0), permthresh(l - (l / 10 + 1));

		for (int32_t t = 0; t < numt; ++t)
			searchSpaces[t].Resize(graph.NumVertices());
		for (uint16_t i(0); i < l; ++i) {
			processed[i].resize(graph.NumVertices(), false);
			covered[i].resize(graph.NumVertices(), false);
		}
		if (verbose) cout << "done." << endl;

		/*
		Main iterations loop. Each iteration computes one seed vertex.
		*/
		globalTimer.Start();
		while (seedSet.size() < N) {
			SeedType newSeed;
			exinfloc = 0.0;

			/*
			BFS computation to build sketches.
			*/
			if (!saturated) {
				if (verbose) cout << "[" << seedSet.size() + 1 << "] Computing sketches from rank " << rank << "... " << flush;
				timer.Start();
				while (rank < nl) {
					// Select next vertex/instance pair.
					const Types::SizeType vi = rank % graph.NumVertices();
					if (vi == 0)	{
						if (permutation.size() != graph.NumVertices()) {
							permutation.resize(graph.NumVertices(), 0);
							for (uint32_t u(0); u < graph.NumVertices(); ++u) permutation[u] = u;
						}
						shuffle(permutation.begin(), permutation.end(), rnd);
						++numperm;
					}
					const uint32_t sourceVertexId = permutation[vi];
					uint16_t i = 0;
					if (numperm < permthresh) {
						do {
							i = distr(rnd);
						} while (processed[i][sourceVertexId]);
					}
					else {
						i = distr(rnd) % (l - numperm + 1);
						for (uint16_t j = 0; j < l; ++j) {
							if (!processed[j][sourceVertexId]) {
								if (i == 0) {
									i = j;
									break;
								}
								--i;
							}
						}
					}
					processed[i][sourceVertexId] = true;

					++rank; // Increase value for rank.

					// Shortcut to some variables.
					vector<bool> &cov = covered[i];
					vector<uint32_t> &invSketch = invSketches[make_pair(sourceVertexId, i)];

					// Only process such ranks that are not yet covered.
					if (cov[sourceVertexId]) continue;

					// Perform the BFS.
					S0.Clear();
					S0.Insert(sourceVertexId);
					uint32_t ind = 0;
					while (ind < S0.Size()) {
						uint32_t u = S0.KeyByIndex(ind++);
						++sketchSizes[u];
						invSketch.push_back(u);

						// pruning.
						if (sketchSizes[u] == k) {
							// Set the vertex and compute marginal influence.
							newSeed.VertexId = u;
							newSeed.EstimatedInfluence = static_cast<double>(k - 1) * static_cast<double>(graph.NumVertices()) / static_cast<double>(rank);
							break;
						}

						// arc expansion.
						FORALL_INCIDENT_ARCS_BACKWARD(graph, u, a) {
							if (!a->Backward()) break;
							const uint32_t v = a->OtherVertexId();
							if (Contained<modelType>(v, u, i, l) && !cov[v] && !S0.IsContained(v))
							//if (ContainedRandom<modelType>(v, u) && !cov[v] && !S0.IsContained(v))
								S0.Insert(v);
						}
					}
					if (newSeed.VertexId != NullVertex)
						break;
				} // end sketch building.
				sketchms += timer.LiveElapsedMilliseconds();
				newSeed.BuildSketchesElapsedMilliseconds = sketchms;
				if (verbose) cout << " done (u: " << newSeed.VertexId << ", est: " << newSeed.EstimatedInfluence << " r: " << rank << ", ms: " << newSeed.BuildSketchesElapsedMilliseconds << ")" << endl;

				// Out of new vertices...
				if (newSeed.VertexId == NullVertex) {
					if (verbose) cout << "GRAPH SATURATED (|S|=" << seedSet.size() << ", rank=" << rank << ")." << endl;
					if (verbose) cout << "Building buckets for the remaining vertices... " << flush;
					buck.resize(k);
					buckind.resize(graph.NumVertices(), 0);
					uint32_t num(0);
					FORALL_VERTICES(graph, u) {
						if (sketchSizes[u] > 0) {
							buckind[u] = uint32_t(buck[sketchSizes[u]].size());
							buck[sketchSizes[u]].push_back(u);
							buckp = max<uint16_t>(buckp, sketchSizes[u]);
							++num;
						}
					}
					if (verbose) cout << "done (" << num << " vertices)." << endl;
					saturated = true;
				}
			}
			if (saturated) {
				while (buckp > 0 && buck[buckp].empty()) --buckp;
				if (buckp == 0) {
					if (verbose) cout << endl << "TOTAL COVERAGE REACHED (|S|=" << seedSet.size() << ")." << endl;
					break;
				}
				// Select the next seed vertex as the one that has the highest number of things in the sketch.
				if (verbose) cout << "[" << seedSet.size() + 1 << "] Determining the vertex that has highest marginal influence... " << flush;
				Assert(!buck[buckp].empty());
				newSeed.VertexId = buck[buckp].back();
				newSeed.EstimatedInfluence = double(sketchSizes[newSeed.VertexId]) / l;
				newSeed.BuildSketchesElapsedMilliseconds = sketchms;
				if (verbose) cout << " done (u: " << newSeed.VertexId << ", est: " << newSeed.EstimatedInfluence << ")" << endl;
			}


			/*
			BFS computation on each instance to get the exact influence.
			Also updates the sketch sizes.
			*/
			if (verbose) cout << "[" << seedSet.size() + 1 << "] Computing influence... " << flush;
			timer.Start();

			// Call sequential or parallel BFS to compute influences.
			if (runParallel) {
#pragma omp parallel num_threads(numt) reduction(+ : exinfloc)
				{
					// Get thread id.
					const int32_t t = omp_get_thread_num();
					// Shortcut to thread-local search spaces.
					auto &S = searchSpaces[t];
					auto &Q = updateQueues[t];
					Q.clear();

#pragma omp for
					for (int32_t i = 0; i < l; ++i) {
						// Shortcut to some variables.
						vector<bool> &cov = covered[i];

						// Run a BFS.
						S.Clear();
						if (!cov[newSeed.VertexId])
							S.Insert(newSeed.VertexId);
						uint32_t ind = 0;
						while (ind < S.Size()) {
							uint32_t u = S.KeyByIndex(ind++);
							cov[u] = true;
							++exinfloc;

							// Update counters and sketches.
							const pair<uint32_t, uint16_t> key(u, i);
							if (invSketches.count(key)) {
								Q.push_back(key);
							}

							FORALL_INCIDENT_ARCS(graph, u, a) {
								if (!a->Forward()) break;
								const uint32_t v = a->OtherVertexId();
								if (Contained<modelType>(u, v, i, l) && !S.IsContained(v) && !cov[v])
									S.Insert(v);
							}
						}
					} // end exact influence computation.
				} // end parallel section.

				// Update the counters.
				for (int32_t t = 0; t < numt; ++t) {
					vector<pair<uint32_t, uint16_t>> &Q = updateQueues[t];
					for (const pair<uint32_t, uint16_t> &key : Q) {
						const vector<uint32_t> &invSketch = invSketches[key];
						if (!saturated) {
							for (const uint32_t &v : invSketch)
								--sketchSizes[v];
						}
						else {
							for (const uint32_t &v : invSketch) {
								uint16_t s = sketchSizes[v];
								// Erase from bucket.
								buckind[buck[s].back()] = buckind[v];
								swap(buck[s][buckind[v]], buck[s].back());
								buck[s].pop_back();
								if (sketchSizes[v] > 1) {
									buckind[v] = uint32_t(buck[sketchSizes[v] - 1].size());
									buck[sketchSizes[v] - 1].push_back(v);
								}
								--sketchSizes[v];
							}
						}
						invSketches.erase(key);
					}
				}
			} // end parallel branch.
			else { // begin sequential branch.
				for (int32_t i = 0; i < l; ++i) {
					// Shortcut to some variables.
					vector<bool> &cov = covered[i];

					// Run a BFS.
					S0.Clear();
					if (!cov[newSeed.VertexId])
						S0.Insert(newSeed.VertexId);
					uint32_t ind = 0;
					while (ind < S0.Size()) {
						uint32_t u = S0.KeyByIndex(ind++);
						cov[u] = true;
						++exinfloc;

						// Update counters and sketches.
						const pair<uint32_t, uint16_t> key(u, i);
						if (invSketches.count(key)) {
							const vector<uint32_t> &invSketch = invSketches[key];
							if (!saturated) {
								for (const uint32_t &v : invSketch)
									--sketchSizes[v];
							}
							else {
								for (const uint32_t &v : invSketch) {
									uint16_t s = sketchSizes[v];
									// Erase from bucket.
									buckind[buck[s].back()] = buckind[v];
									swap(buck[s][buckind[v]], buck[s].back());
									buck[s].pop_back();
									if (sketchSizes[v] > 1) {
										buckind[v] = uint32_t(buck[sketchSizes[v] - 1].size());
										buck[sketchSizes[v] - 1].push_back(v);
									}
									--sketchSizes[v];
								}
							}
							invSketches.erase(key);
						}

						FORALL_INCIDENT_ARCS(graph, u, a) {
							if (!a->Forward()) break;
							const uint32_t v = a->OtherVertexId();
							if (Contained<modelType>(u, v, i, l) && !S0.IsContained(v) && !cov[v])
								S0.Insert(v);
						}
					}
				} // end exact influence computation.
			} // end sequential branch.

			newSeed.ExactInfluence = double(exinfloc) / double(l);
			infms += timer.LiveElapsedMilliseconds();
			newSeed.ComputeInfluenceElapsedMilliseconds = infms;
			estinf += newSeed.EstimatedInfluence;
			exinf += newSeed.ExactInfluence;
			seedSet.push_back(newSeed);
			if (verbose) cout << " done (inf: " << newSeed.ExactInfluence << ", ms: " << newSeed.ComputeInfluenceElapsedMilliseconds << ")." << endl;
			if (verbose) cout << endl;

		} // end greedy iteration.
		const double totalms = globalTimer.LiveElapsedMilliseconds();

		// Compute the exact influence? This is not measured in the running time.
		if (lEval != 0)
			exinf = ComputeExactInfluence<modelType>(seedSet, lEval);

		/*
		Print results.
		*/
		if (verbose) cout << endl;
		graph.DumpStatistics(cout);
		cout << "Random seed: " << randomSeed << "." << endl
			<< "Number of seed vertices computed: " << seedSet.size() << "." << endl
			<< "Number of ranks used: " << rank << "." << endl
			<< "Permutations computed: " << numperm << " (each of size: " << permutation.size() << ")." << endl
			<< "Building sketches: " << sketchms / 1000.0 << " sec." << endl
			<< "Computing influence: " << infms / 1000.0 << " sec." << endl
			<< "Total time: " << totalms / 1000.0 << " sec." << endl
			<< "Estimated spread of solution: " << estinf << " (" << (100.0*estinf / static_cast<double>(graph.NumVertices())) <<  " %)." << endl
			<< "Exact spread of solution: " << exinf << " (" << (100.0*exinf / static_cast<double>(graph.NumVertices())) << " %)." << endl
			<< "Quality gap: " << 100.0 * (1.0 - exinf / estinf) << " %" << endl;


		/*
		Dump statistics to a file.
		*/
		if (!statsFilename.empty()) {
			IO::FileStream file;
			file.OpenNewForWriting(statsFilename);
			if (file.IsOpen()) {
				stringstream ss;
				ss << "NumberOfVertices = " << graph.NumVertices() << endl
					<< "NumberOfArcs = " << graph.NumArcs() / 2 << endl
					<< "TotalEstimatedInfluence = " << estinf << endl
					<< "TotalExactInfluence = " << exinf << endl
					<< "TotalElapsedMilliseconds = " << totalms << endl
					<< "SketchBuildingElapsedMilliseconds = " << sketchms << endl
					<< "InfluenceComputationElapsedMilliseconds = " << infms << endl
					<< "NumberOfRanksUsed = " << rank << endl
					<< "NumberOfSeedVertices = " << seedSet.size() << endl
					<< "RankComputationMethod = " << "shuffle" << endl
					<< "NumberOfPermutationsComputed = " << numperm << endl;
				double sumEstimatedInfluence(0.0), sumExactInfluence(0.0);
				for (Types::IndexType i = 0; i < seedSet.size(); ++i) {
					sumEstimatedInfluence += seedSet[i].EstimatedInfluence;
					sumExactInfluence += seedSet[i].ExactInfluence;
					ss << i << "_MarginalEstimatedInfluence = " << seedSet[i].EstimatedInfluence << endl
						<< i << "_CumulativeEstimatedInfluence = " << sumEstimatedInfluence << endl
						<< i << "_MarginalExactInfluence = " << seedSet[i].ExactInfluence << endl
						<< i << "_CumulativeExactInfluence = " << sumExactInfluence << endl
						<< i << "_VertexId = " << seedSet[i].VertexId << endl
						<< i << "_TotalElapsedMilliseconds = " << seedSet[i].BuildSketchesElapsedMilliseconds + seedSet[i].ComputeInfluenceElapsedMilliseconds << endl
						<< i << "_SketchBuildingElapsedMilliseconds = " << seedSet[i].BuildSketchesElapsedMilliseconds << endl
						<< i << "_InfluenceComputationElapsedMilliseconds = " << seedSet[i].ComputeInfluenceElapsedMilliseconds << endl;
				}
				file.WriteString(ss.str());
			}
		}

		if (!coverageFilename.empty()) {
			IO::FileStream file;
			file.OpenNewForWriting(coverageFilename);
			if (file.IsOpen()) {
				stringstream ss;
				ss << graph.NumVertices() << endl;
				ss << seedSet.size() << endl;
				ss << seedSet.back().ComputeInfluenceElapsedMilliseconds + seedSet.back().BuildSketchesElapsedMilliseconds << endl;
				double sumExactInfluence(0.0);
				double elapsedMilliseconds(0.0);
				for (Types::IndexType i = 0; i < seedSet.size(); ++i) {
					sumExactInfluence += seedSet[i].ExactInfluence;
					elapsedMilliseconds = seedSet[i].BuildSketchesElapsedMilliseconds + seedSet[i].ComputeInfluenceElapsedMilliseconds;
					ss << seedSet[i].VertexId << "\t" << sumExactInfluence << "\t" << elapsedMilliseconds << endl;
				}
				file.WriteString(ss.str());
			}
		}
	}



protected:

	// This evaluates the influence using a separate BFS with a separate seed.
	template<ModelType modelType>
	inline double ComputeExactInfluence(vector<SeedType> &seedSet, const uint16_t l) {
		// This essentially runs a bunch of BFSes in all l instances, one from each
		// seed vertex. It then updates the exact influence value for the respective
		// seed.
		if (verbose) cout << "Allocating data structures... " << flush;
		DataStructures::Container::FastSet<uint32_t> searchSpace(graph.NumVertices());
		vector<vector<bool>> marked(l);
		for (uint16_t i = 0; i < l; ++i)
			marked[i].resize(graph.NumVertices(), false);
		if (verbose) cout << "done." << endl;

		// For each seed vertex, perform a BFS in every instance, and count the sarch space sizes.
		if (verbose) cout << "Running BFSes to compute exact influence in " << l << " instances and " << seedSet.size() << " vertices:" << flush;
		double exinf(0);
		for (SeedType &s : seedSet) {
			uint64_t size = 0;
			for (uint16_t i = 0; i < l; ++i) {
				vector<bool> &m = marked[i];
				if (m[s.VertexId]) continue;
				searchSpace.Clear();
				searchSpace.Insert(s.VertexId);
				uint64_t cur = 0;
				while (cur < searchSpace.Size()) {
					const uint32_t u = searchSpace.KeyByIndex(cur++);
					m[u] = true;
					++size;
					FORALL_INCIDENT_ARCS(graph, u, arc) {
						if (!arc->Forward()) continue;
						const uint32_t v = arc->OtherVertexId();
						if (Contained<modelType>(u, v, i, l) && !m[v] && !searchSpace.IsContained(v))
							searchSpace.Insert(v);
					}
				}
			}
			s.ExactInfluence = double(size) / double(l);
			exinf += s.ExactInfluence;
			if (verbose) cout << " " << s.ExactInfluence << flush;
		}
		if (verbose) cout << endl << "done (exinf=" << exinf << ")." << endl;
		return exinf;
	}

protected:

	// Returns true if the (forward) arc from u to v is contained in instance i.
	template<ModelType modelType>
	inline bool Contained(const uint32_t u, const uint32_t v, const uint16_t i, const uint16_t l) {
		assert(false);
		return false;
	}


	//// Returns true if the (forward) arc from u to v is contained in instance i.
	//template<ModelType modelType>
	//inline bool ContainedRandom(const uint32_t u, const uint32_t v) {
	//	assert(false);
	//	return false;
	//}


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

	// Indicates whether the algorithm procuces output.
	bool verbose = true;

	// This is the random seed.
	uint32_t randomSeed;

	// This is the graph we are using
	GraphType &graph;

	// The resolution for integer probabilities using the hash function.
	const uint32_t resolution;

	// The degrees of each vertex.
	vector<ArcIdType> indeg;

	// The binary probability.
	uint32_t binprob;

	// The trivalency probabilities.
	const array<uint32_t, 3> triprob;

	// Random distribution.
	//uniform_int_distribution<uint32_t> dis;

	// Random number generator.
	//mt19937 gen;
};


template<>
inline bool SKIM::Contained<SKIM::WEIGHTED>(const uint32_t u, const uint32_t v, const uint16_t i, const uint16_t l) {
	uint32_t prob = min(resolution, resolution / indeg[v]);
	return (Murmur3Hash(u, v, i, l) % resolution) < prob;
}
template<>
inline bool SKIM::Contained<SKIM::BINARY>(const uint32_t u, const uint32_t v, const uint16_t i, const uint16_t l) {
	return (Murmur3Hash(u, v, i, l) % resolution) < binprob;
}
template<>
inline bool SKIM::Contained<SKIM::TRIVALENCY>(const uint32_t u, const uint32_t v, const uint16_t i, const uint16_t l) {
	const uint32_t index = (Murmur3Hash(u, v, i, l) % triprob.size());
	return (Murmur3Hash(u, v, i, l) % resolution) < triprob[index];
}

}
}