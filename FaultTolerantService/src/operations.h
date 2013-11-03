//Dascalu Laurentiu, 342C3
//Tema 2 SPRC - Sortare toleranta la defecte in MPI
#ifndef OPERATIONS_H_
#define OPERATIONS_H_

#include "task_manager.h"
#include <mpi.h>

void send_task(Task *task, int dest, int tag = 0, int header = 0,
		MPI_Comm comm = MPI_COMM_WORLD);

Task *receive_task(int from, int tag = 0, MPI_Comm comm = MPI_COMM_WORLD);

void
check_recv_from_slaves(int &ntasks_in_queue, int &ntasks_to_receive,
		TasksManager *manager, MPI_Comm comm[MAX_CHILDREN_NUMBER]);

// Verifica daca un proces este in picioare
bool is_process_alive(int header, int dest, int tag, MPI_Comm comm);

#endif /* OPERATIONS_H_ */
