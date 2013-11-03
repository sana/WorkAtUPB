#!/bin/bash
#Tema 4 SO - Test 12
#citire primii 10 bytes dintr-un fisier de 20 de bytes

source f.sh
startTest 12

rm -f input output output2
./generate 20 2>input | head -c 10 >output2

../client 127.0.0.1 rd input 0 10 >output
result1=$?
diff -q output output2
result2=$?

if [ $result1 -ne 0 -o $result2 -ne 0 ]
	then do_exit 1
	else do_exit 0
fi
