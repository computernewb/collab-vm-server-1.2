#!/bin/bash
CVM_HOME=$(pwd)

log(){
	printf "[>] $1\n"
}

triplet_install(){
	pacman -S --noconf mingw-w64-i686-$1;
}

tar_xrm(){
	log "Downloading $1..."
	wget --quiet $1
	log "Extracting...";
	tar xf $1;
	rm $1;
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
	CPPFLAGS="-I$CVM_HOME/cvmlib32/include" LDFLAGS="-L$CVM_HOME/cvmlib32/lib" ./configure --prefix="$CVM_HOME/cvmlib32" >/dev/null 2>/dev/null;
	log "Building..."
	make -j$(nproc) >/dev/null 2>/dev/null
	log "Installing into prefix..."
	make install >/dev/null 2>/dev/null
}

configure_build_uuid(){
	# Behaviour to place includes correctly (ossp/)
	log "Configuring..."
	CPPFLAGS="-I$CVM_HOME/cvmlib32/include" LDFLAGS="-L$CVM_HOME/cvmlib32/lib" ./configure --prefix="$CVM_HOME/cvmlib32" --includedir="$CVM_HOME/cvmlib32/include/ossp" >/dev/null 2>/dev/null;
	log "Building..."
	make -j$(nproc) >/dev/null 2>/dev/null
	log "Installing into prefix..."
	make install >/dev/null 2>/dev/null
}

main(){
	log "Dependency grab started on $(date +"%x %I:%M %p").";
	triplet_install "libvncserver";
	triplet_install "cairo";
	triplet_install "dlfcn";
	triplet_install "boost";
	triplet_install "sqlite3";
	triplet_install "cyrus-sasl";

	# Install other deps
	[[ ! -d "cvmlib_src32/" ]] && mkdir cvmlib_src32;
	cd cvmlib_src32;
	log "Compiling local copy of libodb..";
	tar_xrm https://www.codesynthesis.com/download/odb/2.4/libodb-2.4.0.tar.gz "libodb-2.4.0.tar.gz"
	cd libodb-2.4.0
	configure_build;
	cd ..
	log "Compiling local copy of libodb-sqlite..";
	tar_xrm https://www.codesynthesis.com/download/odb/2.4/libodb-sqlite-2.4.0.tar.gz "libodb-sqlite-2.4.0.tar.gz"
	cd libodb-sqlite-2.4.0
	configure_build;
	cd ..
	log "Compiling local copy of ossp uuid..";
	git clone https://github.com/sean-/ossp-uuid.git
	cd ossp-uuid
	configure_build_uuid;
	log "All source dependencies built."
	cd ..;
	log "Dependency grab finished.";
};main;
