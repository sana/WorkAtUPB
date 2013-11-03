Dascalu Laurentiu 335CA, Tema 4 ASC Image Quilting.

Continutul arhivei:

spu/spu.cpp - implementarea algoritmului de pe SPU
spu/Makefile - Makefile pentru SPU

input/gmesh.ppm - imagine de test 1 
input/S29.ppm - imagine de test 2

Makefile - Makefile pentru PPU

ppm.cpp - implementarea interfetei imaginilor PPM si
	a operatiilor pe acestea (citire, scriere etc)
ppm.hpp - interfata imaginii PPM

ppu.cpp - implementarea algoritmului de pe PPU

quilt.cpp
quilt.hpp

run_script.sh - script pentru rularea testelor
copy_script.sh - script pentru copierea rezultatelor ( uz intern )


Algoritmul implementat este urmatorul:
	Image quilting are doua componente:
		- quilting vertical
		- quilting orizontal
	Initial, se aplica algoritmul de quilting vertical si apoi cel de quilting orizontal

	
Algoritmii pentru quilting vertical/orizontal sunt foare asemanatori:
	Se aleg ncandidati cu offset-uri random si se pastreaza doar acel candidat
care are diferenta minima pe fasie. PPU-ul trimite SPU-urilor fasiile pe care se
calculeaza diferenta; SPU-ul poate primi mai multi astfel de candidati si va returna
doar offset-ul minim impreuna cu valoarea minima. PPU-ul primeste offset-urile minime
de la toate SPU-urile si alege perechea (offset minim, id offset minim).
	Dupa aceasta etapa incepe netezirea (interclasarea fasiilor); metoda implementata
este prezentata in enuntul temei si se bazeaza pe o programare dinamica dupa
eroarea la nivel de pixel. Dupa calculul matricii E se reface traseul si se construieste
fasia interclasata.
	Se repeta 1 atat timp cat nu am ajuns la sfarsitul quilt-ului pe verticala/orizontala.
	
	SPU-urile folosesc double buffering pentru transferul fasiilor; se citesc doua fasii
apoi se da startul la citirea altor doua fasii si se executa operatia de diff(). Dupa ce
se termina operatia diff se asteapta terminarea celui de-al doilea transfer DMA si se incepe
al treilea. Se bucleaza pana nu mai exista candidati pentru fasia respectiva.
	PPU-ul foloseste evenimente pentru raspunsul de la SPU-uri.
	
	La sfarsitul executiei aplicatiei, se aplica un filtru de netezire a imaginii.
static void filter_image(Picture *pout);
	