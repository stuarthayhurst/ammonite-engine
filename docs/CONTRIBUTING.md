# Contributing to Ammonite
## Overview:
  - Contributions and pull requests are very welcome, this project can always use extra help
    - All contributions must be allowed under the license (`LICENCE.md`)
    - All contributions must abide by the Code of Conduct (`docs/CODE_OF_CONDUCT.md`)
  - The general process to contribute would be as follows:
    - Fork the project, optionally create a branch from that
    - Make your change and write a sensible commit message
    - Ensure the project builds without any warnings, passes the linter and all workflows pass
    - Create a merge request, and follow the template
  - The aim of this document is to provide technical information, not be a general how-to guide

## Build system:
  - ### Targets:
    - `build`, `debug`, `library`, `demo`, `threads` and `lint` support `-j[CORE COUNT]`
    - `make build` - Builds the demo and thread demo
    - `make debug` - Cleans build directory, then runs `make build` in debug mode
    - `make library` - Builds `build/libammonite.so`
    - `make demo` - Builds a demo binary, a working demonstration of the renderer
    - `make threads` - Builds a test program for the thread pool
    - `make install` - Installs `libammonite.so` to system directories
      - The install path can be configured, by setting the environment variable `INSTALL_DIR`
    - `make headers` - Installs Ammonite headers to the system
      - The install path can be configured, by setting the environment variable `HEADER_DIR`
    - `make uninstall` - Removes installed library
      - Custom install locations can be removed using the environment variable `INSTALL_DIR`
    - `make icons` - Creates `assets/icons/icon-*.png` from `assets/icons/icon.svg`
    - `make lint` - Lints the project using `clang-tidy`
    - `make clean` - Cleans the build area (`build/`) and default runtime cache (`cache/`)
    - `make cache` - Clears the default runtime binary cache, useful if running into issues with caching
  - ### Flags:
    - `DEBUG`: `true / false` - Compiles the target in debug mode
    - `FAST`: `true / false` - Compiles with `-march=native` and uses a no-error context
    - `INSTALL_DIR` - Install `libammonite.so` to a different location
    - `HEADER_DIR` - Install Ammonite headers to a different location
    - `CHECK_LEAKS` - Enables `-fsanitize=leak`

## Debug mode:
  - To compile in debug mode, use `make debug` or `DEBUG=true make ...`
    - This enables additional checks and debug output from the engine
    - Makes use of `-fsanitize=address,undefined,leak`
    - It'll also enable graphics API debug warnings, messages and errors
      - This will use a debug graphics context, if available
    - Each object is compiled with debugging symbols, and `strip` is skipped
  - Before swapping back to regular builds, run `make clean`
  - `make debug` will create a fresh build every time
  - `DEBUG=true make ...` can be used on any target, and only rebuilds changed objects
    - If an initial `make clean` isn't used, the build may fail or produce broken results

## Miscellaneous help:
  - Spellings should use British English for both documentation and code
  - New or modified build system targets need to be documented in the "Build system" section
    - Non-development related targets should also be documented in `README.md`
  - Any scripts should be placed in `scripts/`
  - Icons should be regenerated using `make icons`
