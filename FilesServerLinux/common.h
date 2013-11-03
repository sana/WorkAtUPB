#ifndef COMMON_H_
#define COMMON_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define SERVER_PORT          (8192)
#define READ                    (1)
#define WRITE                   (2)
#define LS                      (3)
#define MAX_PACKET_SIZE      (4096)
#define MAX_STRING_SIZE       (128)

#define DEBUG

#ifdef DEBUG

#define MY_PRINT(fmt, args...) printf(" [DBG] " fmt, ##args)

#define DEBUG_CHAR_BLOCK(block, size)\
do\
{\
	int i;\
	for (i = 0 ; i < size ; i++)\
		printf("%d ", block[i]);\
	printf("\n");\
} while(0)

#else

#define MY_PRINT(fmt, args...)

#define DEBUG_CHAR_BLOCK(block, size)

#endif

#define NUMBER_CONVERT(buffer, current_size, number, __buffer)\
do\
{\
	sprintf(__buffer, "%d", number);\
	buffer[current_size] = strlen(__buffer);\
	strncpy(buffer + current_size + 1, __buffer, buffer[current_size]);\
	current_size += buffer[current_size] + 1;\
} while(0)

#define NUMBER_CONVERT_REVERSE(buffer, offset, size, number, __buffer)\
do\
{\
	size = buffer[offset];\
	strncpy(__buffer, buffer + offset + 1, size);\
	__buffer[size] = '\0';\
	number = atoi(__buffer);\
	offset += size + 1;\
} while(0)

#define STRING_CONVERT(buffer, current_size, string)\
do\
{\
	buffer[current_size] = strlen(string);\
	strncpy(buffer + current_size + 1, string, buffer[current_size]);\
	current_size += buffer[current_size] + 1;\
} while(0)

#define STRING_CONVERT_REVERSE(buffer, offset, size, string)\
do\
{\
	size = buffer[offset];\
	strncpy(string, buffer + offset + 1, size);\
	string[size] = '\0';\
	offset += size + 1;\
} while(0)

#define COMMAND_GENERIC(buffer, current_size, fisier, offset, len, __buffer)\
do\
{\
	STRING_CONVERT(buffer, current_size, fisier);\
	NUMBER_CONVERT(buffer, current_size, offset, __buffer);\
	NUMBER_CONVERT(buffer, current_size, len, __buffer);\
} while(0)

#define COMMAND_GENERIC_REVERSE(buffer, current_size, size, fisier, offset, len, __buffer)\
do\
{\
	STRING_CONVERT_REVERSE(buffer, current_size, size, fisier);\
	NUMBER_CONVERT(buffer, current_size, offset, __buffer);\
	NUMBER_CONVERT(buffer, current_size, len, __buffer);\
} while(0)

#define SEND_AND_RECEIVE(server_socket, buffer)\
do\
{\
	my_send(server_socket, buffer, MAX_PACKET_SIZE, 0);\
	memset(buffer, 0, MAX_PACKET_SIZE);\
	my_recv(server_socket, buffer, MAX_PACKET_SIZE, 0);\
} while(0)

#endif /* COMMON_H_ */
