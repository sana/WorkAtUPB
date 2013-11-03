//Dascalu Laurentiu, 342C3
//Tema 2 SPRC - Sortare toleranta la defecte in MPI
#include "operations.h"

static MPI_Status status;
MPI_Request requests[MAX_CHILDREN_NUMBER];
MPI_Request req;
static int task_buffer[MAX_PACKET_SIZE];
static int task_size;
extern int rank;
char task_sent[MAX_CHILDREN_NUMBER];
extern map<MPI_Comm, int> processes_existing;

void send_task(Task *task, int dest, int tag, int header, MPI_Comm comm)
{
	enable_debug();
	PRINT("{%d} send header %d, dest %d, tag %d, comm %p, size %d\n", rank, header, dest, tag, comm, task->getSize());
	disable_debug();

	if (header || (task == NULL))
	{
		// trimit un pachet de QUIT
		PRINT("Trimit pachet de QUIT\n");
		MPI_Send(&header, 1, MPI_INT, dest, tag, comm);
		return;
	}

	write_task(task, task_buffer, &task_size);
	PRINT("Trimit pachet de [merge/sort] task\n");
	MPI_Send(&header, 1, MPI_INT, dest, tag, comm);
	MPI_Send(&task_size, 1, MPI_INT, dest, tag, comm);
	MPI_Send(task_buffer, task_size, MPI_INT, dest, tag, comm);
}

Task *receive_task(int from, int tag, MPI_Comm comm)
{
	int header;

	MPI_Recv(&header, 1, MPI_INT, from, tag, comm, &status);

	enable_debug();
	PRINT("{%d} recv header %d, from %d, tag %d\n", rank, header, from, tag);
	disable_debug();

	if (header)
		return NULL;

	MPI_Recv(&task_size, 1, MPI_INT, from, tag, comm, &status);
	MPI_Recv(&task_buffer, task_size, MPI_INT, from, tag, comm, &status);
	return read_task(task_buffer);
}

void check_recv_from_slaves(int &ntasks_in_queue, int &ntasks_to_receive,
		TasksManager *manager, MPI_Comm comm[MAX_CHILDREN_NUMBER])
{
	int header = 0, flag;

	memset(requests, 0, sizeof(requests));

	// Verific daca a fost un slave care a trimis un task
	for (int it = 0; it < MAX_CHILDREN_NUMBER; it++)
	{
		if (!task_sent[it])
			continue;

		if (!is_process_alive((rand() % 4) == 0, 0, 2, comm[it]))
		{
			// Un proces are probabilitatea 20% sa cada
			processes_existing.erase(comm[it]);
			enable_debug();
			PRINT(
					"[Master %d] procesul %d din comunicatorul %p a cazut !!!!\n",
					rank, it, comm[it]);
			disable_debug();
			MPI_Comm_spawn((char *) "./my_worker", MPI_ARGV_NULL, 1,
					MPI_INFO_NULL, 0, MPI_COMM_SELF, &comm[it],
					MPI_ERRCODES_IGNORE);
			manager->set_task_failed(it);
			task_sent[it] = 0;
			ntasks_to_receive--;
			processes_existing.insert(make_pair(comm[it], it));
			continue;
		}

		MPI_Recv(&header, 1, MPI_INT, 0, 1, comm[it], &status);
		MPI_Recv(&task_size, 1, MPI_INT, status.MPI_SOURCE, status.MPI_TAG,
				comm[it], &status);
		MPI_Recv(&task_buffer, task_size, MPI_INT, status.MPI_SOURCE,
				status.MPI_TAG, comm[it], &status);
		ntasks_to_receive--;
		task_sent[it] = 0;
		manager->set_task_successed(it);
		ntasks_in_queue--;
		if (manager->putMergeTask(read_task(task_buffer)))
			ntasks_in_queue++;
		break;
	}
}

bool is_process_alive(int header, int dest, int tag, MPI_Comm comm)
{
	int flag;

	int ret = MPI_Isend(&header, 1, MPI_INT, dest, tag, comm, &req);
	MPI_Irecv(&header, 1, MPI_INT, dest, tag, comm, &req);

	for (int i = 0; i < TIMEOUT; i++)
	{
		MPI_Test(&req, &flag, &status);
		if (flag)
			return true;
		sleep(1);
	}
	return false;
}
