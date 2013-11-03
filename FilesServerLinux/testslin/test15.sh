#!/bin/bash
#Tema 4 SO - Test 15
#clientul primeste comanda sa citeasca 10 bytes, dar serverul are doar 8 bytes in fisier
#operatia trebuie sa reuseasca si trebuie intorsi 8 bytes

source f.sh
startTest 15

rm -f input output output2
./generate 8 >input 2>/dev/null

../client 127.0.0.1 rd input 0 10 >output
result1=$?
diff -q input output
result2=$?

if [ $result1 -ne 0 -o $result2 -ne 0 ]
	then do_exit 1
	else do_exit 0
fi
