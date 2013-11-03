Dascalu Laurentiu, 342 C3, Tema 1 SPRC

Introducere
	Aplicatia implementeaza cerintele temei si se comporta ca un shell remote.
Pentru autentificare am folosit UNIX_AUTH, pentru ca nu am reusit sa folosesc DES_AUTH:
	- pe fep authdes_create(servername, 60, NULL, NULL); intoarcea NULL
	- pe local intorcea un pointer valid, dar se bloca intr-un write; am debugat
cu strace si cred ca problema locala era de la ypbind

Detalii de implementare

[ rshell.x ] Interfata comuna dintre client si server contine:
	- metodele exportate de server
		- execute simple command
		- execute command
		- get pwd
	- structurile de date
		- command
		- result
Am folosit structuri imbricate: o comanda contine un array de comenzi simple si
numarul lor.

	Aplicatia are doua componente
1. [rclient.c] Client
	Client-ul este dezvoltat folosind scheletul din enuntul temei.
Etapele executiei sunt:
	- aflu pwd-ul curent al server-ului si-l afisez
	- citesc o comanda de la utilizator
	- parsez comanda si o adaptez la structur Command
	- daca comanda este simpla, apelez metoda execute_simple_command
	- daca nu, apelez metoda execute_command
	- primesc raspuns de la server pe care-l afisez
Separatorul de comenzi peste ";". Pe baza acestui construiesc array-ul de comenzi

2. [rserver.c] Server
	Server-ul foloseste autentificarea UNIX;
	- de fapt, in metodele exportate se folosesc functia check() care este
wrapper pentru metodele de autentificare: am folosit #ifdef pentru a afla cum
e compilat serverul: fara autentificare, cu atentificare UNIX sau cu autentificare
DES (neimplementat, am explicat mai sus ca n-a mers)
	Fiecare metoda apeleaza check() pentru a afla daca utilizatorul are dreptul
de a executa respectiva comanda pe server. In verificare, dau dreptul oricui sa
execute, dar afisez UID-ul user-ului - aici se poate interveni pentru limitarea
accesului.

	Pentru fiecare comanda primita:
	- parcurg lista de comenzi simple si le execut
	- functia de executie este execute_string_command
	- pentru executia unei comenzi si interceptarea output-ului ei
	am folosit popen(); si pentru ce rezultat intoarce pclose();
	- afisez doar primele 1023 de caractere (util pentru securitate si transfer
	avand in vedere ca shell-ul este remote)

