/*
Algorithm for Influence Estimation and Maximization

Copyright (c) Microsoft Corporation

All rights reserved.

MIT License

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the ""Software""), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED *AS IS*, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <iostream>
#include <string>

using namespace std;

#ifdef __CYGWIN__
#define WINVER 0x0602
#define _WIN32_WINNT 0x0602
#endif

#include "FastStaticGraphs.h"
#include "Multicore.h"
#include "MetisGraphBuilder.h"
#include "DimacsGraphBuilder.h"
#include "CommandLineParser.h"
#include "RSInfluenceOracle.h"

void Usage(const string name) {
	cout << name << " -i <graph> [options]" << endl
		<< endl
		<< "Options:" << endl
		<< " -type <str>  -- type of input from {metis, dimacs, bin} (default: metis)." << endl
		<< " -undir       -- treat the input as an undirected graph." << endl
		<< " -nopar       -- remove parallel arcs in input." << endl
		<< " -trans       -- transpose the input (reverse graph)." << endl
		<< endl
		<< " -m <model>   -- IC model used (\"binary\", \"trivalency\", \"weighted\"; default: \"weighted\")." << endl
		<< " -p <double>  -- probability with which an arc is in the graph (binary model)." << endl
		<< endl
//		<< " -a           -- this flag indicates to compute influence of all vertices." << endl
		<< " -N <int>     -- sizes of random seed sets (default: 1-50)." << endl
		<< " -g <string>  -- method to generate seed sets (\"neigh\", \"uni\"; default: \"uni\")." << endl
		<< " -n <int>     -- number of random queries (default: 100)." << endl
		<< " -k <int>     -- the k-value from the reachability sketches (default: 64)." << endl
		<< " -l <int>     -- number of instances in the ic model (default: 64)." << endl
		<< " -leval <int> -- number of instances in the ic model for evaluation (default: same as -l)." << endl
		<< " -seed <int>  -- seed for random number generator (default: 31101982)." << endl
		<< " -os <string> -- filename to output statistics to." << endl
		<< " -v           -- omit output to console." << endl;
	exit(0);
}


template<Algorithms::InfluenceMaximization::FastRSInfluenceOracle::ModelType modelType>
inline void RunQueries(const Tools::CommandLineParser &clp) {
	// Read first batch of parameters.
	const string graphFilename = clp.Value<string>("i");
	const string graphType = clp.Value<string>("type", "metis");
	const uint16_t k = clp.Value<uint16_t>("k", 64);
	const uint16_t l = clp.Value<uint16_t>("l", 64);
	const uint32_t s = clp.Value<uint32_t>("seed", 31101982);
	const string statsFilename = clp.Value<string>("os");
	const bool verbose = !clp.IsSet("v");

	// Load the graph.
	DataStructures::Graphs::FastUnweightedGraph graph;
	if (graphType == "metis")
		RawData::BuildMetisGraph(graphFilename, graph, true, clp.IsSet("trans"), !clp.IsSet("undir"), true, clp.IsSet("nopar"), verbose);
	else if (graphType == "dimacs")
		RawData::BuildDimacsGraph(graphFilename, graph, true, clp.IsSet("trans"), !clp.IsSet("undir"), true, clp.IsSet("nopar"), verbose);
	else if (graphType == "bin")
		graph.Read(graphFilename, true, verbose);
	else
		Usage(clp.ExecutableName());

	// Create the algorithm.
	Algorithms::InfluenceMaximization::FastRSInfluenceOracle oracle(graph, s, verbose);

	// Set the binary probability.
	oracle.SetBinaryProbability(clp.Value<double>("p", 0.1));
	
	// Run preprocessing of the oracle
	oracle.RunPreprocessing<modelType>(k, l);

	// Run random queries?
	if (!clp.IsSet("a")) {
		const uint16_t lEval = clp.Value<uint16_t>("leval", l);
		const int32_t n = clp.Value<int32_t>("n", 100);
		const string N = clp.Value<string>("N", "1-50");
		Algorithms::InfluenceMaximization::FastRSInfluenceOracle::SeedMethodType m(Algorithms::InfluenceMaximization::FastRSInfluenceOracle::UNIFORM);

		// Which method to use for generating random queries?
		const string methodString = clp.Value<string>("g", "uni");
		if (methodString == "neigh")
			m = Algorithms::InfluenceMaximization::FastRSInfluenceOracle::NEIGHBORHOOD;

		// Run random queries.
		oracle.Run<modelType>(N, m, n, k, l, lEval, statsFilename);
	}

	// Run queries for every vertex and just estimate influence for every vertex.
	else {
		Tools::FancyProgressBar bar(graph.NumVertices(), "Running queries");
		vector<uint32_t> S(1, 0);
		vector<double> influence(graph.NumVertices(), 0.0);
		FORALL_VERTICES(graph, vertexId) {
			S[0] = vertexId;
			influence[vertexId] = oracle.RunSpecificQuery(S, k, l);
			++bar;
		}
		if (!statsFilename.empty()) {
			ofstream file(statsFilename);
			if (file.is_open()) {
				stringstream ss;
				FORALL_VERTICES(graph, vertexId) {
					ss << vertexId << "\t" << influence[vertexId] << endl;
				}
				file << ss.str();
				file.close();
			} 
		}
	}
}




int main(int argc, char **argv) {

	Tools::CommandLineParser clp(argc, argv);
	if (!clp.IsSet("i")) Usage(argv[0]);
	const string modelStr = clp.Value<string>("m", "weighted");

	// Attach to numa node, if requested.
	if (clp.IsSet("numa")) {
		cout << "Setting affinity mask of this process to " << Platform::GetAffinityMaskForNumaNode(clp.Value<uint32_t>("numa")) << "... " << flush;
		Platform::PinProcessToNumaNode(clp.Value<uint32_t>("numa"));
		cout << "done." << endl;
	}

	// Determine IC model and run algorithm.
	if (modelStr == "binary") {
		RunQueries<Algorithms::InfluenceMaximization::FastRSInfluenceOracle::BINARY>(clp);
	}
	if (modelStr == "trivalency") {
		RunQueries<Algorithms::InfluenceMaximization::FastRSInfluenceOracle::TRIVALENCY>(clp);
	}
	else {
		RunQueries<Algorithms::InfluenceMaximization::FastRSInfluenceOracle::WEIGHTED>(clp);
	}

	return 0;
}