# Butilca Rares

# The args for the program
PROCESS_NUM=12
VECTOR_LENGTH=15
ERROR_TYPE=0 # 0, 1 or 2

build:
	mpic++ -o distributed_vector Distributed_vector.cpp Error0-1.cpp Error2.cpp -Wall

run:
	mpirun --oversubscribe -np $(PROCESS_NUM) ./distributed_vector $(VECTOR_LENGTH) $(ERROR_TYPE)

clean:
	rm -rf distributed_vector
