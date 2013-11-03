#ifndef SERVER_H_
#define SERVER_H_

#define SLOTS      100

#include "common.h"
#include "os_wrapper.h"
#include <sys/epoll.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>

#include <linux/types.h>
#include <linux/signalfd.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <dirent.h>

#define SIZEOF_SIG      (_NSIG / 8)
#define SIZEOF_SIGSET   (SIZEOF_SIG > sizeof(sigset_t) ? \
                                  sizeof(sigset_t): SIZEOF_SIG)

#endif /* SERVER_H_ */
