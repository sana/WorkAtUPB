Dascalu Laurentiu, 335CA
Tema3 SO, Memoria Virtuala

	Pentru simularea memoriei virtuale am mapat in memorie cu PROT_NONE o adresa
cu dimensiunea egala cu suma numarului de pagini fizice si a numarului de pagini swap.
Peste aceasta memorie, se mapeaza, in urma primirii SEGFAULT-ului o portiune fisier
descrisa prin offset (id-ul paginii) si dimensiune (page_size, valoare constanta).

	Cand o pagina noua este accesata atunci este adusa in memorie (mapare peste fisier ram
cu MAP_FIXED) si i se da dreptul PROT_READ; daca pe o pagina cu dreptul PROT_READ se primeste
un alt SEGFAULT atunci i se da dreptul PROT_WRITE. Algoritmul este implementat in functia
sigsegv_handler().

	Daca o pagina nu exista atunci si exista loc liber in RAM atunci ea este pusa
direct in ram; daca insa ram-ul este full atunci se face swap cu o pagina
din ram- functia care face acest lucru este swap_data().

	Daca o pagina exista dar este in swap atunci este adusa de acolo in memorie
folosind functia swap_data_swap().

	In program retin, pentru fiecare pagina virtuala urmatoarea structura:
typedef struct _page_record
{
	int id_real;
	int id_swap;
	char info;
	char dirty;
	int swaped;
} page_record;
	id_real si id_swap sunt mutual exclusive; nu pot fi simultan diferite de -1 si reprezinta
id-ul paginii in fisierul de ram/swap. info este informatia atasa paginii - ce drepturi are,
iar swaped indica daca pagina a fost in swap cel putin o data.
	
	In functiile swap_data() si swap_data_swap() se verifica daca o functie a fost murdarita
(este dirty) si se optimeaza operatia de scriere in swap.
	In final se elibereaza toate memoria alocata de biblioteca.
	