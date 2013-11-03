#!/bin/bash
#Tema 4 SO - Test 10
#testare listing director existent

source f.sh
startTest 10

rm -f input output
#creaza un director de test cu cateva directoare si fisiere
rm -rf testdir
mkdir testdir
cd testdir
mkdir dir1 dir2 dir3 dir666
cp ../test*.sh .
cd ..

#se afla cum ar trebui sa arate outputul
ls -lp --time-style=long-iso testdir | tail -n +2 | tr -s " " | cut -d" " -f5,8 | /usr/bin/sort -b >output2

../client 127.0.0.1 ls testdir | tr -s " " | /usr/bin/sort -b >output
result1=$?
diff -w output output2
result2=$?

if [ $result1 -ne 0 -o $result2 -ne 0 ]
	then do_exit 1
	else do_exit 0
fi
