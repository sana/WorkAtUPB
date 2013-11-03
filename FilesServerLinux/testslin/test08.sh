#!/bin/bash
#Tema 4 SO - Test 08
#scriere intr-un fisier inexistent, operatia trebuie sa reuseasca

source f.sh
startTest 08

rm -f input output
./generate 1 >input 2>/dev/null
../client 127.0.0.1 wr output 0 1 <input

result=$?

diff -q input output

if [ $result -ne 0 -o $? -ne 0 ]
	then do_exit 1
	else do_exit 0
fi
