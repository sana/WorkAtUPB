/* Dascalu Laurentiu, 342 C3, Tema 1 SPRC */

/*
// Dimensiunea maxima a unei comenzi simple si
// a raspunsului server-ului la o comanda data
// de client
*/
#define MAX_LEN     1024

/*
// Numarul maxim de comenzi simple pe care clientul
// le poate da intr-o comanda compusa
*/
#define MAX_CMDS      16

/*
// Rezultatul unei comenzi este un string de maxim
// MAX_LEN caractere; daca rezultatul este mai mare
// atunci acesta se truncheaza
*/
struct Reponse
{
	char result[MAX_LEN];
};

/* O comanda simpla este un simplu sir de caractere */
struct SimpleCommand
{
	char command[MAX_LEN];
};

/*
// O comanda este un vector de comenzi simple, terminat
// printr-un string cu lungimea 0
*/
struct Command
{
	int n_commands;
	struct SimpleCommand commands[MAX_CMDS];
};

program RSHELL_PROG {
	version RSHELL_VERS {
		struct Reponse EXECUTE_SIMPLE_COMMAND(struct Command) = 1;
		struct Reponse EXECUTE_COMMAND(struct Command) = 2;
		struct Reponse GET_REMOTE_PWD() = 3;
	} = 1;
} = 177134;

