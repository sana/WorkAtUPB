#!/bin/bash
#Tema 4 SO - Test 23
#mai scrieri consecutive in ordine inversa de blocuri avand in total $size bytes

source f.sh
startTest 23


size=$[64 * 1024]
noBlocks=4
blockSize=$[$size/$noBlocks]

rm -f input output
./generate $size >input 2>/dev/null

for (( i = $[$noBlocks - 1]; i>=0; i-- ))
do
	start=$[$i * $blockSize]
	end=$[$start + $blockSize]
	head -c $end input | tail --bytes=$blockSize | ../client 127.0.0.1 wr output $start $blockSize
	
	if [ $? -ne 0 ]
		then do_exit 1
	fi
done

diff -q input output

if [ $? -ne 0 ]
	then do_exit 1
	else do_exit 0
fi
