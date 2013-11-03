#include "include/common.h"

void packet_handler(packet p, mqd_t queue, sem_t *sem[BUCKET_COUNT],
		char *hash_table)
{
	static int prio = 0;
	char buffer[MAX_PACKET_SIZE], all = 0;
	int i, sem_get = 0;

	memset(buffer, 0, MAX_PACKET_SIZE);

	switch (p.header)
	{
	case A:
		//fprintf(stdout, "Add %s\n", p.data);
		sprintf(buffer, "%c%s", p.header, p.data);
		sem_get = hash(p.data);
		break;

	case R:
		//fprintf(stdout, "Remove %s\n", p.data);
		sprintf(buffer, "%c%s", p.header, p.data);
		sem_get = hash(p.data);
		break;

	case C:
		//fprintf(stdout, "Clear\n");
		sprintf(buffer, "%c", p.header);
		all = 1;
		break;

	case S:
	{
		//fprintf(stdout, "Sleep pentru %s milisecunde\n", p.data);
		mysleep(atoi(p.data));
		return;
	}
	case P:
	{
		//fprintf(stdout, "Print table de pe memoria partajata\n");
		int i, j, k;

		sprintf(buffer, "%c", p.header);

		if (mq_send(queue, buffer, strlen(buffer) + 1, prio) < 0)
		{
			fprintf(stderr, "mq_send failed!\n");
			exit(EXIT_FAILURE);
		}

		for (i = 0; i < BUCKET_COUNT; i++)
		{
			sem_wait(sem[i]);
			for (j = 0; j < MAX_WORDS_PER_BUCKET; j++)
			{
				if (*POSITION_AT(hash_table, i, j) == 0)
					continue;
				printf("%s ", POSITION_AT(hash_table, i, j));
			}
			printf("\n");
			sem_post(sem[i]);
		}
		return;
	}
	case E:
		//fprintf(stdout, "Exit\n");
		sprintf(buffer, "%c", p.header);
		break;
	default:

		fprintf(stderr, "Invalid packet...\n");
		return;
	}

	if (mq_send(queue, buffer, strlen(buffer) + 1, prio) < 0)
	{
		fprintf(stderr, "mq_send failed!\n");
		exit(EXIT_FAILURE);
	}

	if (all)
	{
		// Astept dupa toate semafoarele
		for (i = 0; i < BUCKET_COUNT; i++)
		{
			if (sem_wait(sem[i]))
			{
				fprintf(stderr, "sem_wait() failed!\n");
				exit(EXIT_FAILURE);
			}
		}
	}
	else if (sem_get)
	{
		// Astept doar pentru semaforul ce se ocupa de un bucket
		if (sem_wait(sem[sem_get]))
		{
			fprintf(stderr, "sem_wait() failed!\n");
			exit(EXIT_FAILURE);
		}
	}
}

/*
 * Client-ul poate da urmatoarele comenzi :
 * 	a S - adauga cuvantul S
 * 	r S - sterge cuvantul S
 * 	c - sterge tabela server-ului
 * 	s m - sleep pentru m milisecunde
 * 	p - afiseaza tabela server-ului
 * 	e - inchide server-ul
 */
int main(int argc, char **argv)
{
	packet *packets = NULL;
	int npackets = 0, i;
	mqd_t queue;
	static sem_t *sem[BUCKET_COUNT];
	int shm_fd;
	char *hash_table;
	static char buffer[MAX_WORD_SIZE];

	if (argc < 2)
	{
		fprintf(stderr, "Invalid input\n");
		return EXIT_FAILURE;
	}

	queue = mq_open(QUEUE, O_WRONLY);

	for (i = 0; i < BUCKET_COUNT; i++)
	{
		sprintf(buffer, SEMAPHORE "%d", i);
		sem[i] = sem_open(buffer, O_CREAT| O_RDWR, 0644, 0);
		if (sem[i] == (sem_t *) -1)
		{
			fprintf(stderr, "sem_open() failed!\n");
			return EXIT_FAILURE;
		}
	}

	shm_fd = shm_open(SHARED_MEMORY, O_RDWR, 0444);

	if (shm_fd < 0)
	{
		fprintf(stderr, "shm_open() failed!\n");
		return EXIT_FAILURE;
	}

	hash_table = mmap(0, SHARED_MEMORY_SIZE, PROT_READ | PROT_WRITE,
			MAP_SHARED, shm_fd, 0);

	if (hash_table == MAP_FAILED)
	{
		fprintf(stderr, "mmap() failed!\n");
		return EXIT_FAILURE;
	}

	if (queue < 0)
	{
		fprintf(
				stderr,
				"Cannot establish a queue or a semaphore on current system....the server may be down!\n");
		return EXIT_FAILURE;
	}

	packets = (packet *) malloc(argc * sizeof(packet));

	i = 1;
	while (i < argc)
	{
		if (!strcasecmp(argv[i], "a"))
		{
			packets[npackets].header = A;
			memset(packets[npackets].data, 0, MAX_WORD_SIZE + 1);
			strcpy(packets[npackets].data, argv[i + 1]);
			i++;
		}
		else if (!strcasecmp(argv[i], "r"))
		{
			packets[npackets].header = R;
			memset(packets[npackets].data, 0, MAX_WORD_SIZE + 1);
			strcpy(packets[npackets].data, argv[i + 1]);
			i++;
		}
		else if (!strcasecmp(argv[i], "c"))
		{
			packets[npackets].header = C;
			memset(packets[npackets].data, 0, MAX_WORD_SIZE + 1);
		}
		else if (!strcasecmp(argv[i], "s"))
		{
			packets[npackets].header = S;
			memset(packets[npackets].data, 0, MAX_WORD_SIZE + 1);
			strcpy(packets[npackets].data, argv[i + 1]);
			i++;
		}
		else if (!strcasecmp(argv[i], "p"))
		{
			packets[npackets].header = P;
			memset(packets[npackets].data, 0, MAX_WORD_SIZE + 1);
		}
		else if (!strcasecmp(argv[i], "e"))
		{
			packets[npackets].header = E;
			memset(packets[npackets].data, 0, MAX_WORD_SIZE + 1);
		}
		else
		{
			fprintf(stderr, "Error while parsing input ...\n");
			return EXIT_FAILURE;
		}

		npackets++;
		i++;
	}

	for (i = 0; i < npackets; i++)
		packet_handler(packets[i], queue, sem, hash_table);

	free(packets);

	if (munmap(hash_table, SHARED_MEMORY_SIZE) < 0)
	{
		fprintf(stderr, "munmap() failed!\n");
		return EXIT_FAILURE;
	}

	if (close(shm_fd) < 0)
	{
		fprintf(stderr, "close() failed!\n");
		return EXIT_FAILURE;
	}

	if (mq_close(queue) < 0)
	{
		fprintf(stderr,"mq_close() failed!\n");
		return EXIT_FAILURE;
	}

	for (i = 0; i < BUCKET_COUNT; i++)
	{
		if (sem_close(sem[i]) < 0)
		{
			fprintf(stderr, "close() failed!\n");
			return EXIT_FAILURE;
		}
	}

	return 0;
}

int __nsleep(const struct timespec *req, struct timespec *rem)
{
	struct timespec temp_rem;
	if (nanosleep(req, rem) == -1)
		return __nsleep(rem, &temp_rem);
	else
		return 1;
}

int mysleep(unsigned long milisec)
{
	struct timespec req =
	{ 0 }, rem =
	{ 0 };
	time_t sec = (int) (milisec / 1000);
	milisec = milisec - (sec * 1000);
	req.tv_sec = sec;
	req.tv_nsec = milisec * 1000000L;
	__nsleep(&req, &rem);
	return 1;
}
