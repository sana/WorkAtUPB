#!/bin/bash
#Tema 4 SO - Test 05
#citire 1 byte dintr-un fisier

source f.sh
startTest 05

rm -f input output
./generate 1 >input 2>/dev/null
../client 127.0.0.1 rd input 0 1 >output

result=$?

diff -q input output

if [ $result -ne 0 -o $? -ne 0 ]
	then do_exit 1
	else do_exit 0
fi
