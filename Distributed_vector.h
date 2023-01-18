#include "mpi.h"

#include <iostream>
#include <map>
#include <fstream>
#include <vector>
#include <algorithm>

using namespace std;

// MPI_Send with a print to console
int send(int this_rank, const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);

// solves the situations where there's no error or there is a broken link between nodes 0 and 1
void  error0_1(int argc, char** argv);

// solves the situation where node 1 is isolated from the others
void error2(int argc, char** argv);
