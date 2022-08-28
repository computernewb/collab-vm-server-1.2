The Collab3 monorepo is a fully rewritten version of the CollabVM project.

# Philosophy

- One monorepo for everything in the project
  - Checking out the repository shouldn't be painful
  - Better development experience (one clone command versus 3-4...)
    - No project submodules (less time needed to update a superproject, more time to write code)
  
- Fast, readable, and modular modern C++20 code
  - Hacking on the codebase shouldn't be a daunting task 
    - Awaitable + coroutines really help with this on the ASIO front
  - Development & additional features encouraged by core developers
    - (hopefully made more possible by code clarity/design) 

- Microservices for each core part of the Collab3 server 
  - Currently supported by design with the modular server/api, server/... libraries
    - Which will be split into different services when ready.
