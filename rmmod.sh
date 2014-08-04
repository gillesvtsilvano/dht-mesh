#!/bin/bash

script_name=$0

modules="dht.ko nbt.ko"

function main(){
	for i in $modules; do
		sudo rmmod $i;
	done
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

