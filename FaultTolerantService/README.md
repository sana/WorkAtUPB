Dascalu Laurentiu, 342C3
Tema 2 SPRC - Sortare toleranta la defecte

1. Introducere
 Aplicatia raspunde cerintelor formulate in enuntul temei, folosind un algoritm
distribuit implementat in openMPI. Pentru asigurarea tolerantei la defecte am
folosit procese dinamice si un mecanism propriu de serializare/deserializare
a obiectelor.
 Continutul arhivei:
 	Makefile
 	src/
 		debugger.cpp - functii folosite pentru testare
 		manager.cpp - implementarea procesului master
 		operations.cpp - implementarea transferului de task-uri intre procese
 		operations.h - interfata de functii aplicabile asupra Task-urilor
 		task_manager.cpp - implementarea manager-ului de task-uri; fiecare
 			instanta tine evidenta task-urilor ce trebuiesc rezolvate;
 			contine si implementarea task-urilor (SortTask si MergeTask)
 		task_manager.h - interfata functiilor precizate mai sus
 		worker.cpp - implementarea workerului
 In Makefile exista un exemplu in care atat master-ul central, cat si procesele
slave crashuiesc - pentru detalii de implementare citeste mai jos.
 Rulare simpla:
 $ make	# face clean, build si run

2. Detalii de implementare
	Fisierele de intrare si de iesire se dau ca parametri in linia de comanda.
In cele ce urmeaza, explicatiile sunt date pentru comenzile din Makefile.

2.1. Formatul fisierului de intrare
	Formatul fisierului de intrare este binar si este un flux de int-uri.
Generarea lui se face cu ajutorul lui test_gen, compilat din test_gen.c
	for (i = 0; i < test_size; i++)
	{
		key = rand();
		fwrite(&key, sizeof(int), 1, fout);
	}
	Fisierul de intrare este input.bin

2.2. Serializarea/deserializarea obiectelor
	Un TasksManager contine o coada de task-uri pe care le serializeaza scriind:
	serializare manager = [numarul de obiecte | serializare obiect 1 | ... | serializare obiect N]
	Clasa Task are doua functii virtuale pure pentru acest scop: serialize(), deserialize()
ce sunt implementate de clasele SortTask si MergeTask. Obiectele sunt tranduse intr-un flux
de octeti si scrise intr-un fisier.
	serializare obiect = [header | flux octeti]
	Mecanismul invers, deserializarea, construieste managerul pe baza datelor salvate in fisier.

2.3. Formatul datelor de iesire
	Inainte de a iesi, un proces master verifica daca vectorul este sortat corect. Daca nu este,
atunci se arunca o eroare interna; apoi, vectorul este scris in fisierul output.bin dupa
urmatorul format:
	numarul de numere
	cate un numar pe linie

2.4. Algoritmul de functionare
	1. Initial, se spawneaza 2 master: 1 master de baza si unul mirror.
	2. Daca unul din masteri cade, atunci celalalt ii ia locul si toti copii
	spawnati de primul sunt inchisi. Practic, se revine in starea dinainte
	sa cada primul master, dar se folosesc alte procese slaves.
	3. Masterul ia un task din coada si-l trimite unui slave; un slave primeste
	un task, il proceseaza si trimite rezultatul
	Se poate ca un slave sa pice, caz in care master-ul isi da seama de acest
	lucru si reintroduce task-ul in coada. Practic, exista doua cozi: trunk si
	buffered; task-urile trimise la slaves sunt buffered si cand s-a primit
	solutia pentru ele se sterg 
	4. Cat timp numarul de task-uri din coada este > 1 atunci
	executa instructiunile de la pasul 3
	5. In coada a mai ramas un task pe care-l solutioneaza procesul master
	si rezultatul este afisat.

	In cazul in care un master pica, slave-ul a carui parinte a murit se
	inchide si el.
	
	Refacerea starii dinainte de caderea unui master se face folosind
	conceptul de check_point. La fiecare iteratie, server-ul master
	scrie pe disc managerul de task-uri curent (informatii despre toate
	tipurile de task-uri si alte informatii). Cand masterul de baza pica,
	masterul mirror reface coada dinainte de crash si apeleaza functia main_loop().
	Deoarece se lucreaza in iteratii si o iteratie este definita de starea cozii,
	se poate afirma ca master-ul mirror reia calculele de unde a ramas masterul
	de baza; se pierd procesele slaves si calculele facute de acestea.
	
3. Concluzii
	Functiile oferite de MPI pentru toleranta la defecte sunt cam complicate
si nu am reusit sa implementez unele lucruri, spre exemplu:
	- MPI_Comm_accept si MPI_Comm_connect mergeau in exemplu separat
	dar cand am integrat codul in tema nu mai mergeau
	(strace zicea ca se face epoll pe niste fd-uri deci nu se putea conecta
	clientul la server), asa ca am abandonat varianta optima in care
	slaves se conectau la celalalt master si isi continuau lucrul
	
3.1.
	Codul nu compileaza pe fep, tool-urile folosite de mine sunt:
$ mpic++ -v
Target: i486-linux-gnu
Configured with: ../src/configure -v --with-pkgversion='Ubuntu 4.4.1-4ubuntu8' --with-bugurl=file:///usr/share/doc/gcc-4.4/README.Bugs --enable-languages=c,c++,fortran,objc,obj-c++ --prefix=/usr --enable-shared --enable-multiarch --enable-linker-build-id --with-system-zlib --libexecdir=/usr/lib --without-included-gettext --enable-threads=posix --with-gxx-include-dir=/usr/include/c++/4.4 --program-suffix=-4.4 --enable-nls --enable-clocale=gnu --enable-libstdcxx-debug --enable-objc-gc --enable-targets=all --disable-werror --with-arch-32=i486 --with-tune=generic --enable-checking=release --build=i486-linux-gnu --host=i486-linux-gnu --target=i486-linux-gnu
Thread model: posix
gcc version 4.4.1 (Ubuntu 4.4.1-4ubuntu8) 
	
$make -v
GNU Make 3.81
	Am vazut ca pe fep e un compilator de la SUN; codul scris de mine
e C++99 compliant deci nu stiu de ce nu merge. Sper sa nu fie asta
o problema. Am aflat tarziu ca se va testa pe fep si am lucrat doar
pe masina locala.
