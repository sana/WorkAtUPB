#ifndef COMMON_H_
#define COMMON_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/mman.h>

#include <fcntl.h>
#include <mqueue.h>
#include <semaphore.h>
#include <unistd.h>

#include <time.h>

#include "hash.h"

#define QUEUE				("/my_queue")
#define SEMAPHORE			"/my_semaphore"
#define SHARED_MEMORY		("/my_shared_memory")
#define MAX_PACKET_SIZE		1024
#define SHARED_MEMORY_SIZE	((MAX_WORD_SIZE + 1) * BUCKET_COUNT * MAX_WORDS_PER_BUCKET)

// Tipuri de mesaje client-server

// Add
#define A		(10)

// Remove
#define R		(11)

// Clear
#define C		(12)

// Sleep
#define S		(13)

// Print
#define P		(14)

// Exit
#define E		(15)

#define POSITION_AT(hash_table, i, j)	(hash_table + i * MAX_WORDS_PER_BUCKET * MAX_WORD_SIZE + j * MAX_WORD_SIZE)

struct _packet
{
	// Tip-ul pachetului
	char header;

	// Continutul pachetului
	char data[MAX_WORD_SIZE + 1];
}__attribute__((__packed__));

typedef struct _packet packet;
typedef struct _packet *ppacket;
int mysleep(unsigned long milisec);

#endif /* COMMON_H_ */
