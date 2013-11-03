/* Dascalu Laurentiu, 342 C3, Tema 1 SPRC */
// Implementarea clientului RPC

#include <stdio.h> 
#include <time.h> 
#include <rpc/rpc.h>
#include <string.h>
#include <stdlib.h>

#include "rshell.h" 
#define RMACHINE "localhost"

#define COMMAND        222
#define SIMPLE_COMMAND 333

// Uncomment this for debugging messages
//#define DEBUG

// DES authentification
//#define DES_AUTH

// UNIX authentification
#define UNIX_AUTH

static char buffer[1024];

static void debug_reponse(struct Reponse *reponse)
{
#ifdef DEBUG
	if (!reponse)
		printf("[DEBUG] [rezultat] am primit NULL pointer\n");
	else
		printf("[DEBUG] [rezultat] \"%s\"\n", reponse->result);
#endif
}

static void prompt(struct Reponse *reponse, int type)
{
	if (!reponse)
		return;
	if (!type)
		printf("%s ", reponse->result);
	else
		printf("%s\n", reponse->result);
}

static int parse_command(struct Command *command)
{
	char *str = strtok(buffer, ";\n");
	int i = 0;

	if (!strcasecmp(buffer, "quit"))
		return 0;
	if (!strcasecmp(buffer, "exit"))
		return 0;

	while(1)
	{
		if (!str)
			break;

		memset(command->commands[i].command, 0,
			sizeof(command->commands[i]));
		strcpy(command->commands[i++].command,
			str);

		str = strtok(NULL, ";\n");
	}
	command->n_commands = i;

	return 1;
}

static int get_command_type(struct Command *command)
{
	if (command->n_commands == 1)
		return SIMPLE_COMMAND;
	return COMMAND;
}

int main(int argc, char *argv[])
{
	/* variabila clientului */
	CLIENT *handle;
	struct Command command;
	struct Reponse *reponse;
	char *hostname;
	char *username;
	char servername[128];

	if (argc != 2 && argc != 3)
	{
		fprintf(stderr, "Usage: ./rclient hostname [username]\n");
		return 1;
	}

	hostname = strdup(argv[1]);
	if (argc == 3)
		username = strdup(argv[2]);
	else
		username = strdup("");

#ifdef DEBUG
	printf("[DEBUG] Using [hostname \"%s\"] [username \"%s\"]\n", hostname, username);
#endif

	handle=clnt_create(
		hostname,		/* numele masinii unde se afla server-ul */
		RSHELL_PROG,		/* numele programului disponibil pe server */
		RSHELL_VERS,		/* versiunea programului */
		"tcp");			/* tipul conexiunii client-server */

#ifdef DEBUG
	printf("[DEBUG] [clnt_create] returned \"%p\"\n", handle);
#endif

	if(handle == NULL)
        {
		perror("");
		return -1;
	}

#ifdef DES_AUTH
	host2netname(servername, hostname, NULL);
	printf("[host2netname] returned \"%s\"\n", servername);
	handle->cl_auth = authdes_create(servername, 60, NULL, NULL);
	printf("[authdes_create] returned %p\n", handle->cl_auth);
#endif
#ifdef UNIX_AUTH
	handle->cl_auth = authunix_create_default();
#endif

#ifdef DEBUG
	printf("[DEBUG] Remote shell using RPC\n");
	printf("= Quit methods: quit, exit and CTRL+C =\n");
#endif

	while(1)
	{
		reponse = get_remote_pwd_1(handle, handle);
		prompt(reponse, 0);
		fgets(buffer, 1024, stdin);
		
		if (!parse_command(&command))
			goto end;
		else
		{
			switch(get_command_type(&command))
			{
				case COMMAND:
				{
#ifdef DEBUG
					int i;
					printf("[DEBUG] Numar comenzi %d\n", command.n_commands);
					for (i = 0 ; i < command.n_commands ; i++)
						printf("[DEBUG] [Comanda %d] \"%s\"\n", i, command.commands[i].command);
#endif
				}
					reponse = execute_command_1(&command, handle);
					break;
				case SIMPLE_COMMAND:
#ifdef DEBUG
					printf("[DEBUG] Execut comanda simpla \"%s\"\n", command.commands[0].command);
#endif
					reponse = execute_simple_command_1(&command, handle);
					break;
				default:
#ifdef DEBUG
					printf("[DEBUG] Invalid command type\n");
#endif
					break;
			}
		}

		debug_reponse(reponse);
		prompt(reponse, 1);
	}

end:
	free(hostname);
	free(username);
	
	return 0;
}

