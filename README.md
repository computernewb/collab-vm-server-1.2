# collab-vm-server
This repository contains all the necessary source files to compile the collab-vm-server. The collab-vm-server powers Collab VM, and it is what you use to host it. You can either download already compiled binaries or compile it yourself. Compilation instructions are below. 

Collab VM was coded and written by Cosmic Sans, Dartz, and Geodude.

Live Demo: http://computernewb.com/collab-vm/

# How to use
To start collab-vm-server make sure it has executable permissions and type ./collab-vm-server (port) (HTTP Directory (optional)). For instance, if you want to start collab-vm-server on port 6004 with the directory for the HTML files in a folder called "collabvm" you'd type ./collab-vm-server 6004 collabvm 

The HTTP Directory is optional, by default, it looks and will use a folder called http.

When you start the server you'll receive a message that a new database was created. The first thing you will want to do is configure the admin panel javascript to make sure it works properly. Go into the http directory, admin, open admin.min.js with a text editor, replace any instance of 127.0.0.1:6004 with your server's IP (or you can keep it localhost if you really want to). Afterwards, go to http://(your localhost):6004/admin/ and login with either the password "collabvm". The first thing we recommend is changing this password immediately because it is highly insecure. Type in the VM name you want to use, link, the startup command, choose your snapshot mode, and make sure auto start is checked if you want it to start with collab-vm-server! 

# Compilation
Compilation is semi-complicated as this was intended for personal use only, sorry. In the future there will probably be easier compilation methods.

By the way, compilation was only tested on i386 and amd64 machines. I don't know if this will work on any other architecture, but theoretically, Collab VM Server should be able to run on armhf or powerpc.

Having trouble compiling collab-vm-server? Download a build environment here: https://github.com/computernewb/collab-vm-server/wiki/Collab-VM-Server-Build-Environment

The username and password to the build enivornment is "dartz", no quotes or capitals.

## Windows
You need Visual Studio 2015, Cygwin or MinGW with MSYS2 to compile the server properly.

To compile on Microsoft Windows you need the dependencies for Guacamole. You can compile these yourself but it's easier to cross-compile them from Linux or find already compiled libraries for Guacamole on Windows, as these were designed for Linux.

Visual Studio 2015: Open the collab-vm-server.sln file and make sure before anything you go to Project > collab-vm-server Properties > C/C++ > Additional Include Directories then make sure to select the location of the header files. Next go to Project > collab-vm-server Properties > Linker > General and change the Additional Library Directories to include the location of the .dll and .lib files. Then compile the server.

Cygwin/MinGW: Instructions are the same for Linux.

To compile the database files you need ODB, available here: http://www.codesynthesis.com/products/odb/download.xhtml Grab the exe from the website.

## Linux
This is specifically for Debian-based distributions like Ubuntu but it should work in most distributions of Linux. Like Windows the dependencies for Guacamole are required. You can download these from the Ubuntu package manager. These instructions also apply for Windows Subsystem for Linux.

The first thing we need to do is compile the databases with ODB. You can grab it here: http://www.codesynthesis.com/products/odb/download.xhtml Download the .deb, .rpm, etc. Note if you aren't on i386 or amd64 Linux you will need to compile it yourself. The source code for odb is available on the same page.

Go to collab-vm-server/src/Database and type this command in: odb -d sqlite -s -q Config.h VMSettings.h

Make sure you have the mysql and sqlite database runtime libraries before compiling, again available in the CodeSynthesis website. 

Next we go into the collab-vm-server folder and type in the following commands:

aclocal

automake --add-missing

autoconf

./configure


Then finally, type make to compile the whole thing.

If you cannot compile, open src/Makefile.am and make sure the library paths are correct.

## Mac OS
I don't have MacOS, so I can't tell you how to compile it there or if these instructions are different. If someone can compile it and can make a pull request with instructions, it would be vastly appreciated.

## BSD
I don't have a computer running any variant of BSD, so the instructions might be different. If anyone can compile it and make a pull request with instructions, that would be great.  

## Any other operating systems
QEMU is also available for other platforms, like OpenSolaris and OpenIndiana. collab-vm-server is only supported on Windows and Unix-like operating systems. It is unknown if it would run on any other operating systems, but your free to try (let me know if it works). There wouldn't be any point in porting the server to any other platform where QEMU is not supported, however (although in the future, there will be many more emulators/hypervisors supported).

# Required Dependencies
* All of Guacamole's Dependencies (https://guacamole.incubator.apache.org/doc/gug/installing-guacamole.html)
* Guacamole
* libsqlite3-dev
* ODB
* ODB MySQL
* ODB SQLite
* RapidJSON
* UriParser
* Websocket++

# License
Collab VM Server, as well as the Web App and Admin Web App are licensed under the [Apache License](https://www.apache.org/licenses/LICENSE-2.0).
