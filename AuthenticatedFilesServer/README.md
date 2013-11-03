Tema 3 SPRC

1. Introducere

Dezvoltarea temei s-a facut pornind de la exemplul 6 din laboratorul 4 de SPRC.
Am implementat cerintele de criptare folosind algoritmul DES.

2. Detalii de implementare
	- client.sh, server.sh, post.sh si prerequisited.sh genereaza
certificate si keystore-uri pentru cele 3 entitati: client, server si
server de autentificare
	- Readme.txt - explicatiile exemplului de la care am pornit in
rezolvarea temei

Server de autorizare
	Verifica daca un utilizator poate downloada un fisier sau nu.
Serverul trimite un request catre acesta si va primi un mesaj de tipul
"DENIED" sau "ALLOWED".
	Cripteaza utilizatorii banned folosind algoritmul DES
		encryptionCipher = Cipher.getInstance("DES");
		encryptionCipher.init(Cipher.ENCRYPT_MODE, key);
		decryptionCipher = Cipher.getInstance("DES");
		decryptionCipher.init(Cipher.DECRYPT_MODE, key);
	Primeste cereri de banare a utilizatorilor pentru o perioada TIME
si ii salveaza criptat in fisierul banned_encrypted.

Client
	Clientul se conecteaza la server prin SSL si asteapta input din partea
utilizatorului: list, download, upload sau quit.

	Quit - iesire din sistem
	List - se trimite serverului o cerere si apoi se asteapta primirea
unui raspuns, sub forma unui vector de String-uri: 3*k nume fisier, 3*k+1
identitate proprietar, 3*k+2 departament.
	Upload file - se trimite serverului o cerere prin care este anuntat
ca acest client vrea sa uploadeze un fisier pe server. Modul de operare
este descris in partea de server; utilizatorul poate fi banat.
	Download file - se trimite serverului o cerere prin care ii este
cerut un fisier; serverul verifica drepturile prin consultarea serverului
de autorizare si trimite intr-un flux de char-uri datele catre client.
* Clientul primeste datele si le salveaza intr-un fisier din directorul
downloads. 

Server
	CTRL + C - quit
	Cerere de upload - un client vrea sa uploadeze ceva pe server,
operatia este de download pentru server. Se citesc datele intr-un flux
de char-uri, se convertesc la String si apoi se cauta cuvinte interzise.
Daca apare un cuvant interzis, atunci utilizatorul este banat si i se inchide
conexiunea. Pentru a sterge un user din lista de utilizatori banati, inainte
de expirarea termenului limita, trebuie sters fisierul banned_encrypted
din serverul de autorizare
	Cerere de download - un client vrea sa downloadeze ceva de pe server,
verific drepturile prin consultarea serverului de autorizare si trimit datele
sub forma unui flux de char-uri.
	Cerere de list - un client vrea sa afle inregistrarile de pe server si
dau ca raspuns un vector de string-uri cu semnificatiile pozitiilor explicate
in partea de client. De asemenea, server-ul mentine upload list-ul serializat
intr-un fisier, pentru a-l putea reface in cazul in care aplicatia se opreste.
Astfel, la a doua rulare se porneste de unde programul s-a oprit. Fisierul
serilizat se numeste __upload_list.bin

Exemplu de rulare
     list
     [java] There are available %1% files for download
     [java] File #0
     [java] 	Nume Fisier: test.txt
     [java] 	Proprietar: sana
     [java] 	Departament: IT
     
     download test.txt
     [java] [Client] downloading file test.txt

     quit
     [java] Client is going down....

     upload test.txt
     [java] [Client] uploading file test.txt
