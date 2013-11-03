#!/bin/bash
#Tema 4 SO - functii

startTest()
{
	no=$1
	echo "Started test $no..."
	../server &
	sleep 1
}


do_exit()
{
	result=$1
	kill %1
	wait 2>/dev/null
	if [ $result -ne 0 ]
		then echo "-- FAILED --"
		else echo "-- PASSED --"
	fi
	echo ""
	rm -f input output output2
	rm -rf testdir
	exit $result
}
