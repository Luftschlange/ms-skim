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
#include "SKIM.h"

void Usage(const string name) {
	cout << name << " -i <graph> [options]" << endl
		<< endl
		<< "Options:" << endl
		<< " -type <str>  -- type of input from {metis, dimacs, bin} (default: metis)." << endl
		<< " -undir       -- treat the input as an undirected graph." << endl
		<< " -nopar       -- remove parallel arcs in input." << endl
		<< " -trans       -- transpose the input (reverse graph)." << endl
		<< endl
		<< " -m <string>  -- IC model used (binary, trivalency, weighted; default: weighted)." << endl
		<< " -p <double>  -- probability with which an arc is in the graph (binary model)." << endl
		<< endl
		<< " -N <int>     -- size of seed set to compute (default: graph size)." << endl
		<< " -k <int>     -- the k-value from the reachability sketches (default: 64)." << endl
		<< " -l <int>     -- number of instances in the ic model (default: 64)." << endl
		<< " -leval <int> -- the number of instances to evaluate exact influence on (0 = off; default)." << endl
		<< endl
		<< " -t <int>     -- number of threads (default: 1)." << endl
		<< " -numa <int>  -- pinned NUMA node to run on (default: any and all)." << endl
		<< " -seed <int>  -- seed for random number generator (default: 31101982)." << endl
		<< " -os <string> -- filename to output statistics to." << endl
		<< " -oc <string> -- filename to output detailed coverage information to." << endl
		<< " -v           -- omit output to console." << endl;
	exit(0);
}

int main(int argc, char **argv) {

	Tools::CommandLineParser clp(argc, argv);
	
	// Read parameters from the command line.
	const string graphFilename = clp.Value<string>("i");
	const string graphType = clp.Value<string>("type", "metis");
	const uint16_t k = clp.Value<uint16_t>("k", 64);
	const uint16_t l = clp.Value<uint16_t>("l", 64);
	const uint16_t lEval = clp.Value<uint16_t>("leval", 0);
	const bool verbose = !clp.IsSet("v");
	const uint32_t s = clp.Value<uint32_t>("seed", 31101982);
	const string statsFilename = clp.Value<string>("os");
	const string coverageFilename = clp.Value<string>("oc");
	const int32_t numt = clp.Value<int32_t>("t", 1);
	const string modelStr = clp.Value<string>("m", "weighted");

	if (graphFilename.empty()) Usage(clp.ExecutableName());

	if (clp.IsSet("numa")) {
		cout << "Setting affinity mask of this process to " << Platform::GetAffinityMaskForNumaNode(clp.Value<uint32_t>("numa")) << "... " << flush;
		Platform::PinProcessToNumaNode(clp.Value<uint32_t>("numa"));
		cout << "done." << endl;
	}
	
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

	const uint32_t N = clp.Value<uint32_t>("N", 0);

	// Create the algorithm.
	Algorithms::InfluenceMaximization::SKIM skim(graph, s, verbose);

	// Determine IC model and run algorithm.
	if (modelStr == "binary")  {
		skim.SetBinaryProbability(clp.Value<double>("p", 0.1));
		skim.Run<Algorithms::InfluenceMaximization::SKIM::BINARY>(N, k, l, lEval, numt, statsFilename, coverageFilename);
	}
	if (modelStr == "trivalency")
		skim.Run<Algorithms::InfluenceMaximization::SKIM::TRIVALENCY>(N, k, l, lEval, numt, statsFilename, coverageFilename);
	if (modelStr == "weighted")
		skim.Run<Algorithms::InfluenceMaximization::SKIM::WEIGHTED>(N, k, l, lEval, numt, statsFilename, coverageFilename);

	return 0;
}