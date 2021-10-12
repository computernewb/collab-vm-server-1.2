#!/bin/bash

LIST=$(ldd $2/collab-vm-server.exe | cut -d '>' -f2 | cut -d '(' -f1 | cut -d ' ' -f2 | sed -E '/\/c\//d' | sed -E '/\/home\//d' | sed -E '/not/d')
echo $LIST
if [ "$LIST" != "" ]; then cp $LIST $2 2>/dev/null; fi
