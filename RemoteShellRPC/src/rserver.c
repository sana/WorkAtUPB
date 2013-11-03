/* Dascalu Laurentiu, 342 C3, Tema 1 SPRC */
// Implementarea server-ului RPC

#include <stdio.h> 
#include <time.h> 
#include <rpc/rpc.h>
#include <rpc/auth_des.h>
#include <sys/time.h>
#include <unistd.h>
#include <assert.h>

#include "rshell.h"

// Uncomment this for debugging messages
#define DEBUG

// DES Authentification
//#define DES_AUTH

// UNIX Authentification
#define UNIX_AUTH

static void debug_command(struct Command *cmd)
{
	int i;

#ifdef DEBUG
	printf("[%d comenzi simple]\n", cmd->n_commands);

	for (i = 0 ; i < cmd->n_commands ; i++)
		printf("[comanda %d] \"%s\"\n", i, cmd->commands[i].command);

	fflush(stdout);
#endif
}

static int check_unix(struct svc_req *rqstp)
{
	struct authunix_parms *unix_cred;
	int uid;
	unsigned long nusers;

	if (rqstp->rq_proc == NULLPROC)
		return 0;

	switch (rqstp->rq_cred.oa_flavor)
	{
		case AUTH_UNIX:
			unix_cred = (struct authunix_parms *)rqstp->rq_clntcred;
			uid = unix_cred->aup_uid;
			break;
		default:
			return 0;
	}

	printf("[DEBUG] accepting uid = %d\n", uid);
	fflush(stdout);

	return 1;
}

static int check_des(struct svc_req *rqstp)
{
	// Implementation FAILED
	return 1;
}

static int check(struct svc_req *rqstp)
{
#ifdef UNIX_AUTH
	return check_unix(rqstp);
#endif
#ifdef DES_AUTH
	return check_des(rqstp);
#endif
	return 1;
}

static void execute_string_command(char command[1024], struct Reponse *reponse)
{
	static char buffer[1024];
	static char __buffer[128];
	int result;

	// Redirectez STDERR-ul la STDOUT
	strcat(command, " 2>&1");
	FILE *fin = popen(command, "r");

	if (!fin)
	{
		sprintf(reponse->result, "command execution failed!");
		return;
	}

	// Salvez doar primii 1024 de caractere din output
	buffer[0] = '\0';

	while(fgets(__buffer, 128, fin))
	{
		strcat(buffer, __buffer);
		if (strlen(buffer) >= 1023)
			break;
	}

        result = pclose(fin);
	snprintf(reponse->result, 1023, "[%d] %s\n", result, buffer);
}

static void execute_command(struct Command *cmd, struct Reponse *reponse)
{
	int i;

        reponse->result[0] = 0;

	for (i = 0 ; i < cmd->n_commands ; i++)
		execute_string_command(cmd->commands[i].command, reponse);
}

struct Reponse *execute_simple_command_1_svc(struct Command *cmd, struct svc_req *req)
{
	static struct Reponse reponse;

	memset(&reponse, 0, sizeof(reponse));

	if (!check(req))
		return &reponse;

	debug_command(cmd);
	execute_command(cmd, &reponse);

	return &reponse;
}

struct Reponse *execute_command_1_svc(struct Command *cmd, struct svc_req *req)
{
	static struct Reponse reponse;

	memset(&reponse, 0, sizeof(reponse));

	if (!check(req))
		return &reponse;

	debug_command(cmd);
	execute_command(cmd, &reponse);

	return &reponse;
}

struct Reponse *get_remote_pwd_1_svc(void *dummy1, struct svc_req *req)
{
	static struct Reponse reponse;

	memset(&reponse, 0, sizeof(reponse));
	assert(getcwd(reponse.result, 1024) != NULL);

	return &reponse;
}

