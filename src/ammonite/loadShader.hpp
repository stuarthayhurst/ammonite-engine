#ifndef SHADER
#define SHADER

namespace renderer {
  namespace shaders {
    GLuint loadShaders(const char* vertex_file_path, const char* fragment_file_path);
  }
}

#endif
