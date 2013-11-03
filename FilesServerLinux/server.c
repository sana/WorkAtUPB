#include "server.h"

static char buffer[MAX_PACKET_SIZE];
static char __buffer[MAX_STRING_SIZE];

#define MY_EPOLL_ADD(ev, epfd, my_fd)\
do\
{\
	memset(&ev, 0, sizeof(struct epoll_event));\
	ev.data.fd = my_fd;\
	ev.events = EPOLLIN;\
	epoll_ctl(epfd, EPOLL_CTL_ADD, my_fd, &ev);\
\
} while(0)

#define MY_EPOLL_REMOVE(ev, epfd, my_fd)\
do\
{\
	memset(&ev, 0, sizeof(struct epoll_event));\
	ev.data.fd = my_fd;\
	ev.events = EPOLLIN;\
	epoll_ctl(epfd, EPOLL_CTL_DEL, my_fd, &ev);\
\
} while(0)

int main()
{
	struct epoll_event ev, rev_ev;
	int epfd = epoll_create(SLOTS);
	int listen_socket = build_server(SERVER_PORT, SLOTS), newsockfd;
	int size, offset, current_size;
	socklen_t len;
	struct sockaddr_in client_addr;
	static char fisier[MAX_STRING_SIZE];

	// Adaug in epoll STDIN
	MY_EPOLL_ADD(ev, epfd, STDIN_FILENO);

	// Adaug in epoll socket-ul pe care ascult conexiuni
	MY_EPOLL_ADD(ev, epfd, listen_socket);

	MY_PRINT("Server is up and running...\n");

	while (1)
	{
		epoll_wait(epfd, &rev_ev, 1, -1);

		if (rev_ev.data.fd == listen_socket && ((rev_ev.events & EPOLLIN) != 0))
		{
			len = sizeof(struct sockaddr_in);

			if ((newsockfd = accept(listen_socket,
					(struct sockaddr *) &client_addr, &len)) == -1)
			{
				perror("accept() failed!");
				exit(1);
			}

			MY_PRINT("New connection from %s %d\n", inet_ntoa(
							client_addr.sin_addr), newsockfd);

			MY_EPOLL_ADD(ev, epfd, newsockfd);
		}
		else if (rev_ev.data.fd == STDIN_FILENO && ((rev_ev.events & EPOLLIN)
				!= 0))
		{
			MY_PRINT("Server in going down....\n");
			break;
		}
		else
		{
			if ((size = my_recv(rev_ev.data.fd, buffer, MAX_PACKET_SIZE, 0))
					<= 0)
			{
				if (size == 0)
					MY_PRINT("Connection from %d terminated\n", rev_ev.data.fd);
				close(rev_ev.data.fd);
				MY_EPOLL_REMOVE(ev, epfd, rev_ev.data.fd);
			}
			else
			{
				if (buffer[0] == LS)
				{
					DIR *dp;
					struct dirent *ep;
					int current_size = 2;

					char *director = strdup(buffer + 1);
					dp = opendir(buffer + 1);

					memset(buffer, 0, MAX_PACKET_SIZE);
					buffer[1] = 1;

					// Codific lista de fisiere in felul urmator:
					// strlen(nume_fisier) + caractere(nume_fisier) +
					// strlen(dimensiune_fisier) + caractere(dimensiune_fisier)
					if (dp != NULL)
					{
						buffer[1] = 0;
						while ((ep = readdir(dp)) != NULL)
						{
							STRING_CONVERT(buffer, current_size, ep->d_name);

							// Dimensiunea maxima, in numar de blocuri, am ales sa fie de 10 cifre.
							static char __block_size[MAX_STRING_SIZE];
							static char __full_name[MAX_STRING_SIZE];
							static struct stat __buf;

							memset(__full_name, 0, MAX_STRING_SIZE);
							sprintf(__full_name, "%s%s", director, ep->d_name);

							assert(stat(__full_name, &__buf) == 0);

							NUMBER_CONVERT(buffer, current_size, (int)__buf.st_size, __block_size);
							MY_PRINT("Adaug %s cu size %ld\n", ep->d_name, __buf.st_size);
						}
						closedir(dp);
					}
					else
						perror("Couldn't open the directory");

					buffer[0] = LS;
					my_send(rev_ev.data.fd, buffer, MAX_PACKET_SIZE, 0);
					free(director);
				}
				else if (buffer[0] == READ)
				{
					current_size = 1;
					COMMAND_GENERIC_REVERSE(buffer, current_size, size, fisier, offset, len, __buffer);
					printf("%s %d %d\n", fisier, offset, len);
					my_send(rev_ev.data.fd, buffer, MAX_PACKET_SIZE, 0);
					buffer[0] = READ;
					buffer[1] = 0;
				}
				else if (buffer[0] == WRITE)
				{
					buffer[0] = WRITE;
					buffer[1] = 1;

					my_send(rev_ev.data.fd, buffer, MAX_PACKET_SIZE, 0);
				}
				else
					assert(0);
			}
		}
	}

	close_socket(listen_socket);

	return 0;
}
