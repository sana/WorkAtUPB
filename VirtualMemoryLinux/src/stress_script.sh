#!/bin/sh
for i in `seq 1000`
do
	rm __*
	echo `make -f Makefile.checker | grep failed | wc -l`
done
