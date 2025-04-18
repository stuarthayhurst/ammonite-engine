#ifndef ENUMS
#define ENUMS

//Constants for loading assumptions
static constexpr bool ASSUME_FLIP_MODEL_UVS = true;
static constexpr bool ASSUME_FLIP_SKYBOX_FACES = false;
static constexpr bool ASSUME_SRGB_TEXTURES = true;

enum AmmoniteEnum : unsigned char {
  //Draw modes
  AMMONITE_DRAW_INACTIVE,
  AMMONITE_DRAW_ACTIVE,
  AMMONITE_DRAW_WIREFRAME,
  AMMONITE_DRAW_POINTS,

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
  AMMONITE_RESPECT_BLOCK,

  //Cache return values
  AMMONITE_CACHE_HIT,
  AMMONITE_CACHE_MISS,
  AMMONITE_CACHE_INVALID,
  AMMONITE_CACHE_COLLISION
};

#endif
