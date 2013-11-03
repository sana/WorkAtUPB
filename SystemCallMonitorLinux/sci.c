/*
 * Dascalu Laurentiu, 342 C3
 * Tema 1 SO2, interceptor de syscall-uri
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/pid.h>
#include <asm/unistd.h>
#include <linux/spinlock.h>
#include <linux/list.h>
#include "sci_lin.h"

MODULE_DESCRIPTION("Sci");
MODULE_AUTHOR("Laurentiu Dascalu");
MODULE_LICENSE("GPL");

#define LOG_LEVEL	KERN_EMERG
#define MY_REQUEST_SYSCALL_INTERCEPT      (1)
#define MY_REQUEST_SYSCALL_RELEASE       (~1)
#define MY_REQUEST_START_MONITOR          (2)
#define MY_REQUEST_STOP_MONITOR          (~2)
#define ROOT_UID                          (0)

#define GET_UID(task)  (task->real_cred->uid)
#define IS_ROOT(task)  (GET_UID(task) == ROOT_UID)

extern void *sys_call_table[];
extern long my_nr_syscalls;

// Functii wrapper peste un spin_lock_t
static void acquire_lock(void);
static void release_lock(void);

void **old_sys_call_table;
DEFINE_SPINLOCK( lock);

struct syscall_params
{
	long ebx, ecx, edx, esi, edi, ebp, eax;
};

struct pid_list
{
	pid_t pid;
	struct list_head list;
};

struct syscall_info_t
{
	// Comanzi agregate din userspace
	int cmd;

	// PID-urile proceselor care doresc pornirea/oprirea interceptorului
	struct list_head pid;

	void *old_syscall;
};

struct syscall_info_t *info;

int have_pid(struct list_head *lh, pid_t pid)
{
	struct list_head *i;

	//printk(LOG_LEVEL "Caut %d", pid);
	acquire_lock();

	list_for_each(i, lh)
	{
		struct pid_list *pl = list_entry(i, struct pid_list, list);
		if (pl->pid == pid)
		{
			release_lock();
			return 1;
		}
	}

	release_lock();
	return 0;
}

static int add_pid(struct list_head *lh, pid_t pid)
{
	struct pid_list *ple;

	//printk(LOG_LEVEL "Adaug %d", pid);
	acquire_lock();

	ple = (struct pid_list *) kmalloc(sizeof(struct pid_list), GFP_KERNEL);

	if (!ple)
	{
		release_lock();
		return -ENOMEM;
	}

	INIT_LIST_HEAD(&ple->list);
	ple->pid = pid;
	list_add(&ple->list, lh);

	release_lock();

	return 0;
}

static int del_pid(struct list_head *lh, pid_t pid)
{
	struct list_head *i, *tmp;
	struct pid_list *ple;

	//printk(LOG_LEVEL "Sterg %d", pid);
	acquire_lock();

	list_for_each_safe(i, tmp, lh)
	{
		ple = list_entry(i, struct pid_list, list);

		if (ple->pid == pid)
		{
			list_del(i);
			kfree(ple);
			release_lock();
			return 0;
		}
	}

	release_lock();
	return -EINVAL;
}

static void destroy_list(struct list_head *lh)
{
	struct list_head *i, *n;
	struct pid_list *ple;

	list_for_each_safe(i, n, lh)
	{
		ple = list_entry(i, struct pid_list, list);
		list_del(i);
		kfree(ple);
	}
}

/*
 * Verific daca un pid este in lista de procese ce intercepteaza
 * un apel de sistem.
 */
static int check_pid_log(long id)
{
	if (info[id].cmd & MY_REQUEST_START_MONITOR)
	{
		// Daca pid-ul este in lista atunci [OK]
		if (have_pid(&(info[id].pid), current->pid))
			return 1;
		// Daca nu, caut daca lista contine pid-ul 0 [OK]
		else if (have_pid(&(info[id].pid), 0))
			return 1;
		return 0;
	}
	return 0;
}

asmlinkage long interceptor(struct syscall_params sp)
{
	long result =
			((long(*)(struct syscall_params sp)) info[sp.eax].old_syscall)(sp);

	if (check_pid_log(sp.eax))
		log_syscall(current->pid, sp.eax, sp.ebx, sp.ecx, sp.edx, sp.esi,
				sp.edi, sp.ebp, result);

	return result;
}

static spinlock_t mr_lock = SPIN_LOCK_UNLOCKED;

asmlinkage long my_syscall(int cmd, int syscall, int pid)
{
	struct task_struct *task;
	long i;

	if (syscall == MY_SYSCALL_NO || syscall == __NR_exit)
		return -EINVAL;
	if (syscall == __NR_exit_group)
	{
		// Parcurg listele de pid-uri per syscall si scot pid-ul
		for (i = 0; i < my_nr_syscalls; i++)
		{
			if (i != (long) my_syscall)
				del_pid(&(info[i].pid), pid);
		}

		return 0;
	}

	if (cmd == REQUEST_SYSCALL_INTERCEPT)
	{
		// Doar utilizatorul privilegiat poate porni interceptarea unui apel de sistem
		if (!IS_ROOT(current))
			return -EPERM;

		if (info[syscall].cmd & MY_REQUEST_SYSCALL_INTERCEPT)
			return -EBUSY;

		info[syscall].cmd |= MY_REQUEST_SYSCALL_INTERCEPT;

		// Salvez vechiul syscall
		//acquire_lock();
		spin_lock_irq(&mr_lock);
		info[syscall].old_syscall = sys_call_table[syscall];
		sys_call_table[syscall] = interceptor;
		spin_unlock_irq(&mr_lock);
		//release_lock();
	}
	else if (cmd == REQUEST_SYSCALL_RELEASE)
	{
		if (!IS_ROOT(current))
			return -EPERM;

		if (!(info[syscall].cmd & MY_REQUEST_SYSCALL_INTERCEPT))
			return -EINVAL;

		//acquire_lock();
		spin_lock_irq(&mr_lock);
		info[syscall].cmd &= MY_REQUEST_SYSCALL_RELEASE;

		// Fac restore la vechiul syscall
		sys_call_table[syscall] = info[syscall].old_syscall;
		spin_unlock_irq(&mr_lock);
		//release_lock();
	}
	else if (cmd == REQUEST_START_MONITOR)
	{
		if (info[syscall].cmd & MY_REQUEST_START_MONITOR)
			return -EBUSY;

		if (!(info[syscall].cmd & MY_REQUEST_SYSCALL_INTERCEPT))
			return -EINVAL;

		if (pid == 0)
		{
			if (!IS_ROOT(current))
				return -EPERM;
		}
		else
		{
			task = pid_task(find_get_pid(pid), PIDTYPE_PID);

			if (!task)
				return -EINVAL;

			// Daca utilizatorul curent nu este root si nici nu detine task-ul curent
			// atunci semnalez un acces nepermis
			if (!IS_ROOT(current) && GET_UID(task) != GET_UID(current))
				return -EPERM;
		}

		add_pid(&(info[syscall].pid), current->pid);
		info[syscall].cmd |= MY_REQUEST_START_MONITOR;
	}
	else if (cmd == REQUEST_STOP_MONITOR)
	{
		if (pid == 0)
		{
			if (!IS_ROOT(current))
				return -EPERM;
		}
		else
		{
			task = pid_task(find_get_pid(pid), PIDTYPE_PID);

			if (!task)
				return -EINVAL;

			// Daca utilizatorul curent nu este root si nici nu detine task-ul curent
			// atunci semnalez un acces nepermis
			if (!IS_ROOT(current) && GET_UID(task) != GET_UID(current))
				return -EPERM;
		}

		if (!(info[syscall].cmd & MY_REQUEST_START_MONITOR))
			return -EINVAL;

		del_pid(&(info[syscall].pid), current->pid);
		info[syscall].cmd &= MY_REQUEST_STOP_MONITOR;
	}

	return 0;
}

static int my_hello_init(void)
{
	int i;
	if (!(old_sys_call_table = kmalloc(my_nr_syscalls * sizeof(void *),
			GFP_ATOMIC)))
		return -ENOMEM;

	if (!(info = kmalloc(my_nr_syscalls * sizeof(struct syscall_info_t),
			GFP_ATOMIC)))
		return -ENOMEM;

	memcpy(old_sys_call_table, sys_call_table, my_nr_syscalls * sizeof(void *));
	memset(info, 0, my_nr_syscalls * sizeof(struct syscall_info_t));
	for (i = 0; i < my_nr_syscalls; i++)
		INIT_LIST_HEAD(&(info[i].pid));

	sys_call_table[MY_SYSCALL_NO] = my_syscall;
	return 0;
}

static void hello_exit(void)
{
	int i;
	memcpy(sys_call_table, old_sys_call_table, my_nr_syscalls * sizeof(void *));

	kfree(old_sys_call_table);

	for (i = 0; i < my_nr_syscalls; i++)
		destroy_list(&(info[i].pid));
	kfree(info);
}

static void acquire_lock(void)
{
	spin_lock(&lock);
}

static void release_lock(void)
{
	spin_unlock(&lock);
}

module_init( my_hello_init);
module_exit( hello_exit);
