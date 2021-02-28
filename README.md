## collab-vm-server 1.2.11
This repository contains the source code to the CollabVM Server. The CollabVM Server (obviously) powers CollabVM, and it is what you will use to host a CollabVM UserVM or mirror.

You can either download already compiled binaries, or compile the CollabVM Server yourself. Compilation instructions are provided in the BUILDING.md file. 

CollabVM was coded and written by Cosmic Sans, Dartz, Geodude, and modeco80.

### License
CollabVM Server, as well as the Web App and Admin Web App are licensed under the [Apache License](https://www.apache.org/licenses/LICENSE-2.0).

### How to use
To start the server, first make sure it has executable permissions and then type ./collab-vm-server (port) (HTTP Directory (optional)). 

For example, to start CollabVM Server on port 6004 with the directory for the HTML files served from a folder called "collabvm", type `./collab-vm-server 6004 collabvm`.

The HTTP Directory is optional, if not provided it looks for a folder called "http".

When you start the server for the first time, you'll get a message that a new database was created. 

The first thing you'll want to do is go to the admin panel which is located at localhost:(port)/admin/config.html. Type the password "collabvm" (No quotes). 

The first thing you'll want to do after entering the admin panel is immediately change the password as it is highly insecure.

After that, you can create a VM. CollabVM currently only supports QEMU at this time, however plans for supporting other VM software are in the works.

For more information on the admin panel and its usage, visit [the wiki page on the Admin Panel](https://computernewb.com/wiki/CollabVM%20Server%201.x/Admin%20Panel).
