#!/bin/bash
#Tema 4 SO - Test 04
#scriere dupa EOF, trebuie sa reuseasca

source f.sh
startTest 04

rm -f input output output2
./generate 10 >input 2>output
cp input output2
cat input >>output2

../client 127.0.0.1 wr output 10 10 <input
result1=$?
diff -q output output2
result2=$?

if [ $result1 -ne 0 -o $result2 -ne 0 ]
	then do_exit 1
	else do_exit 0
fi
