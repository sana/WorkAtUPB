#!/bin/bash
#Tema 4 SO - Test 14
#clientul primeste comanda sa scrie 10 bytes, dar i se da un fisier de 2 bytes la intrare
#serverul va scrie intr-un fisier deja creat de 20 de bytes
#in final operatia trebuie sa reuseasca si doar primii 2 bytes din fisierul output trebuie sa fie scrisi

source f.sh
startTest 14

rm -f input output output2
./generate 20 >output 2>/dev/null
./generate 2 >input 2>output2
tail -c 18 output >>output2

../client 127.0.0.1 wr output 0 10 <input
result1=$?
diff -q output output2
result2=$?

if [ $result1 -ne 0 -o $result2 -ne 0 ]
	then do_exit 1
	else do_exit 0
fi
