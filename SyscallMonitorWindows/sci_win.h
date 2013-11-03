#ifndef _TEMA1_H
#define _TEMA1_H

#define REQUEST_START_MONITOR           0
#define REQUEST_STOP_MONITOR            1
#define REQUEST_SYSCALL_INTERCEPT       2
#define REQUEST_SYSCALL_RELEASE         3

int my_syscall (int cmd, int syscall_no, HANDLE pid);

#define MY_SYSCALL_NO			0x200

struct log_packet {
	HANDLE pid;
	int syscall;
	int syscall_arg_no;
	int syscall_ret;
	int syscall_arg[0];
};

#ifdef _NTDDK_

#include <ntddk.h>

/* service table descriptor */
struct std {
	void **st;	/* service table */
	int *ct;		/* counter table */
	int ls;			/* last service no */
	unsigned char *spt;	/* service parameter table */
};

/* descriptors of the system service table */
_declspec(dllimport) struct std KeServiceDescriptorTable[2];
struct std *KeServiceDescriptorTableShadow;

static void get_shadow(void)
{
	KeServiceDescriptorTableShadow=KeServiceDescriptorTable-2;
}
/* turn write protect off */
#define WPOFF() \
	_asm mov eax, cr0 \
	_asm and eax, NOT 10000H \
	_asm mov cr0, eax

/* turn write protect on */
#define WPON() \
	_asm mov eax, cr0 \
	_asm or eax, 10000H \
	_asm mov cr0, eax

#define TOKEN_ASSIGN_PRIMARY            (0x0001)
#define TOKEN_DUPLICATE                 (0x0002)
#define TOKEN_IMPERSONATE               (0x0004)
#define TOKEN_QUERY                     (0x0008)
#define TOKEN_QUERY_SOURCE              (0x0010)
#define TOKEN_ADJUST_PRIVILEGES         (0x0020)
#define TOKEN_ADJUST_GROUPS             (0x0040)
#define TOKEN_ADJUST_DEFAULT            (0x0080)

#define TOKEN_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED |\
                          TOKEN_ASSIGN_PRIMARY     |\
                          TOKEN_DUPLICATE          |\
                          TOKEN_IMPERSONATE        |\
                          TOKEN_QUERY              |\
                          TOKEN_QUERY_SOURCE       |\
                          TOKEN_ADJUST_PRIVILEGES  |\
                          TOKEN_ADJUST_GROUPS      |\
                          TOKEN_ADJUST_DEFAULT)

#define TOKEN_READ       (STANDARD_RIGHTS_READ     |\
                          TOKEN_QUERY)

#define TOKEN_WRITE      (STANDARD_RIGHTS_WRITE    |\
                          TOKEN_ADJUST_PRIVILEGES  |\
                          TOKEN_ADJUST_GROUPS      |\
                          TOKEN_ADJUST_DEFAULT)

#define TOKEN_EXECUTE    (STANDARD_RIGHTS_EXECUTE)

#define TOKEN_SOURCE_LENGTH 8

typedef enum _TOKEN_INFORMATION_CLASS
{
  TokenUser = 1,
  TokenGroups,
  TokenPrivileges,
  TokenOwner,
  TokenPrimaryGroup,
  TokenDefaultDacl,
  TokenSource,
  TokenType,
  TokenImpersonationLevel,
  TokenStatistics,
  TokenRestrictedSids,
  TokenSessionId,
  TokenGroupsAndPrivileges,
  TokenSessionReference,
  TokenSandBoxInert,
  TokenAuditPolicy,
  TokenOrigin
} TOKEN_INFORMATION_CLASS;

typedef struct _SID_AND_ATTRIBUTES {
  PSID Sid;
  int Attributes;
} SID_AND_ATTRIBUTES, *PSID_AND_ATTRIBUTES;

typedef struct _TOKEN_USER {
  SID_AND_ATTRIBUTES User;
} TOKEN_USER, *PTOKEN_USER;

typedef struct _TOKEN_PRIVILEGES {
	ULONG PrivilegeCount;
	LUID_AND_ATTRIBUTES Privileges[1];
} TOKEN_PRIVILEGES, *PTOKEN_PRIVILEGES;

extern NTSTATUS PsLookupProcessByProcessId(HANDLE, PEPROCESS*);
extern ULONG RtlLengthSid(PSID);
extern NTKERNELAPI void ExFreePoolWithTag(PVOID, ULONG);
extern ZwOpenThreadToken(HANDLE thread, ACCESS_MASK am, BOOLEAN utc, HANDLE *token);
extern ZwOpenProcessToken(HANDLE process, ACCESS_MASK am, HANDLE *token);
extern ZwQueryInformationToken(HANDLE token, long tic, void *ti, unsigned long til, unsigned long *rtil);
extern BOOLEAN RtlEqualSid (PSID, PSID);
NTSTATUS ZwOpenProcess (OUT PHANDLE ProcessHandle, IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes, IN PCLIENT_ID ClientId OPTIONAL);
extern NTSTATUS ZwAdjustPrivilegesToken(HANDLE tokenHandle,
		BOOLEAN DisableAllPrivileges,
		PTOKEN_PRIVILEGES NewState,
		ULONG BufferLength,
		PTOKEN_PRIVILEGES PreviousState,
		PULONG ReturnLength);


/* register a new system service table */
extern BOOLEAN KeAddSystemServiceTable(void *base, int *ct, int ls, unsigned char *spt, int index);

static NTSTATUS AdjustPrivilege(ULONG Privilege, BOOLEAN Enable)
{
	NTSTATUS status;
	TOKEN_PRIVILEGES privSet;
	HANDLE tokenHandle;

	status = ZwOpenProcessToken(NtCurrentProcess(),
			TOKEN_ALL_ACCESS,
			&tokenHandle);
	if (!NT_SUCCESS(status)) {
		DbgPrint("NtOpenProcessToken failed, status 0x%x\n", status);
		return status;
	}

	privSet.PrivilegeCount = 1;
	privSet.Privileges[0].Luid = RtlConvertUlongToLuid(Privilege); // SE_LOAD_DRIVER_PRIVILEGE
	if (Enable) {
		privSet.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED; //trebuie pus pek enable
	} else {
		privSet.Privileges[0].Attributes = 0;
	}

	status = ZwAdjustPrivilegesToken(tokenHandle,
		FALSE, // don't disable all privileges
		&privSet,
		sizeof(privSet),
		NULL, // old privileges - don't care
		NULL); // returned length
	if (!NT_SUCCESS(status)) {
		DbgPrint("ZwAdjustPrivilegesToken failed, status 0x%x\n", status);
	}

	// Close the process token handle
	(void) ZwClose(tokenHandle);

	return status;
}

/* check for (high) privileges */
static BOOLEAN UserAdmin()
{
	AdjustPrivilege(SE_LOAD_DRIVER_PRIVILEGE, 1);
        return SeSinglePrivilegeCheck(RtlConvertLongToLuid(SE_LOAD_DRIVER_PRIVILEGE), UserMode);
}

/* check for same user */
static BOOLEAN CheckUsers(TOKEN_USER *u1, TOKEN_USER *u2)
{
	return RtlEqualSid(u1->User.Sid, u2->User.Sid);
}

/* get the user token of the pid specified process */
static NTSTATUS GetUserOf(HANDLE pid, TOKEN_USER **user)
{
	HANDLE process;
	OBJECT_ATTRIBUTES oa;
	HANDLE cid[2];
	HANDLE token;
	NTSTATUS status;
	unsigned long len;

	InitializeObjectAttributes(&oa, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
	cid[0]=pid; cid[1]=0;
	if ((status=ZwOpenProcess(&process, GENERIC_READ, &oa, (PCLIENT_ID)cid)) != STATUS_SUCCESS)
		return status;
	status=ZwOpenProcessToken(process, TOKEN_READ, &token);
	if (status != STATUS_SUCCESS)
		return status;
	ZwQueryInformationToken(token, TokenUser, NULL, 0, &len);
	if (!(*user=ExAllocatePoolWithTag(NonPagedPool, len, 'ot1t'))) {
		status=STATUS_NO_MEMORY;
		goto out;
	}
	if ((status=ZwQueryInformationToken(token, TokenUser, *user, len, &len)) != STATUS_SUCCESS)
		ExFreePoolWithTag(*user, 'ot1t');
out:
	ZwClose(process);
	return status;
}

/* get the user token for the current thread/process */
static NTSTATUS GetCurrentUser(TOKEN_USER **user)
{
	HANDLE token;
	NTSTATUS status;
	unsigned long len;


	if ((status=ZwOpenThreadToken(NtCurrentThread(), TOKEN_READ, TRUE, &token)) != STATUS_SUCCESS)
		status=ZwOpenProcessToken(NtCurrentProcess(), TOKEN_READ, &token);
	if (status != STATUS_SUCCESS)
		return status;
	ZwQueryInformationToken(token, TokenUser, NULL, 0, &len);
	if (!(*user=ExAllocatePoolWithTag(NonPagedPool, len, 'ot1t')))
		return STATUS_NO_MEMORY;
	if ((status=ZwQueryInformationToken(token, TokenUser, *user, len, &len)) != STATUS_SUCCESS)
		ExFreePoolWithTag(*user, 'ot1t');

	return status;
}

#endif

#endif /* _TEMA1_H */
