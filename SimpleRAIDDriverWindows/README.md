Dascalu Laurentiu, 342C3

1. Introducere

  Tema este un Software RAID simplu peste doua partitii. Consistenta
datelelor de pe o partitie este asigurata de CRC-uri. Cand userspace-ul
acceseaza dispozitivul logic se creaza doua cereri de read sau write,
in functie de ce se doreste.
  Initial, se inregistreaza doua dispozitive fizice si un dispozitiv logic.
Functiile de open/close/read/write pentru dispozitivul logic sunt definite
in acest modul.
  La operatia write se scriu pe "discurile fizice" datele si CRC-ul lor.
La operatia de read se citesc datele de pe cele doua partitii si se
compara CRC-urile; daca sunt corecte atunci se pune in userspace datele de
pe una din partitii. Daca nu, se afla ce partitia cu datele corecte si
se trimit acele date cu actualizare pe partitia complementara. Daca ambele
partitii au CRC-ul gresit atunci se semnaleaza aceasta eroare si citirea
esueaza.

2. Detalii de implementare

* SSR_PHYSICAL_DEVICE - structura driverului pentru lucrul cu dispozitivele fizice
* SSR_DEVICE_DATA - structura driverului pentru lucrul cu dispozitivul logic

// scrie/citeste date de pe o partitie
static NTSTATUS SendIrp(...);

// scrie/citeste CRC-ul de pe o partitie
static NTSTATUS SendCRCIrp(...);

Functiile sunt flexibile in sensul ca primesc buffer-ul ca parametru pentru
citire/scriere.

// operatii pe dispozitivul logic
MyOpen, MyClose, MyRead, MyWrite


Functiile LoadPhysicalDisk, LoadLogicalDisk sunt folosite pentru "incarcarea"
dispozitivelor. Dupa cum spunea, in DriverEntry se incarca dispotivele si in
DriverUnload se elibereaza resursele.