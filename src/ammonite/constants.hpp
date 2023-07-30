#ifndef CONSTANTS
#define CONSTANTS

enum AmmoniteEnum : unsigned char {
  //Keybinding constants
  AMMONITE_EXIT,
  AMMONITE_FORWARD,
  AMMONITE_BACK,
  AMMONITE_UP,
  AMMONITE_DOWN,
  AMMONITE_LEFT,
  AMMONITE_RIGHT,

  //Draw modes
  AMMONITE_DRAW_INACTIVE,
  AMMONITE_DRAW_ACTIVE,
  AMMONITE_DRAW_WIREFRAME,
  AMMONITE_DRAW_POINTS,

  //Model types
  AMMONITE_MODEL,
  AMMONITE_LIGHT_EMITTER,

  //OpenGL / GLFW context hints
  AMMONITE_DEFAULT_CONTEXT,
  AMMONITE_NO_ERROR_CONTEXT,
  AMMONITE_DEBUG_CONTEXT,

  //Texture types
  AMMONITE_DIFFUSE_TEXTURE,
  AMMONITE_SPECULAR_TEXTURE
};

#endif
