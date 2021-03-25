#!/bin/bash
CVM_HOME=$(pwd)

log(){
	printf "$1\n"
}

triplet_install(){
	pacman -S --noconf mingw-w64-x86_64-$1;
}

tar_xrm(){
	log "Downloading $1..."
	wget --quiet $1
	log "Extracting...";
	tar xf $2;
	rm -f $2;
}

configure_build(){
	log "Configuring..."
	CPPFLAGS="-I$CVM_HOME/cvmlib/include" LDFLAGS="-L$CVM_HOME/cvmlib/lib" ./configure --prefix="$CVM_HOME/cvmlib";
	log "Building..."
	make -j$(nproc)
	log "Installing into prefix..."
	make install
}

configure_build_uuid(){
	# Behaviour to place includes correctly (ossp/)
	log "Configuring..."
	CPPFLAGS="-I$CVM_HOME/cvmlib/include" LDFLAGS="-L$CVM_HOME/cvmlib/lib" ./configure --prefix="$CVM_HOME/cvmlib" --includedir="$CVM_HOME/cvmlib/include/ossp";
	log "Building..."
	make -j$(nproc)
	log "Installing into prefix..."
	make install
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
