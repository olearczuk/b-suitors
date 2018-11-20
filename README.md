# Overview
Goal was to implement parallel b-suitor algorithm based on the paper: [*Efficient approximation algorithms for weighted b-matching*](https://www.cs.purdue.edu/homes/apothen/Papers/bMatching-SISC-2016.pdf)
## Introduction
>Problem is introduced in linked paper:
We describe a half-approximation algorithm, b-Suitor, for
computing a b-Matching of maximum weight in a graph, implement it on serial and
shared-memory parallel processors, and compare its performance against approximation
algorithms that have been proposed earlier. b-Matching is a generalization of
the well-known Matching problem in graphs, where the objective is to choose a subset
M of edges in the graph such that at most b(v) edges in M are incident on each
vertex v, and subject to this restriction we maximize the sum of the weights of the
edges in M. (Here b(v) is a non-negative integer.)

## Parallel Algorithm
>Parallel algorithm is also presented inmentioned paper:
>In this Subsection we describe a shared
memory parallel b-Suitor algorithm. It uses iteration rather than recursion; it queues
vertices whose proposals have been rejected for later processing unlike the recursive
algorithm that processes them immediately. It is to be noted that b-Suitor finds
the solution irrespective of the order of the vertices as well as the edges are processed
which means the solution is stable irrespective of how operating system schedules
the threads. It uses locks for synchronizing multiple threads to ensure sequential
consistency.
The parallel algorithm is described in Algorithm 3. The algorithm maintains a
queue of unsaturated vertices Q which it tries to find partners for during the current
iteration of the while loop, and also a queue of vertices Q that become deficient in
this iteration to be processed again in the next iteration. The algorithm then attempts
to find a partner for each vertex u in Q in parallel. It tries to find b(u) proposals for
u to make while the adjacency list N(u) has not been exhaustively searched thus far
in the course of the algorithm.

## Pseudo code
Multithreaded shared memory algorithm for approximate b-Matching.
**Input**: A graph G = (V;E;w) and a vector b. **Output**: A 1/2-approximate edge weighted b-Matching M.

```python
    procedure parallal_b-suitor(G, b):
        Q = V
	Q' = {}
	while Q is not {}:
	    for all vertices u in Q do in parallel:
	        i = 1
		while i <= b(u) and N(u) ix not exhausted:
		    Let p in N(u) be an eligible partner of u;
		    if p is not NULL:
			Lock p
			if p is still eligible:
			    i = i + 1
			    Make u a Suitor of p
			    if u annuls the proposal of a vertex v:
				Add v to Q'
				Update db(v)
			Unlock p
		    else:
		        N(u) = exhausted
	    Update Q using Q'
	    Update b using db

```
