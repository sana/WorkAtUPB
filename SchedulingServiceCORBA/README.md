Dascalu Laurentiu, 342C3
Tema 4 SPRC, Serviciu de planificare bazat pe CORBA

1. Introducere
	Aplicatia rezolva cerintele temei si consta in 3
subprograme: Client, Scheduler si Server. Programul
poate fi rulat prin intermediul unui Makefile si
poate fi testat automat prin scriptul run.sh.

	Fisierul IDL este IScheduler.idl care genereaza
pachetul ISChedulerCORBA. Implementarea temei, care
foloseste pachetul generat, se afla in pachetul
Scheduler. Astfel, am facut o separare intre codul
generat de utilitarele IDL CORBA si codul scris
pentru tema.

2. Detalii de implementare
2.1. Client
	Fisierul de implementare este: Client.java

Algoritmul este:
	Construiesc un pachet care abstractizeaza o comanda.
	while(true)
		raspuns = cere serverului raspuns la cerere
		daca raspunsul este nul atunci
			comanda este in executie
		daca raspunsul nu este nul atunci
			afisez raspunsul primit si ies

Am ales sa ies dupa ce am primit un raspuns valid
de la scheduler din motive de simplitate. Prin testul
descris in sectiunea 3, am aratat cum se pot executa
mai multe comenzi.

2.2. Scheduler
	Fisierele de implementare sunt: Scheduler.java
		si JobMapper.java
	
	Se expune o functie prin care se poate asigna
unui server un id specific. Schedulerul va folosi
acel id pentru a accesa un server.
	
	Se construieste un alt thread: JobMapper,
in care se introduc comenzi si se scot rezultate.
Astfel, am mutat logica de mapping a task-urilor
intr-un alt thread.

Algoritmul de executie a unei comenzi este:
	Se parcurge lista de servere si se determina
serverul cu incarcarea minima (memorie + procesor).
	Se construieste un nou thread SJobThread care
abstractizeaza urmatoarele date: comanda, id-ul comenzii
si serverul pe care se va executa.
	
Bucla principala, in care interactioneaza Schedulerul
si JobMapperul este:
	while(true)
		Muta toate thread-urile SJobThread terminate din
			running in rezultate
		Daca s-au mai facut cereri, atunci creaza
			threaduri pentru fiecare cerere, conform
			algoritmului explicat mai sus, si adauga
			noile entitati in coada de running

2.3. Server
	Fisierul de implementare este: Server.java
	
Algoritmul este:
	Aflu de la Scheduler id-ul server-ului curent.
	Expun functiile de executie a comenzilor sub
	numele de Server${id primit de la scheduler}.
	
	Functiile expuse de serviciu sunt triviale.
	public SJobResult executeJob(SJob job);
	public String getCPU();
	public String getMem();
	
3. Concluzii
	Am creat un script pentru rularea automata unui test
complex: Debugger.java.
	./run.sh - clean, compile si run
	Se creaza un scheduler, 3 servere si 100 de clienti.
Evolutia aplicatiei este consemnata in debug.txt. Astfel,
se pot observa implementarile tuturor cerintelor temei.
	Durata testului este de aproximativ 10 secunde.