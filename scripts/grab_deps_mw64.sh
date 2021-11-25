#!/bin/bash
CVM_HOME=$(pwd)

log(){
	printf "$1\n"
}

triplet_install(){
	pacman -S --noconf mingw-w64-x86_64-$1;
}

main(){
	log "Dependency grab started on $(date +"%x %I:%M %p").";
	triplet_install "libvncserver";
	triplet_install "curl";
	triplet_install "cairo";
	triplet_install "dlfcn";
	triplet_install "boost";
	triplet_install "sqlite3";
	triplet_install "cyrus-sasl";
	triplet_install "winpthreads-git";

	log "Dependency grab finished.";
};main;
