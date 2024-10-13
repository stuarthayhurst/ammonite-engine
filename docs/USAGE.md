## Basic usage:
  - Some basic usage can be found here, with much better coverage planned in future
  - This is mostly a staging ground for information that won't be automatically generated from source files

## Return codes:
  - Successes return `true`
  - Failures return `false`
  - Object IDs use type `AmmoniteId`, and are specific to each object type
    - Valid IDs start at `1`
    - Unset IDs or failures use `0`
    - This means a camera, light source and model can all share ID `1`

## Model draw modes:
  - `ammonite::models::draw::setDrawMode(modelId, drawMode)` takes 2 arguments, a model ID and a draw mode
    - `modelId` is the model to modify the draw mode of
    - `drawMode` determines how the model should be drawn
      - `AMMONITE_DRAW_INACTIVE`: Hides model from view
      - `AMMONITE_DRAW_ACTIVE`: Default, model is drawn as normal
      - `AMMONITE_DRAW_WIREFRAME`: Draws the model as a mesh
      - `AMMONITE_DRAW_POINTS`: Draws the model as a set of vertices only

## Keybindings:
  - `ammonite::controls::setKeybind(engineId, keycode)` takes 2 arguments, the action to rebind, and a key to bind to
    - `keycode` should be a valid [GLFW key token](https://www.glfw.org/docs/3.3/group__keys.html)
    - `engineId` is the action within Ammonite to rebind
      - `AMMONITE_EXIT`: Hints that the engine should be destroyed
      - `AMMONITE_FORWARD`: Moves the bound camera forwards
      - `AMMONITE_BACK`: Moves the bound camera backwards
      - `AMMONITE_UP`: Moves the bound camera up
      - `AMMONITE_DOWN`: Moves the bound camera down
      - `AMMONITE_LEFT`: Moves the bound camera left
      - `AMMONITE_RIGHT`: Moves the bound camera right
