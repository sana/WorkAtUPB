#ifndef DEBUG_H_
#define DEBUG_H_

#define ON                       1
#define OFF                      0
#define DEBUG                   ON

#if DEBUG == ON
#define LOG(s)					\
	do {					\
		DbgPrint(s "\n");	\
	} while (0)

#define _DBG(...)                \
	do {                    \
		DbgPrint(__VA_ARGS__);    \
	} while(0)
#else
#define LOG(s)					\
	do {					\
	} while (0)

#define _DBG(...)				\
	do {					\
	} while (0)

#endif

#define print_sock_address(addr)		\
	do {					\
		DbgPrint("connection established to "	\
				NIPQUAD_FMT ":%d\n", 		\
				NIPQUAD(addr.sin_addr.s_addr),	\
				ntohs(addr.sin_port));		\
	} while (0)
#endif

/*
 * display an IP address in readable format
 */
#define NIPQUAD(addr) 	\
	((unsigned char *)&(addr))[0], \
	((unsigned char *)&(addr))[1], \
	((unsigned char *)&(addr))[2], \
	((unsigned char *)&(addr))[3]
#define NIPQUAD_FMT	"%u.%u.%u.%u"
