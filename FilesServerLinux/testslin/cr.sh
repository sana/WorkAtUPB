#!/bin/bash
#sterge toate \r din toate scripturile din directorul curent

for file in `ls *.sh`
do
	if [ $file != "cr.sh" ]
	then
		tr -d "\r" <$file >tmp
		mv tmp $file
	fi
done
