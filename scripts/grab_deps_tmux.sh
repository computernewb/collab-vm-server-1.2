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
    pkg install -y git boost libsqlite wget clang make autoconf automake libtool pkg-config libjpeg-turbo libpng libcairo
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
	wget "https://git.savannah.gnu.org/gitweb/?p=config.git;a=blob_plain;f=config.guess" -O config.guess
	wget "https://git.savannah.gnu.org/gitweb/?p=config.git;a=blob_plain;f=config.sub" -O config.sub
	configure_build_uuid;
	cd ..
	log "Compiling local copy of LibVNCServer..";
	git clone https://github.com/LibVNC/libvncserver
	cd libvncserver
	git checkout 8415ff4
	sed -i 's/ __THROW//g' common/md5.h
	sed -i -e 's/KEY_SOFT1, KEY_SOFT2, //g' -e '/KEY_CENTER/d' -e '/KEY_SHARP/d' -e '/KEY_STAR/d' examples/android/jni/fbvncserver.c
	autoreconf -fi
	configure_build;
	log "All source dependencies built.";
	log "Dependency grab finished.";
	log "Be sure to copy the ODB-compiled files (*-odb.*xx) to $CVM_HOME/obj.";
};main;
