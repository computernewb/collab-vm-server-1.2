#!/bin/bash

cp cvmlib/bin/*.dll $2
LIST=$(ldd $2/collab-vm-server.exe | cut -d '>' -f2 | cut -d '(' -f1 | cut -d ' ' -f2 | sed -E '/\/c\//d' | sed -E '/\/home\//d' | sed -E '/not/d')

if [ -z "$LIST" ]; then exit 0; fi

echo $LIST
cp $LIST $2 2>/dev/null
