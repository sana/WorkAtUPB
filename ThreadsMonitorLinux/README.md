Dascalu Laurentiu, 335CA - Tema 4 SO, Monitor Generic

Continutul arhivei:

Headere:
LibMonitorInterface.h - functiile aplicabile monitorului
LibMonitorStructure.h - structura interna a monitorului - pentru flexibilitate am inclus-o intr-un header;
nu o foloseste decat implementarea monitorului

Surse:
LibRW.cpp - implementarea problemei de cititori-scriitori
LibMonitor.cpp - implementarea monitorului

Makefile

1) Implementarea monitorului:

- structura unui monitor este definita in: struct _monitor (LibMonitorStructure.h)

- in aceasta implementarea un thread este reprezentat de un semafor pe care asteapta;
un thread se afla intr-o coada <=> asteapta pe un semafor din coada respectiva.

- contine 3 cozi de thread-uri(entry_queue, signaled_queue, waiting_queue),
un vector de cozi de conditii(conditions_queue) si un dictionar(entered_threads)
folosit pentru a vedea daca un thread a intrat sau nu in monitor.

- fiecare thread, inainte sa se blocheze apeleaza functia planifica(); aici se face
contextul switch-ul. Parcurg cozile Waiting, Signaled si Entry si decid: daca iau din
Waiting atunci dau drumul thread-ului, daca e din Signaled atunci il mut in Waiting si
fac o noua planificare, daca e din Entry ii dau drumul. Datul drumului se face prin
sem_post(semafor).

- sem_new() construieste un nou semafor cu atribute default si initializat la 0; dupa
cum spunea.

Comportamentul functiilor Enter(), Leave(), Wait(), Signal si Broadcast() este cel
prezentat in enuntul temei.


2) Implementarea problemei Readers-Writers:

- se bazeaza pe un monitor si 4 variabile globale: numarul de cititori, numarul de cititori care
asteapta, numarul de scriitori si numarul de scriitori care asteapta;

- monitorul este format din doua variabile de conditii : 0 si 1;
0 reprezinta faptul ca se poate citi, iar 1 ca se poate scrie;

- implementarea este straight-forward - contine scurte comentarii,
insa codul vorbeste de la sine

Comportamentul functiilor CreateRWMonitor(), GetNrConds(), StartCit(), StopCit(),
StartScrit() si StopScrit() este cel descris in enunt.
