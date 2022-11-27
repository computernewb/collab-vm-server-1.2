# Development guidelines

These are some guidelines for code which is part of the repo.

# Javascript

To be written, but for now don't write some IE8 tier stuff.

# C++

## Formatting

Use clang-format.

## Naming

Use .cpp for source files, .hpp for headers. Inline functions do *NOT* need another implementation file.

Function and type names should be in PascalCase.

Variable names are camelCase.

Underscores should only be used very rarely for private/introspection only fields which *need* to be public (for instance; in classes which need to stay POD), as they risk UB.

Class members have no special prefix (because if you're not sure, you can use this-> or the instance to qualify name lookup).

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

The Collab3 C++ code uses the C++ language, obviously, but we do not use some of the features
of the C++ language. The later sections describe our C++ language subset in depth.

### Exceptions

Exceptions and the `throw` keyword are banned wholly in the collab3 C++ code.
Rather:

- Non-fatal errors are `collab3::core::Result<Value, Error>`, which can also return/hold success cases (it's also cheap to pass, and doesn't allocate anything, unlike exceptions).

  One can handle errors without worrying about the logic being complicated ala:


    auto myResult = co_await MyCoroutineWhichRettysAResult(...);
  
    myResult.Map([](auto& res) {
        // My coroutine worked
    }).MapError([](auto& err) {
        // It did not.
    }).OrElse([]() {
        // Something went *hideously* wrong and the Result was defaulted.
        // (shouldn't need this case in most cases in "good" code.)
    });

- Fatal errors should call `collab3::core::Panic()` with an optional message.
  - The affected service will shut down with a message and print a stack trace (with symbolized names in Debug builds).
    - `COLLAB3_CORE_ASSERT(...)` and `COLLAB3_CORE_VERIFY(...)` both call Panic() upon failure.
      - `_EXPECT(..., ErrorType, ...)` variants will instead return a Result initialized with an `ErrorType`.

Exceptions to this rule:
- Constant expressions are allowed to use the `throw` keyword to induce
  compile time errors, as this is pretty much the only way to cleanly do so.

- Any code which currently throws before this rule was documented (refactor it out!!!)