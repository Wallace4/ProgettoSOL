#!/bin/sh

conn_c=0
disc_c=0
rtv_c=0
str_c=0
dlt_c=0
str_s=0

while read line; do
	if [ line = "Starting Test" ]; then
		continue;
	fi
	names=$(echo $line | tr " :" "\n")
	if [ "${names[1]}" == "os_connect" ]; then
		((conn_c++))
	elif [ "${names[1]}" == "os_disconnect" ]; then
		((disc_c++))
	elif [ "${names[2]}" == "os_store" ]; then
		((str_c++))
		if [ ${names[1]} -gt ${names[3]} ]; then
			failed_usr=${names[0]}
			failed_op=os_store
		fi
	elif [ "${names[2]}" == "os_retrieve" ]; then
		((rtv_c++))
		if [ ${names[1]} -gt ${names[3]} ]; then
			failed_usr=${names[0]}
			failed_op=os_retrieve
		fi
	elif [ "${names[2]}" == "os_delete" ]; then
		((dlt_c++))
		if [ ${names[1]} -gt ${names[3]} ]; then
			failed_usr=${names[0]}
			failed_op=os_delete
		fi
	fi
done < "testout.log"
if [ $conn_c -gt $disc_c ]; then
	echo $(($conn_c-$disc_c)) connessioni sono terminate a metà per errori
else
	echo Nessuna connessione terminata prematuramente
fi
i=0
for fu in $failed_usr ; do
	echo fallito il client di $fu sull\'operazione ${failed_op[i]}
	let i=i+1
done
echo Totale operazioni fallite: $i


#magari mettere un if che se questo non funzia allora fare la kill normale con ps -ef
pkill -SIGUSR1 -f ./bin/server
