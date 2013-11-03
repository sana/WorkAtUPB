#!/bin/bash
#Tema 4 SO - Test 01 - din oficiu :)
#testeaza daca serverul asculta pe portul 8192
#citire 0 bytes dintr-un fisier
#scriere 0 bytes intr-un fisier
#citire dupa EOF, trebuie sa reuseasca si sa citeasca 0 bytes

source f.sh

listenBefore=`netstat -talp --inet | grep ":8192" | grep -v "TIME_WAIT" | wc -l`

startTest 01

sleep 2
listenAfter=`netstat -talp --inet | grep ":8192" | grep -v "TIME_WAIT" | wc -l`

if [ $[$listenAfter - $listenBefore] -ne 1 ]
	then do_exit 1
fi

#citire 0 bytes
rm -f input output
./generate 0 >input 2>/dev/null
../client 127.0.0.1 rd input 0 0 >output

result=$?

diff -q input output

if [ $result -ne 0 -o $? -ne 0 ]
	then do_exit 1
fi

#scriere 0 bytes
rm -f input output
./generate 0 >input 2>/dev/null
echo "zzz" >output
cp output output2
../client 127.0.0.1 wr output 0 0 <input

result=$?

diff -q output output2

if [ $result -ne 0 -o $? -ne 0 ]
	then do_exit 1
fi

#citire de dupa EOF
rm -f input output output2
./generate 10 >input 2>/dev/null
touch output2

../client 127.0.0.1 rd input 20 10 >>output
result1=$?
diff -q output output2
result2=$?

if [ $result1 -ne 0 -o $result2 -ne 0 ]
	then do_exit 1
	else do_exit 0
fi
