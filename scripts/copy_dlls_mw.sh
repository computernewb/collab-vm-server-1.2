#!/bin/bash

if [ "$1" == "x86" ]; then
	cp cvmlib32/bin/*.dll $2
else
	cp cvmlib/bin/*.dll $2
fi

LIST=$(ldd $2/collab-vm-server.exe | cut -d '>' -f2 | cut -d '(' -f1 | cut -d ' ' -f2 | sed -E '/\/c\//d' | sed -E '/\/home\//d')

echo $LIST
cp $LIST $2 2>/dev/null