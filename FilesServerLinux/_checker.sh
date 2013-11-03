#!/bin/sh
#Teste tema 4 SO

cd testslin
make

if [ $? -ne 0 ]
then
	echo "cannot compile generate.cpp, aborting"
	exit 1
fi


tests="test*.sh"
passed=0
failed=0

for test in $tests
do
	bash $test
	
	if [ $? -ne 0 ]
		then failed=$[$failed + 1]
		else passed=$[$passed + 1]
	fi
done

echo "$passed tests passed"
echo "$failed tests failed"
