#!/bin/bash

FMT_PWD=`pwd`

FMT_C_CPP(){
	PATH_1=$1
	FMT_PATH=${FMT_PWD}/${PATH_1}
	echo "${FMT_PATH}"
	
	find ${FMT_PATH} -name "*.cpp" -type f | xargs sed -i 's/\t/    /g'
	find ${FMT_PATH} -name "*.c" -type f | xargs sed -i 's/\t/    /g'
	find ${FMT_PATH} -name "*.h" -type f | xargs sed -i 's/\t/    /g'

	find ${FMT_PATH} -name "*.cpp" -type f | xargs sed -i 's/\r//g'
	find ${FMT_PATH} -name "*.c" -type f | xargs sed -i 's/\r//g'
	find ${FMT_PATH} -name "*.h" -type f | xargs sed -i 's/\r//g'
}

FMT_C_CPP inc
FMT_C_CPP src
FMT_C_CPP test
