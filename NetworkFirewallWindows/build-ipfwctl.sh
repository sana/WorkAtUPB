#!/bin/sh

if test -n "$1"; then
	os="$1"
fi

if test "$os" = "linux"; then
	make -f Makefile.ipfwctl build
elif test "$os" = "windows"; then
	if test -n "$winbuildenv"; then
		# VMchecker: setup build env and run the build
		echo "$winbuildenv && nmake /nologo /f NMakefile.ipfwctl build" > __checker.bat
		cmd /E:ON /V:ON /T:0E /C __checker.bat
	else
		# let's hope build env is good to go
		nmake /nologo /f NMakefile.ipfwctl build
	fi
else
	echo "Usage: $0 linux|windows"
	exit 1
fi
