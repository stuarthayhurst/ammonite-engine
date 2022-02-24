## Ammonite Renderer
[![Donate](https://img.shields.io/badge/Donate-PayPal-green.svg)](https://paypal.me/stuartahayhurst)
  - An OpenGL based renderer to display models and generated datasets, focused on ease of use
  - Any donations are greatly appreciated :)

![Icon](assets/icons/icon.png)

## Requirements:
  - A `c++17` compatible compiler (`g++ 8+`)
  - An OpenGL 3.2+ compatible driver

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

## Options:
  - Compiled demos have a few arguments supported:
    - `--help`: Displays a help menu
    - `--benchmark`: Start a benchmark
    - `--vsync`: Enable / disable VSync (`true` / `false`)

## Build system:
  - `build`, `local-build` and `library` support building on multiple cores with `-jX`
  - `make build` - Builds demo binary, a working demonstration of the renderer
  - `make local-build` - Same as `build`, but only allow running from the project's root directory
  - `make library` - Buils `build/libammonite.so`
  - `make install` - Installs `libammonite.so` to system directories
  - `make uninstall` - Removes installed library
  - `make icons` - Creates `assets/icons/icon.png` from `assets/icons/icon.svg`
  - `make clean` - Cleans the build area (`build/`) and default runtime cache (`cache/`)
  - `make cache` - Clears the default runtime binary cache, useful if running into issues with caching

## Dependencies:
  - `make`
  - `g++`
  - `pkg-config`
  - ### Libraries:
    - `libglm-dev libglfw3-dev libglew-dev libstb-dev libtinyobjloader-dev`

## Notes:
  - All targets are compiled with `-Wall` and `-Wextra`
  - Targets are also compiled with `-O3` and `-flto`, which may cause some systems to struggle to compile, or produce unstable results
    - These can be changed by modifying `CXXFLAGS` in `Makefile`
    - `-flto` is strongly recommended, otherwise the build results can be very large

## Credits:
 - Some models in `assets/` may have been created by third parties, attribution can be found in `docs/CREDITS.md`

## License
  - This project is available under the terms of the GNU Lesser General Public License (v3.0)
    - These terms can be found in `COPYING` and `COPYING.LESSER`
