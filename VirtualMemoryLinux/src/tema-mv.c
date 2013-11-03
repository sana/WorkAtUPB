/*
 * Dascalu Laurentiu, 335CA.
 * Tema 3 SO - Memoria Virtuala
 * Fisierul de implementare a interfetei tema-vm.h.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

#include "tema-mv.h"

#define CHECK(x) \
	do { \
		if (!(x)) { \
			perror(#x); \
			exit(1); \
		} \
	} while(0)

#define RAM_FILENAME           "__ram"
#define SWAP_FILENAME          "__swap"

typedef struct _page_record
{
	// Identificatorul real al paginii - in ce pagina este mapat in memorie
	int id_real;

	// Identificatorul de pe swap - folosit in cazul paginilor not dirty
	int id_swap;

	/*
	 * Informatia asociata unei pagini virtuale: nealocata, in RAM cu drept de READ,
	 * in RAM cu drept de WRITE, pe SWAP.
	 */
	char info;

	// Pagina a fost sau nu modificata
	char dirty;

	// Pagina a fost in swap cel putin o data
	int swaped;
} page_record;

page_record *pages_info;

// Numarul maxim de pagini fizica (ram), rezidente in memorie
static int __ram_pages = -1;

// Numarul maxim de pagini fizice pe disc (swap), rezidente pe disc
static int __swap_pages = -1;

// Descriptorii de fisier
static int __fd_ram = -1;
static int __fd_swap = -1;

// Variabila care determina daca biblioteca a fost initializata sau nu
static char __vm_init__ = 0;

// Dimensiunea unei pagini de memorie
static int __page_size = -1;

static struct sigaction old_action;

static char *__virt_mem = NULL;

#define NONE      (0)
#define RAM_READ  (1)
#define RAM_WRITE (2)
#define SWAP      (3)

static void sigsegv_handler(int signum, siginfo_t *info, void *context);
static void set_signal();
static void restore_signal();

static void swap_data(int ram_id, int swap_id, int exists_in_swap);
static void swap_data_swap(int ram_page_id, int swap_page_id);

// Intoarce identificatorul unei pagini libere din RAM/SWAP,
// daca exista si -1 daca nu.
static int search_free_page_ram();
static int search_free_page_swap();
static int search_page_id_ram(int page_real);
static int search_page_id_swap(int id_swap);

char *buffer1, *buffer2, *buffer;

void debug()
{
	int i;
	printf("id \t id_real \t id_swap\n");
	for (i = 0; i < __ram_pages + __swap_pages; i++)
		printf("%d\t\t%d\t\t%d\n", i, pages_info[i].id_real,
				pages_info[i].id_swap);
	printf("\n\n");
}

void *vinit(size_t virt_pages, size_t phys_pages)
{
	if (virt_pages > MAX_VIRTUAL_PAGES || phys_pages > MAX_PHYSICAL_PAGES
			|| phys_pages > virt_pages)
	{
		/*
		 * Daca numarul de pagini virtuale/fizice depaseste dimensiunea maxima
		 * admisa a bibliotecii atunci functia esueaza si intoarce NULL.
		 * Functia intoarce NULL si daca numarul de pagini fizice este
		 * mai mare decat numarul de pagini virtuale.
		 */
		return NULL;
	}

	int i;
	__page_size = getpagesize();
	buffer = calloc(__page_size, sizeof(char));

	// Retin in variabilele globale numarul de pagini virtuale si de pagini fizice
	__ram_pages = phys_pages;
	__swap_pages = virt_pages;

	__fd_ram = open(RAM_FILENAME, O_CREAT | O_TRUNC | O_RDWR, 0644);
	CHECK(__fd_ram != -1);

	__fd_swap = open(SWAP_FILENAME, O_CREAT | O_TRUNC | O_RDWR, 0644);
	CHECK(__fd_swap != -1);

	CHECK(ftruncate(__fd_ram, __ram_pages * __page_size) == 0);
	CHECK(ftruncate(__fd_swap, __swap_pages * __page_size) == 0);

	for (i = 0; i < __ram_pages; i++)
		write(__fd_ram, buffer, __page_size);

	for (i = 0; i < __swap_pages; i++)
		write(__fd_swap, buffer, __page_size);

	lseek(__fd_ram, 0, SEEK_SET);
	lseek(__fd_swap, 0, SEEK_SET);

	__virt_mem = mmap(NULL, __swap_pages * __page_size, PROT_NONE, MAP_SHARED
			| MAP_ANONYMOUS, -1, 0);
	CHECK(__virt_mem != (void *) -1);

	set_signal();

	pages_info = malloc((__swap_pages + __ram_pages)
			* sizeof(struct _page_record));
	CHECK(pages_info != NULL);

	memset(pages_info, 0, (__swap_pages + __ram_pages)
			* sizeof(struct _page_record));

	for (i = 0; i < __swap_pages + __ram_pages; i++)
		pages_info[i].id_real = pages_info[i].id_swap = -1;

	CHECK((buffer1 = malloc(__page_size * sizeof(char))) != NULL);
	CHECK((buffer2 = malloc(__page_size * sizeof(char))) != NULL);

	__vm_init__ = 1;

	return __virt_mem;
}

int get_ram_fd(void)
{
	return __fd_ram;
}

int get_swap_fd(void)
{
	return __fd_swap;
}

void ram_sync(void)
{
	// wrapper peste msync
	int i;

	for (i = 0; i < __swap_pages + __ram_pages; i++)
		if (pages_info[i].info != NONE)
			CHECK(msync(__virt_mem + i *__page_size, __page_size, MS_SYNC) == 0);
}

int vend(void)
{
	// Daca biblioteca nu a fost initializata atunci
	// raportez eroarea si intorc -1.
	if (!__vm_init__)
		return -1;

	restore_signal();

	if (__fd_ram != -1)
		close(__fd_ram);
	if (__fd_swap != -1)
		close(__fd_swap);

	free(pages_info);
	free(buffer1);
	free(buffer2);
	free(buffer);

	munmap(__virt_mem, __ram_pages * __page_size);

	return 0;
}

static void sigsegv_handler(int signum, siginfo_t *info, void *context)
{
	char *addr = (char*) info->si_addr;

	// semnalul trebuie sa fie sigsegv
	if (signum != SIGSEGV)
	{
		old_action.sa_sigaction(signum, info, context);
		return;
	}

	int page = (addr - __virt_mem) / __page_size;

	/*
	 * Cand primesc acces invalid pe o pagina execut urmatoarele :
	 * 		- verific daca este mapata in RAM:
	 * 			- daca nu este:
	 * 				- o caut in swap si daca o gasesc atunci fac swap
	 * 				- daca nu o gasesc in swap atunci o mapez in memoria
	 * 				fizica si fac swap (daca e cazul)
	 * 			- daca nu, atunci schimb drepturile in WRITE, stiu ca initial
	 * 				avea drept de READ
	 */

	// Daca pagina nu este in limitele bibliotecii atunci
	// apelez old_actionul cu parametri primiti.
	if (page < 0 || (page >= __ram_pages + __swap_pages))
	{
		old_action.sa_sigaction(signum, info, context);
		return;
	}
	if (pages_info[page].info == NONE)
	{
		// Pagina nu se afla in memoria virtuala si nici in swap
		int page_id = search_free_page_ram();

		// Daca, in RAM, sunt ocupate toate paginile
		// atunci incep sa folosesc swap-ul.
		if (page_id == -1)
		{
			page_id = search_free_page_swap();
			swap_data(search_page_id_ram(0), page_id, 0);
		}
		else
		{
			/*
			 * Mapez pagina virtuala _*page*_ peste
			 * pagina fizica _*page_id*_.
			 */
			pages_info[page].id_real = page_id;
			pages_info[page].id_swap = -1;
			pages_info[page].info = RAM_READ;

			lseek(__fd_ram, page_id * __page_size, SEEK_SET);
			write(__fd_ram, buffer, __page_size);

			CHECK(mprotect(__virt_mem + page * __page_size, __page_size, PROT_READ) == 0);

			CHECK(mmap(__virt_mem + page * __page_size, __page_size,
							PROT_READ, MAP_FIXED | MAP_SHARED,
							__fd_ram, page_id * __page_size) != (void *) -1);
		}
		pages_info[page].dirty = 0;
	}
	else if (pages_info[page].info == RAM_READ)
	{
		// Schimb dreptul paginii in READ
		pages_info[page].info = RAM_WRITE;
		pages_info[page].dirty = 1;
		CHECK(mprotect(__virt_mem + page * __page_size, __page_size, PROT_WRITE) == 0);
	}
	else if (pages_info[page].info == SWAP)
		// Pagina se afla in swap si o aduc in memoria virtuala
		swap_data_swap(search_page_id_ram(0), page);
}

static void set_signal()
{
	struct sigaction action;

	action.sa_sigaction = sigsegv_handler;
	sigemptyset(&action.sa_mask);
	sigaddset(&action.sa_mask, SIGSEGV);
	action.sa_flags = SA_SIGINFO;

	CHECK(sigaction(SIGSEGV, &action, &old_action) != -1);
}

static void restore_signal()
{
	struct sigaction action;

	action.sa_sigaction = old_action.sa_sigaction;
	sigemptyset(&action.sa_mask);
	sigaddset(&action.sa_mask, SIGSEGV);
	action.sa_flags = SA_SIGINFO;

	CHECK(sigaction(SIGSEGV, &action, NULL) != -1);
	signal(SIGSEGV, SIG_DFL);
}

static int search_free_page_ram()
{
	int i;
	for (i = 0; i < __ram_pages; i++)
		if (pages_info[i].info == NONE)
			return i;
	return -1;
}

static int search_free_page_swap()
{
	int i;

	for (i = 0; i < __swap_pages; i++)
	{
		if (pages_info[i + __ram_pages].info == NONE)
			return i;
	}
	return -1;
}

static int search_page_id_ram(int page_real)
{
	int i;

	for (i = 0; i < __swap_pages + __ram_pages; i++)
		if (pages_info[i].id_real == page_real)
			return i;
	return -1;
}

static int search_page_id_swap(int id_swap)
{
	int i;

	for (i = 0; i < __swap_pages + __ram_pages; i++)
		if (pages_info[i].id_swap == id_swap)
			return i;
	return -1;
}

// Fac swap intre RAM si SWAP
static void swap_data(int ram_id, int swap_id, int exists_in_swap)
{
	int map_ram_id = -1, map_swap_id = -1;

	pages_info[swap_id + __ram_pages].info = RAM_READ;
	pages_info[swap_id + __ram_pages].id_real = pages_info[ram_id].id_real;
	pages_info[swap_id + __ram_pages].id_swap = -1;
	pages_info[swap_id + __ram_pages].dirty = 0;

	pages_info[ram_id].info = SWAP;
	pages_info[ram_id].id_real = -1;
	pages_info[ram_id].id_swap = swap_id;

	map_swap_id = swap_id + __ram_pages;
	map_ram_id = ram_id;

	if (!pages_info[ram_id].dirty && pages_info[ram_id].swaped)
		pages_info[ram_id].id_swap = pages_info[ram_id].id_real;
	else
	{
		pages_info[ram_id].id_swap = swap_id;
		pages_info[ram_id].swaped |= 1;
	}

	if (!exists_in_swap)
	{
		if (pages_info[map_ram_id].dirty)
		{
			lseek(__fd_ram, map_ram_id * __page_size, SEEK_SET);
			read(__fd_ram, buffer1, __page_size);
		}
		else
		{
			lseek(__fd_swap, map_swap_id * __page_size, SEEK_SET);
			read(__fd_swap, buffer1, __page_size);
		}

		lseek(__fd_swap, map_swap_id * __page_size, SEEK_SET);
		write(__fd_swap, buffer1, __page_size);

		lseek(__fd_ram, map_ram_id * __page_size, SEEK_SET);
		write(__fd_ram, buffer, __page_size);
	}
	else
	{
		if (pages_info[map_ram_id].dirty)
		{
			lseek(__fd_ram, map_ram_id * __page_size, SEEK_SET);
			read(__fd_ram, buffer1, __page_size);
		}
		else
		{
			lseek(__fd_swap, map_swap_id * __page_size, SEEK_SET);
			read(__fd_swap, buffer1, __page_size);
		}

		lseek(__fd_swap, map_swap_id * __page_size, SEEK_SET);
		read(__fd_swap, buffer2, __page_size);

		lseek(__fd_swap, map_swap_id * __page_size, SEEK_SET);
		write(__fd_swap, buffer1, __page_size);

		lseek(__fd_ram, map_ram_id * __page_size, SEEK_SET);
		write(__fd_ram, buffer2, __page_size);
	}

	CHECK(mmap(__virt_mem + ram_id * __page_size, __page_size,
					PROT_NONE, MAP_FIXED | MAP_SHARED,
					__fd_swap, map_swap_id * __page_size) != (void *) -1);

	CHECK(mmap(__virt_mem + (swap_id + __ram_pages) * __page_size, __page_size,
					PROT_READ, MAP_FIXED | MAP_SHARED,
					__fd_ram, map_ram_id * __page_size) != (void *) -1);

	CHECK(mprotect(__virt_mem + ram_id * __page_size, __page_size, PROT_NONE) == 0);
	CHECK(mprotect(__virt_mem + (swap_id + __ram_pages) * __page_size, __page_size, PROT_READ) == 0);
}

static void swap_data_swap(int ram_page_id, int swap_page_id)
{
	int swap_id = pages_info[swap_page_id].id_swap;
	int ram_id = pages_info[ram_page_id].id_real;

	pages_info[swap_page_id].info = RAM_READ;
	pages_info[swap_page_id].id_real = ram_id;
	pages_info[swap_page_id].id_swap = -1;
	pages_info[swap_page_id].dirty = 0;

	pages_info[ram_page_id].info = SWAP;
	pages_info[ram_page_id].id_real = -1;
	pages_info[ram_page_id].id_swap = swap_id;

	if (!pages_info[ram_page_id].dirty && pages_info[ram_page_id].swaped)
		pages_info[ram_page_id].id_swap = pages_info[ram_page_id].id_real;
	else
	{
		pages_info[ram_page_id].id_swap = swap_id;
		pages_info[ram_page_id].swaped |= 1;
	}

	if (pages_info[ram_id].dirty)
	{
		lseek(__fd_ram, ram_id * __page_size, SEEK_SET);
		read(__fd_ram, buffer1, __page_size);
	}
	else
	{
		lseek(__fd_swap, ram_id * __page_size, SEEK_SET);
		read(__fd_swap, buffer1, __page_size);
	}

	lseek(__fd_swap, swap_id * __page_size, SEEK_SET);
	read(__fd_swap, buffer2, __page_size);

	lseek(__fd_swap, swap_id * __page_size, SEEK_SET);
	write(__fd_swap, buffer1, __page_size);

	lseek(__fd_ram, ram_id * __page_size, SEEK_SET);
	write(__fd_ram, buffer2, __page_size);

	CHECK(mmap(__virt_mem + ram_page_id * __page_size, __page_size,
					PROT_NONE, MAP_FIXED | MAP_SHARED,
					__fd_swap, swap_id * __page_size) != (void *) -1);

	CHECK(mmap(__virt_mem + swap_page_id * __page_size, __page_size,
					PROT_READ, MAP_FIXED | MAP_SHARED,
					__fd_ram, ram_id * __page_size) != (void *) -1);

	CHECK(mprotect(__virt_mem + ram_page_id * __page_size, __page_size, PROT_NONE) == 0);
	CHECK(mprotect(__virt_mem + swap_page_id * __page_size, __page_size, PROT_READ) == 0);
}
