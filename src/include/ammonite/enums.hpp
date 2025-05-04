#ifndef AMMONITEENUMS
#define AMMONITEENUMS

/*
 - Public constants and enums shared between systems
*/

//Constants for loading assumptions
static constexpr bool ASSUME_SRGB_TEXTURES = true;

//Graphics context hint
enum AmmoniteContextEnum : unsigned char {
  AMMONITE_DEFAULT_CONTEXT,
  AMMONITE_NO_ERROR_CONTEXT,
  AMMONITE_DEBUG_CONTEXT,
};

#endif
