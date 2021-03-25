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

main(){
	log "Dependency grab started on $(date +"%x %I:%M %p").";
        if [[ ! -f "$CVM_HOME/setup.exe" ]];then log "Please copy the Cygwin setup file to $CVM_HOME/setup.exe before running this script.";else
	$CVM_HOME/setup.exe -X -q -W -P libvncserver-devel,libcairo-devel,libboost-devel,libsqlite3-devel,libsasl2-devel,libturbojpeg-devel,libjpeg-devel,wget,git,make,unzip
	if [ "$(uname -m)" == "x86_64" ]; then
		$CVM_HOME/setup.exe -X -q -W -s http://ctm.crouchingtigerhiddenfruitbat.org/pub/cygwin/circa/64bit/2016/08/30/104235 -P gcc-core,gcc-g++
	else
		$CVM_HOME/setup.exe -X -q -W -s http://ctm.crouchingtigerhiddenfruitbat.org/pub/cygwin/circa/2016/08/30/104223 -P gcc-core,gcc-g++
	fi
	cd ..;
	log "Dependency grab finished.";
        fi
};main;
