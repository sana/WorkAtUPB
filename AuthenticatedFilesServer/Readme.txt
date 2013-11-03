/**
	Sisteme de programe pentru retele de calculatoare
	
	Copyright (C) 2008 Ciprian Dobre & Florin Pop
	Univerity Politehnica of Bucharest, Romania

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
 */

 
Capitolul V. Securitate in Java.


	Listing 6. Studiu de caz: sistem de chat securizat

Directorul exemplului contine:
	- cate un director pentru fiecare entitate: 
		- certification_authority (informatiile de securitate ale autoritatii de certificare)
		- authorization_server (sursele si, in directorul 'security', informatiile de securitate ale serviciului de autorizare)
		- department_server (sursele si, in directorul 'security' informatiile de securitate ale fiecarui server de departament, in subdirectoare ce poarta numele departamentului asociat) 
		- client (sursele si, in directorul 'security' informatiile de securitate ale fiecarui client, in subdirectoare ce poarta numele clientului respectiv) 
	- o serie de scripturi bash prin care se genereaza si se importa certificatele:
		- prerequisites.sh: creeaza certificatul CA si al serviciului de autorizare, semneaza certificatul SA si importa certificatul CA in SA si reciproc
		- server.sh: creeaza in 'department_server/security' un subdirector cu numele dat ca parametru; in acest subdirector vor fi depuse informatiile de securitate asociate serverului de departament al carui nume a fost dat ca parametru (inclusiv certificatele CA si SA)
		- client.sh: creeaza in 'client/security' un subdirector cu numele dat ca prim parametru; in acest subdirector vor fi depuse informatiile de securitate asociate clientului al carui nume a fost dat ca parametru (inclusiv certificatele CA si SA)
		- post.sh: importa certificatul fiecarui client catre fiecare server, si certificatul fiecarui server de departament catre fiecare client
Parolele, numele keystore-urilor sunt standard si pornesc de la numele clientului/serverului la care se adauga niste sufixe cunoscute.

Structura de directoare contine deja informatiile de securitate pentru 4 departamente si 6 clienti: IT, MANAGEMENT, ACCOUNTING, HUMAN_RESOURCES, 
respectiv: gigi (apartine departamentului IT), goe (MANAGEMENT) si gogu (ACCOUNTING).

Prioritatile sunt specificate in cod: HUMAN_RESOURCES si ACCOUNTING au prioritatea 1, IT are prioritatea 2, iar MANAGEMENT are prioritatea 3.

Serverul de autorizare asculta cereri de conectare din partea serverelor de departament pe portul standard (cunoscut) 7000.

Target-ul 'run' din fisierul 'build.xml' asociat serverului de departament duce la instantierea unui server de departament, 
asteptand 3 parametri din linia de comanda: numele departamentului (care default e "IT"), portul pe care serverul de departament asculta cereri 
din partea clientilor (default, 7001), hostname-ul serverului de autorizare (default, localhost). IMPORTANT: Pentru instantierea unui server 
de departament cu argumentele default e suficient: 'ant run'
IMPORTANT: Pentru instantierea unui server de departament cu argumente noi: 'ant run -Dname=nume_departament -Dport=port_nou 
-Dhostname=hostname_SA'

Target-ul 'run' din fisierul 'build.xml' asociat clientului duce la instantierea unui client, asteptand 4 parametri din linia de comanda: 
numele clientului (care default e "gigi"), numele departamentului din care face parte acest client (default, "IT"), hostname-ul serverului de 
departament la care sa se conecteze (default, localhost), portul pe care serverul de departament la care se incearca o conectare asculta cereri 
din partea clientilor (default, 7001).
IMPORTANT: Pentru instantierea unui client cu argumentele default e suficient: 'ant run'
IMPORTANT: Pentru instantierea unui client cu argumente noi: 'ant run -Dname=nume_client -Ddepartment=nume_departament -Dhostname=hostname_SA 
-Dport=port_nou'


Detalii de implementare:
		- serverul de departament accepta blocant cereri de conectare venite din partea unor clienti; daca certificatele serverului si al 
		clientului acceptat sunt emise de aceeasi CA, se trimite un mesaj serverului de autorizare pentru a verifica daca respectivul client poate 
		sa foloseasca respectivul server de chat (prioritatea departamentului din care face parte clientul e mai mare sau egala cu prioritatea 
		departamentului serverului, clientul respectiv nu se afla in lista clientilor blocati)
		- dupa autorizarea unui client, comunicatia cu acesta se face intr-un thread separat ('ConnectionHandler'); un obiect de tip 
		'ConnectionHandler' trimite pe socket-ul clientului asociat toate mesajele care se transmit pe server si transmite tuturor celorlalti 
		clienti conectati pe server mesajele trimise de clientul asociat; daca in mesajele trimise de client apare cuvantul "bomba", clientului 
		respectiv i se trimite un mesaj standard si conexiunea cu acesta e inchisa; totodata, e trimis serverului de autorizare un mesaj prin care
		 se comanda includerea clientului respectiv in fisierul clientilor blocati
		- serverul de autorizare modifica fisierul 'banned_encrypted' astfel incat sa includa si clientul tocmai blocat; asociaza apoi un 
		timer la expirarea caruia numele clientului e sters din 'banned_encrypted'

		