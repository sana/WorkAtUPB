#!/bin/bash
#Tema 4 SO - Test 17
#scriere 4096 de bytes

source f.sh
startTest 17

rm -f input output
./generate 4096 >input 2>/dev/null
../client 127.0.0.1 wr output 0 4096 <input
result1=$?
diff -q input output
result2=$?

if [ $result1 -ne 0 -o $result2 -ne 0 ]
	then do_exit 1
	else do_exit 0
fi
