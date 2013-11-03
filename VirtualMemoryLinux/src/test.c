/*
 * test.c - implementare test Tema 5 Sisteme de Operare - Memorie Virtuala
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>
#include <time.h>
#include <signal.h>

#include "tema-mv.h"

#define	INFO	4
#define NOTICE	3
#define ALERT	2
#define URGENT	1
#define	NONE	0

#define DEBUG	ALERT

static int p_sz; /* dimensiunea unei pagini */
static void *start; /* zona de start pentru memoria mapata */

static size_t virt_pages; /* numarul de pagini din memoria virtuala */
static size_t phys_pages; /* numarul de pagini din memoria fizica */

static int ram_fd; /* descriptorul fisierului ce simuleaza memoria RAM */
static int swap_fd; /* descriptorul fisierului ce simuleaza memoria RAM */

static int sig_handler_fault; /* expect page fault - pentru testare */

static struct sigaction oldaction; /* handler-ul din tema */

/* macrodefinitii pentru a indica daca se asteapta sau nu page fault */
#define PAGE_FAULT		1
#define NO_PAGE_FAULT		0
#define NO_CHECK		-1

#define RAM_POISON		0x19820504
#define SWAP_POISON		0x525A564E

#define test(s, t)					\
	do {						\
		int i;					\
							\
	        printf ("test: %s", s);			\
		fflush (stdout);			\
							\
		for (i = 0; i < 60 - strlen(s); i++)	\
			putchar('.');			\
							\
		if (!(t))				\
		        printf ("failed\n");		\
		else					\
			printf ("passed\n");		\
							\
		fflush (stdout);			\
	} while (0)

/*
 * SIGSEGV handler
 */

static void segv_handler(int signum, siginfo_t *info, void *context)
{
	if (signum != SIGSEGV)
		return;
	if (info->si_signo != SIGSEGV)
		return;
	if (info->si_code != SEGV_ACCERR)
		return;

	sig_handler_fault++;

	oldaction.sa_sigaction(signum, info, context);
}

/*
 * set SIGSEGV handler
 */

static void set_signal(void)
{
	struct sigaction action;

	action.sa_sigaction = segv_handler;
	sigemptyset(&action.sa_mask);
	sigaddset(&action.sa_mask, SIGSEGV);
	action.sa_flags = SA_SIGINFO;

	sigaction(SIGSEGV, &action, &oldaction);
}

/*
 * testeaza argumente apeluri exportate de biblioteca
 */

static void test_args(void)
{
	test ("more_phys", vinit (rand () % 100, rand () % 100 + 100) == NULL);
	test ("extra_virt",
			vinit (rand () % 100 + MAX_VIRTUAL_PAGES, rand () % 100) == NULL);
	test ("extra_phys",
			vinit (rand () % 100, rand () % 100 + MAX_PHYSICAL_PAGES) == NULL);

	test ("vend", vend () == -1);
}

static void test_ram_swap(size_t v_pages, size_t p_pages)
{
	struct stat sbuf;

	virt_pages = v_pages;
	phys_pages = p_pages;

	ram_fd = get_ram_fd();
	assert (fstat (ram_fd, &sbuf) == 0);
#if DEBUG >= NOTICE
	printf ("ram size = %ld\n", sbuf.st_size);
#endif
	test ("ram_size", sbuf.st_size == p_sz * p_pages);

	swap_fd = get_swap_fd();
	assert (fstat (swap_fd, &sbuf) == 0);
#if DEBUG >= NOTICE
	printf ("swap size = %ld\n", sbuf.st_size);
#endif
	test ("swap_size", sbuf.st_size == p_sz * v_pages);
}

static void _read(size_t page_id, int expected, size_t offset)
{
	int mval;
	int expect_fault;

	/* read - one possible fault */
	sig_handler_fault = 0;
	expect_fault = (expected == PAGE_FAULT) ? 1 : 0;
	mval = *(int *) ((char *) start + page_id * p_sz + offset);
	if (expected != NO_CHECK)
		test ("access", sig_handler_fault == expect_fault);
}

static void read_write(size_t page_id, int expected, size_t offset)
{
	int mval;
	int expect_fault;

	/* read - one possible fault */
	sig_handler_fault = 0;
	expect_fault = (expected == PAGE_FAULT) ? 1 : 0;
	mval = *(int *) ((char *) start + page_id * p_sz + offset);
	if (expected != NO_CHECK)
		test ("access", sig_handler_fault == expect_fault);

	/* write - another possible fault */
	sig_handler_fault = 0;
	expect_fault = (expected == PAGE_FAULT) ? 1 : 0;
	mval = page_id << 16 | offset;
	*(int *) ((char *) start + page_id * p_sz + offset) = mval;
	if (expected != NO_CHECK)
		test ("access", sig_handler_fault == expect_fault);
}

static void write_read(size_t page_id, int expected, size_t offset)
{
	int mval;
	int expect_fault;

	/* write - two faults */
	sig_handler_fault = 0;
	expect_fault = (expected == PAGE_FAULT) ? 2 : 0;
	mval = page_id << 16 | offset;
	*(int *) ((char *) start + page_id * p_sz + offset) = mval;
	if (expected != NO_CHECK)
		test ("access", sig_handler_fault == expect_fault);

	/* read - no faults */
	sig_handler_fault = 0;
	expect_fault = 0;
	mval = *(int *) ((char *) start + page_id * p_sz + offset);
	if (expected != NO_CHECK)
		test ("access", sig_handler_fault == expect_fault);
}

static void poison_swap(size_t vpages)
{
	size_t i;
	int poison;

	poison = SWAP_POISON;
	lseek(swap_fd, 0, SEEK_SET);
	for (i = 0; i < vpages * p_sz / sizeof(int); i++)
		write(swap_fd, &poison, sizeof(int));
}

static void poison_ram_swap(size_t vpages, size_t ppages)
{
	size_t i;
	int poison;

	poison = SWAP_POISON;
	lseek(swap_fd, 0, SEEK_SET);
	for (i = 0; i < vpages * p_sz / sizeof(int); i++)
		write(swap_fd, &poison, sizeof(int));

	poison = RAM_POISON;
	lseek(ram_fd, 0, SEEK_SET);
	for (i = 0; i < ppages * p_sz / sizeof(int); i++)
		write(ram_fd, &poison, sizeof(int));
}

static void check_ram0(size_t ppages)
{
	size_t read_val;
	int i;
	int check = 1;

	lseek(ram_fd, 0, SEEK_SET);
	for (i = 0; i < ppages * p_sz / 4; i++)
	{
		read(ram_fd, &read_val, 4);
		if (read_val != 0)
		{
			check = 0;
			break;
		}
	}

	test ("ram_clean", check);
}

static void check_ram(size_t offset, size_t ppages, size_t checks)
{
	size_t i, j;
	size_t read_val;
	int exp_val;
	int check = 1;

	for (i = 0; i < ppages; i++)
	{
		lseek(ram_fd, i * p_sz + offset, SEEK_SET);
		for (j = 0; j < checks; j++)
		{
			exp_val = (i << 16) | (offset + j * 4);
			read(ram_fd, &read_val, 4);

			if (exp_val != read_val)
				check = 0;
			/* you gotta love them binary ops :-D */
			/* check &= !(exp_val ^ read_val); */
		}
	}

	test ("check_ram", check);
}

static void check_ram_poison(size_t offset, size_t ppages, size_t checks)
{
	size_t i, j;
	size_t read_val;
	size_t exp_val;
	size_t page_id;
	int check = 1;
	int poison_count = 0;

	for (i = 0; i < ppages; i++)
	{
		lseek(ram_fd, i * p_sz + offset, SEEK_SET);
		for (j = 0; j < checks; j++)
		{
			read(ram_fd, &read_val, 4);
			if (j == 0)
			{
				page_id = read_val >> 16;
			}
			exp_val = (page_id << 16) | (offset + j * 4);

			if (exp_val != read_val && read_val != SWAP_POISON)
			{
				check = 0;
				goto end;
			}
			if (read_val == SWAP_POISON)
				poison_count++;
		}
	}

	end:
	test ("check_ram_poison", check && poison_count);
}

static void check_swap0(size_t vpages)
{
	size_t read_val;
	int i;
	int check = 1;

	lseek(swap_fd, 0, SEEK_SET);
	for (i = 0; i < vpages * p_sz / 4; i++)
	{
		read(swap_fd, &read_val, 4);
		if (read_val != SWAP_POISON)
		{
			check = 0;
			break;
		}
	}

	test ("swap_clean", check == 1);
}

static void check_swap(size_t offset, size_t vpages, size_t ppages,
		size_t checks, int have_count)
{
	size_t *mapped_vpages;
	size_t read_val;
	size_t page_id;
	int check = 1;
	size_t swap_count;
	size_t i, j, k;

	mapped_vpages = (size_t *) malloc(sizeof(size_t) * ppages);
	assert (mapped_vpages != NULL);

	for (i = 0; i < ppages; i++)
	{
		lseek(ram_fd, i * p_sz + offset, SEEK_SET);
		for (j = 0; j < checks; j++)
		{
			read(ram_fd, &read_val, 4);
			if (j == 0)
			{
				page_id = read_val >> 16;
				mapped_vpages[i] = page_id;
			}
			check &= !(page_id ^ (read_val >> 16));
			check &= !((offset + j * 4) ^ (read_val & 0x0000FFFF));
		}
	}

	test ("swap_ram_precheck", check);

	check = 1;
	swap_count = 0;

	for (i = 0; i < vpages; i++)
	{
		lseek(swap_fd, i * p_sz + offset, SEEK_SET);
		read(swap_fd, &read_val, 4);
		if (read_val == SWAP_POISON)
			continue;

		swap_count++;
		lseek(swap_fd, i * p_sz + offset, SEEK_SET);
		for (j = 0; j < checks; j++)
		{
			read(swap_fd, &read_val, 4);
			if (j == 0)
			{
				page_id = read_val >> 16;
			}
			for (k = 0; k < ppages; k++)
				if (page_id == mapped_vpages[k] && have_count != 0)
					check = 0;
			check &= !(page_id ^ (read_val >> 16));
			check &= !((offset + j * 4) ^ (read_val & 0x0000FFFF));
		}
	}

	test ("swap_check", check);
	if (have_count != 0)
		test ("swap_count", swap_count == have_count);

	free(mapped_vpages);
}

static void check_end(void)
{
	struct stat sbuf;
	struct sigaction act;

	test ("ram_closed", fstat (ram_fd, &sbuf) == -1);
	test ("swap_closed", fstat (swap_fd, &sbuf) == -1);

	act.sa_handler = SIG_DFL;
	sigemptyset(&act.sa_mask);
	sigaction(SIGSEGV, &act, &oldaction);

	test ("sig_closed", oldaction.sa_sigaction == SIG_DFL);
}

static void run_test1(void)
{
	start = vinit(MAX_VIRTUAL_PAGES - 1, MAX_PHYSICAL_PAGES - 1);
	test ("init1", start != NULL);

	test_ram_swap(MAX_VIRTUAL_PAGES - 1, MAX_PHYSICAL_PAGES - 1);

	sig_handler_fault = 0;
	set_signal();

	test ("vend1", vend () == 0);
	check_end();
}

static void run_test2(void)
{
	size_t rand_pos;

	start = vinit(4, 3);
	test ("init2", start != NULL);

	test_ram_swap(4, 3);
	poison_ram_swap(4, 3);

	rand_pos = (rand() % (p_sz / 4 - 2)) * 4;
#if DEBUG >= INFO
	printf ("rand_pos = %d\n", rand_pos);
#endif

	sig_handler_fault = 0;
	set_signal();

	read_write(0, PAGE_FAULT, rand_pos);
	write_read(0, NO_PAGE_FAULT, rand_pos + 4);
	read_write(0, NO_PAGE_FAULT, rand_pos + 8);

	write_read(1, PAGE_FAULT, rand_pos);
	read_write(1, NO_PAGE_FAULT, rand_pos + 4);
	write_read(1, NO_PAGE_FAULT, rand_pos + 8);

	read_write(2, PAGE_FAULT, rand_pos);
	write_read(2, NO_PAGE_FAULT, rand_pos + 4);
	read_write(2, NO_PAGE_FAULT, rand_pos + 8);

	ram_sync();
	check_ram(rand_pos, 3, 3);

	write_read(3, PAGE_FAULT, rand_pos);
	read_write(3, NO_PAGE_FAULT, rand_pos + 4);
	write_read(3, NO_PAGE_FAULT, rand_pos + 8);

	ram_sync();
	check_swap(rand_pos, 4, 3, 3, 1);

	test ("vend2", vend () == 0);
	check_end();
}

static void run_test3(void)
{
	size_t i;
	size_t rand_pos;

	start = vinit(6, 3);
	test ("init3", start != NULL);

	test_ram_swap(6, 3);
	poison_ram_swap(6, 3);

	sig_handler_fault = 0;
	set_signal();

	rand_pos = (rand() % (p_sz / 4 - 2)) * 4;

	for (i = 0; i < 3; i++)
	{
		_read(i, PAGE_FAULT, rand_pos);
		_read(i, NO_PAGE_FAULT, rand_pos + 4);
	}

	ram_sync();
	check_ram0(3);

	for (i = 3; i < 6; i++)
	{
		_read(i, PAGE_FAULT, rand_pos);
		_read(i, NO_PAGE_FAULT, rand_pos + 4);
	}

	ram_sync();
	check_ram0(3);
	check_swap0(6);

	test ("vend3", vend () == 0);
	check_end();
}

static void run_test4(void)
{
	size_t i;
	size_t rand_pos;

	start = vinit(8, 5);
	test ("init4", start != NULL);

	test_ram_swap(8, 5);
	poison_ram_swap(8, 5);

	sig_handler_fault = 0;
	set_signal();

	rand_pos = (rand() % (p_sz / 4 - 2)) * 4;

	for (i = 0; i < 5; i++)
	{
		read_write(i, PAGE_FAULT, rand_pos);
		write_read(i, NO_PAGE_FAULT, rand_pos + 4);
		read_write(i, NO_PAGE_FAULT, rand_pos + 8);
	}

	ram_sync();
	check_ram(rand_pos, 5, 3);
	check_swap0(8);

	for (i = 5; i < 8; i++)
	{
		_read(i, PAGE_FAULT, rand_pos);
		_read(i, NO_PAGE_FAULT, rand_pos + 4);
	}

	for (i = 0; i < 8; i++)
	{
		write_read(i, NO_CHECK, rand_pos);
		read_write(i, NO_CHECK, rand_pos + 4);
		write_read(i, NO_CHECK, rand_pos + 8);
	}

	ram_sync();
	check_swap(rand_pos, 8, 5, 3, 0);

	poison_swap(8);
	for (i = 0; i < 8; i++)
	{
		poison_swap(8);
		_read(i, NO_CHECK, rand_pos);
		_read(i, NO_CHECK, rand_pos + 4);
		_read(i, NO_CHECK, rand_pos + 8);
	}

	ram_sync();
	check_ram_poison(rand_pos, 5, 3);

	test ("vend4", vend () == 0);
	check_end();
}

/*
 * run tests - ruleaza testele
 */

static void run_tests(void)
{
	srand(time(NULL));

	run_test1();
	puts("");
	run_test2();
	puts("");
	run_test3();
	puts("");
	run_test4();
}

int main(void)
{
	/* obtinem dimensiunea unei pagini */
	p_sz = getpagesize();

	/* intercepteaza semnal SIGSEGV */
	set_signal();

	test_args();
	puts("");

	run_tests();

	return 0;
}
