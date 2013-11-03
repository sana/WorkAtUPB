#!/bin/bash
#Tema 4 SO - Test 13
#scriere intr-un fisier de 20 de bytes a 10 bytes incepand cu offset 0

source f.sh
startTest 13

rm -f input output output2
./generate 20 >output 2>/dev/null
./generate 10 >input 2>output2
tail -c 10 output >>output2

../client 127.0.0.1 wr output 0 10 <input
result1=$?
diff -q output output2
result2=$?

if [ $result1 -ne 0 -o $result2 -ne 0 ]
	then do_exit 1
	else do_exit 0
fi
