Dascalu Laurentiu, 335CA
Tema 1 SO, Mini-Shell Linux

1. Descriere :
Tema am scris-o in C++ pentru clasele vector, string si stack din STL.

Aplicatia implementeaza cerintele din enuntul temei. Pentru parsare am folosit flex/bison, pus la dispozitie pe site.
Executia comenzilor se reduce la parcurgerea arborelui sintanctic generat de parser
si la incarcarea comenzilor in memorie, in functie de structura lor.

Algoritmul este "simplu" si reprezinta o adaptare a functiei de afisare data ca exemplu :

executa_comenzi(arbore a)
	daca a contine o comanda simplu atunci o execut
	altfel
		switch(operatie)
		{
			fac lucruri dependente de tip-ul operatiei (redirectari etc) si apelez executa_comenzi(a->stanga)
			si executa_comenzi(a->dreapta)
		}

Pentru cresterea portabilitatii am o functie care executa o comanda (fork + execvp); insa portabilitatea nu este
de 100% pentru ca comenzile paralele si redirectarile le fac cu mai multe fork-uri decat exec-uri.

Intre parcurgerea unui nod de arbore si crearea unui proces, am construit o clasa numita MyInternalCommand.
Aceasta retine : commanda, parametrii, fisierele de IN/OUT/ERR si flag-urile de I/O. Pentru executia corecta
a comenzilor am folosit o stiva.

2. Continutul arhivei :
	- fisierele sursa, in directorul src
		- tema1_mini_shell.cpp, contine implementarea propriu-zisa
		- include/tema1_mini_shell.hpp, contine declaratii de clase
		- include/parser.h, interfata cu obiectele obj/parser.tab.o si obj/parser.yy.o
	- Makefile
	- testele publice :
		- directorul inputs cu testele propriu-zise
		- Makefile.checker si README.checker
