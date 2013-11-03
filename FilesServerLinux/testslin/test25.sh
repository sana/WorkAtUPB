#!/bin/bash
#Tema 4 SO - Test 25
#multe citiri si scrieri in paralel! :)
#se lanseaza mai multi clienti care fac ca la testul anterior

source f.sh
startTest 25

noClients=2


for (( i=1; i<=$noClients; i++ ))
do
	file1=input$i
	file2=tmp$i
	file3=output$i
	bash do_reverse_test.sh $[64 * 1024] 4 $file1 $file2 $file3 &
done

for (( i=2; i<=$[$noClients + 1]; i++ ))
do
	wait %$i 2>/dev/null
	
	if [ $? -ne 0 ]
		then
			do_exit 1
	fi
	
	#echo "client $[$i - 1] finished"
done

do_exit 0
