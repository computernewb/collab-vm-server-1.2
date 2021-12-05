## collab-vm-server
This repository contains the source code to the CollabVM Server. The CollabVM Server powers CollabVM, and it is what you will use to host a CollabVM UserVM or mirror.

You can either download already compiled binaries, or compile the CollabVM Server yourself. Compilation instructions are provided in the BUILDING.md file. 

CollabVM was coded and written by Cosmic Sans, Dartz, Geodude, and modeco80... And maybe a [few others.](https://github.com/computernewb/collab-vm-server/graphs/contributors)

### License
CollabVM Server is licensed under the [GNU Public License 3](https://www.gnu.org/licenses/gpl-3.0.en.html).

The CollabVM Server Plugin Interface "CVMpi" is licensed under the [Lesser GNU Public License 3](https://www.gnu.org/licenses/lgpl-3.0.en.html).

Currently, the Web App and Admin Web App are licensed under the [Apache License 2.0](https://www.apache.org/licenses/LICENSE-2.0).

### How to use
You can read `man doc/collab-vm-server.1`, for arguments, and setting it up is such as:

To start the server, first make sure it (the binary) has executable permissions, and you can write to the directory which you'll run collab-vm-server in (for the settings database) and then type ./collab-vm-server -p (port) -r (HTTP Directory (optional)). 

For example, to start CollabVM Server on TCP port 6004 with the directory for the HTML files served from a folder called "collabvm", type `./collab-vm-server -p 6004 -r collabvm`.

The HTTP Directory is optional, if not provided it looks for a folder called "http".

When you start the server for the first time, you'll get a message that a new database was created. 

The first thing you'll want to do is go to the admin panel which is located at localhost:(port)/admin/config.html. Type the password "collabvm" (No quotes). 

The first thing you'll want to do after entering the admin panel is immediately change the password as it is highly insecure.

After that, you can create a VM. CollabVM currently only supports QEMU at this time, however plans for supporting other VM software are in the works.

For more information on the admin panel and its usage, visit [the wiki page on the Admin Panel](https://computernewb.com/wiki/CollabVM%20Server%201.x/Admin%20Panel).
