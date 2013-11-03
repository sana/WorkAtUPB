#in caz ca va plictisiti de tema :D

if [ $# -gt 0 ]
	then max=$1
	else max=100
fi

if [ $# -gt 1 ]
	then stuff=$2
	else stuff=beer
fi

for (( i=$max; i > 1; i-- ))
do
	echo "$i bottles of $stuff on the wall"
	echo "$i bottles of $stuff!"
	echo "And if one bottle happens to fall"
	if [ $[$i - 1] -ge 1 ]
		then echo "There would be $[$i - 1] bottles of $stuff on the wall!"
		else echo "There would be one bottle of $stuff on the wall!"
	fi
	echo ""
done

echo "One bottle of $stuff on the wall"
echo "One bottle of $stuff!"
echo "And if the bottle happens to fall"
echo "There would be no bottles of $stuff on the wall!"
echo ""
