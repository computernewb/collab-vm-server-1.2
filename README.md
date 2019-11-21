
## collab-vm-server
This repository contains all the necessary source files to compile the collab-vm-server. The collab-vm-server powers Collab VM, and it is what you use to host it. You can either download already compiled binaries or compile it yourself. Compilation instructions are below. 

Collab VM was coded and written by Cosmic Sans, Dartz, and Geodude.

Live Demo: http://computernewb.com/collab-vm/

### How to use
To start collab-vm-server make sure it has executable permissions and type ./collab-vm-server (port) (HTTP Directory (optional)). For instance, if you want to start collab-vm-server on port 6004 with the directory for the HTML files in a folder called "collabvm" you'd type ./collab-vm-server 6004 collabvm 

The HTTP Directory is optional, by default, it looks and will use a folder called http.

When you start the server you'll receive a message that a new database was created. The first thing you will want to do is configure the admin panel javascript to make sure it works properly. Go into the http directory, admin, open admin.min.js with a text editor, replace any instance of 127.0.0.1:6004 with your server's IP (or you can keep it localhost if you really want to). Afterwards, go to http://(your localhost):6004/admin/ and login with either the password "collabvm". The first thing we recommend is changing this password immediately because it is highly insecure. Type in the VM name you want to use, link, the startup command, choose your snapshot mode, and make sure auto start is checked if you want it to start with collab-vm-server! 

### Compilation
Compilation was only tested on i386, amd64, and armhf machines on Linux and Windows. It is unknown if this will work on any other architecture but theoretically Collab VM Server should be able to run on MIPS or PowerPC.

#### Windows
You need MSYS2 with MinGW-w64 to compile the server properly.

Visual Studio is currently not supported, but if you have successfully compiled collab-vm-server with Visual Studio, let us know.

To compile on Microsoft Windows you need the dependencies for Guacamole. You can compile these yourself but it's easier to cross-compile them from Linux or find already compiled libraries for Guacamole on Windows, as these were designed for Linux.

To compile the database files you need ODB, available [here.](http://www.codesynthesis.com/products/odb/download.xhtml)
 - Grab the executable for your platform from the website.
	 - Go to the src/Database folder and type this command in: `odb -d sqlite -s -q Config.h VMSettings.h`

MSYS2 with MinGW-w64:
Run the following commands:

```
pacman -Syu
# Restart MSYS for the arch you want to compile (e.g on win32 MSYS MinGW 32bit, or on Win64 MSYS MinGW x86_64)
# Install the Win32 compiler
pacman -S --noconf mingw-w64-i686-toolchain git
# Restart MSYS again
# Install the Win64 compiler
pacman -S --noconf mingw-w64-x86_64-toolchain git
# Restart MSYS again.
# Get all of the CollabVM Server dependencies (for Win64).
./scripts/grab_deps_mw64.sh
# Get all of the CollabVM Server dependencies (for Win32).
./scripts/grab_deps_mw32.sh
# If you get a Permission Denied error, go into the scripts directory and type chmod +x *.sh.
make
# Or for Win32.
make WARCH=w32
```

#### Linux

Download ODB from either [the CodeSynthesis website](http://www.codesynthesis.com/products/odb/download.xhtml) or download it from your local repositories. (On Ubuntu, you can type `sudo apt install -y odb`).

Then run the following commands:

```
# on Arch Linux
sudo pacman -S base-devel
# on Debian/Ubuntu
sudo apt install -y build-essential
# on Fedora/Red Hat/CentOS/Etc
sudo yum groupinstall 'Development Tools'
# Get all of the CollabVM Server dependencies for Linux.
# If you get a Permission Denied error, go into the scripts directory and type chmod +x *.sh.
./scripts/grab_deps_linux.sh 
# Finally, build the CollabVM Server (This now includes ODB compile steps).
make
```

#### MacOS X
**NOTE**: This is untested and isn't guaranteed to work.

- Open a Terminal and a web browser. Go to https://brew.sh.
- Copy the command from the home page.
- Enter your password if it asks and start the installation.
- Once it is finished installing, run the following command: **brew install boost cairo gcc ossp-uuid sqlite3 **
- Because libvncserver is not available in the Homebrew Forumlas, it will have to be compiled manually. Type **git clone https://github.com/LibVNC/libvncserver**. Make sure you have the dependencies and build it with **cmake -DLIBVNCSERVER_WITH_WEBSOCKETS=OFF**
- Compile libodb and libodb-sqlite by downloading the files from Code Synthesis's website.
- Verify all the dependencies have been successfully installed and build the solution.

#### BSD
While the server should build on BSD-based operating systems, I don't have a BSD computer to test this on so I don't know what the build instructions are, but they should be the same as Linux. (If you get it to compile and you can write instructions, that'd be vastly appreciated!)

####  Others?
QEMU is also available for other platforms, like OpenSolaris and OpenIndiana. collab-vm-server is only supported on Windows and Unix-like operating systems. It is unknown if it would run on any other operating systems, but your free to try (let me know if it works). There wouldn't be any point in porting the server to any other platform where QEMU is not supported, however (although in the future, there will be many more emulators/hypervisors supported).

### All Required Dependencies
* Boost (I've only tested 1.68, however older versions with a proper chrono::steady_timer constructor that allow a delayed construction should work)
* GCC 5 (at least)
* libvncserver 
* libpng
* libturbojpeg
* libsqlite3-dev
* libodb
* libodb-sqlite
* libossp-uuid-dev
* RapidJSON
* UriParser
* Websocket++

### License
Collab VM Server, as well as the Web App and Admin Web App are licensed under the [Apache License](https://www.apache.org/licenses/LICENSE-2.0).

