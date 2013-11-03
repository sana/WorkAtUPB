#include "os_wrapper.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

int init_sockets()
{
	return 0;
}

int new_socket()
{
	return socket(AF_INET,SOCK_STREAM, 0);
}

int close_socket(int sockfd)
{
	if (shutdown(sockfd, SHUT_RDWR) < 0)
	{
		perror("shutdown");
		return -1;
	}

	return close(sockfd);
}

int connect_to_server(int server_socket, char *ip, int port)
{
	struct sockaddr_in server_addr;
	struct hostent *hent;

	if ((hent = gethostbyname(ip)) == NULL)
	{
		perror("gethostbyname() failed!\n");
		return -1;
	}

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	memcpy(&server_addr.sin_addr.s_addr, hent->h_addr,
			sizeof(server_addr.sin_addr.s_addr));

	if (connect(server_socket, (struct sockaddr *) &server_addr,
			sizeof(struct sockaddr)) < 0)
	{
		perror("connect() failed!\n");
		return -1;
	}

	return 0;
}

int build_server(int port, int slots)
{
	int server_socket;
	struct sockaddr_in server_addr;
	int sock_opt = 1;

	if ((server_socket = socket(AF_INET,SOCK_STREAM, 0)) < 0)
	{
		perror("socket() failed!\n");
		return -1;
	}

	if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &sock_opt,
			sizeof(int)) < 0)
	{
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(server_socket, (struct sockaddr *) &server_addr,
			sizeof(server_addr)) < 0)
	{
		perror("bind() failed!\n");
		return -1;
	}

	if (listen(server_socket, slots) < 0)
	{
		perror("listen() failed!\n");
		return -1;
	}

	return server_socket;
}

int my_send(int socket, const char *buffer, int max_size, int flags)
{
	return send(socket, buffer, max_size, flags);
}

int my_recv(int socket, char *buffer, int max_size, int flags)
{
	return recv(socket, buffer, max_size, flags);
}
