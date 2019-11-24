#!/bin/bash
CVM_HOME=$(pwd)

log(){
	printf "[>] $1\n"
}

# TODO: expand to other package managers
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
		[[  "$IS_DEB"  == "true" ]] && sudo apt install -y libboost-all-dev;
		return;
	fi

	if [ "$1" == "turbojpeg" ]; then
		[[ "$IS_DEB" == "true" ]] && sudo apt install -y libturbojpeg0-dev;
		return
	fi

	if [ "$IS_DEB" == "true" ]; then
		if [  "$2" == "" ]; then
		  sudo apt install -y $1-dev;
		else
		  sudo apt install -y $1;
		fi
	fi
}

main() {
	log "Dependency grab started on $(date +"%x %I:%M %p").";
	triplet_install "libvncserver";
	triplet_install "cairo";
	triplet_install "boost";
	triplet_install "sqlite";
	triplet_install "odb" "nodev-but-this-string-is-meaningless";
	triplet_install "libossp-uuid";
	triplet_install "turbojpeg";

	log "Dependency grab finished.";
};main;

