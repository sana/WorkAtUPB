Dascalu Laurentiu, 335CA, Tema 2 SO - Mecanisme IPC

Tema implementeaza cerinta temei de rezolvare a unei probleme folosind 3 mecanisme IPC : memorie partajata,
semafoare si cozi de mesaje.

Implementarea este compusa din doua module :

	- server
		Server-ul initiaza memoria partajata pe care se tine hash-table-ul; semafoarele, initial setate pe 0;
	coada de mesaje, prin care primeste cereri de la clienti. In functie de tipul pachetului primit server-ul
	actioneaza : adauga in hash-table un cuvant, sterge din hash-table un cuvant, sterge tot continutul
	hash-tablelului sau isi intrerupe activitatea.
	
	Pe memoria partajata se tine hash-table-ul si dimensiunea este egala cu produsul celor 3 dimensiuni :
	numarul de bucket-uri, numarul de cuvinte dintr-un bucket si numarul de caractere ale unui cuvant.
	
	Sincronizarea, prin semafoare, o voi descrie intr-un topic separat.
	
	- client
		Client-ul primeste ca parametri in linia de comanda diverse actiuni; intial se trece de la formatul
	comenzilor ca argumente in linia de comanda la pachete ce vor fi trimise server-ului. Pachetele se trimit
	cu ajutorul cozii de mesaje.
	
Sincronizarea se face prin semafoare. Sunt disponibile MAX_BUCKET semafoare ( cate unul pentru fiecare bucket ),
acest lucru este un compromis intre viteza si numarul de semafoare.
	- adaugarea unui cuvant presupune acquire-ul semaforului de pe bucket-ul respectiv. Clientul trimite server-ului
	cererea si face acquire pe semafor. Deoarece semaforul este egal cu 0 ( valoarea initiala ), clientul va astepta
	pana cand server-ul adauga noul cuvant si va face release pe semafor.
	- stergerea unui cuvant este asemanatoare
	- diferenta intre adaugare/stergere si afisare este dat de faptul ca se folosesc toate semafoarele
	pentru sincronizarea accesului. In rest, principiul este acelasi.

La sfarsit se elibereaza resursele alocate; la fiecare pas se verifica codul intors de functii de sistem.

Continutul arhivei :
	Makefile cu target de build si clean
	src - directorul cu fisierele sursa :
			- hash.c
			- client.c
			- server.c
			- directorul cu headere :
				- common.h - interfata comuna a client-ului si a server-ului.
				- hash.h