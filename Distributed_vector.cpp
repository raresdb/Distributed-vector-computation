#include "Distributed_vector.h"

int send(int this_rank, const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm) {
	cout << "M(" << this_rank << "," << dest << ")\n";
	return MPI_Send(buf, count, datatype, dest, tag, comm);
}

int main(int argc, char *argv[]) {
	int comm_error_type = atoi(argv[2]);

	if(comm_error_type < 2) {
		error0_1(argc, argv);
	} else {
		error2(argc, argv);
	}
	
	return 0;
}