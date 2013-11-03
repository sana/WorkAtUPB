#!/bin/bash
#Tema 4 SO - Test 19
#scriere 64KB

source f.sh
startTest 19

rm -f input output
./generate 65536 >input 2>/dev/null
../client 127.0.0.1 wr output 0 65536 <input
result1=$?
diff -q input output
result2=$?

if [ $result1 -ne 0 -o $result2 -ne 0 ]
	then do_exit 1
	else do_exit 0
fi
