#!/bin/bash

script_name=$0

modules="nbt.ko dht.ko"

function main(){
	for i in $modules; do
		sudo insmod $i;
	done
	tailf /var/log/syslog
}

function usage(){
	#echo "usage: $script_name [[-f file ] | [-h]]"
	echo "usage: TODO"
}


while [ "$1" != "" ]; do
    case $1 in
        -f | --file )           shift
                                ;;
        -h | --help )           usage
                                exit
                                ;;
        * )                     #Whatever
                                exit 1
    esac
    shift
done

main

