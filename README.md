# Influence Computation and Maximization
{:.no_toc}

This project constitutes the original C++ implementation of the influence oracle and influence maximization algorithms, which were developed in the paper _[Sketch-Based Influence Computation and Maximization](http://dl.acm.org/citation.cfm?id=2662077)_ by [Edith Cohen](http://www.cohenwang.com/edith/), [Daniel Delling](http://danieldelling.com/), [Thomas Pajor](http://tpajor.com/), and [Renato Werneck](http://www.cs.princeton.edu/~rwerneck/). The paper was presented at [CIKM 2014](http://cikm2014.fudan.edu.cn/) in Shanghai, China.

The code was developed at [Microsoft Research](http://research.microsoft.com) and has been released under MIT license.

## Table of Contents
{:.no_toc}

* TOC
{:toc}

## Citing the Algorithm

If you wish to use or compare to this algorithm in your work, please cite it using the following BibTeX entry:

	@inproceedings{cdpw-simcs-14,
		acmid = {2662077},
		address = {New York, NY, USA},
		author = {Cohen, Edith and Delling, Daniel and Pajor, Thomas and Werneck, Renato F.},
		booktitle = {Proceedings of the 23rd {ACM} International Conference on Information and Knowledge Management},
		doi = {10.1145/2661829.2662077},
		isbn = {978-1-4503-2598-1},
		keywords = {algorithms, estimation, experimentation, theory},
		location = {Shanghai, China},
		numpages = {10},
		pages = {629--638},
		publisher = {ACM},
		series = {CIKM '14},
		title = {Sketch-based Influence Maximization and Computation: Scaling Up with Guarantees},
		url = {http://doi.acm.org/10.1145/2661829.2662077},
		year = {2014},
	}

## Compilation

The algorithm is provided as C++ source code, distributed under the MIT license.

Except for Make and GCC there are no dependencies, as the algorithm is implemented with custom data structures, solely using the standard template library.

Download (and possibly unpack) the source code to an arbitrary location, open a terminal window, and navigate to the location you put the code to. From there simply call:

	make

This will produce two binary files: `RunSKIM` and `RunInfluenceOracle` inside the `bin` subdirectory of the project. The first runs the influence maximization algorithm, while the latter runs the influence oracle.

## Solving the Influence Maximization Problem

To solve the Influence Maximization Problem, you need to run `RunSKIM`. This program runs the sketch-based influence maximization algorithm _SKIM_. You call it from the command line by issuing the command

	bin/RunSKIM -i <graph> [options]
	
Note that running `bin/RunSKIM` without any parameters will print a list of available parameters and their options to the console. The following section explains each parameter in turn.

### Parameters

Parameters are grouped by their purpose.

#### Input Parameters

- `-i <string>`: The input graph filename. This parameter is mandatory.
- `-type <string>`: Type of input from. Can be either `metis`, `dimacs` or `bin` (binary). Default: `metis`. This parameter is mandatory.
- `-undir`: Treat the input as an undirected graph.
- `-nopar`: Remove parallel arcs in input.
- `-trans`: Transpose the input (to obtain the reverse graph).

#### IC Model Parameters

- `-m <string>`: IC model to use (`binary`, `trivalency`, `weighted`; default: `weighted`).
- `-p <double>`: Probability with which an arc is in the graph (has an effect for the `binary` model only).

#### Algorithm Parameters

- `-N <int>`: Size of seed set to compute. If not set or set to `0` it defaults to the graph size (number of vertices).
- `-k <int>`: The _k_-value from the reachability sketches (default: `64`).
- `-l <int>`: Number of instances in the IC model (default: `64`).
- `-leval <int>`: The number of (different) instances to evaluate the exact influence on (`0` = off; default). If this parameter is `0` or not specified, the exact influence is evaluated on the same (number of) instances as specified by the `-l` parameter.

#### Output and Miscellaneous Parameters

- `-os <string>`: Filename to output general statistics to (default: none).
- `-oc <string>`: Filename to output detailed coverage information to (default: none).

- `-t <int>`: Number of threads used (default: 1).
- `-numa <int>`: Preferred NUMA node to allocate memory and run the algorithm on (default: any and all).
- `-seed <int>`: Seed for random number generator (default: `31101982`).
- `-v`: Omit intermediate output to console. Only a short summary at the end will be output.


### Typical Call

A typical call to `RunSKIM` could look like this

	bin/RunSKIM -i mygraph.metis -type metis -k 64 -l 64 -N 1000 -leval 512 -os mygraph.imstats

This will run SKIM on `mygraph.metis`, which is of METIS format, using a _k_-value of 64 and 64 instances in the IC model. The algorithm will use 512 different simulation instances to evaluate the influence spread of the seed sets computed by SKIM. It is generally a good idea to set `-leval` higher than `-l` in order to evaluate the quality more accurately. In the original publication `-leval` has been set to 512. Statistical information, such as the running time and spread will be written to `mygraph.imstats`. 

### Statistics

The `RunSKIM` program can generate two types of statistics: general statistics specified via the `-os` parameter and detailed coverage statistics specified via the `-oc` parameter.

#### General Statistics

General statistics (when specified via the `-os` parameter) are written to the specified text file. Each line of the file contains a key-value pair of the form `key = value`. The following items are output.
	
- `NumberOfVertices` -- The number of vertices in the input graph.
- `NumberOfArcs` -- The number of (directed) arcs in the input graph.
- `TotalEstimatedInfluence` -- The influence of the entire seed set as obtained by the estimator of the SKIM algorithm.
- `TotalExactInfluence` -- The influence of the entire seed set as obtained by running explicit simulations on as many instances as specified by the `-leval` parameter.
- `TotalElapsedMilliseconds` -- The total amount of time spent running the algorithm in milliseconds.
- `SketchBuildingElapsedMilliseconds` -- The amount of time spent in the algorithm building sketches in milliseconds.
- `InfluenceComputationElapsedMilliseconds` -- The amount of time spent in the algorithm computing influence.
- `NumberOfRanksUsed` -- The number of vertex-instance pairs used for building sketches.
- `NumberOfSeedVertices` -- The number of vertices in the final seed set.

Furthermore, for each vertex that is added to the seed set, the file contains specific statistics about the current seed set. The keys are prepended with a zero-based integral number i that refers to the seed set after the i+1-th vertex has been added.

- `i_VertexId` -- The zero-based integral vertex id that has been added to the seed set.
- `i_MarginalEstimatedInfluence` -- The marginal influence of adding the i+1-th vertex to the seed set as obtained by the estimator of the SKIM algorithm.
- `i_CumulativeEstimatedInfluence` -- The influence of the entire seed set after adding the i+1-th vertex as obtained by the estimator of the SKIM algorithm.
- `i_MarginalExactInfluence` -- The marginal influence of adding the i+1-th vertex to the seed set as obtained by running explicit simulations on as many instances as specified by the `-leval` parameter.
- `i_CumulativeExactInfluence` -- The influence of the entire seed set after adding the i+1-th vertex as obtained by running explicit simulations on as many instances as specified by the `-leval` parameter.
- `i_TotalElapsedMilliseconds` -- The total amount of time spent running the algorithm up to the current point.
- `i_SketchBuildingElapsedMilliseconds` -- The amount of time spent in the algorithm building sketches up to the current point.
- `i_InfluenceComputationElapsedMilliseconds` -- The amount of time spent in the algorithm computing influence up to the current point.

Note that the running time for evaluating the influence on a different set of instances (if specified by `-leval`) is not measured.

#### Coverage Statistics

Progressive statistics about the running time and influence after each vertex that is added to the seed set can be output in a more concise form using the `-oc` parameter. This will produce a text file of the following form.

- The first line contains a single integer depicting the number of vertices in the graph.
- The second line contains a single integer depicting the number of vertices in the final seed set.
- The third line contains a single number depicting the total running time of the algorithm in milliseconds.
- Each subsequent line contains three numbers, separated by tab characters (\\t). The i-th of these lines refers to the seed set after the i-th vertex has been added.
  - The first number in each line depicts the zero-based integral id of the vertex added in the current iteration.
  - The second number depicts the influence of the entire seed set computed up to this point as obtained by running explicit simulations.
  - The third number depicts the total amount of time spent running the algorithm up to this point.

The coverage statistics file (on the [Epinions](https://snap.stanford.edu/data/soc-Epinions1.html) social network graph with 64 instances, _k_-value 64 and the weighted IC model) for computing a seed set of size 5 could look like this:

	75888
	5
	144.564
	763	1350.66	51.0681
	645	2322.52	90.447
	634	3073.89	111.939
	5232	3722.3	132.183
	1835	4134.41	144.564


## Solving the Influence Estimation Problem

To run experiments on the influence estimation algorithm, you need to run `RunInfluenceOracle`. This will first compute the sketches of the graph in a preprocessing step and then evaluate the influence estimator on these sketches. You call the algorithm from the command line by

	bin/RunInfluenceOracle -i <graph> -t <type> [options]

Calling `RunInfluenceOracle` without any parameters will print a list of available parameters to the console. They are explained in the following. Note that many of the parameters are the same as for `RunSKIM`. For completeness they are repeated below, however.

### Parameters

Parameters are grouped by their purpose.

#### Input Parameters

These parameters are identical to `RunSKIM`.

- `-i <string>`: The input graph filename. This parameter is mandatory.
- `-type <string>`: Type of input from. Can be either `metis`, `dimacs` or `bin` (binary). Default: `metis`. This parameter is mandatory.
- `-undir`: Treat the input as an undirected graph.
- `-nopar`: Remove parallel arcs in input.
- `-trans`: Transpose the input (to obtain the reverse graph).

#### IC Model Parameters

These parameters are also identical to `RunSKIM`.

- `-m <string>`: IC model to use (`binary`, `trivalency`, `weighted`; default: `weighted`).
- `-p <double>`: Probability with which an arc is in the graph (has an effect for the `binary` model only).
	
#### Query Parameters

These parameters specify how to generate "queries," i.e., random seed sets for evaluating the influence estimator.

- `-n <int>`: The number of random queries to generate (default: `100`).
- `-N <int>`: The sizes of random seed sets (default: `1-50`). The value of this parameter can be a comma separated list of ranges. For example to generate seed sets, whose sizes are between 5 and 10 and between 20 and 30, set `-N 5-10,20-30`.
	
	The algorithm will run *for each seed set size* as many queries as specified by the `-n` parameter.
- `-g <string>`: The method by which seed sets are generated (`neigh` or `uni`; default: `uni`).
	- `uni`: To obtain a seed set of s nodes, sample s nodes from the graph uniformly at random.
	- `neigh`: To obtain a seed set of s nodes, first sample a single node u with probability proportional to its degree. Then, _exhaustively_ grow a BFS tree rooted at u to the smallest depth, such that the tree contains at least s vertices. Now sample s vertices from the tree uniformly at random. This generator produces seeds, whose sets of influenced vertices have a very high overlap.

#### Algorithm Parameters

These parameters are identical to `RunSKIM`.

- `-k <int>`: The _k_-value from the reachability sketches (default: `64`).
- `-l <int>`: Number of instances in the IC model (default: `64`).
- `-leval <int>`: The number of (different) instances to evaluate the exact influence on (`0` = off; default). If this parameter is `0` or not specified, the exact influence is evaluated on the same (number of) instances as specified by the `-l` parameter.

#### Output and Miscellaneous Parameters

These parameters are also identical to `RunSKIM`.

- `-os <string>`: Filename to output general statistics to (default: none).

- `-seed <int>`: Seed for random number generator (default: `31101982`).
- `-v`: Produce significantly less verbose output.

### Typical Call

A typical call to `RunInfluenceOracle` could look like this

	bin/RunInfluenceOracle -i mygraph.metis -type metis -k 64 -l 64 -N 1-50 -n 100 -g neigh -leval 512 -os mygraph.eststats

This will run the influence oracle on `mygraph.metis`, which is of METIS format, using a _k_-value of 64 and 64 instances in the IC model. After computing the combined reachability sketches, the estimator is evaluated by running 5000 queries (100 queries for each seed set size between 1 and 50). Each query will be generated using the "neigh" method, and the estimated influence will be compared to the (exact) influence computed on 512 separate instance using the naïve simulation approach. Statistics will be output to the file `mygraph.eststats`. 

### Statistics

Similarly to `RunSKIM`, setting the `-os` parameter in `RunInfluenceOracle` will write statistics to the specified text file. Each line of the file contains a key-value pair of the form `key = value`. The following items are output.
	
- `NumberOfVertices` -- The number of vertices in the input graph.
- `NumberOfArcs` -- The number of (directed) arcs in the input graph.
- `PreprocessingElapsedMilliseconds` -- The total amount of time spent for computing the combined reachability sketches.
- `NumberOfQueries` -- The number of queries that were run per seed set size (as set by the `-n` parameter).
- `NumberOfSeedSetSizes` -- The number of different seed set sizes.
- `SeedSizeRange` -- The value supplied by the `-N` parameter.
- `SeedGenerator` -- The method used for generating seed sets. A value of `0` equals the `uni` method and a value of `1` equals the `neigh` method.
- `TotalSketchesSize` -- The total number of entries in all reachability sketches.
- `TotalSketchesBytes` -- The total space consumption of all sketches in memory. 

For each distinct seed set size, the following items are output. Different seed set sizes are associated with zero-based consecutive identifiers i.

- `i_SeedSetSize` -- The seed set size.
- `i_AverageEstimatedInfluence` -- The estimated influence (as returned by the oracle), averaged over all queries for the given seed set size.
- `i_AverageExactInfluence` -- The exact influence (as computed by the naïve simulation-based approach), averaged over all queries for the given seed set size.
- `i_AverageError` -- The error of the estimated influence with respect to the exact influence, averaged over all queries for the given seed set size.
- `i_AverageEstimatorElapsedMilliseconds` -- The response time of the influence oracle in milliseconds, averaged over all queries for the given seed set size.
- `i_AverageExactElapsedMilliseconds` -- The running time spent in the naïve simulation-based approach in milliseconds, averaged over all queries for the given seed set size.

In addition, detailed statistics for each query j and each seed set size index i are output.

- `i_j_VertexIds` -- A comma-separated list of vertex ids contained in the seed set.
- `i_j_EstimatedInfluence` -- The estimated influence as returned by the oracle.
- `i_j_ExactInfluence` -- The exact influence as returned by the simulation-based approach.
- `i_j_Error` -- The error of the estimated influence with respect to the exact influence.
- `i_j_EstimatorElapsedMilliseconds` -- The response time of the oracle in milliseconds.
- `i_j_ExactElapsedMilliseconds` -- The running time of the simulation-based algorithm in milliseconds.


## Input Data Formats

Both `RunSKIM` and `RunInfluenceOracle` accept the input graph in two text formats: METIS and DIMACS.

### METIS Format

The METIS format is a simple text-based file format to specify graphs. Vertex ids are *one-based* consecutive *integral* numbers. That is, the vertex ids of a graph on n vertices are between 1 and n.

The input file is made up as follows.

- The first line contains the number of vertices and arcs, separated by a space character.
- All subsequent lines specify the arcs of the graph.
- The *i-th line* of the file contains the head vertex ids of the outgoing arcs from *vertex id i-1* (recall that the first line is the header and vertex ids start at 1).
- The vertex ids in each line are separated by a space character.

Please refer to [this page](http://people.sc.fsu.edu/~jburkardt/data/metis_graph/metis_graph.html) for some example graph files in METIS format.

### DIMACS Format

The DIMACS format is another simple text-based file format to specify graphs. It has been used in the [9th DIMACS Implementation Challenge](http://www.dis.uniroma1.it/challenge9/) on the Shortest Path Problem. As in METIS, vertex ids are *one-based* consecutive *integral* numbers. That is, the vertex ids of a graph on n vertices are between 1 and n.

The input file is made up as follows:

- Lines beginning with `c` are ignored.
- The first line is a header of the form `p sp <n> <m>`, where `n` is the number of vertices, and `m` is the number of (directed) arcs of the graph. The first line of a graph with 10 vertices and 30 arcs would thus be `p sp 10 30`.
- Each subsequent line specifies one arc in the form of `a <u> <v> <weight>`, where `u` and `v` are the tail and head vertex ids of the arc, and `weight` is the weight of the arc. (Note that weights are ignored when reading the input.)

Please refer to [this page](http://www.dis.uniroma1.it/challenge9/format.shtml#graph) for the official specification of the file format and to [this page](http://www.dis.uniroma1.it/challenge9/download.shtml#benchmark) for some example road graphs in DIMACS format.

## License

The source code of this project is released subject to the following license.

> Algorithm for Influence Estimation and Maximization
>
> Copyright (c) Microsoft Corporation
>
> All rights reserved.
>
> MIT License
>
> Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
>
> The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
>
> THE SOFTWARE IS PROVIDED *AS IS*, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
