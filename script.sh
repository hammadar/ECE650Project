#!/bin/bash
chmod u+x script.sh

vertices=(5 10 15 20 25 30 35 40 45 50)

for vertex in "${vertices[@]}"
do
	for ((graphcount=0; graphcount<10; graphcount++))
	do
		echo $graphcount $vertex
		echo Graph $graphcount >> output$vertex.txt
		./graphGen $vertex > graph.txt
		echo V$vertex G$graphcount >> graphs.txt
		cat graph.txt >> graphs.txt 

		for ((counter=0; counter<10; counter++))
		do
			cat graph.txt | build/./ece650-prj -b -t 60 >> output$vertex.txt
		done
	done
	git add output$vertex.txt
done

rm graph.txt
git add graphs.txt
git commit -m "automated script adding outputs to git"


return