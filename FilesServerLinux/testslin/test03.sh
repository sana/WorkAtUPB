#!/bin/bash
#Tema 4 SO - Test 03
#testeaza daca clientul iese cu exitcode != 0 daca primeste argumente gresite

source f.sh
startTest 03

./generate 1 >input 2>/dev/null

#lipsa toate argumentele 
../client >/dev/null
if [ $? -eq 0 ]
	then do_exit 1
fi

#lipsa 4 argumente
../client 127.0.0.1 >/dev/null
if [ $? -eq 0 ]
	then do_exit 1
fi

#lipsa nume fisier, lungime si offset
../client 127.0.0.1 rd >/dev/null
if [ $? -eq 0 ]
	then do_exit 1
fi

../client 127.0.0.1 wr >/dev/null
if [ $? -eq 0 ]
	then do_exit 1
fi

#lipsa ultimul argument
../client 127.0.0.1 ls >/dev/null
if [ $? -eq 0 ]
	then do_exit 1
fi

#argumentul 2 gresit
../client 127.0.0.1 rr input 0 0 >/dev/null
if [ $? -eq 0 ]
	then do_exit 1
fi


#offset si lungime lipsa
../client 127.0.0.1 rd input >/dev/null
if [ $? -eq 0 ]
	then do_exit 1
fi

../client 127.0.0.1 wr input >/dev/null
if [ $? -eq 0 ]
	then do_exit 1
fi


#lungime lipsa
../client 127.0.0.1 rd input 0 >/dev/null
if [ $? -eq 0 ]
	then do_exit 1
fi


../client 127.0.0.1 wr output 0 >/dev/null
if [ $? -eq 0 ]
	then do_exit 1
fi

#offset negativ
../client 127.0.0.1 rd input -1 >/dev/null
if [ $? -eq 0 ]
	then do_exit 1
fi

#lungime negativa
../client 127.0.0.1 rd input 0 -1 >/dev/null
if [ $? -eq 0 ]
	then do_exit 1
fi

#argumente in plus
../client 127.0.0.1 rd input 0 2 1 >/dev/null
if [ $? -eq 0 ]
	then do_exit 1
fi

mkdir -p dir

../client 127.0.0.1 ls dir 33 >/dev/null
if [ $? -eq 0 ]
	then do_exit 1
fi

rm -rf dir

do_exit 0
