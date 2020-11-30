#!/bin/bash
for ((i = 0; i < 1000; i++))
do
	./fcntl &
done
sync ./counter.txt
sleep .5 #wait while sync file is syncing
cat counter.txt
echo ""
