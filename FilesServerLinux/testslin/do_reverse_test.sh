#!/bin/bash
#genereaza un fisier de $1 KB, apoi inverseaza ordinea celor $2 blocuri ($2 trebuie sa il divida pe $1)
#dupa o noua inversare, trebuie sa se obtina fisierul original

function do_reverse()
{
	blockSize=$[$size/$noBlocks]
	file1=$1
	file2=$2
	
	#echo "from $file1 to $file2"
	for (( i=0; i<$noBlocks; i++ ))
	do
		start=$[$i * $blockSize]
		end=$[$start + $blockSize]
		revStart=$[$size - $end]
		../client 127.0.0.1 rd $file1 $start $blockSize | ../client 127.0.0.1 wr $file2 $revStart $blockSize
		
		if [ $? -ne 0 ]
			then 
				rm -f $file1 $file2
				exit 1
		fi
	done
	
}


size=$1
noBlocks=$2
in=$3
tmp=$4
out=$5

#echo "do_reverse_test from $in via $tmp to $out"

rm -f $in $tmp $out
./generate $size >$in 2>/dev/null

do_reverse $in $tmp
do_reverse $tmp $out
diff -q $in $out

if [ $? -ne 0 ]
	then 
		rm -f $in $tmp $out
		exit 1
fi
rm -f $in $tmp $out
exit 0
