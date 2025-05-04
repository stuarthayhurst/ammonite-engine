#ifndef AMMONITEENUMS
#define AMMONITEENUMS

//Constants for loading assumptions
static constexpr bool ASSUME_FLIP_MODEL_UVS = true;
static constexpr bool ASSUME_FLIP_SKYBOX_FACES = false;
static constexpr bool ASSUME_SRGB_TEXTURES = true;

//Graphics context hint
enum AmmoniteContextEnum : unsigned char {
  AMMONITE_DEFAULT_CONTEXT,
  AMMONITE_NO_ERROR_CONTEXT,
  AMMONITE_DEBUG_CONTEXT,
};

#endif
