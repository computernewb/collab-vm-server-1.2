## Building collab-vm-server 

Compiling the server is fairly easy.

### All Required Dependencies

GCC 10+ or Clang 11+ are recommended compiler versions.

* Boost 1.75 or above
* libvncserver 


### CMake

These example directions are for Linux, but they should work basically everywhere CMake is.

```bash
$ cmake -B build -GNinja -DCMAKE_BUILD_TYPE=Release
$ cmake --build .
# ...
# profit?
```