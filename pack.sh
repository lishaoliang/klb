#!/bin/bash

PACK_PWD=`pwd`

PACK_ONE(){
	PACK_OS=$1
	PACK_ARCH=$2
	PACK_GO_TAGS=$3
	echo "os = ${PACK_OS}"
	echo "arch = ${PACK_ARCH}"
	echo "go tags = ${PACK_GO_TAGS}"
	

	make clean OS=$PACK_OS ARCH=$PACK_ARCH TAGS=$PACK_GO_TAGS
	make OS=$PACK_OS ARCH=$PACK_ARCH TAGS="$PACK_GO_TAGS"
}

# make OS=linux ARCH=amd64 TAGS="debug normal"
PACK_ONE linux amd64 "debug normal"

# make OS=linux ARCH=arm TAGS="debug hi3519v101 himppv3"
#PACK_ONE linux arm "debug hi3519v101 himppv3"

# make OS=linux ARCH=arm TAGS="debug hi3516av100 himppv2"
#PACK_ONE linux arm "debug hi3516av100 himppv2"