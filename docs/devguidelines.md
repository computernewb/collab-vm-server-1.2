# Development guidelines

These are some guidelines for code which is part of the repo.

# JavaScript/TypeScript

To be written, but for now, just don't write some IE8 tier stuff and you'll be fine.

# C++

## Formatting

Use clang-format.

## Naming

Use .cpp for source files, .hpp for headers, and .cppm for module interface units.

Inline functions do *not* need another implementation file, and inline class member functions should be implemented inside the class body.

Function and type names should be in PascalCase.

Variable names are camelCase.

Underscores should only be used very rarely for private/introspection only fields which *need* to be public (for instance; in classes which need to stay POD), as they risk UB.

Class members have no special prefix.

Example (unformatted because I'm lazy):
```cpp
struct MyStructure {
    
    void DoThing();
    
private:
    
    std::uint32_t thing{};
    std::uint32_t otherThing{};
    
};

void FreeFunction();
```

## Language Subset

The Collab3 C++ code uses the C++ language, obviously, but we do not use some features
of the C++ language. The later sections describe our C++ language subset in depth.

### Exceptions

Exceptions and the `throw` keyword are banned in the collab3 server code.
Rather:

- Non-fatal errors are `collab3::core::Result<Value, Error>`
  - This currently is an alias to `tl::expected`.
  - We do not use `std::error_code`, as it's pretty awful and also requires polymorphism & other iffy things.
    - `core::Error<EC>` & `core::ErrorCategory<EC>` are our reimplementation, which makes `sizeof(Error<EC>) == sizeof(EC)`.

- Fatal/unrecoverable errors should call `collab3::core::Panic()` with an optional message.
  - The affected application will shut down with a message and print a stack trace (with symbolized names in Debug builds).
    - `COLLAB3_CORE_ASSERT(...)` and `COLLAB3_CORE_VERIFY(...)` both call Panic() upon failure.
      - `_EXPECT(..., Error)` variants will instead return a Result initialized with an `Error`.

Exceptions to this rule:
- Constant expressions are allowed to use the `throw` keyword to induce
  compile time errors, as this is pretty much the only way to cleanly do so.

- Any code which currently throws before this rule was documented (refactor it out!!!)

### RTTI

RTTI is allowed, but mostly because it'd be annoying to have to ban it.