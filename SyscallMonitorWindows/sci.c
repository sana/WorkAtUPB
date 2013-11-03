// Dascalu Laurentiu, 342C3
#include <ntddk.h>
#define _NTDDK_
#include "sci_win.h"

DRIVER_INITIALIZE DriverEntry;
DRIVER_UNLOAD DriverUnload;

#define MY_REQUEST_SYSCALL_INTERCEPT      (1)
#define MY_REQUEST_SYSCALL_RELEASE       (~1)
#define MY_REQUEST_START_MONITOR          (2)
#define MY_REQUEST_STOP_MONITOR          (~2)

// Old service descriptor table
struct std *__ksdt;

// New service descriptor table
struct std *ksdt;

#define MY_REQUEST_SYSCALL_INTERCEPT      (1)
#define MY_REQUEST_SYSCALL_RELEASE       (~1)
#define MY_REQUEST_START_MONITOR          (2)
#define MY_REQUEST_STOP_MONITOR          (~2)

struct PidListStruct
{
	SINGLE_LIST_ENTRY list;
	HANDLE pid;
};

struct syscall_info_t
{
	// Comanzi agregate din userspace
	int cmd;

	// PID-urile proceselor care doresc pornirea/oprirea interceptorului
	SINGLE_LIST_ENTRY pid;

	// Informatii despre vechiul syscall
	void *old_syscall_fptr;
};

struct syscall_info_t *info;
PVOID my_driver;
KSPIN_LOCK lock;
KIRQL      irql;


static NTSTATUS addPID(int syscall_no, HANDLE pid)
{
	struct PidListStruct *entry;

	if (!(entry = ExAllocatePoolWithTag(NonPagedPool, sizeof(*entry), '1gat')))
		return STATUS_NO_MEMORY;

	entry->pid = pid;
	entry->list.Next = NULL;

	PushEntryList(&(info[syscall_no].pid), &entry->list);

	return STATUS_SUCCESS;
}

static NTSTATUS removePID(int syscall_no, HANDLE pid)
{
	SINGLE_LIST_ENTRY *i, *j;
	struct PidListStruct *ple = NULL;
	NTSTATUS ret = STATUS_INVALID_PARAMETER;
 
	for(j = &(info[syscall_no].pid), i = info[syscall_no].pid.Next; i; j = i, i = i->Next)
	{
		ple = CONTAINING_RECORD(i, struct PidListStruct, list);
		if (ple->pid == pid)
		{
			PopEntryList(j);
			ret = STATUS_SUCCESS;
			break;
		}
	}
	
	if (ret == STATUS_SUCCESS)
		ExFreePoolWithTag(ple, 'lp1t');
	return ret;
}

static NTSTATUS searchPID(int syscall_no, HANDLE pid)
{
	SINGLE_LIST_ENTRY *i, *j;
	struct PidListStruct *ple = NULL;
	NTSTATUS ret = STATUS_INVALID_PARAMETER;
 
	for(j = &(info[syscall_no].pid), i = info[syscall_no].pid.Next; i; j = i, i = i->Next)
	{
		ple = CONTAINING_RECORD(i, struct PidListStruct, list);
		if (ple->pid == pid)
		{
			ret = STATUS_SUCCESS;
			break;
		}
	}

	return ret;
}

static NTSTATUS checkPID(int syscall_no, HANDLE pid)
{
	NTSTATUS status;

	// Daca nu gasesc pid-ul curent, caut pid-ul NULL si intorc rezultatul
	status = searchPID(syscall_no, pid);
	if (status != STATUS_SUCCESS)
		status = searchPID(syscall_no, NULL);

	return status;
}

static VOID my_exit_function(HANDLE ParentId, HANDLE ProcessId, BOOLEAN Create)
{
	int i;

	if (!Create)
	{
		// Daca procesul ProcessId s-a terminat, atunci il scot din toate
		// listele syscall-urilor
		for (i = 0 ; i < MY_SYSCALL_NO ; i++)
			removePID(i, ProcessId);
	}
}

void DriverUnload(PDRIVER_OBJECT driver)
{
	WPOFF();
	memcpy(KeServiceDescriptorTable, __ksdt, sizeof(struct std));
	memcpy(KeServiceDescriptorTableShadow, __ksdt, sizeof(struct std));
	WPON();

	ExFreePoolWithTag(__ksdt, '1gat');
	ExFreePoolWithTag(ksdt, '1gat');
	ExFreePoolWithTag(info, '1gat');

	PsSetCreateProcessNotifyRoutine(my_exit_function, TRUE);
}

NTSTATUS DriverEntry(PDRIVER_OBJECT driver, PUNICODE_STRING registry)
{
	my_driver = driver;
	driver->DriverUnload = DriverUnload;
	KeInitializeSpinLock(&lock);

	if (PsSetCreateProcessNotifyRoutine(my_exit_function, FALSE) != STATUS_SUCCESS)
		return STATUS_INVALID_PARAMETER;

	if (!(info = ExAllocatePoolWithTag(NonPagedPool, MY_SYSCALL_NO * sizeof(struct syscall_info_t), '1gat')))
		return STATUS_NO_MEMORY;
	memset(info, 0, MY_SYSCALL_NO * sizeof(struct syscall_info_t));

	WPOFF();
	get_shadow();

	if(!(__ksdt = ExAllocatePoolWithTag(NonPagedPool, sizeof(struct std), '1gat')))
		return STATUS_NO_MEMORY;
	memcpy(__ksdt, KeServiceDescriptorTable, sizeof(struct std));

	if (!(ksdt = ExAllocatePoolWithTag(NonPagedPool, sizeof(struct std), '1gat')))
		return STATUS_NO_MEMORY;
	ksdt->ls = MY_SYSCALL_NO + 1;

	if (!(ksdt->spt = ExAllocatePoolWithTag(NonPagedPool, (MY_SYSCALL_NO + 1) * sizeof(unsigned char), '1gat')))
		return STATUS_NO_MEMORY;
	memcpy(ksdt->spt, __ksdt->spt, __ksdt->ls * sizeof(unsigned char));
	ksdt->spt[MY_SYSCALL_NO] = sizeof(int) + sizeof(int) + sizeof(HANDLE);

	if (!(ksdt->st = ExAllocatePoolWithTag(NonPagedPool, (MY_SYSCALL_NO + 1) * sizeof(void *), '1gat')))
		return STATUS_NO_MEMORY;
	memcpy(ksdt->st, __ksdt->st, __ksdt->ls * sizeof(void *));
	ksdt->st[MY_SYSCALL_NO] = my_syscall;

	memcpy(KeServiceDescriptorTable, ksdt, sizeof(struct std));
	memcpy(KeServiceDescriptorTableShadow, ksdt, sizeof(struct std));
	WPON();

	return STATUS_SUCCESS;  
}

// Daca utilizatorul curent nu este root si nici nu detine task-ul curent
// atunci semnalez un acces nepermis
#define DO_CHECK()\
do\
{\
		if (pid == 0)\
		{\
			if (!UserAdmin())\
				return STATUS_ACCESS_DENIED;\
		}\
		else\
		{\
			if ((status = PsLookupProcessByProcessId(pid, &task)) != STATUS_SUCCESS)\
				return status;\
\
			if ((status = GetCurrentUser(&u1)) != STATUS_SUCCESS)\
				return status;\
\
			if ((status = GetUserOf(pid, &u2)) != STATUS_SUCCESS)\
				return status;\
\
			if (!UserAdmin() && !CheckUsers(u1, u2))\
				return STATUS_ACCESS_DENIED;\
		}\
} while(0)

NTSTATUS interceptor()
{
	int syscall, params, result, i;
	void *old_stack, *new_stack;
	struct log_packet *logp;
	IO_ERROR_LOG_PACKET *ielp;

	_asm mov syscall, eax

	params = KeServiceDescriptorTable[0].spt[syscall];
	
	_asm mov old_stack, ebp
	_asm add old_stack, 8
	_asm sub esp, params
	_asm mov new_stack, esp

	memcpy(new_stack, old_stack, params);
	result = ((NTSTATUS (*)()) info[syscall].old_syscall_fptr)();

	if (checkPID(syscall, PsGetCurrentProcessId()) == STATUS_SUCCESS)
	{
		if (!(logp = ExAllocatePoolWithTag(NonPagedPool,  sizeof(struct log_packet) + params, '1gat')))
			return STATUS_NO_MEMORY;

		logp->pid = PsGetCurrentProcessId();
		logp->syscall = syscall;
		logp->syscall_arg_no = params / sizeof(int);
		logp->syscall_ret = result;
		memcpy(logp->syscall_arg, old_stack, params);

		if (!(ielp = IoAllocateErrorLogEntry(my_driver, sizeof(IO_ERROR_LOG_PACKET) + sizeof(struct log_packet) + params)))
			return STATUS_NO_MEMORY;

		ielp->ErrorCode = 0;
		ielp->DumpDataSize = sizeof(struct log_packet) + params;
		memcpy(ielp->DumpData, logp, ielp->DumpDataSize);
		IoWriteErrorLogEntry(ielp);
	}

	return result;
}

int my_syscall(int cmd, int syscall_no, HANDLE pid)
{
	PEPROCESS task;
	TOKEN_USER *u1, *u2;
	NTSTATUS status;

	if (syscall_no < 0 || syscall_no >= MY_SYSCALL_NO)
		return STATUS_INVALID_PARAMETER;

	if (cmd == REQUEST_SYSCALL_INTERCEPT)
	{
		if (!UserAdmin())
			return STATUS_ACCESS_DENIED;
		if (info[syscall_no].cmd & MY_REQUEST_SYSCALL_INTERCEPT)
			return STATUS_DEVICE_BUSY;

		// Protejez zona critica cu un spin_lock
		KeAcquireSpinLock(&lock, &irql);
		info[syscall_no].cmd |= MY_REQUEST_SYSCALL_INTERCEPT;
		info[syscall_no].old_syscall_fptr = ksdt->st[syscall_no];
		ksdt->st[syscall_no] = interceptor;
		KeReleaseSpinLock(&lock, irql);
	}
	else if (cmd == REQUEST_SYSCALL_RELEASE)
	{
		if (!UserAdmin())
			return STATUS_ACCESS_DENIED;
		if (!(info[syscall_no].cmd & MY_REQUEST_SYSCALL_INTERCEPT))
			return STATUS_INVALID_PARAMETER;

		// Protejez zona critica cu un spin_lock
		KeAcquireSpinLock(&lock, &irql);
		info[syscall_no].cmd &= MY_REQUEST_SYSCALL_RELEASE;
		ksdt->st[syscall_no] = info[syscall_no].old_syscall_fptr;
		KeReleaseSpinLock(&lock, irql);
	}
	else if (cmd == REQUEST_START_MONITOR)
	{
		DO_CHECK();

		if (info[syscall_no].cmd & MY_REQUEST_START_MONITOR)
			return STATUS_DEVICE_BUSY;

		if (!(info[syscall_no].cmd & MY_REQUEST_SYSCALL_INTERCEPT))
			return STATUS_INVALID_PARAMETER;

		// Protejez zona critica cu un spin_lock
		KeAcquireSpinLock(&lock, &irql);
		info[syscall_no].cmd |= MY_REQUEST_START_MONITOR;
		addPID(syscall_no, pid);
		KeReleaseSpinLock(&lock, irql);
	}
	else if (cmd == REQUEST_STOP_MONITOR)
	{
		DO_CHECK();

		if (!(info[syscall_no].cmd & MY_REQUEST_START_MONITOR))
			return STATUS_INVALID_PARAMETER;

		// Protejez zona critica cu un spin_lock
		KeAcquireSpinLock(&lock, &irql);
		if ((status = removePID(syscall_no, pid)) != STATUS_SUCCESS)
		{
			KeReleaseSpinLock(&lock, irql);
			return status;
		}

		info[syscall_no].cmd &= MY_REQUEST_STOP_MONITOR;
		KeReleaseSpinLock(&lock, irql);
	}
	else
		return STATUS_INVALID_PARAMETER;
	return 0;
}
