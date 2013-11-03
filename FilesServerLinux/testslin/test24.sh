#!/bin/bash
#Tema 4 SO - Test 24
#multe citiri si scrieri :)
#mai exact, se citesc pe rand mai multe blocuri care sunt puse in ordine inversa in alt fisier
#se repeta operatia cu fisierele schimbate, trebuie sa rezulte fisierul original

source f.sh
startTest 24

bash do_reverse_test.sh $[64 * 1024] 4 input output output2

if [ $? -ne 0 ]
	then do_exit 1
	else do_exit 0
fi
