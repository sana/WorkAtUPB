#include "client.h"

static void command_ls(int server_socket, char *director);
static void command_read(int server_socket, char *fisier, int offset, int len);
static void command_write(int server_socket, char *fisier, int offset, int len);

// Buffer intern pentru conversia dintre string-urile retinute in memorie
// si stringurile serializate in vederea transmiterii pe retea.
static char __buffer[MAX_STRING_SIZE];

// Buffer intern folosit pentru compunerea pachetului
static char buffer[MAX_PACKET_SIZE];

int main(int argc, char **argv)
{
	/*
	 * Sintaxa de client este:
	 * client adresa_server rd fisier offset len = read din fisier de la offset si size
	 * client adresa_server wr fisier offset len = write in fisier de la offset si size
	 * client adresa_server ls director = listeaza continutul directorului
	 *
	 * Marimea directoarelor este cea intoarsa de ls => 0 pe Windows,
	 * multiplu de block size pe Linux
	 */

	// ip-ul server-ului
	char *server_ip = NULL;

	// numele fisierului/directorului
	char *fisier = NULL;

	int offset = -1, len = -1, op = -1;

	// socket-ul prin care comunic cu server-ul
	int server_socket = -1;

	if (argc == 6 || argc == 4)
	{
		if (argc == 4)
		{
			if (strcasecmp(argv[2], "ls"))
				return EXIT_FAILURE;
			fisier = strdup(argv[3]);
			op = LS;
		}
		else
		{
			if (!strcasecmp(argv[2], "rd"))
				op = READ;
			else if (!strcasecmp(argv[2], "wr"))
				op = WRITE;
			else
				return EXIT_FAILURE;
			fisier = strdup(argv[3]);
			offset = atoi(argv[4]);
			len = atoi(argv[5]);
		}

		server_ip = strdup(argv[1]);
	}
	else
		return EXIT_FAILURE;

	server_socket = new_socket();

	connect_to_server(server_socket, server_ip, SERVER_PORT);
	memset(buffer, 0, MAX_PACKET_SIZE);

	if (op == LS)
		command_ls(server_socket, fisier);
	else if (op == READ)
		command_read(server_socket, fisier, offset, len);
	else if (op == WRITE)
		command_write(server_socket, fisier, offset, len);
	else
		assert(0);

	close_socket(server_socket);

	free(server_ip);
	free(fisier);

	return EXIT_SUCCESS;
}

static void command_ls(int server_socket, char *director)
{
	static char nume_fisier[MAX_STRING_SIZE];
	static int dimensiune_fisier = -1;

	memset(__buffer, 0, MAX_STRING_SIZE);

	buffer[0] = LS;
	sprintf(buffer + 1, "%s", director);

	SEND_AND_RECEIVE(server_socket, buffer);

	assert(buffer[0] == LS);

	int size, off;

	if (buffer[1] == 1)
		MY_PRINT("Serverul a intors ceva != 0\n");
	else
	{
		off = 2;
		while (1)
		{
			size = buffer[off];

			if (size == 0)
				break;

			STRING_CONVERT_REVERSE(buffer, off, size, nume_fisier);
			NUMBER_CONVERT_REVERSE(buffer, off, size, dimensiune_fisier, __buffer);

			printf("%s cu size %d\n", nume_fisier, dimensiune_fisier);
		}
	}
}

static void command_read(int server_socket, char *fisier, int offset, int len)
{
	static int current_size = 1;
	buffer[0] = READ;
	COMMAND_GENERIC(buffer, current_size, fisier, offset, len, __buffer);
	SEND_AND_RECEIVE(server_socket, buffer);
}

static void command_write(int server_socket, char *fisier, int offset, int len)
{
	static int current_size = 1;
	buffer[0] = WRITE;
	COMMAND_GENERIC(buffer, current_size, fisier, offset, len, __buffer);

	assert(fgets(__buffer, len, stdin) != NULL);
	strncpy(buffer + current_size, __buffer, len);

	SEND_AND_RECEIVE(server_socket, buffer);
}
