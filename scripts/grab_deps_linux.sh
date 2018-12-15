#!/bin/bash
CVM_HOME=$(pwd)

log(){
	printf "[>] $1\n"
}

triplet_install(){
	IS_DEB="false";

	[[ ! "$(which dpkg 2>/dev/null)" == "" ]] && IS_DEB="true";

	# TODO: expand this to detect other package managers
	[[ "$IS_DEB" == "false" ]] && sudo pacman -S --noconf $1 && return;

	if [ "$1" == "cairo" ]; then
		[[  "$IS_DEB"  == "true" ]] && sudo apt install -y libcairo2-dev;
		return;
	fi
	if [ "$1" == "sqlite" ]; then
		[[  "$IS_DEB"  == "true" ]] && sudo apt install -y libsqlite3-dev;
		return;
	fi
	if [ "$1" == "boost" ]; then
		# TODO: we may have to hack in building boost 1.68 in some cases
		# (e.g: xenial/deb8)
		[[  "$IS_DEB"  == "true" ]] && sudo apt install -y libboost-dev;
		return;
	fi
	[[ "$IS_DEB" == "true" ]] && sudo apt install -y $1-dev;
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
	CPPFLAGS="-I$CVM_HOME/cvmlib_lx/include" LDFLAGS="-L$CVM_HOME/cvmlib_lx/lib" ./configure --prefix="$CVM_HOME/cvmlib_lx" >/dev/null 2>/dev/null;
	log "Building..."
	make -j$(nproc) >/dev/null 2>/dev/null
	log "Installing into prefix..."
	make install >/dev/null 2>/dev/null
}

configure_build_uuid(){
	# Behaviour to place includes correctly (ossp/)
	log "Configuring..."
	CPPFLAGS="-I$CVM_HOME/cvmlib_lx/include" LDFLAGS="-L$CVM_HOME/cvmlib_lx/lib" ./configure --prefix="$CVM_HOME/cvmlib_lx" --includedir="$CVM_HOME/cvmlib_lx/include/ossp" >/dev/null 2>/dev/null;
	log "Building..."
	make -j$(nproc) >/dev/null 2>/dev/null
	log "Installing into prefix..."
	make install >/dev/null 2>/dev/null
}

main(){
	log "Dependency grab started on $(date +"%x %I:%M %p").";
	triplet_install "libvncserver";
	triplet_install "cairo";
	triplet_install "boost";
	triplet_install "sqlite";

	# Install other deps
	[[ ! -d "cvmlib_src_lx/" ]] && mkdir cvmlib_src_lx;
	cd cvmlib_src_lx;

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

