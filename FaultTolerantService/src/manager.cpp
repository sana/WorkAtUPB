//Dascalu Laurentiu, 342C3
//Tema 2 SPRC - Sortare toleranta la defecte in MPI
#include <mpi.h>
#define MANAGER
using namespace std;

extern "C"
{
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
}

#include "task_manager.h"
#include "operations.h"
#include <vector>
#include <map>

#define USAGE     "Expected ./tema2 input_file output_file\n"

static int get_file_numbers_size(char *filename);
static void send_quit_to_slaves();
static Task *finish_solving();
static Task *mirror_master_server();
static void main_loop();
static void check_task(Task *task, char *arg);
static char *output;

TasksManager *manager = NULL;
MPI_Status status;
extern MPI_Request requests[MAX_PROCESSES_NUMBER];
map<MPI_Comm, int> processes_existing;

static void check_point();
static Task *next_task(int *buffer, int count, int optim_size);
static Task *tolerant_distributed_sort(int rank, int orig_size);
static vector<int> managers_world;
static Task *master_server();

int rank, slaves_size;
extern char task_sent[MAX_PROCESSES_NUMBER];

void spawn_workers(int &size, bool mirror_server = false);
MPI_Comm slaves_comm[MAX_CHILDREN_NUMBER];

void start_connections();
void stop_connections();

/**
 * @brief MPI Tolerant Sorting
 * @param Fisierul de intrare
 * Formatul fisierului de intrare este binar si detalii despre el
 * gasiti in README
 * @param Fisierul de iesire
 */
int main(int argc, char *argv[])
{
	int orig_size, optim_size;
	int *buffer = NULL;
	int npackets;
	int my_numbers, my_offset, additional_offset;
	Task *task;
	MPI_File infile;

	assert(argc == 3);

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &orig_size);

	memset(requests, 0, sizeof(requests));
	output = argv[2];

	if (orig_size > MAX_PROCESSES_NUMBER)
	{
		fprintf(stderr, "Maximum processes number is %d\n",
				MAX_PROCESSES_NUMBER);
		return EXIT_FAILURE;
	}

#undef DEBUG
#ifdef DEBUG
	if (rank == 0)
	{
		debug();
		communication_debug();
	}
	MPI_Finalize();
	exit(EXIT_SUCCESS);
#endif

	if (argc != 3)
	{
		fprintf(stderr, USAGE);
		return EXIT_FAILURE;
	}

	my_numbers = get_file_numbers_size(argv[1]);
	additional_offset = my_numbers % orig_size;
	if (rank == 0)
	{
		my_numbers = my_numbers / orig_size + additional_offset;
		my_offset = 0;
	}
	else
	{
		my_numbers = my_numbers / orig_size;
		my_offset = my_numbers * rank + additional_offset;
	}

	optim_size = get_optim_buffer_size();
	buffer = (int *) malloc(my_numbers * sizeof(int));

	MPI_File_open(MPI_COMM_WORLD, argv[1], MPI_MODE_RDONLY, MPI_INFO_NULL,
			&infile);

	MPI_File_set_view(infile, my_offset * sizeof(int), MPI_INT, MPI_INT,
			(char *) "native", MPI_INFO_NULL);
	MPI_File_read(infile, buffer, my_numbers, MPI_INT, &status);
	MPI_File_close(&infile);

	if ((my_numbers % optim_size) == 0)
		npackets = my_numbers / optim_size;
	else
		npackets = my_numbers / optim_size + 1;

#ifdef VERBOSE
	PRINT("rank = %d, my_numbers = %d, my_offset = %d, npackets = %d\n",
			rank, my_numbers, my_offset, npackets);
#endif

	if (rank == 0)
	{
		processes_existing.insert(make_pair(MPI_COMM_WORLD, 1));

		manager = new TasksManager();

		// Introduc task-urile in coada manager-ului
		while ((task = next_task(buffer, my_numbers, optim_size)) != NULL)
			manager->putTask(task);

		for (int i = 1; i < orig_size; i++)
		{
			MPI_Recv(&npackets, 1, MPI_INT, i, 0, MPI_COMM_WORLD, &status);
#ifdef VERBOSE
			PRINT("[Master process] am primit [%d, %d]\n", i, npackets);
#endif

			for (int j = 0; j < npackets; j++)
				manager->putTask(receive_task(i, 0, MPI_COMM_WORLD));
		}
	}
	else
	{
		vector<Task *> tasks;
		int tasks_number = 0;

		// Introduc task-urile intr-o coada
		while ((task = next_task(buffer, my_numbers, optim_size)) != NULL)
		{
			tasks.push_back(task);
			tasks_number++;
		}

		MPI_Send(&tasks_number, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);

		// Trimit task-urile catre master
		for (int i = 0; i < tasks_number; i++)
			send_task(tasks[i], 0, 0, 0, MPI_COMM_WORLD);
	}

	check_point();
	MPI_Barrier(MPI_COMM_WORLD);

#ifdef VERBOSE
	if (rank == 0)
	{
		PRINT("Coada de task-uri inainte de intrarea in zona toleranta la defecte\n");
		manager->debug(false);
	}
#endif

	if (rank == 1)
	{
		manager = new TasksManager();
		manager->deserialize();
	}

	/**
	 * In acest moment, exista doi master care au aceeasi
	 * coada de task-uri
	 *
	 * Primul master se ocupa de scheduling-ul task-urilor
	 * pe slaves si face mirroring catre celalalt master
	 *
	 * Daca primul master cade, atunci cel de-al doilea ii
	 * ia rolul; daca al doilea master cade atunci nu se
	 * mai face mirroring
	 */

	spawn_workers(slaves_size);
	task = tolerant_distributed_sort(rank, orig_size);

	if (rank == 0 && task == NULL)
	{
		fprintf(stderr, "Internal error; sorting algorithm failed!\n");
		return EXIT_FAILURE;
	}

	if (rank == 0)
		check_task(task, argv[2]);

#ifdef DEBUG
	if (rank == 0)
	{
		FILE *fout = fopen("debug.txt", "wt");
		manager->debug(false, fout);
		fclose(fout);
	}
#endif

	MPI_Barrier(MPI_COMM_WORLD);
	PRINT("[Master process %d] am terminat executia\n", rank);
	free(buffer);
	MPI_Finalize();

	return EXIT_SUCCESS;
}

static int get_file_numbers_size(char *filename)
{
	struct stat result;
	assert(stat(filename, &result) == 0);
	return (int) result.st_size / sizeof(int);
}

static void check_point()
{
	// Intr-un checkpoit serializez procesul master
	if (manager)
		manager->serialize();
}

static Task *next_task(int *buffer, int count, int optim_size)
{
	static int current_offset = 0;
	Task *task = NULL;

	if (current_offset >= count)
		return NULL;

	if (current_offset + optim_size <= count)
		task = new SortTask(buffer + current_offset, optim_size);
	else
		task = new SortTask(buffer + current_offset, count - current_offset);

	current_offset += optim_size;

	return task;
}

static Task *tolerant_distributed_sort(int rank, int orig_size)
{
	/*
	 * In acest moment, master-ul are coada de task-uri
	 * si o serializare pe disc
	 *
	 * Daca discul cade, atunci se pierd toate datele
	 *
	 */
	if (rank == 0)
		return master_server();
	return mirror_master_server();
}

void spawn_workers(int &size, bool mirror_server)
{
	int flag;
	MPI_Status status;

	if (!mirror_server && rank)
		return;

	size = MAX_CHILDREN_NUMBER;

	for (int it = 0; it < MAX_CHILDREN_NUMBER; it++)
	{
		MPI_Comm_spawn((char *) "./my_worker", MPI_ARGV_NULL, 1, MPI_INFO_NULL,
				0, MPI_COMM_SELF, &slaves_comm[it], MPI_ERRCODES_IGNORE);
		processes_existing.insert(make_pair(slaves_comm[it], it));
	}
}

void send_quit_to_slaves()
{
	// Trimit tuturor slave-urilor un mesaj de QUIT
	int header = 1;
	MPI_Request request;

	for (map<MPI_Comm, int>::iterator it = processes_existing.begin(); it
			!= processes_existing.end(); it++)
	{
		if (it->first == MPI_COMM_WORLD)
			MPI_Send(&header, 1, MPI_INT, 1, 1, it->first);
		else
			MPI_Send(&header, 1, MPI_INT, 0, 2, it->first);
	}
}

Task *finish_solving()
{
	// Ultimul task de merge il rezolva master-ul
	Task *task = manager->removeTask();
	task->computeResult();
	Task *buffered_task = manager->get_buffered_task();

	if (buffered_task)
	{
		MergeTask *merge_task = new MergeTask(task->getData(), task->getSize(),
				buffered_task->getData(), buffered_task->getSize());
		merge_task->computeResult();
		delete task;
		task = merge_task;
	}

	check_task(task, output);
	MPI_Finalize();
	exit(0);
	return task;
}

static char port_name[128];

Task *mirror_master_server()
{
	MPI_Status status;
	MPI_Request request;
	int header, flag;
	static char errmsg[128];
	int msglen, merr;

	memset(port_name, 0, sizeof(port_name));
	MPI_Open_port(MPI_INFO_NULL, port_name);

	merr = MPI_Publish_name((char *) "sana", MPI_INFO_NULL, port_name);
	if (merr)
	{
		MPI_Error_string(merr, errmsg, &msglen);
		fprintf(stderr, "Error in Publish_name: \"%s\"\n", errmsg);
		fflush(stderr);
		MPI_Finalize();
		exit(1);
	}
	else
		PRINT("[backup server] port name-ul meu este %s\n", port_name);
	MPI_Close_port(port_name);

	MPI_Irecv(&header, 1, MPI_INT, 0, 1, MPI_COMM_WORLD, &request);

	while (true)
	{
		// Eu sunt serverul de backup
		MPI_Test(&request, &flag, &status);

		if (flag)
		{
			if (header == 111)
			{
				enable_debug();
				PRINT("[Backup Server] Master-ul a cazut, intru in functiune!!\n");
				disable_debug();
				// Construiesc lista de comunicatori si sar in main-loop
				start_connections();
				main_loop();
				stop_connections();
				return finish_solving();
			}
			else
				return NULL;
		}
		sleep(1);
	}

	merr = MPI_Unpublish_name((char *) "sana", MPI_INFO_NULL, port_name);
	if (merr)
	{
		MPI_Error_string(merr, errmsg, &msglen);
		fprintf(stderr, "[Mirror Master %d] Error in Unpublish name: \"%s\"\n",
				rank, errmsg);
		fflush(stderr);
		MPI_Finalize();
		exit(1);
	}
}

void main_loop()
{
	int ntasks_to_receive = 0;
	int ntasks_in_queue = 0;
	int current_slave = -1, header;
	MPI_Request request;

	ntasks_in_queue = manager->get_queue_size();
	current_slave = 0;
	memset(task_sent, 0, sizeof(task_sent));

	while ((ntasks_in_queue + ntasks_to_receive) > 0)
	{
		/*
		 * Iau un task din coada si-l trimit slave-ului curent
		 */
		if (ntasks_in_queue > 1)
		{
			// Caut un slave caruia sa-i trimit un task
			while (true)
			{
				check_point();

#ifdef SERVER_CRASH
				if (rank == 0)
				{
					header = 111;
					MPI_Send(&header, 1, MPI_INT, 1, 1, MPI_COMM_WORLD);
					MPI_Finalize();
					exit(0);
				}
#endif

				if (!task_sent[current_slave])
				{
					Task *task = manager->removeTask(current_slave);
					ntasks_to_receive++;
					PRINT("[Master %d] ", rank);

					send_task(task, 0, 1, 0, slaves_comm[current_slave]);

					task_sent[current_slave] = 1;
					PRINT("[Master %d] am trimis task %d lui %d [%d, %d]\n\n",
							rank, task->getSize(), current_slave,
							ntasks_in_queue, ntasks_to_receive);
					break;
				}
				current_slave = (current_slave + 1) % slaves_size;
				check_recv_from_slaves(ntasks_in_queue, ntasks_to_receive,
						manager, slaves_comm);
			}
		}
		else if (ntasks_in_queue == 1 && ntasks_to_receive == 0)
			break;
		check_recv_from_slaves(ntasks_in_queue, ntasks_to_receive, manager,
				slaves_comm);
	}
}

Task *master_server()
{
	enable_debug();
	PRINT("==== Starting tolerant distributed sort ====\n\n\n");
	disable_debug();

	main_loop();

	enable_debug();
	PRINT("==== End of tolerant distributed sort ====\n\n\n");
	disable_debug();

	send_quit_to_slaves();
	return finish_solving();
}

void start_connections()
{
#ifdef USE_MPI_PORT
	MPI_Status status;
	MPI_Comm comm;
#endif

#ifdef USE_MPI_RESPAWN
	int size = MAX_CHILDREN_NUMBER;
#endif

#ifdef USE_MPI_PORT
	for (int i = 0; i < MAX_CHILDREN_NUMBER; i++)
	{
		PRINT("[Master Process %d] incearca cineva sa se conecteze\n", rank);
		// Accept conexiunile din partea slave-uilor
		MPI_Comm_accept(port_name, MPI_INFO_NULL, 0, MPI_COMM_SELF, &comm);
		slaves_comm[i] = comm;
		PRINT("[Master Process %d] s-a conectat un client dar nu am implementarea definita [%s, %d]",
				rank, __FILE__, __LINE__);
		exit(0);
	}
#endif

#ifdef USE_MPI_RESPAWN
	spawn_workers(size, true);
#endif
}

void stop_connections()
{
#ifdef USE_MPI_PORT
	for (int it = 0; it < MAX_CHILDREN_NUMBER; it++)
	{
		MPI_Comm_free(&slaves_comm[it]);
		MPI_Comm_disconnect(&slaves_comm[it]);
	}

	MPI_Unpublish_name((char *) "sana", MPI_INFO_NULL, port_name);
	MPI_Close_port(port_name);
#endif

#ifdef USE_MPI_RESPAWN
	send_quit_to_slaves();
#endif
}

void check_task(Task *task, char *arg)
{
	enable_debug();
	PRINT("Checking sorted vector for sorting consistency\n");
	// Verific daca numerele sunt sortate
	for (int i = 0; i < task->getSize() - 1; i++)
		if (task->getData()[i] > task->getData()[i + 1])
		{
			fprintf(stderr, "Internal error; sorting algorithm failed!\n");
			return;
		}

	PRINT("Sorting algorithm finished for %d numbers\n", task->getSize());
	disable_debug();
	// Scriu rezultatul in fisierul de iesire
	FILE *fout = fopen(arg, "wt");
	fprintf(fout, "Size: %d\n", task->getSize());
	for (int i = 0; i < task->getSize(); i++)
		fprintf(fout, "%d\n", task->getData()[i]);
}
