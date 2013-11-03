#!/bin/sh

if test $# -ne 1; then
	echo "Usage: $0 <outside_ip>"
	exit 1
fi

sed -i "s/OUTSIDE_IP =.*/OUTSIDE_IP = $1/g" Makefile.checker
