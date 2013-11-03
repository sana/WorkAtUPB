//Dascalu Laurentiu, 342C3
//Tema 2 SPRC - Sortare toleranta la defecte in MPI
#include <cstdio>
#include <mpi.h>
#include <map>
#include "task_manager.h"
#include "operations.h"

int rank;
map<MPI_Comm, int> processes_existing;

using namespace std;

int main(int argc, char **argv)
{
	MPI_Comm parent;
	MPI_Status status;
	MPI_Request request;
	int size, flag, header;
	bool alive;
	static char port_name_out[128];
	static char errmsg[128];
	int msglen, merr;

	MPI_Init(&argc, &argv);

	MPI_Comm_get_parent(&parent);

	if (parent == MPI_COMM_NULL)
		goto end;

	MPI_Comm_remote_size(parent, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	while (true)
	{
		MPI_Irecv(&header, 1, MPI_INT, 0, 2, parent, &request);

		alive = false;

		for (int i = 0; i < 10 * TIMEOUT; i++)
		{
			MPI_Test(&request, &flag, &status);
			if (flag)
			{
				alive = true;
				break;
			}
			sleep(1);
		}

		if (!alive)
		{
#ifdef USE_MPI_PORTS
			// Daca a picat server-ul
			// atunci ma conectez la serverul secundar
			// si comunic cu el
			enable_debug();
			PRINT(
					"[Slave %d] a cazut server-ul master, ma conectez la server-ul secundar!\n",
					rank);
			disable_debug();

			merr = MPI_Lookup_name((char *) "sana", MPI_INFO_NULL,
					port_name_out);
			if (merr)
			{
				MPI_Error_string(merr, errmsg, &msglen);
				PRINT("[Slave %d] error in Lookup name: \"%s\"\n", rank, errmsg);
				fflush(stdout);
				break;
			}
			PRINT("[Slave %d] am executat cu succes lookup-ul %s\n", rank,
					port_name_out);
			MPI_Comm_connect(port_name_out, MPI_INFO_NULL, 0,
					MPI_COMM_SELF, &parent);
			PRINT(
					"[Slave %d] m-am conectat la server-ul de backup dar nu am implementarea definita [%s, %d]",
					rank, __FILE__, __LINE__);
			exit(1);
#endif

#ifdef USE_MPI_RESPAWN
			// Daca serverul master a cazut, atunci ies si eu
			PRINT("[Slave %d] server-ul principal a cazut, ma inchid si eu\n", rank);
			break;
#endif
		}

		// Daca parintele imi da un numar diferit de 0
		// atunci eu ies fara sa fac calcule
		if (header)
			break;
		MPI_Send(&header, 1, MPI_INT, 0, 2, parent);

		PRINT("[Slave %d] ", rank);
		Task *task = receive_task(0, 1, parent);

		PRINT("[Slave %d] am primit un task %p\n", rank, task);
		if (!task)
			break;

		PRINT("[Slave %d] ", rank);
		task->computeResult();

		PRINT("[Slave %d] ", rank);
		if (dynamic_cast<SortTask *> (task))
			send_task(task, 0, 1, 0, parent);
		else if (dynamic_cast<MergeTask *> (task))
		{
			// Convertesc din formatul MergeTask in formatul SortTask
			MergeTask *merge_task = dynamic_cast<MergeTask *> (task);

			Task *aux_task = new SortTask(merge_task->getData(),
					merge_task->getSize());
			send_task(aux_task, 0, 1, 0, parent);
			delete merge_task;
			merge_task = NULL;
		}
		PRINT("[Slave %d] am trimis rezultatul\n\n", rank);
	}

	PRINT("[Slave %d] am terminat executia\n", rank);
	end: MPI_Finalize();
	return 0;
}
