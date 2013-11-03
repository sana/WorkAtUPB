Dascalu Laurentiu, 342 C3
Tema 1 SO2

// Functiile de lucru cu listele sunt adaptate din codul laboratorului.
static NTSTATUS addPID(int syscall_no, HANDLE pid);
static NTSTATUS removePID(int syscall_no, HANDLE pid);
static NTSTATUS searchPID(int syscall_no, HANDLE pid);

// Cauta un pid si daca nu exista atunci cauta pid-ul 0
// Intoarce cel mai bun rezultat dintre cele doua cautari
static NTSTATUS checkPID(int syscall_no, HANDLE pid);

// Scoate din fiecare lista de pid-uri, per syscall, pid-ul procesului
// terminat (Create = FALSE)
static VOID my_exit_function(HANDLE ParentId, HANDLE ProcessId, BOOLEAN Create)

// Punctul de intrare in driver
NTSTATUS DriverEntry(PDRIVER_OBJECT driver, PUNICODE_STRING registry);
- Fac initializari ale structurilor folosite in timpul executiei:
 1. my_driver
 2. PsSetCreateProcessNotifyRoutine(my_exit_function, FALSE)
 3. info per syscall

- Salvez vechea tabela de pointeri la functii
 memcpy(__ksdt, KeServiceDescriptorTable, sizeof(struct std));

- Creez o noua tabela
memcpy(ksdt->st, __ksdt->st, __ksdt->ls * sizeof(void *));
[..]
ksdt->st[MY_SYSCALL_NO] = my_syscall;

- Suprascriu datele din kernel cu noua tabela (ce era inainte + MY_SYSCALL_NO)
WPOFF();
memcpy(KeServiceDescriptorTable, ksdt, sizeof(struct std));
memcpy(KeServiceDescriptorTableShadow, ksdt, sizeof(struct std));
WPON();

// Punctul de iesire din driver
void DriverUnload(PDRIVER_OBJECT driver);
- Fac operatiile inverse celor din constructor
 1. eliberez memoria alocata
 2. refac structurile kernel
 3. scot functia de interceptare a terminarii proceselor

// Interceptorul care logheaza informatia
// o parte din cod este luat din cursul 3, PSO 2009 - partea de asamblare
- Verific daca a fost activata logarea si construiesc pachetul cu date
NTSTATUS interceptor();

// Functia principala a temei
- Pentru fiecare tip de comanda fac verificari asupra corectitudinii
parametrilor
- REQUEST_SYSCALL_INTERCEPT si REQUEST_SYSCALL_RELEASE definesc informatiile
din structura info[syscall_no] si modifica in table din kernel un pointer
la functie(se pune interceptorul)
- REQUEST_START_MONITOR si REQUEST_STOP_MONITOR opereaza asupra listei de
pid-uri ce s-au inregistrat sa monitorizez un apel de sistem
int my_syscall(int cmd, int syscall_no, HANDLE pid);

Pentru sincronizare am folosit spin_lock-uri (vezi codul functiei my_syscall()).
Denumirile variabilelor sunt sugestive si n-au nevoie de comentarii suplimentare.