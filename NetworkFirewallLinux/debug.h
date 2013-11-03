#ifndef DEBUG_H_
#define DEBUG_H_

#define ON                       1
#define OFF                      0
#define DEBUG                   OFF

#if DEBUG == ON
#define LOG(s)					\
	do {					\
		printk(KERN_DEBUG s "\n");	\
	} while (0)
#define DBG(...)                \
	do {                    \
		printk(LOG_LEVEL __VA_ARGS__);    \
	} while(0)
#else
#define LOG(s)					\
	do {					\
	} while (0)

#define DBG(...)				\
	do {					\
	} while (0)

#endif

#define print_sock_address(addr)		\
	do {					\
		printk(LOG_LEVEL "connection established to "	\
				NIPQUAD_FMT ":%d\n", 		\
				NIPQUAD(addr.sin_addr.s_addr),	\
				ntohs(addr.sin_port));		\
	} while (0)
#endif
