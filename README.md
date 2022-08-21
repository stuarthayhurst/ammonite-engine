<p align="center">
  <img src="https://github.com/stuarthayhurst/ammonite/raw/master/assets/icons/icon.svg" alt="ammonite" width="200px">
</p>

## Ammonite Renderer
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
  - Individual demos are stored in separate git branches
  - `master` contains a very basic demo, largely for testing
  - Each branch ending in `-demo` contains a different demo
    - These can be listed using `git branch -a |grep "\demo" |grep -v "remotes"`
  - Each branch isn't always up to date with `master`
    - This can normally be sorted with `git rebase master [DEMO BRANCH]`
    - However, manual intervention is sometimes required
  - After choosing a branch, it can be compiled and run like normal

## Building + running demo:
  - `make clean` will clean the build area, to start from fresh
  - Run either `make build` or `make system-build; sudo make install`, to build the demo
    - `build` will only allow running from this project's root directory (recommended, as the code is only a demo)
    - `system-build` + `install` will build the demo, but use the system copy of `libammonite.so`
  - Run `./build/demo` to launch the demo

## Options:
  - Compiled demos have a few arguments supported:
    - `--help`: Displays a help menu
    - `--benchmark`: Start a benchmark
    - `--vsync`: Enable / disable VSync (`true` / `false`)

## Debug mode:
  - To compile in debug mode, use `make debug` or `DEBUG=true make ...`
    - This will compile some extras in the code to help with debugging (every header gets `iostream`)
    - It will also enable OpenGL debug warnings, messages and errors
    - Before swapping back to regular builds, run `make clean`

## Build system:
  - ### Targets:
    - `build`, `debug` `system-build` and `library` support building on multiple cores with `-jX`
    - `make build` - Builds demo binary, a working demonstration of the renderer
    - `make debug` - Cleans build directory, then runs `make build` in debug mode
    - `make system-build` - Same as `build`, but uses the system copy of `libammonite.so`
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
  - ### Libraries:
    - `libglm-dev libglfw3-dev libglew-dev libstb-dev libassimp-dev`
  - ### Icons:
    - `inkscape optipng`

## Issues:
  - Due to the small size of this project, only a small range of hardware can be tested
    - If you hardware / driver meets the requirements listed [here](#requirements), but the engine doesn't work, please file a bug report
  - Issues, feature requests and bug reports can be filed [here](https://github.com/stuarthayhurst/ammonite/issues)
  - Feel free to work on any issues / feature ideas, suggestions are welcome :)

## Notes:
  - All targets are compiled with `-Wall` and `-Wextra`
  - Targets are also compiled with `-O3` and `-flto`
    - This should be fine for 99% of systems, but some may struggle to compile, or produce unstable results
    - These can be changed by modifying `CXXFLAGS` in `Makefile`

## Credits:
 - Some models in `assets/` may have been created by third parties, attribution can be found in `docs/CREDITS.md`
 - Reference materials:
   - [OpenGL Tutorial](https://www.opengl-tutorial.org/)
   - [Learn OpenGL](https://learnopengl.com/Introduction)

## License
  - This project is available under the terms of the GNU Lesser General Public License (v3.0)
    - These terms can be found in `COPYING` and `COPYING.LESSER`
