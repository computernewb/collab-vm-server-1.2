#!/bin/bash
CVM_HOME=$(pwd)

log(){
	printf "$1\n"
}

triplet_install(){
	pacman -S --noconf mingw-w64-i686-$1;
}

tar_xrm(){
	log "Downloading $1..."
	wget --quiet $1
	log "Extracting...";
	tar xf $2;
	rm -f $2;
}

main(){
	log "Dependency grab started on $(date +"%x %I:%M %p").";
	triplet_install "libvncserver";
	triplet_install "cairo";
	triplet_install "dlfcn";
	triplet_install "boost";
	triplet_install "sqlite3";
	triplet_install "cyrus-sasl";

	cd ..;
	log "Dependency grab finished.";
};main;
