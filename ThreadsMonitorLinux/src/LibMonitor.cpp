/*
 * @author Dascalu Laurentiu, 335CA
 * Implementarea unui monitor generic
 */
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <assert.h>

#include <pthread.h>
#include <unistd.h>
#include <features.h>

#include "LibMonitorStructure.h"

extern "C"
{

#include "Constante.h"
#include "CallbackMonitor.h"
#include "LibMonitorInterface.h"

static pthread_mutex_t mutex_destroy =
PTHREAD_MUTEX_INITIALIZER;

const pthread_mutex_t __mutex =
PTHREAD_MUTEX_INITIALIZER;

// Intoarce true daca a reusit sa planifice pe cineva
static bool planifica(pmonitor m);
static sem_t *new_sem();

//TODO:erase
static void debug(pmonitor m);

Monitor* Create(int conditions, char policy)
{
	//printf("\n<Create()>\n");

	// Functia intoarce NULL daca parametri sunt invalizi:
	// numarul de variabile conditii este mai mic decat 0
	// semnalul are o alta valoare decat SIGNAL_AND_WAIT si SIGNAL_AND_CONTINUE.
	if (conditions < 0 && policy != SIGNAL_AND_WAIT && policy
			!= SIGNAL_AND_CONTINUE)
		return NULL;

	assert(pthread_mutex_init(&mutex_destroy, NULL) == 0);

	pmonitor m = (pmonitor) malloc(sizeof(struct _monitor));
	assert(m != NULL);
	memset(m, 0, sizeof(struct _monitor));

	m->conditions = conditions;
	m->policy = policy;
	m->destroyed = false;

	m->entry_queue = new queue<sem_t *> ();
	assert(m->entry_queue != NULL);

	m->signaled_queue = new queue<sem_t *> ();
	assert(m->signaled_queue != NULL);

	m->waiting_queue = new queue<sem_t *> ();
	assert(m->waiting_queue != NULL);

	m->conditions_queue = new vector<queue<sem_t *> > (conditions, queue<
			sem_t *> ());
	assert(m->conditions_queue != NULL);

	m->entered_threads = new map<pthread_t, pthread_t> ();

	m->mutex = __mutex;
	assert(pthread_mutex_init(&m->mutex, NULL) == 0);

	//printf("</Create()>\n\n");

	return (Monitor *) m;
}

int Destroy(Monitor *_m)
{
	//printf("\n<Destroy()>\n");

	pmonitor m = (pmonitor) _m;

	if (m->destroyed)
		return -1;

	int rc = pthread_mutex_trylock(&m->mutex);

	if (rc == 0)
	{
		assert(pthread_mutex_lock(&mutex_destroy) == 0);
		assert(pthread_mutex_unlock(&m->mutex) == 0);

		delete m->entry_queue;
		delete m->signaled_queue;
		delete m->waiting_queue;
		delete m->conditions_queue;
		delete m->entered_threads;
		m->destroyed = true;

		pthread_mutex_destroy(&m->mutex);

		free(m);

		assert(pthread_mutex_unlock(&mutex_destroy) == 0);
	}
	else
		return -1;

	//printf("</Destroy()>\n\n");

	return 0;
}

int Enter(Monitor *_m)
{
	//printf("\n<Enter(%ld)>\n", pthread_self());

	pmonitor m = (pmonitor) _m;

	// Daca id-ul thread-ului curent este indexata in container
	// inseamna ca el a apelat deja aceasta functie.
	if (m->entered_threads->find(pthread_self()) != m->entered_threads->end())
		return -1;

	int rc = pthread_mutex_trylock(&m->mutex);

	if (rc == 0)
		m->entered_threads->insert(make_pair(pthread_self(), pthread_self()));
	else if (rc == EBUSY)
	{
		// Mutexul a fost luat de altcineva
		// si il bag in coada Entry
		sem_t *sem = new_sem();
		m->entry_queue->push(sem);
		IncEnter();

		assert(sem_wait(sem) == 0);
		assert(sem_destroy(sem) == 0);
		free(sem);

		// Iau ownership-ul monitorului
		assert(pthread_mutex_lock(&m->mutex) == 0);
		m->entered_threads->insert(make_pair(pthread_self(), pthread_self()));
	}

	//printf("</Enter(%ld)>\n\n", pthread_self());

	return 0;
}

int Leave(Monitor *_m)
{
	//printf("\n<Leave(%ld)>\n", pthread_self());

	pmonitor m = (pmonitor) _m;

	if (m->entered_threads->find(pthread_self()) == m->entered_threads->end())
		return -1;

	m->entered_threads->erase(pthread_self());

	// Planific pe altcineva si parasesc monitorul
	planifica(m);

	//printf("</Leave(%ld)>\n\n", pthread_self());

	return 0;
}

int Wait(Monitor *_m, int cond)
{
	//printf("\n<Wait(%ld) pe conditia %d>\n", pthread_self(), cond);

	pmonitor m = (pmonitor) _m;

	if (cond >= m->conditions)
		return -1;

	sem_t *sem = new_sem();
	(*m->conditions_queue)[cond].push(sem);
	IncCond(cond);

	planifica(m);

	assert(sem_wait(sem) == 0);
	assert(sem_destroy(sem) == 0);
	free(sem);

	//printf("</Wait(%ld) pe conditia %d>\n\n", pthread_self(), cond);

	return 0;
}

int Signal(Monitor *_m, int cond)
{
	//printf("\n<Signal(%ld) pe conditia %d>\n", pthread_self(), cond);

	pmonitor m = (pmonitor) _m;

	if (cond >= m->conditions)
		return -1;

	vector<queue<sem_t *> > *aux = m->conditions_queue;

	if ((*aux)[cond].size() > 0)
	{
		sem_t *sem = (*aux)[cond].front();
		(*aux)[cond].pop();
		DecCond(cond);

		m->waiting_queue->push(sem);
		IncWait();
	}

	if (m->policy == SIGNAL_AND_WAIT)
	{
		sem_t *sem = new_sem();

		m->signaled_queue->push(sem);
		IncSignal();

		planifica(m);

		assert(sem_wait(sem) == 0);
		assert(sem_destroy(sem) == 0);
		free(sem);
	}
	else if (m->policy == SIGNAL_AND_CONTINUE)
	{
		// Nothing to do ..
	}

	//printf("</Signal(%ld) pe conditia %d>\n\n", pthread_self(), cond);

	return 0;
}

int Broadcast(Monitor *_m, int cond)
{
	//printf("\n<Broadcast(%ld) pe conditia %d>\n", pthread_self(), cond);
	pmonitor m = (pmonitor) _m;

	if (cond >= m->conditions)
		return -1;

	queue<sem_t *> *queue_cond = &(*m->conditions_queue)[cond];

	while (queue_cond->size() > 0)
	{
		m->waiting_queue->push(queue_cond->front());
		queue_cond->pop();
		IncWait();
		DecCond(cond);
	}

	if (m->policy == SIGNAL_AND_WAIT)
	{
		sem_t *sem = new_sem();
		m->signaled_queue->push(sem);
		IncSignal();

		planifica(m);

		assert(sem_wait(sem) == 0);
		assert(sem_destroy(sem) == 0);
		free(sem);
	}
	else if (m->policy == SIGNAL_AND_CONTINUE)
	{
		// Nothing to do ...
	}
	else
		return -1;

	//printf("</Broadcast(%ld) pe conditia %d>\n\n", pthread_self(), cond);

	return 0;
}

static bool planifica(pmonitor m)
{
	bool ok = true;

	if (m->waiting_queue->size() > 0)
	{
		sem_t *sem = m->waiting_queue->front();
		m->waiting_queue->pop();
		DecWait();

		// Dau drumul primului thread din coada Signaled
		assert(sem_post(sem) == 0);
	}
	else if (m->signaled_queue->size() > 0)
	{
		sem_t *sem = m->signaled_queue->front();
		m->signaled_queue->pop();
		DecSignal();

		m->waiting_queue->push(sem);
		IncWait();

		return planifica(m);
	}
	else if (m->entry_queue->size() > 0)
	{
		sem_t *sem = m->entry_queue->front();
		m->entry_queue->pop();
		DecEnter();

		// Dau drumul primului thread din coada Entry
		assert(sem_post(sem) == 0);
	}

	else
		ok = false;

	// Cedez dreptul asupra monitorului
	assert(pthread_mutex_unlock(&m->mutex) == 0);
	return ok;
}

sem_t *new_sem()
{
	sem_t *sem = (sem_t *) malloc(sizeof(sem_t));
	sem_init(sem, 0, 0);
	return sem;
}

}

static void debug(pmonitor m)
{
	printf("%d %d %d ? ", m->entry_queue->size(), m->signaled_queue->size(),
			m->waiting_queue->size());

	vector<queue<sem_t *> > *aux = m->conditions_queue;

	for (unsigned int i = 0; i < aux->size(); i++)
		printf("%d ", (*aux)[i].size());
	printf("\n");
}
