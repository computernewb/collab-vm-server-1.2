Compiling the Collab3 monorepo code is fairly easy.

# Notice

Windows is not supported by the Collab3 developers, and the project will not configure if targeting a Windows platform.

This is an intentional change, and it will not be reverted until:

- DBus p2p works on Windows
- Windows QEMU supports modules, and passing handles to and from QEMU and another process

# Dependencies

GCC 11+ or Clang 14+ are recommended compiler versions, however newer versions are acceptable too.

Libraries:

* Boost 1.75 or above
* fmt
* spdlog

You can use either system library versions (if preferred) or use vcpkg for the grunt of them.

# Building Collab3 Monorepo C++ Code

```bash
$ cmake -B build -GNinja -DCMAKE_BUILD_TYPE=Release
$ cd build
$ cmake --build . # or if you know your generator, use make, ninja, whatever.
# ...
# profit?
```

To use vcpkg, add `-DCMAKE_TOOLCHAIN_FILE=$VCPKG/scripts/buildsystems/vcpkg.cmake` to the first CMake invocation (clue:
the one with `-B build`).

For now, you'll get one `collab3-host` executable in the build root. This is the one you should run.

<!-- TODO: The agent will need another configure/build step.. -->

# Building Collab3 Monorepo Documentation

Note that usually you won't have to do this, as the [web site](https://computernewb.github.io/collab3) will update
upon any changes to documentation. For development purposes though:

You'll need to install `mkdocs` via pip.

Run `mkdocs build`. 

The resulting output will be in `site/`, and you can host this wherever you want.