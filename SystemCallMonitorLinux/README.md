Laurentiu Dascalu, 342C3
Tema 1 SO2

1. Detalii de implementare

Pentru fiecare apel de sistem, retin intr-o variabile urmatoarele informatii:
struct syscall_info_t
{
	// Comanzi agregate din userspace
	int cmd;

	// PID-urile proceselor care doresc pornirea/oprirea interceptorului
	struct list_head pid;

	// Adresa vechiului apel de sistem
	void *old_syscall;
};

Cand primesc o cerere valida de monitorizare, adaug pid-ul curent in lista de pid-uri
ce monitorizeaza apelul de sistem referit. Cererea de renuntare la monitorizare scoate
pid-ul din lista.

Cand un proces iese, atunci il sterg din toate listele de pid-uri
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

Functiile de lucru cu listele este:
// Cauta un pid intr-o lista de pid-uri 
static int have_pid(struct list_head *lh, pid_t pid);

// Adauga un pid intr-o lista
static int add_pid(struct list_head *lh, pid_t pid);

// Sterge un pid dintr-o lista
static int del_pid(struct list_head *lh, pid_t pid);

In functia interceptor, verific daca pid-ul curent a cerut
monitorizarea unui apel de sistem:

if (check_pid_log(sp.eax))
	log_syscall();
	
Functia check_pid cauta pid-ul in lista de pid-uri al
syscall-ului si daca nu exista, atunci cauta pid-ul 0;
intoarce 1 daca a gasit cel putin un pid si 0 daca nu.

Functia principala
asmlinkage long my_syscall(int cmd, int syscall, int pid);
- Pentru fiecare comanda verific daca parametri sunt valizi
- REQUEST_SYSCALL_INTERCEPT si REQUEST_SYSCALL_RELEASE modifica
informatiile despre comanda atasata unui syscall si pointerul
la functie ce va fi intercepta un apel de sistem
- REQUEST_START_MONITOR si REQUEST_STOP_MONITOR adauga pid-ul
primit ca parametru in lista de piduri ce monitorizeaza apelul
de sistem curent.

// Entry point-ul driverului
static int my_hello_init(void);
- Aloc memorie, salvez vechea tabela de syscall-uri si fac initializari

// Exit point-ul driverului
static void hello_exit(void);
- Eliberez memoria si refac vechea structura de syscall-uri

Observatie: pid-ul 0 se adauga ca un pid normal si este folosit atunci
cand el apare intr-o lista de pid-uri, iar pid-ul curent nu. Astfel, pentru
acest caz, cautarea intoarce SUCCESS.

2. Posibile probleme intampinate

Am comentat o bucata de cod care foloseste direct spin_lock-uri; la un stress test,
kernelul imi crapa. Am mutat codul intr-o functie si totul a fost ok. Problema cred
ca este un race condition undeva; in curs scrie ca un race este posibil.
