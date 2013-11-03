#!/bin/bash
#Tema 4 SO - Test 07
#citire dintr-un fisier inexistent, clientul trebuie sa intoarca exit code != 0

source f.sh
startTest 07

rm -f input output

../client 127.0.0.1 rd input 0 1 >output
if [ $? -eq 0 ]
	then do_exit 1
	else do_exit 0
fi
