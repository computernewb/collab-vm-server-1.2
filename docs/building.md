Compiling the Collab3 monorepo code is fairly easy.

# Notice

Windows is not supported by the Collab3 developers, and the project will not configure if targeting a Windows platform.

This is an intentional change, and it will not be reverted.
# Dependencies

GCC 11+ or Clang 14+ are recommended compiler versions, however newer versions are acceptable too.

Libraries:

* Boost 1.75 or above
* fmt
* spdlog

You can use either system library versions (if preferred) or use Conan.

TODO: it is planned to use conan by default, and have system libraries be opt-in.

# Building Collab3 Monorepo C++ Code

The general formula is:

```bash
$ cmake -B build --preset $PRESET
$ cmake --build build
# ...
# profit?
```

where $PRESET can be:

- release : Release build
- debug : Debug build
- debug-asan : Debug build (with AddressSanitizer enabled)
- debug-tsan : Debug build (with ThreadSanitizer enabled)
- debug-ubsan : Debug build (with UBSanitizer enabled)

There are also others, but they are *only* for use by our CI, and shouldn't be used by users directly.

For now, you'll get one `collab3-host` executable in the build root. This is the one you should run.

<!-- TODO: The agent will need another configure/build step. -->

# Building Collab3 Monorepo Documentation

Note that usually you won't have to do this, as the [web documentation](https://computernewb.github.io/collab3) will update
upon any changes to the documentation in the `master` branch. 

For development/local preview purposes, though:

## Prerequisites
- `mkdocs`

Run `mkdocs build`. 

The resulting output will be in `site/`, and you can host this wherever you want.
