Dascalu Laurentiu, 342 C3
Tema 5 Firewall Linux

1. Introducere

   Am implementat cerintele temei folosind hookuri, spin-lock-uri bh si timere.
Implementarea contine 2 fisiere sursa:
    a) ip_firewall.c: implementarea propriu-zisa a firewall-ului
    b) rules.c: implementarea operatiilor pe listele de reguli TCP si UDP.

    Hookurile de IN si OUT sunt executate atunci cand intra sau ies pachete
si in cadrul lor operez pe listele de reguli. In hookul de OUT generez reguli
dinamice, iar in hookul de IN aplic reguli in vederea droparii de pachete.

    Pentru retinerea regulilor folosesc 3 liste de reguli: statice, dinamice TCP
si dinamice UDP. Atunci cand userspaceul apeleaza ioctl() cu FW_LIST, serializez
regulile intr-un buffer din kernel (zona de cod protejata de un spin_lock_bh) si
pe care-l transfer (eliberez spin_lock-ul pentru ca copy_to_user poate fi
blocant/sleepy).

2. Detalii de implementare

    a) rules.c contine urmatoarele functii:

// Aceste functii se apeleaza la expirarea timer-ului
static void udp_timer_callback(unsigned long data);
static void tcp_timer_callback(unsigned long data);
static void null_timer_callback(unsigned long data);


//   Adauga o regulia rule in lista head; on_stack inseamna
// ca regula a fost alocata static de compilator pe stiva si
// pointerul nu va fi valid pe viitor (se face kmalloc && memcpy)
//   In functie de proto se decide daca se armeaza sau nu un timer
// si ce tip de timer trebuie initializat; proto = -1 inseamna
// ca regula e statica si se initializeaza cu null_timer_callback.
// Am ales sa folosesc null_timer_callback pentru ca _uneori_ cand
// distrugeam lista primeam OOPS la distrugerea unui timer neinitializat
//   Functia foloseste spin_lock_bh() pentru sincronizarea cu un bottom half
int add_rule(struct fwr *rule, struct list_head *head, int on_stack, int proto);

// Sterge o regula din lista; doua reguli se considera ca sunt identice
// fara memcmp intoarce 0 pentru &rule1 si &rule2
int del_rule(struct fwr *rule, struct list_head *head);


//   Functie interna care verifica daca doua adrese sunt egale dupa aplicarea
// mastii de retea si daca portul sursei se afla in intervalul inchis al
// destinatiei
static int match_rule(struct fwr *src, struct fwr *dest);

//   Verifica daca exista o regula care sa satisfaca regula rule;
// Initial, construiesc o regula din pachetul primit si o caut in lista de reguli.
// Practic, in acest caz, rule este un adaptor de la pachet la formatul propriu-zis
// al regulii
struct rules_list_t *search_rule(struct fwr *rule, struct list_head *head);

// Elibereaza memoria si distruge timerele din lista
void destroy_list(struct list_head *head);

int list_size(struct list_head *head);

// Serializeaza regulile din list head in buffer
void add_rules_to_buffer(void *buffer, struct list_head *head);

    b) ip_firewall.c

// Construieste o noua regula cu parametrii dati
static void new_rule(struct fwr *rule, unsigned int ip_src,
        unsigned int ip_dst, unsigned int ip_src_mask,
        unsigned int ip_dst_mask, unsigned short port_src0,
        unsigned short port_src1, unsigned short port_dst0,
        unsigned short port_dst1);

/* Hook pentru pachetele care ies */
Se contruiesc reguli dinamice:
    1. TCP
     a) if (tcph->syn && !tcph->ack) => add_rule
     b) if (tcph->fin) => armeaza un timer care va scoate regula
    2. UDP
     a) Daca am primit un pachet atunci armez un timer (daca urmeaza sa
        expire atunci nu va mai expira)
     b) Daca regula nu exista, atunci o adaug si o armez
Important: Toate pachetele sunt acceptate!

static unsigned int my_nf_hookfn_out(unsigned int hooknum, struct sk_buff *skb,
        const struct net_device *in, const struct net_device *out,
        int(*okfn)(struct sk_buff *));

/* Hook pentru pachetele care intra */
Se construiesc o regula pe baza pachetului (in rules.c eu compar reguli)
    1. Se cauta in regulile statice, daca exista atunci se accepta
    2. Se cauta in lista dinamica de reguli TCP, daca exista atunci se accepta
    3. Se cauta in lista dinamica de reguli UDP, daca exista atunci se accepta
si se armeaza timer-ul ("conexiunea" e inca in viata)
    4. Daca am ajuns aici cu pachet TCP sau UDP, atunci dropez pachetul
    5. Restul pachetelor se accepta (se filtreaza doar pachete TCP si UDP)

Important: Se filtreaza! Unele pachete primesc NF_DROP;
static unsigned int my_nf_hookfn_in(unsigned int hooknum, struct sk_buff *skb,
        const struct net_device *in, const struct net_device *out,
        int(*okfn)(struct sk_buff *));
