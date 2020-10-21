#!/bin/bash
CVM_HOME=$(pwd)

log(){
	printf "$1\n"
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
	CPPFLAGS="-I$CVM_HOME/cvmlib/include" LDFLAGS="-L$CVM_HOME/cvmlib/lib -lpthread" ./configure --prefix="$CVM_HOME/cvmlib";
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
        if [[ ! -f "$CVM_HOME/setup.exe" ]];then log "Please copy the Cygwin setup file to $CVM_HOME/setup.exe before running this script.";else
	$CVM_HOME/setup.exe -X -q -W -P libvncserver-devel,libcairo-devel,libboost-devel,libsqlite3-devel,libsasl2-devel,libturbojpeg-devel,libjpeg-devel,wget,git,make,unzip
	if [ "$(uname -m)" == "x86_64" ]; then
		$CVM_HOME/setup.exe -X -q -W -s http://ctm.crouchingtigerhiddenfruitbat.org/pub/cygwin/circa/64bit/2016/08/30/104235 -P gcc-core,gcc-g++
	else
		$CVM_HOME/setup.exe -X -q -W -s http://ctm.crouchingtigerhiddenfruitbat.org/pub/cygwin/circa/2016/08/30/104223 -P gcc-core,gcc-g++
	fi
	# Install other deps
	[[ ! -d "cvmlib_src/" ]] && mkdir cvmlib_src;
	[[ ! -d "cvmlib/" ]] && mkdir cvmlib;
	cd cvmlib_src;
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
	log "Downloading ODB compiler.."
	cd ../../cvmlib
	wget https://www.codesynthesis.com/download/odb/2.4/odb-2.4.0-i686-windows.zip
	unzip -qq -u odb-2.4.0-i686-windows.zip
	cd ..;
	log "Dependency grab finished.";
        fi
};main;
