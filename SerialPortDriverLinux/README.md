Laurențiu Dascălu, 342 C3

Tema 2 SO2, driver pentru serială.

Inițial, am definit structura atașată char device-ului:
struct serial_dev, care conține două buffere: unul pentru citire
și unul pentru scrierea. De asemenea, bufferele au atașate variabile
contor și de ready (atomic_t); pentru sincronizarea cu întreruperile
am folosit wait queue. 

Rutina de tratare a întreruperii este serial_interrupt_handle. Inițial,
citesc IIR-ul și văd dacă am primit RDAI (read) sau THREI (write). Dacă
primesc read atunci citesc toți octeții și deblochez serial_read-ul care
mă așteaptă. Dacă primesc write atunci scriu toți octeții(nu mai mult de 14,
atât cât poate versiunea asta de UART) și anunț write-ul că am terminat.
Sincronizarea o fac prin wait queue (vezi wake_up-urile de la sfârșit).

serial_read() este funcția în care fac citirea datelor și trecerea lor în userspace.
Aștept să mă deblochez din întrerupere sau read_ready e 1 și trimit datele cu
copy_to_user, după care reactivez întreruperea RDAI.

serial_write() este foarte asemănătoare ca idee cu funcția de read, dar are logică
inversă. Mă aștept să citesc cel mult 14 octeți din userspace, după care setez
că sunt gata de transfer și reactivez întreruperile THREI. Aștept cu un wait
queue să se termine de transferat datele în întrerupere, după care returnez
numărul de octeți scriși.

În serial_ioctl() folosesc un macro din laborator, iar în do_requests() înregistrez
un baseport(region, irq etc.). Pe destructor eliberez toate aceste resurse.

