# collab-vm-server
This repository contains all the necessary source files to compile the collab-vm-server. The collab-vm-server powers Collab VM, and it is what you use to host it. You can either download already compiled binaries or compile it yourself. Compilation instructions are below. 

Collab VM was coded and written by Cosmic Sans, Dartz, and Geodude.

Live Demo: http://computernewb.com/collab-vm/

# How to use
To start collab-vm-server make sure it has executable permissions and type ./collab-vm-server (port) (HTTP Directory (optional)). For instance, if you want to start collab-vm-server on port 6004 with the directory for the HTML files in a folder called "collabvm" you'd type ./collab-vm-server 6004 collabvm 

The HTTP Directory is optional, by default, it looks and will use a folder called http.

When you start the server you'll receive a message that a new database was created. The first thing you will want to do is configure the admin panel javascript to make sure it works properly. Go into the http directory, admin, open admin.min.js with a text editor, replace any instance of 127.0.0.1:6004 with your server's IP (or you can keep it localhost if you really want to). Afterwards, go to http://(your localhost):6004/admin/ and login with either the password "collabvm". The first thing we recommend is changing this password immediately because it is highly insecure. Type in the QEMU parameters, choose your snapshot mode, and make sure auto start is checked! 

By default when you download a pre-configured copy of collab-vm-server it will have a database and a micro Linux distribution that starts up automatically. Pre-configured copies will have the password collabvm. 

# Compilation
Compilation is semi-complicated as this was intended for personal use only, sorry. In the future there will probably be easier compilation methods.

By the way, compilation was only tested on i386 and amd64 machines. I don't know if this will work on any other architecture, but theoretically, Collab VM Server should be able to run on armhf or powerpc.

## Windows
You need Visual Studio 2015 to compile the server on Windows. Community (the free version) will work just fine.

To compile on Microsoft Windows you need the dependencies for Guacamole. You can compile these yourself but it's easier to find already compiled libraries for Guacamole on Windows since they were designed for Linux.

Open the collab-vm-server.sln file and make sure before anything you go to Project > collab-vm-server Properties > C/C++ > Additional Include Directories then make sure to select the location of the header files. Next go to Project > collab-vm-server Properties > Linker > General and change the Additional Library Directories to include the location of the .dll and .lib files.

To compile the database files you need ODB, available here: http://www.codesynthesis.com/products/odb/download.xhtml Grab the .deb, .rpm, or whatever you might have, and type odb when its done installing to make sure its working.

## Linux
This is specifically for Debian-based distributions like Ubuntu but it really should work in most distributions of Linux. Like Windows the dependencies for Guacamole are required. You can download these from the Ubuntu package manager. 

When you've got all the dependencies for Guacamole you need to get ODB. Again it is available in the link above. This is compiled with GCC 6, although it might work with older versions. 

The first thing we need to do is compile the databases. When you have odb ready, go to collab-vm-server/src/Database and type this command in:

odb -d sqlite -s -q Config.h VMSettings.h

Make sure you have the mysql and sqlite database runtime libraries before compiling, again available in the codesynthesis website. Just extract the odb folder from both of the zip files into collab-vm-server/src.

Next we go into the collab-vm-server folder and type in the following commands:

aclocal

automake --add-missing

autoconf

./configure

Then finally, type make to compile the whole thing.

If you cannot compile open src/Makefile.am and make sure the library paths are correct.

## Macintosh
I don't have a Macintosh computer so these instructions might be different on Mac OS. If anyone could write compilation instructions for the Mac this would be vastly appreciated.

## BSD
I don't have a computer running BSD so again the instructions could be different, although it should be pretty much the same as Linux. If anyone could write compilation instructions for BSD-based operating systems this would also be vastly appreciated.

## Any other operating systems
Collab VM Server is only supported on Windows and Unix-like operating systems, so I don't know if it would run on any other operating system. There would not be much point on porting Collab VM Server to a non-Linux/BSD/Mac/Windows system anyways because QEMU only runs on those platforms.

# Dependencies
* All of Guacamole's Dependencies (https://guacamole.incubator.apache.org/doc/gug/installing-guacamole.html)
* Asio (without Boost)
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
