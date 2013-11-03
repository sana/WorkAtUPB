#!/bin/bash
#Tema 4 SO - Test 11
#testare listing director inexistent, clientul trebuie sa iasa cu exitcode != 0

source f.sh
startTest 11

rm -rf testdir

../client 127.0.0.1 ls testdir 1>/dev/null

if [ $? -eq 0 ]
	then do_exit 1
	else do_exit 0
fi
