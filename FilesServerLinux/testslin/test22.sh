#!/bin/bash
#Tema 4 SO - Test 22
#mai multe citiri consecutive de blocuri

source f.sh
startTest 22

size=$[64 * 1024]
noBlocks=4
blockSize=$[$size/$noBlocks]


rm -f input output
./generate $size >input 2>/dev/null

for (( i=0; i<$noBlocks; i++ ))
do
	start=$[$i * $blockSize]
	../client 127.0.0.1 rd input $start $blockSize >>output
	
	if [ $? -ne 0 ]
		then do_exit 1
	fi
done

diff -q input output

if [ $? -ne 0 ]
	then do_exit 1
	else do_exit 0
fi
