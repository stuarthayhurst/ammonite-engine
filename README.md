<p align="center">
  <img src="https://github.com/stuarthayhurst/ammonite/raw/master/assets/icons/icon.svg" alt="ammonite" width="200px">
</p>

## Ammonite Engine
[![Donate](https://img.shields.io/badge/Donate-PayPal-green.svg)](https://paypal.me/stuartahayhurst)
  - A simple OpenGL based graphics engine, built to learn C++ and graphics programming
    - This isn't a serious, production-quality engine, please don't treat it as one
  - Despite the project being for learning, any contributions are still welcome
    - See `docs/CONTRIBUTING.md` to get started
    - If you found this project interesting, any donations are greatly appreciated :)

## Features:
  - Model loading, using `libassimp`
  - Shader program caching
  - Key binding and mouse input support
  - Command prompt for engine control
  - Miscellaneous utilities (thread pool, thread-safe logging, timers, random number generation)

## Requirements:
  - A `c++23` compatible compiler (`g++ (16+)` / `clang (18+)`)
    - Toolchain support for thread-local variables
    - All build and runtime dependencies installed
  - A 64-bit Linux system
    - Some functions may have AVX-512 / VAES accelerated versions
    - Build with `FAST=true` to enable these, if supported
  - An OpenGL 4.5+ compatible driver
  - Alternatively, an OpenGL 3.2+ driver supporting the following extensions can be used
    - `ARB_direct_state_access`
    - `ARB_shader_storage_buffer_object`
    - `ARB_texture_storage`
    - `ARB_texture_cube_map_array`
  - OpenGL debugging is supported with `KHR_debug`
  - Program caching is supported with `ARB_get_program_binary`
  - No error contexts are supported with `KHR_no_error`

## Building + installing libammonite:
  - `make library`
  - `sudo make install`

## Running a demo:
  - `make build` will compile the library and all demos on the current branch
  - Demos can be listed with `./launch.sh --demo`
  - Run a specific demo with `./launch.sh --demo [DEMO]`
    - For example: `./launch.sh --demo object-field`
    - Running the binary directly will only work if `libammonite` is installed to the system
  - `make clean` will clean the build area, to start from fresh
  - Screenshots of some demos can be found at the end of the README

## Options:
  - Compiled demos have a few arguments supported:
    - `--help`: Displays a help menu
    - `--benchmark`: Start a benchmark
    - `--demo`: List available demos
    - `--demo [DEMO]`: Launch a specific demo
    - `--vsync`: Enable / disable VSync (`true` / `false`)

## Build system:
  - ### Targets:
    - `build` and `library` support `-j[CORE COUNT]`
    - `make build` - Builds the demo
    - `make library` - Builds `build/libammonite.so`
    - `make install` - Installs `libammonite.so` to the system
    - `make headers` - Installs Ammonite headers and `ammonite.pc` to the system
    - `make uninstall` - Removes installed library and headers
      - Custom install locations must be specified the same way as they were installed
    - `make clean` - Cleans the build area (`build/`) and default runtime cache (`cache/`)
  - All targets and optional flags are documented [here](docs/CONTRIBUTING.md#build-system)
  - Set the environment variable `PREFIX_DIR` to configure the base install path
    - The `install`, `headers` and `uninstall` targets have additional path options

## Dependencies:
  - Package names are correct for Debian, other distros may vary
  - `make`
  - `pkgconf`
  - `coreutils`
  - `sed`
  - `python3`
  - `g++` **OR** `clang`
    - If using clang, use `CXX="clang++" make [TARGET]`
      - `USE_LLVM_CPP=true` might be useful for systems without (a new enough) GCC
    - When swapping between different compilers, run `make clean`
  - ### Libraries:
    - `libglm-dev libglfw3-dev libepoxy-dev libstb-dev libassimp-dev`
    - `libdecor-0-0 libdecor-0-plugin-1-gtk` are required for Wayland window decorations
  - ### Linting:
    - `clang-tidy (19+)`
  - ### Icons:
    - `inkscape optipng`

## Issues:
  - Due to the small size of this project, only a small range of hardware can be tested
    - If you hardware / driver meets the requirements listed [here](#requirements), but the engine doesn't work, please file a bug report
  - Issues, feature requests and bug reports can be filed [here](https://github.com/stuarthayhurst/ammonite/issues)
  - Feel free to work on any issues / feature ideas, suggestions are welcome :)

## Usage:
  - Some very basic usage information can be found in `docs/USAGE.md`
  - Better documentation is planned in the future

## Screenshots:
<p align="center">
  <img src="https://github.com/stuarthayhurst/ammonite/raw/master/docs/demo-1.png" alt="Demo 1">
</p>
<p align="center">
  <img src="https://github.com/stuarthayhurst/ammonite/raw/master/docs/demo-2.png" alt="Demo 2">
</p>

## Credits:
 - Some models in `assets/` may have been created by third parties, attribution can be found in `docs/CREDITS.md`
 - Reference materials:
   - [OpenGL Tutorial](https://www.opengl-tutorial.org/)
   - [Learn OpenGL](https://learnopengl.com/Introduction)

## License
  - This project is available under the terms of the MIT License
    - These terms can be found in `LICENCE.txt`
