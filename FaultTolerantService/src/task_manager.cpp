//Dascalu Laurentiu, 342C3
//Tema 2 SPRC - Sortare toleranta la defecte in MPI
#include "task_manager.h"

bool my_debug = false;

static int *copyData(int *data, int size)
{
	if (data == NULL)
		return NULL;

	int *m_data = (int *) malloc(size * sizeof(int));

	for (int i = 0; i < size; i++)
		m_data[i] = data[i];
	return m_data;
}

void Task::debug(bool showHeader, FILE *fout)
{
	if (!fout)
		PRINT("[Task] debug, size = %d, data = %p\n", m_size, m_data);
	else
		fprintf(fout, "[Task] debug, size = %d, data = %p\n", m_size, m_data);
	if (m_data)
	{
		for (int i = 0; i < m_size; i++)
			if (!fout)
				PRINT("%d ", m_data[i]);
			else
				fprintf(fout, "%d ", m_data[i]);

		if (!fout)
			PRINT("\n\n");
		else
			fprintf(fout, "\n\n");
	}
}

SortTask::SortTask(int *data, int size)
{
#ifdef VERBOSE
	PRINT("[SortTask] create\n");
#endif
	m_data = copyData(data, size);
	m_size = size;
	m_solved = TASK_NOT_SOLVED;
}

void SortTask::computeResult()
{
	PRINT("[SortTask] compute result\n");

	PRINT("[compute] sort task %d\n", m_size);

	for (int i = 0; i < m_size; i++)
	{
		int value = m_data[i];
		int j = i - 1;
		while (j >= 0 && m_data[j] > value)
		{
			m_data[j + 1] = m_data[j];
			j--;
		}
		m_data[j + 1] = value;
	}

	m_solved = TASK_SOLVED;
}

SortTask::~SortTask()
{
#ifdef VERBOSE
	PRINT("[SortTask] destroy\n");
#endif
	free(m_data);
}

void SortTask::serialize(FILE *fout)
{
	PRINT("[SortTask] serialize %d [%d, %p] to %p\n",
			m_solved, m_size, m_data, fout);
	fwrite(&m_solved, sizeof(int), 1, fout);
	fwrite(&m_size, sizeof(int), 1, fout);
	fwrite(m_data, sizeof(int), m_size, fout);
}

void SortTask::deserialize(FILE *fout)
{
	PRINT("[SortTask] deserialize %p\n", fout);
	fread(&m_solved, sizeof(int), 1, fout);
	fread(&m_size, sizeof(int), 1, fout);
	m_data = (int *) malloc(m_size * sizeof(int));
	fread(m_data, sizeof(int), m_size, fout);
}

void SortTask::debug(bool showHeader, FILE *fout)
{
	if (showHeader)
	{
		if (!fout)
			PRINT("[SortTask] debug()\n");
		else
			fprintf(fout, "[SortTask] debug()\n");
	}
	Task::debug(showHeader, fout);
}

MergeTask::MergeTask(int *data1, int size1, int *data2, int size2)
{
#ifdef VERBOSE
	PRINT("[MergeTask] create\n");
#endif

	m_data1 = copyData(data1, size1);
	m_size1 = size1;

	m_data2 = copyData(data2, size2);
	m_size2 = size2;

	m_solved = TASK_NOT_SOLVED;
	m_size = 0;
	m_data = NULL;
}

void MergeTask::computeResult()
{
	PRINT("[MergeTask] compute\n");

	m_size = m_size1 + m_size2;

	PRINT("[compute] merge task %d\n", m_size);

	if (m_size == 0)
		return;

	m_data = (int *) malloc(m_size * sizeof(int));
	int i1, i2, k;

	for (i1 = 0, i2 = 0, k = 0; i1 < m_size1 && i2 < m_size2;)
	{
		if (m_data1[i1] < m_data2[i2])
			m_data[k++] = m_data1[i1++];
		else
			m_data[k++] = m_data2[i2++];
	}

	while (i1 < m_size1)
		m_data[k++] = m_data1[i1++];

	while (i2 < m_size2)
		m_data[k++] = m_data2[i2++];

	m_solved = TASK_SOLVED;
}

MergeTask::~MergeTask()
{
#ifdef VERBOSE
	PRINT("[MergeTask] destroy\n");
#endif

	free(m_data1);
	free(m_data2);
	free(m_data);
}

void MergeTask::serialize(FILE *fout)
{
	PRINT("[MergeTask] serialize %d [%d, %p], [%d %p], [%d %p] to %p\n",
			m_solved, m_size, m_data, m_size1, m_data1, m_size2,m_data2, fout);

	fwrite(&m_solved, sizeof(int), 1, fout);
	fwrite(&m_size, sizeof(int), 1, fout);
	if (m_size > 0)
		fwrite(m_data, sizeof(int), m_size, fout);

	fwrite(&m_size1, sizeof(int), 1, fout);
	fwrite(m_data1, sizeof(int), m_size1, fout);

	fwrite(&m_size2, sizeof(int), 1, fout);
	fwrite(m_data2, sizeof(int), m_size2, fout);
}

void MergeTask::deserialize(FILE *fin)
{
	PRINT("[MergeTask] deserialize\n");

	fread(&m_solved, sizeof(int), 1, fin);
	fread(&m_size, sizeof(int), 1, fin);
	if (m_size > 0)
	{
		m_data = (int *) malloc(m_size * sizeof(int *));
		fread(m_data, m_size, sizeof(int), fin);
	}

	fread(&m_size1, sizeof(int), 1, fin);
	m_data1 = (int *) malloc(m_size1 * sizeof(int *));
	fread(m_data1, m_size1, sizeof(int), fin);

	fread(&m_size2, sizeof(int), 1, fin);
	m_data2 = (int *) malloc(m_size2 * sizeof(int *));
	fread(m_data2, m_size2, sizeof(int), fin);
}

void MergeTask::debug(bool showHeader, FILE *fout)
{
	if (showHeader)
	{
		if (!fout)
			PRINT("[MergeTask] debug()\n");
		else
			fprintf(fout, "[MergeTask] debug()\n");
	}
	Task::debug(showHeader, fout);

	if (m_data1)
	{
		for (int i = 0; i < m_size1; i++)
			if (!fout)
				PRINT("%d ", m_data1[i]);
			else
				fprintf(fout, "%d ", m_data1[i]);

		if (!fout)
			PRINT("\n");
		else
			fprintf(fout, "\n");
	}

	if (m_data2)
	{
		for (int i = 0; i < m_size2; i++)
			if (!fout)
				PRINT("%d ", m_data2[i]);
			else
				fprintf(fout, "%d ", m_data2[i]);

		if (!fout)
			PRINT("\n");
		else
			fprintf(fout, "\n");
	}
}

void TasksManager::putTask(Task *task)
{
	PRINT("[TaskManager] put task %p\n", task);
	task->setTaskStatus(TASK_NOT_SOLVED);
	m_tasks.push_back(task);
}

bool TasksManager::putMergeTask(Task *task)
{
	if (!merge_task1)
	{
		SortTask *sort_task = dynamic_cast<SortTask *> (task);

		if (sort_task)
		{
			PRINT("[Master process] Am primit task buffer %d\n",
					sort_task->getSize());
			for (int i = 0; i < sort_task->getSize(); i++)
				PRINT("%d ", sort_task->getData()[i]);
			PRINT("\n");
			fflush(stdout);
		}
		merge_task1 = task;
		return false;
	}
	else
	{
		merge_task2 = task;

		PRINT("[Master process] Pun in coada un merge task %d %d\n",
				merge_task1->getSize(), merge_task2->getSize());

		task = new MergeTask(merge_task1->getData(), merge_task1->getSize(),
				merge_task2->getData(), merge_task2->getSize());

		m_tasks.push_back(task);

		delete merge_task1;
		delete merge_task2;
		merge_task1 = merge_task2 = NULL;
	}
	return true;
}

Task *TasksManager::removeTask()
{
	if (m_tasks.size() <= 0)
	{
		PRINT("[TaskManager] remove from empty task queue\n");
		return NULL;
	}

	Task *task = m_tasks[0];
	PRINT("[TaskManager] remove task %p\n", task);
	task->setTaskStatus(TASK_IN_PROGRESS);
	m_tasks.erase(m_tasks.begin());
	return task;
}

Task *TasksManager::removeTask(int slave_id)
{
	Task *task = removeTask();
	m_buffered_tasks[slave_id] = task;
	return task;
}

void TasksManager::serialize()
{
	FILE *fout = fopen(CHECK_POINT_FILE, "wb");
	assert(fout != NULL);
	int size = m_tasks.size();
	int header;

	PRINT("[TaskManager] serialize %d tasks\n", size);

	assert(fwrite(&size, sizeof(int), 1, fout) == 1);

	for (int i = 0; i < size; i++)
	{
		if (dynamic_cast<SortTask *> (m_tasks[i]))
			header = SORT_TASK;
		else if (dynamic_cast<MergeTask *> (m_tasks[i]))
			header = MERGE_TASK;
		else
			assert(0);
		fwrite(&header, sizeof(int), 1, fout);
		m_tasks[i]->serialize(fout);
	}

	MergeTask empty_task;
	memset(&empty_task, 0, sizeof(empty_task));
	for (int i = 0; i < MAX_CHILDREN_NUMBER; i++)
	{
		if (m_buffered_tasks[i] != NULL)
			m_buffered_tasks[i]->serialize(fout);
		else
			fwrite(&empty_task, sizeof(empty_task), 1, fout);
	}

	if (merge_task1)
		merge_task1->serialize(fout);
	else
		fwrite(&empty_task, sizeof(empty_task), 1, fout);
	assert(fclose(fout) == 0);
}

void TasksManager::deserialize()
{
	FILE *fin = fopen(CHECK_POINT_FILE, "rb");
	assert(fin != NULL);
	int size, header;
	Task *task;

	fread(&size, sizeof(int), 1, fin);

	for (int i = 0; i < size; i++)
	{
		fread(&header, sizeof(int), 1, fin);
		if (header == SORT_TASK)
		{
			task = new SortTask();
			task->deserialize(fin);
		}
		else if (header == MERGE_TASK)
		{
			task = new MergeTask();
			task->deserialize(fin);
		}
		else
			assert(0);
		m_tasks.push_back(task);
	}

	MergeTask empty_task;
	memset(&empty_task, 0, sizeof(empty_task));

	for (int i = 0; i < MAX_CHILDREN_NUMBER; i++)
	{
		task = (MergeTask *) malloc(sizeof(MergeTask));
		fread(task, sizeof(MergeTask), 1, fin);

		if (!memcmp(task, &empty_task, sizeof(empty_task)))
		{
			m_buffered_tasks[i] = NULL;
			free(task);
		}
		else
			m_buffered_tasks[i] = task;
	}

	task = (MergeTask *) malloc(sizeof(MergeTask));
	fread(task, sizeof(MergeTask), 1, fin);
	if (!memcmp(task, &empty_task, sizeof(empty_task)))
	{
		merge_task1 = NULL;
		free(task);
	}
	else
		merge_task1 = task;
}

int TasksManager::get_queue_size()
{
	return m_tasks.size();
}

Task *TasksManager::get_buffered_task()
{
	return merge_task1;
}

void TasksManager::set_task_failed(int slave_id)
{
	// Daca un task nu s-a executat cu succes
	// atunci il bag din nou in coada
	if (m_buffered_tasks[slave_id])
		putTask( m_buffered_tasks[slave_id]);
	m_buffered_tasks[slave_id] = NULL;
}

void TasksManager::set_task_successed(int slave_id)
{
	m_buffered_tasks[slave_id] = NULL;
}

void TasksManager::debug(bool showHeader, FILE *fout)
{
	PRINT("[TaskManager] debug\n");

	if (!fout)
		PRINT("There are %d tasks in queue\n", m_tasks.size());
	else
		fprintf(fout, "There are %d tasks in queue\n", m_tasks.size());

	for (unsigned int i = 0; i < m_tasks.size(); i++)
		m_tasks[i]->debug(showHeader, fout);
}

int get_optim_buffer_size()
{
	// Ipotetic: dimensiunea liniei de cache L1
	return 3 * 1024;
}

Task *read_task(int *buffer)
{
	Task *task = NULL;

	switch (buffer[0])
	{
	case SORT_TASK:
	{
		task = new SortTask(buffer + 2, buffer[1]);
		break;
	}
	case MERGE_TASK:
	{
		int size1 = buffer[1];
		int size2 = buffer[buffer[1] + 2];
		task = new MergeTask(buffer + 2, size1, buffer + size1 + 3, size2);
		break;
	}
	default:
		assert(0);
	}

	return task;
}

void write_task(Task *task, int *buffer, int *size)
{
	int header;

	if (dynamic_cast<SortTask *> (task))
		header = SORT_TASK;
	else if (dynamic_cast<MergeTask *> (task))
		header = MERGE_TASK;
	else
		assert(0);

	buffer[0] = header;

	if (header == SORT_TASK)
	{
		buffer[1] = task->getSize();
		memcpy(buffer + 2, task->getData(), buffer[1] * sizeof(int));
		*size = buffer[1] + 2;
	}
	else
	{
		MergeTask *merge_task = dynamic_cast<MergeTask *> (task);
		int current_offset = 2;
		buffer[1] = merge_task->m_size1;
		memcpy(buffer + current_offset, merge_task->m_data1,
				merge_task->m_size1 * sizeof(int));
		current_offset += merge_task->m_size1;
		buffer[current_offset++] = merge_task->m_size2;
		memcpy(buffer + current_offset, merge_task->m_data2,
				merge_task->m_size2 * sizeof(int));
		*size = current_offset + merge_task->m_size2;
	}
}
