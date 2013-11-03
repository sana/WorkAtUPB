//Dascalu Laurentiu, 342C3
//Tema 2 SPRC - Sortare toleranta la defecte in MPI
#ifndef TASK_MANAGER_H_
#define TASK_MANAGER_H_

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <vector>

using namespace std;

#define DEBUG

// Nu am reusit implementarea cu MPI ports (connection si accept)
//  - strace zice ca amandoua procesele fac un wait dupa pachete pe
// retea, desi client-ul vede port_address-ul server-ului.
//#define USE_MPI_PORT

// Cand serverul master crapa, copii acestuia se inchid
// si serverul de backup respawneaza alti copii
#define USE_MPI_RESPAWN

void debug();
void communication_debug();

//#undef DEBUG
#ifdef DEBUG
#define PRINT(fmt, args...)\
do\
{\
	if (my_debug)\
		fprintf(stdout, fmt, ##args);\
}\
while(0)\

extern bool my_debug;

#else
#define PRINT(fmt, args...)
#endif

#define TASK_NOT_SOLVED      10
#define TASK_IN_PROGRESS     11
#define TASK_SOLVED          12
#define SORT_TASK            111
#define MERGE_TASK           112
#define CHECK_POINT_FILE     "__check_point.save"
#define MAX_PACKET_SIZE      (16*1024)

// Timeout-ul este de 5 secunde
#define TIMEOUT              5

// Pot fi maxim 2 masteri
#define MAX_PROCESSES_NUMBER (2)
#define MAX_CHILDREN_NUMBER  (4)

class Task
{
public:
	virtual void computeResult() = 0;
	virtual void serialize(FILE *fout) = 0;
	virtual void deserialize(FILE *fin) = 0;

	virtual ~Task()
	{
	}

	int *getData()
	{
		return m_data;
	}

	int getSize()
	{
		return m_size;
	}

	virtual void debug(bool showHeader = true, FILE *fout = NULL);

protected:
	int m_solved;
	int *m_data;
	int m_size;
public:
	void setTaskStatus(int status)
	{
		m_solved = status;
	}
};

/**
 * Un task de sortare este definit ca fiind un tuplu
 * de (array, dimensiune array).
 */
class SortTask: public Task
{
public:
	SortTask(int *data = NULL, int size = 0);
	void computeResult();
	~SortTask();

	void serialize(FILE *fout);
	void deserialize(FILE *fin);
	void debug(bool showHeader = true, FILE *fout = NULL);
};

class MergeTask: public Task
{
public:
	MergeTask(int *data1 = NULL, int size1 = 0, int *data2 = NULL, int size2 =
			0);
	void computeResult();
	~MergeTask();

	void serialize(FILE *fout);
	void deserialize(FILE *fin);
	void debug(bool showHeader = true, FILE *fout = NULL);
public:
	int *m_data1, *m_data2;
	int m_size1, m_size2;
};

class TasksManager
{
public:
	TasksManager()
	{
		PRINT("[TaskManager] create\n");
		merge_task1 = merge_task2 = NULL;
		memset(m_buffered_tasks, 0, sizeof(m_buffered_tasks));
	}

	void putTask(Task *task);
	bool putMergeTask(Task *task);
	Task *removeTask();
	Task *removeTask(int slave_id);
	void serialize();
	void deserialize();

	int get_queue_size();
	void debug(bool showHeader, FILE *fout = NULL);

	~TasksManager()
	{
		PRINT("[TaskManager] destroy\n");
	}

	Task *get_buffered_task();

	void set_task_failed(int slave_id);
	void set_task_successed(int slave_id);

private:
	vector<Task *> m_tasks;
	Task *merge_task1;
	Task *merge_task2;
	Task *m_buffered_tasks[MAX_CHILDREN_NUMBER];
};

int get_optim_buffer_size();

/**
 * Fiind dat un buffer, intoarce obiectul task serializat.
 */
Task *read_task(int *buffer);

/**
 * Fiind dat un obiect, intoarce reprezentarea lui sub forma unui flux de octeti.
 * pereche (buffer, size)
 */
void write_task(Task *task, int *buffer, int *size);

void enable_debug();

void disable_debug();

#endif /* TASK_MANAGER_H_ */
