#include "Distributed_vector.h"

void error2(int argc, char** argv) {
	int num_proc, rank;
	int vec_size = atoi(argv[1]);
	vector<int> v;
	MPI_Status status;
	
	// defined by the coordinator of this group
	int comm_number;

	// maps each coordinator to the list of workers that it manages
	map<int, vector<int>> cluster_workers;

	MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &num_proc);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	// Generate the vector
	if(rank == 0) {
		for(auto i = 0; i < vec_size; i++) {
			v.push_back(vec_size - i - 1);
		}
	}

	// making the lists
	for(auto i = 0; i < 4; i++) {
		cluster_workers.insert(pair<int, vector<int>>(i, vector<int>()));
	}

	int count;

	// -------------------- topology -------------------------- //


		if(rank < 4) {
			ifstream file("inputs/cluster" + to_string(rank) + ".txt");
			int worker_count;
			int worker_rank;

			comm_number = rank;

			file >> worker_count;
			for(auto i = 0; i < worker_count; i++) {
				file >> worker_rank;
				cluster_workers.at(rank).push_back(worker_rank);
			}

			// communicate the workers between coordinators
			switch(rank) {
				case 0:
					count = cluster_workers.at(rank).size();

					// send to 3
					send(rank, &count, 1, MPI_INT, 3, 0, MPI_COMM_WORLD);
					send(rank, &cluster_workers.at(rank)[0], count, MPI_INT, 3, 0, MPI_COMM_WORLD);

					// receive from 3
					for(auto i = 2; i < 4; i++) {
						MPI_Recv(&count , 1 , MPI_INT , 3 , 0 , MPI_COMM_WORLD , &status);
						cluster_workers.at(i).resize(count);
						MPI_Recv(&cluster_workers.at(i)[0] , count , MPI_INT , 3 , 0 , MPI_COMM_WORLD , &status);
					}

					break;

				case 2:
					count = cluster_workers.at(2).size();
					// send to 3
					send(rank, &count, 1, MPI_INT, 3, 0, MPI_COMM_WORLD);
					send(rank, &cluster_workers.at(2)[0], count, MPI_INT, 3, 0, MPI_COMM_WORLD);

					// receive from 3
					MPI_Recv(&count , 1 , MPI_INT , 3 , 0 , MPI_COMM_WORLD , &status);
					cluster_workers.at(0).resize(count);
					MPI_Recv(&cluster_workers.at(0)[0] , count , MPI_INT , 3 , 0 , MPI_COMM_WORLD , &status);
					MPI_Recv(&count , 1 , MPI_INT , 3 , 0 , MPI_COMM_WORLD , &status);
					cluster_workers.at(3).resize(count);
					MPI_Recv(&cluster_workers.at(3)[0] , count , MPI_INT , 3 , 0 , MPI_COMM_WORLD , &status);

					break;

				case 3:
					// receive from 0 and send to 2

					MPI_Recv(&count , 1 , MPI_INT , 0 , 0 , MPI_COMM_WORLD , &status);
					cluster_workers.at(0).resize(count);
					MPI_Recv(&cluster_workers.at(0)[0] , count , MPI_INT , 0 , 0 , MPI_COMM_WORLD , &status);				

					count = cluster_workers.at(0).size();
					send(rank, &count, 1, MPI_INT, 2, 0, MPI_COMM_WORLD);
					send(rank, &cluster_workers.at(0)[0], count, MPI_INT, 2, 0, MPI_COMM_WORLD);
					count = cluster_workers.at(3).size();
					send(rank, &count, 1, MPI_INT, 2, 0, MPI_COMM_WORLD);
					send(rank, &cluster_workers.at(3)[0], count, MPI_INT, 2, 0, MPI_COMM_WORLD);

					// receive from 2 and send to 0

					MPI_Recv(&count , 1 , MPI_INT , 2 , 0 , MPI_COMM_WORLD , &status);
					cluster_workers.at(2).resize(count);
					MPI_Recv(&cluster_workers.at(2)[0] , count , MPI_INT , 2 , 0 , MPI_COMM_WORLD , &status);

					for(auto i = 2; i < 4; i++) {
						count = cluster_workers.at(i).size();
						send(rank, &count, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
						send(rank, &cluster_workers.at(i)[0], count, MPI_INT, 0, 0, MPI_COMM_WORLD);
					}				
					break;
			}

			// comunicate the lists to the workers
			for(auto worker : cluster_workers.at(rank)) {
				for(auto i = 0; i < 4; i++) {
					count = cluster_workers.at(i).size();
					send(rank, &count, 1, MPI_INT, worker, 0, MPI_COMM_WORLD);
					send(rank, &cluster_workers.at(i)[0], count, MPI_INT, worker, 0, MPI_COMM_WORLD);
				}

			}
		} else {
			for(auto i = 0; i < 4; i++) {
				MPI_Recv(&count, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
				cluster_workers.at(i).resize(count);
				MPI_Recv(&cluster_workers.at(i)[0], count, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
			}

			comm_number = status.MPI_SOURCE;
		}

	// print the topology
	cout<< rank;
	cout<< " -> ";
	for(auto i = 0; i < 4; i++) {
		if(!cluster_workers.at(i).size()) {
			continue;
		}

		cout << i << ":" << cluster_workers.at(i)[0];
		for(auto j = 1; j < cluster_workers.at(i).size(); j++) {
			cout<< "," <<cluster_workers.at(i)[j];
		}

		if(i != 3) {
			cout << " ";
		}
	}

	cout << endl;

	// ----------------------- calculate the vector ---------------------------- //

	vector<int> valid_workers;

	for(auto i = 0; i < 4; i++) {
		if(i == 1) {
			continue;
		}

		for(auto wk : cluster_workers.at(i)) {
			 valid_workers.push_back(wk);
		}
	}

	sort(valid_workers.begin(), valid_workers.end(), greater<int>());

	map<int, vector<int>> worker_subvectors;

	// send the vector in chain
	if(rank == 0) {
		count = vec_size;
		send(rank, &vec_size, 1, MPI_INT, 3, 0, MPI_COMM_WORLD);
		send(rank, &v[0], count, MPI_INT, 3, 0, MPI_COMM_WORLD);
	}

	// get the vector from the previous coordinator	
	if(rank == 2 || rank == 3) {
		int source = (rank == 3) ? 0 : 3;
		MPI_Recv(&count , 1 , MPI_INT , source , 0 , MPI_COMM_WORLD , &status);
		v.resize(count);
		MPI_Recv(&v[0]  , count , MPI_INT , source , 0 , MPI_COMM_WORLD , &status);
	}

	// send it forward
	if(rank == 3) {
		send(rank, &count, 1, MPI_INT, 2, 0, MPI_COMM_WORLD);
		send(rank, &v[0], count, MPI_INT, 2, 0, MPI_COMM_WORLD);
	}

	// tell the workers which parts they have to multiply and get their results
	if(rank < 4 && rank != 1) {
		for(auto worker : cluster_workers.at(rank)) {
				worker_subvectors.insert(pair<int,vector<int>>(worker, vector<int>()));
				int wk_id = distance(valid_workers.begin(), find(valid_workers.begin(), valid_workers. end(), worker));
				for(auto i = wk_id; i < v.size(); i+= valid_workers.size()) {
					worker_subvectors.at(worker).push_back(v[i]);
				}

				count = worker_subvectors.at(worker).size();

				send(rank, &count, 1, MPI_INT, worker, comm_number + 1, MPI_COMM_WORLD);
				send(rank, &worker_subvectors.at(worker)[0], count, MPI_INT, worker, comm_number + 1, MPI_COMM_WORLD);		
		}

		for(auto i = 0; i < cluster_workers.at(rank).size(); i++) {
			MPI_Recv(&count , 1 ,  MPI_INT , MPI_ANY_SOURCE  , comm_number + 1 , MPI_COMM_WORLD , &status);
			MPI_Recv(&worker_subvectors.at(status.MPI_SOURCE)[0] , count , MPI_INT , status.MPI_SOURCE , comm_number + 1 , MPI_COMM_WORLD , &status);
		}
	}

	// get the vector modified by the previous coordinator
	if(rank == 0 || rank == 3) {
		int source = (rank == 0) ? 3 : 2;
		MPI_Recv(&v[0] , v.size() , MPI_INT  , source , 0 , MPI_COMM_WORLD , &status);
	}

	// add your part to it
	if(rank < 4 && rank != 1) {
		for(auto wk : cluster_workers.at(rank)) {
			int wk_id = distance(valid_workers.begin(), find(valid_workers.begin(), valid_workers. end(), wk));
			for(auto i = 0; i < worker_subvectors.at(wk).size(); i++) {
				v[i * (valid_workers.size()) + wk_id] = worker_subvectors.at(wk)[i];
			}
		}
	}

	// send it forward
	if(rank == 2 || rank == 3) {
		int dest = (rank == 3) ? 0 : 3;
		send(rank, &v[0], v.size(), MPI_INT, dest, 0, MPI_COMM_WORLD);
	}

	// if you're a worker, get your part of the vector, modify it and send it back to your coordinator
	if(rank > 3 && comm_number != 1) {
		MPI_Recv(&count , 1 , MPI_INT  , comm_number , comm_number + 1 , MPI_COMM_WORLD  ,  &status);
		v.resize(count);
		MPI_Recv(&v[0] , count , MPI_INT , comm_number  , comm_number + 1 , MPI_COMM_WORLD  , &status);

		for(int i = 0; i < v.size(); i++) {
			v[i] *= 5;
		}

		send(rank, &count, 1, MPI_INT, comm_number, comm_number + 1, MPI_COMM_WORLD);
		send(rank, &v[0], v.size(), MPI_INT, comm_number, comm_number + 1, MPI_COMM_WORLD);
	}

	// print the result
	if(rank == 0) {
		cout << "Rezultat:";

		for(auto i : v) {
			cout<<" "<<i;
		}

		cout<<"\n";
	}

	MPI_Finalize();
}