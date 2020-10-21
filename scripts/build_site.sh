#!/bin/bash
# Webpage build script
# (Preprocesses out strings)

log(){
	printf "$1\n"
}

build(){
	log "Build of webpage started on $(date +"%x %I:%M %p").";
	DATE=$(date +"%x %I:%M %p")
	UNAME_ARCH=$1
	UNAME_KERN=$(uname -srv)
	if [[ "$(uname -s)" == *"MINGW64_NT"* || "$(uname -s)" == *"MINGW32_NT"* || "$(uname -s)" == *"CYGWIN_NT"* ]]; then
		# Extract the Windows version from the uname -s result of MINGW64_NT-<WINDOWS_VER>
		# (means we can't get any details (e.g Service Packs),
		# but I'm not sure it matters *that* much)
		WIN_VER=$(printf "$(uname -s)" | cut -d "-" -f 2)
		# Replace the "internal" Windows version number with the
		# "release" version number/name.
		case $WIN_VER in
			"5.1" ) WIN_VER="XP";;
			"5.2" ) WIN_VER="Server 2003";;
			"6.0" ) WIN_VER="Vista";;
			"6.1" ) WIN_VER="7";;
			"6.2" ) WIN_VER="8";;
			"6.3" ) WIN_VER="8.1";;
			"10.0" ) WIN_VER="10";;
		esac
		UNAME_KERN=$(printf "Windows %s" $WIN_VER)
		log "Building on $UNAME_KERN $(uname -m).";
	else
		# LSB defines /etc/os-release (Some distros also use lsb-release, however os-release is ACTUALLY the one LSB defines as standard, and thus will be pressent in more
		# than a couple distros)
		UNAME_KERN=$(cat /etc/os-release | grep "PRETTY_NAME" | sed 's/PRETTY_NAME=//g' | sed 's/\"//g');
		log "Building on $UNAME_KERN.";
	fi

	log "Copying sources."
	[[ ! -d "http/" ]] && mkdir http
	cp -r http_src/* http/
	log "Preprocessing..."

	# Actually preprocess
	INSRC=$(cat http/index.html.in)
	INSRC=${INSRC//"[HOST_DATE]"/$DATE}
	INSRC=${INSRC//"[HOST_UNAME_ARCH]"/$UNAME_ARCH}
	INSRC=${INSRC//"[HOST_UNAME_OS]"/$UNAME_KERN}

	log "Writing preprocessed page(s)..."
	echo $INSRC > http/index.html.in
	mv http/index.html.in http/index.html
	log "Finished."
}; build $1;
