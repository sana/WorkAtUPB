//Dascalu Laurentiu, 342C3
//Tema 2 SPRC - Sortare toleranta la defecte in MPI
#include "task_manager.h"

static void deserialize()
{
	TasksManager manager;
	manager.deserialize();
	Task *task;

	while ((task = manager.removeTask()) != NULL)
		task->debug();
}

void debug()
{
	FILE *fin = fopen("input.bin", "rb");
	assert(fin != NULL);

	int size1 = 10;
	int *buffer1 = (int *) malloc(size1 * sizeof(int));

	int size2 = 20;
	int *buffer2 = (int *) malloc(size2 * sizeof(int));

	assert(fread(buffer1, sizeof(int), size1, fin) == (unsigned int) size1);
	assert(fread(buffer2, sizeof(int), size2, fin) == (unsigned int) size2);

	TasksManager manager;

	Task *t1 = new SortTask(buffer1, size1);
	Task *t2 = new SortTask(buffer2, size2);

	manager.putTask(t1);
	manager.putTask(t2);
	t1 = manager.removeTask();
	t1->computeResult();
	t2 = manager.removeTask();
	t2->computeResult();

	Task *t3 = new MergeTask(t1->getData(), t1->getSize(), t2->getData(),
			t2->getSize());
	manager.serialize();
	manager.putTask(t3);
	t3 = manager.removeTask();
	t3->computeResult();

	t1->debug();
	t2->debug();
	t3->debug();

	delete t1;
	delete t2;
	delete t3;

	assert(fclose(fin) == 0);

	deserialize();
}

#define MAX_TEST_SIZE 32

void communication_debug()
{
	// Serializarea unui task intr-un buffer de int-uri
	// Deserializarea unui task dintr-un buffer de int-uri
	int t_data1[5] =
	{ 5, 4, 3, 2, 1 };
	int t_size1 = 5;

	int t_data2[7] =
	{ 21, 20, 19, 18, 17, 16, 15 };
	int t_size2 = 7;

	int data[MAX_TEST_SIZE];
	int size;

	// SORT TASK
	Task *task1 = new SortTask(t_data1, t_size1);
	task1->computeResult();
	for (int i = 0; i < MAX_TEST_SIZE; i++)
		data[i] = -1;
	write_task(task1, data, &size);

	for (int i = 0; i < MAX_TEST_SIZE; i++)
		PRINT("%d ", data[i]);
	PRINT("\n");

	// SORT TASK
	Task *task2 = new SortTask(t_data2, t_size2);
	task2->computeResult();
	for (int i = 0; i < MAX_TEST_SIZE; i++)
		data[i] = -1;
	write_task(task2, data, &size);

	for (int i = 0; i < MAX_TEST_SIZE; i++)
		PRINT("%d ", data[i]);
	PRINT("\n");

	// MERGE TASK
	Task *task3 = new MergeTask(task1->getData(), task1->getSize(),
			task2->getData(), task2->getSize());
	task3->computeResult();
	for (int i = 0; i < MAX_TEST_SIZE; i++)
		data[i] = -1;
	write_task(task3, data, &size);

	for (int i = 0; i < MAX_TEST_SIZE; i++)
		PRINT("%d ", data[i]);
	PRINT("\n");

	delete task1;
	delete task2;
	delete task3;
}

void enable_debug()
{
	my_debug = true;
}

void disable_debug()
{
	my_debug = false;
}
