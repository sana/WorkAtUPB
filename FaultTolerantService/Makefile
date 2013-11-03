#!/bin/bash
# Dascalu Laurentiu, 342C3
# Tema 2 SPRC - Sortare toleranta la defecte in MPI

NPROCESSES=2
SIZE=20000

all: clean test worker manager

manager:
	mpic++ src/manager.cpp src/task_manager.cpp src/debugger.cpp src/operations.cpp -o my_manager -DSERVER_CRASH
	mpirun -np $(NPROCESSES) ./my_manager input.bin output.bin

worker:
	mpic++ src/worker.cpp src/task_manager.cpp src/debugger.cpp src/operations.cpp -o my_worker

test: src/test_gen.c
	gcc src/test_gen.c -o test
	./test $(SIZE) input.bin

clean:
	rm -rf test input.bin my_manager my_worker

