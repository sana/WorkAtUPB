#include "include/common.h"

int main(int argc, char **argv)
{
	if (argc != 1)
		fprintf(stderr,
		"Server-ul ignora parametrii din linia de comanda....\n");

	mqd_t queue;
	struct mq_attr attr;
	static char buffer[MAX_PACKET_SIZE + 1];
	char done = 0;
	char *hash_table;

	static sem_t *sem[BUCKET_COUNT];
	unsigned int prio = 10, i;

	memset(&attr, 0, sizeof(attr));
	attr.mq_flags = 0;
	attr.mq_maxmsg = 10;
	attr.mq_msgsize = MAX_PACKET_SIZE;
	attr.mq_curmsgs = 0;

	if (daemon(0, 0))
	{
		fprintf(stderr, "daemon() failed!\n");
		return EXIT_FAILURE;
	}

	queue = mq_open(QUEUE, O_CREAT | O_RDONLY, 0644, &attr);

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

	int shm_fd = shm_open(SHARED_MEMORY, O_CREAT | O_RDWR, 0644);

	if (shm_fd < 0)
	{
		fprintf(stderr, "shm_open() failed!\n");
		return EXIT_FAILURE;
	}

	if (ftruncate(shm_fd, SHARED_MEMORY_SIZE) < 0)
	{
		fprintf(stderr, "truncate() failed!\n");
		return EXIT_FAILURE;
	}

	hash_table = mmap(0, SHARED_MEMORY_SIZE, PROT_WRITE | PROT_READ,
			MAP_SHARED, shm_fd, 0);

	if (hash_table == MAP_FAILED)
	{
		fprintf(stderr, "mmap() failed!\n");
		return EXIT_FAILURE;
	}

	memset(hash_table, 0, SHARED_MEMORY_SIZE);

	if (queue < 0)
	{
		fprintf(stderr,
		"Cannot open a queue or a semaphore on current system!\n");
		return EXIT_FAILURE;
	}

	/*
	 * Bucla principala a serverului; iesirea se face la primirea unui mesaj de tip (e)
	 */
	while (!done)
	{
		ssize_t bytes_read;
		memset(buffer, 0, MAX_PACKET_SIZE + 1);

		// Citesc din coada
		if ((bytes_read = mq_receive(queue, buffer, MAX_PACKET_SIZE + 1, &prio))
				< 0)
		{
			fprintf(stderr, "mq_receive() failed!\n");
			return EXIT_FAILURE;
		}

		switch (buffer[0])
		{
		case A:
		{
			//printf("Adaug %s\n", buffer + 1);
			int rezumat = hash(buffer + 1), pos = 0, valid = 1;

			// Caut daca cheia curenta exista sau nu in dictionar
			for (i = 0; i < MAX_WORDS_PER_BUCKET; i++)
			{
				if (!strcmp(buffer + 1, POSITION_AT(hash_table, rezumat, i)))
				{
					valid = 0;
					break;
				}
			}

			if (valid)
			{
				// Adaug o cheie doar daca aceasta nu exista in dictionar
				// Trec peste toate cheile cu acelasi hash
				while (strlen(POSITION_AT(hash_table, rezumat, pos)) > 0)
					pos++;
				strcpy(POSITION_AT(hash_table, rezumat, pos), buffer + 1);
			}

			if (sem_post(sem[rezumat]))
			{
				fprintf(stderr, "sem_post() failed!\n");
				return EXIT_FAILURE;
			}

			break;
		}
		case R:
		{
			//printf("Sterg %s\n", buffer + 1);
			int rezumat = hash(buffer + 1), pos = 0;

			// Caut cuvantul in lista de chei cu acelasi rezumat.
			for (pos = 0; pos < MAX_WORDS_PER_BUCKET; pos++)
			{
				if (!strcmp(POSITION_AT(hash_table, rezumat, pos), buffer + 1))
				{
					memset(POSITION_AT(hash_table, rezumat, pos), 0, strlen(buffer + 1));
					break;
				}
			}

			// Cand termin de sters o cheie, dau voie unui client sa execute o operatie asupra
			// bucket-ului curent.
			if (sem_post(sem[rezumat]))
			{
				fprintf(stderr, "sem_post() failed!\n");
				return EXIT_FAILURE;
			}

			break;
		}
		case P:
			for (i = 0; i < BUCKET_COUNT; i++)
				if (sem_post(sem[i]))
				{
					fprintf(stderr, "sem_post() failed!\n");
					return EXIT_FAILURE;
				}
			break;
		case C:
			//printf("Sterg tabela\n");
			memset(hash_table, 0, SHARED_MEMORY_SIZE);

			for (i = 0; i < BUCKET_COUNT; i++)
			{
				if (sem_post(sem[i]))
				{
					fprintf(stderr, "sem_post() failed!\n");
					return EXIT_FAILURE;
				}
			}
			break;
		case E:
			done = 1;
			break;
		default:
			continue;
		}
	}

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

	if (shm_unlink(SHARED_MEMORY) < 0)
	{
		fprintf(stderr, "unlink() failed!\n");
		return EXIT_FAILURE;
	}

	if (mq_close(queue) < 0)
	{
		fprintf(stderr, "close() failed!\n");
		return EXIT_FAILURE;
	}

	if (mq_unlink(QUEUE) < 0)
	{
		fprintf(stderr, "unlink() failed!\n");
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

	for (i = 0; i < BUCKET_COUNT; i++)
	{
		sprintf(buffer, SEMAPHORE "%d", i);
		if (sem_unlink(buffer) < 0)
		{
			fprintf(stderr, "unlink() failed!\n");
			return EXIT_FAILURE;
		}
	}

	return EXIT_SUCCESS;
}
