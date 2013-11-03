# Laurentiu Dascalu, 342 C3, Tema 1 SPRC

build:
	rpcgen -C rshell.x
	gcc -o rserver rserver.c rshell_svc.c -lnsl rshell_xdr.c
	gcc -o rclient rclient.c rshell_clnt.c -lnsl rshell_xdr.c

clean:
	rm -f rclient rserver rshell.h rshell_svc.c rshell_clnt.c rshell_xdr.c 
