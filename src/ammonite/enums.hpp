#ifndef ENUMS
#define ENUMS

enum AmmoniteEnum : unsigned char {
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
  AMMONITE_SPECULAR_TEXTURE,

  //Keybind input override modes
  //New entries must go between first and last, or update validation
  AMMONITE_ALLOW_OVERRIDE,
  AMMONITE_ALLOW_RELEASE,
  AMMONITE_FORCE_RELEASE,
  AMMONITE_RESPECT_BLOCK
};

#endif
