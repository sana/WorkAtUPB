/*
 * @author Dascalu Laurentiu, 335CA
 * Structura unui monitor generic
 */
#ifndef LIBMONITOR_STRUCTURE_H_
#define LIBMONITOR_STRUCTURE_H_

#include <vector>
#include <queue>
#include <map>

#include <semaphore.h>

using namespace std;

struct _monitor
{
	// Numarul de variabile de conditii
	int conditions;

	// Politica de functionare
	char policy;

	// Monitorul a fost distrus sau nu
	bool destroyed;

	// Coada de intrare
	queue<sem_t *> *entry_queue;

	// Coada thread-urilor care au executat signal dar nu au fost planificate
	queue<sem_t *> *signaled_queue;

	// Coada thread-urilor care au fost trezite de signal si sunt gata de planificare
	queue<sem_t *> *waiting_queue;

	// Vector de cozi; o coada pentru fiecare variabila de conditie
	vector<queue<sem_t *> > *conditions_queue;

	// Mutexul asociat monitorului
	pthread_mutex_t mutex;

	// In acest container tin id-urile thread-urilor
	// care au intrat in monitor; se mapeaza id la id
	map<pthread_t, pthread_t> *entered_threads;
}__attribute__((packed));

typedef struct _monitor *pmonitor;

#endif /* LIBMONITOR_STRUCTURE_H_ */
