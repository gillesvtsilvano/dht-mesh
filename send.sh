#!/bin/bash
user="gilles"
hosts="ubuntu-a ubuntu-b"
command="cd dht-tcc; make clean; make;"

for host in $hosts; do
	ssh $user@$host "rm ~/dht-tcc/*"
	scp -r `pwd` $user@$host:~ 
	ssh $user@$host "$command"
done
