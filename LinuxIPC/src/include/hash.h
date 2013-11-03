
#ifndef _HASH_H
#define _HASH_H

#define MAX_WORD_SIZE		(1<<4)

#define BUCKET_COUNT		(1<<8)

#define MAX_WORDS_PER_BUCKET	(1<<8)


unsigned int hash(const char *str);

#endif

