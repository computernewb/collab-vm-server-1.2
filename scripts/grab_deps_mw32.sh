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

configure_build(){
	log "Configuring..."
	CPPFLAGS="-I$CVM_HOME/cvmlib32/include" LDFLAGS="-L$CVM_HOME/cvmlib32/lib" ./configure --prefix="$CVM_HOME/cvmlib32";
	log "Building..."
	make -j$(nproc)
	log "Installing into prefix..."
	make install
}

configure_build_uuid(){
	# Behaviour to place includes correctly (ossp/)
	log "Configuring..."
	CPPFLAGS="-I$CVM_HOME/cvmlib32/include" LDFLAGS="-L$CVM_HOME/cvmlib32/lib" ./configure --prefix="$CVM_HOME/cvmlib32" --includedir="$CVM_HOME/cvmlib32/include/ossp";
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
