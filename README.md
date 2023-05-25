<p align="center">
  <img src="https://github.com/stuarthayhurst/ammonite/raw/master/assets/icons/icon.svg" alt="ammonite" width="200px">
</p>

## Ammonite Engine
[![Donate](https://img.shields.io/badge/Donate-PayPal-green.svg)](https://paypal.me/stuartahayhurst)
  - A simple OpenGL based graphics engine, built to learn C++ and graphics programming
  - Despite the project being for learning any contributions are still welcome
    - If you found this project interesting, any donations are greatly appreciated :)

## Requirements:
  - A `c++20` compatible compiler (`g++ 10+`)
    - An OpenMP supported compiler + libraries installed
  - An OpenGL 4.5+ compatible driver
  - Alternatively, an OpenGL 3.2+ driver supporting the following extensions can be used
    - `ARB_direct_state_access`
    - `ARB_shader_storage_buffer_object`
    - `ARB_texture_storage`
    - `ARB_texture_cube_map_array`
  - OpenGL debugging is supported with `KHR_debug`
  - Program caching is supported with `ARB_get_program_binary`

## Building + installing libammonite:
  - `make library`
  - `sudo make install`

## Choosing a demo:
  - `make build` will compile the library and all demos on the current branch
  - Demos can be listed with `./launch.sh --demo`
  - Run a specific demo with `./launch.sh --demo [DEMO]`
    - For example: `./launch.sh --demo object-field`
  - Screenshots of some demos can be found at the end of the README

## Building + running a demo:
  - `make clean` will clean the build area, to start from fresh
  - Run `make build` to build the demo
    - Running the binary directly will only work if `libammonite` is installed to the system
  - Run `./launch.sh` to launch the demo, from within the project itself

## Options:
  - Compiled demos have a few arguments supported:
    - `--help`: Displays a help menu
    - `--benchmark`: Start a benchmark
    - `--demo`: List available demos
    - `--demo [DEMO]`: Launch a specific demo
    - `--vsync`: Enable / disable VSync (`true` / `false`)

## Debug mode:
  - To compile in debug mode, use `make debug` or `DEBUG=true make ...`
    - This will compile some extras in the code to help with debugging (every header gets `iostream`)
    - It will also enable OpenGL debug warnings, messages and errors
    - Before swapping back to regular builds, run `make clean`

## Build system:
  - ### Targets:
    - `build`, `debug` and `library` support building on multiple cores with `-jX`
    - `make build` - Builds demo binary, a working demonstration of the renderer
    - `make debug` - Cleans build directory, then runs `make build` in debug mode
    - `make library` - Builds `build/libammonite.so`
    - `make install` - Installs `libammonite.so` to system directories
      - The install path can be configured, by setting the environment variable `INSTALL_DIR`
    - `make headers` - Installs Ammonite headers to the system
      - The install path can be configured, by setting the environment variable `HEADER_DIR`
    - `make uninstall` - Removes installed library
      - Custom install locations can be removed using the environment variable `INSTALL_DIR`
    - `make icons` - Creates `assets/icons/icon.png` from `assets/icons/icon.svg`
    - `make clean` - Cleans the build area (`build/`) and default runtime cache (`cache/`)
    - `make cache` - Clears the default runtime binary cache, useful if running into issues with caching
  - ### Flags:
    - `DEBUG`: `true / false` - Compiles the target in debug mode
    - `FAST`: `true / false` - Compiles with `-Ofast -march=native`

## Dependencies:
  - `make`
  - `pkg-config`
  - `g++ libgomp1` **OR** `clang libomp-dev`
    - If using clang, `CXX="clang++" make [TARGET]`
    - When swapping between different compilers, run `make clean`
  - ### Libraries:
    - `libglm-dev libglfw3-dev libglew-dev libstb-dev libassimp-dev`
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

## Notes:
  - All targets are compiled with `-Wall` and `-Wextra`
  - Targets are also compiled with `-O3` and `-flto`
    - This should be fine for 99% of systems, but some may struggle to compile, or produce unstable results
    - These can be changed by modifying `CXXFLAGS` in `Makefile`

## Screenshots:
<p align="center">
  <img src="https://github.com/stuarthayhurst/ammonite/raw/master/docs/demo-1.png" alt="Demo 1">
</p>

## Credits:
 - Some models in `assets/` may have been created by third parties, attribution can be found in `docs/CREDITS.md`
 - Reference materials:
   - [OpenGL Tutorial](https://www.opengl-tutorial.org/)
   - [Learn OpenGL](https://learnopengl.com/Introduction)

## License
  - This project is available under the terms of the GNU Lesser General Public License (v3.0)
    - These terms can be found in `COPYING` and `COPYING.LESSER`
