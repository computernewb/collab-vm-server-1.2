## vmrun

vmrun is a libexec-binary installed into `$PREFIX/libexec/collab-vm-server`. It handles

- Setting up CGroup2, and said limits
- Creating PID/UTS/Net/Mount namespaces (effectively setting up a container, for QEMU specifically)
- Moving VM tap into the netns
- Using seccomp/Kafka to allow the specific set of system calls QEMU requires
- Creating system bind mounts (so that QEMU will run, but nothing else)
- Creating bind mounts of certain directories to allow read-only access
- Creating tap FD's to give QEMU tap access
- Launching and supervising QEMU, while giving a custom TIPC based socket protocol for the VM controller to use.

Once it is launched, it daemonizes, and writes its pid file 

This architecure was chosen as it:

- delegates the responsibility of supervising QEMU to a binary with (far) less code, which could in theory
	run unattended.. which then..

- Allows the collab-vm-server to crash, while keeping all vmrun VM's up. Optionally if old behaviour is desired,
	the QEMU plugin can kill the vmrun instance and start a new one, if connection succeeds the first time?

- Makes any found QEMU/KVM exploits far less severe (sofar more so than our current system)