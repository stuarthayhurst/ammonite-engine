#ifndef SHADER
#define SHADER

namespace ammonite {
  namespace shaders {
    GLuint createProgram(const GLuint shaderIds[], const int shaderCount);
    GLuint loadShader(const char* shaderPath, const GLenum shaderType, bool* externalSuccess);
  }
}

#endif
