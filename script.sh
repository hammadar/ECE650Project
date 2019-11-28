#!/usr/bin/bash
chmod u+x script.sh

vertices=(5, 10, 15, 20, 25, 30, 35, 40, 45, 50)

for vertex in "${vertices[@]}"
do
	echo $vertex >> output.txt  

	for ((counter=0; counter<10; counter++))
	do
		./graphGen $vertex | build/./ece650-prj -b -t 60 >> output.txt

	done
done

