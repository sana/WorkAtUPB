#!/bin/bash
#Tema 4 SO - Test 21
#scriere 1MB

source f.sh
startTest 21

rm -f input output
./generate 1048576 >input 2>/dev/null
../client 127.0.0.1 wr output 0 1048576 <input
result1=$?
diff -q input output
result2=$?

if [ $result1 -ne 0 -o $result2 -ne 0 ]
	then do_exit 1
	else do_exit 0
fi
