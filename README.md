## Ammonite Renderer
[![Donate](https://img.shields.io/badge/Donate-PayPal-green.svg)](https://paypal.me/stuartahayhurst)
  - A simple OpenGL based renderer to render loaded data, focused on ease of use
  - If you found this project interesting, any donations are greatly appreciated :)

![Icon](assets/icons/icon.svg)

## Requirements:
  - A `c++17` compatible compiler (`g++ 8+`)
  - An OpenMP supported compiler
  - An OpenGL 4.3+ compatible driver
  - Alternatively, an OpenGL 3.2+ driver supporting the following extensions can be used
    - `ARB_shader_storage_buffer_object`
    - `KHR_debug`, when compiled in debug mode
  - Program caching is supported with `ARB_get_program_binary`

## Building + installing libammonite:
  - `make library`
  - `sudo make install`

## Building + running demo:
  - `make clean` will clean the build area, to start from fresh
  - Run `make library` to build the shared library
  - Run either `make local-build` or `make build; sudo make install`, to build the demo
    - `local-build` will only allow running from this project's root directory (recommended, as the code is only a demo)
    - `build` + `install` will build the demo, but use the system copy of `libammonite.so`
  - `./build/demo` will run the built demo
  - To compile in debug mode, use `DEBUG=true make ...`
    - This will compile some extras in the code to help with debugging (every header gets `iostream`)
    - It will also enable OpenGL debug warning, messages and errors
    - It won't trigger a rebuild of the library, so `make clean` should be run before swapping the value

## Options:
  - Compiled demos have a few arguments supported:
    - `--help`: Displays a help menu
    - `--benchmark`: Start a benchmark
    - `--vsync`: Enable / disable VSync (`true` / `false`)

## Build system:
  - `build`, `local-build` and `library` support building on multiple cores with `-jX`
  - `make build` - Builds demo binary, a working demonstration of the renderer
  - `make system-build` - Same as `build`, but uses the system copy of `libammonite.so`
  - `make library` - Builds `build/libammonite.so`
  - `make install` - Installs `libammonite.so` to system directories
    - The install path can be configured, by setting the environment variable `INSTALL_DIR`
    - This should always end in `/ammonite`, for example, `/usr/local/lib/ammonite`
  - `make headers` - Installs Ammonite headers to the system
    - The install path can be configured, by setting the environment variable `HEADER_DIR`
    - This should always end in `/ammonite`, for example, `/usr/local/include/ammonite`
  - `make uninstall` - Removes installed library
    - Custom install locations can be removed using the environment variable `INSTALL_DIR`
  - `make icons` - Creates `assets/icons/icon.png` from `assets/icons/icon.svg`
  - `make clean` - Cleans the build area (`build/`) and default runtime cache (`cache/`)
  - `make cache` - Clears the default runtime binary cache, useful if running into issues with caching

## Dependencies:
  - `make`
  - `g++`
  - `pkg-config`
  - ### Libraries:
    - `libgomp1 libgomplibglm-dev libglfw3-dev libglew-dev libstb-dev libtinyobjloader-dev`

## Notes:
  - All targets are compiled with `-Wall` and `-Wextra`
  - Targets are also compiled with `-O3` and `-flto`, which may cause some systems to struggle to compile, or produce unstable results
    - These can be changed by modifying `CXXFLAGS` in `Makefile`
    - `-flto` is strongly recommended, otherwise the build results can be very large

## Credits:
 - Some models in `assets/` may have been created by third parties, attribution can be found in `docs/CREDITS.md`
 - The [OpenGL Tutorial](http://www.opengl-tutorial.org/) and the [tinyobjloader source](https://github.com/tinyobjloader/tinyobjloader) were both used as reference materials

## License
  - This project is available under the terms of the GNU Lesser General Public License (v3.0)
    - These terms can be found in `COPYING` and `COPYING.LESSER`
