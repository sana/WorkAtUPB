#!/bin/bash
#Tema 4 SO - Test 09
#se testeaza daca serverul inchide fisierul dupa scriere sau citire

source f.sh
startTest 09

#scriere apoi scriere
rm -f input output
./generate 1 >input 2>/dev/null

../client 127.0.0.1 wr output 0 1 <input
result1=$?
diff -q input output
result2=$?

rm output input
./generate 1 >input 2>/dev/null

../client 127.0.0.1 wr output 0 1 <input
result3=$?
diff -q input output
result4=$?

if [ $result1 -ne 0 -o $result2 -ne 0 -o $result3 -ne 0 -o $result4 -ne 0 ]
	then do_exit 1
fi

#citire apoi scriere
rm -f input output
./generate 1 >input 2>/dev/null

../client 127.0.0.1 rd input 0 1 >output
result1=$?
diff -q input output
result2=$?

rm -f input output
./generate 1 >output 2>/dev/null

../client 127.0.0.1 wr input 0 1 <output
result3=$?
diff -q input output
result4=$?

if [ $result1 -ne 0 -o $result2 -ne 0 -o $result3 -ne 0 -o $result4 -ne 0 ]
	then do_exit 1
	else do_exit 0
fi
