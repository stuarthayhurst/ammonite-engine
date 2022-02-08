## Ammonite Renderer
  - An OpenGL based renderer to display models and generated datasets, focused on ease of use

## Requirements:
  - A `c++17` compatible compiler (`g++ 8+`)
  - An OpenGL 3.3+ compatible driver

## Running:
  - `make clean build` will clean the build area and build the demo from fresh
  - `./build/demo` will run the built demo
  - To compile in debug mode, use `DEBUG=true make ...`
    - This will compile some extras in the code to help with debugging (every header gets `iostream`)

## Options:
  - Compiled demos have a few arguments supported:
    - `--help`: Displays a help menu
    - `--vsync`: Enable / disable VSync (`true` / `false`)

## Build system:
  - `make build` - Builds demo binary, a working demonstration of the renderer
  - `make clean` - Cleans the build area (`build/`)
  - `make cache` - Clears the default runtime binary cache, useful if running into issues with caching

## Dependencies:
  - `make`
  - `g++`
  - `pkg-config`
  - ### Libraries:
    - `libglm-dev libglfw3-dev libglew-dev`

## Notes:
  - All targets are compiled with `-Wall` and `-Wextra`
  - Targets are also compiled with `-O3` and `-flto`, which may cause some systems to struggle to compile, or produce unstable results
