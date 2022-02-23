#ifndef SHADER
#define SHADER

#include <string>

namespace ammonite {
  namespace shaders {
    GLuint createProgram(const GLuint shaderIds[], const int shaderCount, bool* externalSuccess);
    GLuint createProgram(const std::string shaderPaths[], const int shaderTypes[], const int shaderCount, bool* externalSuccess, const char* programName);
    GLuint loadShader(const char* shaderPath, const GLenum shaderType, bool* externalSuccess);
    void deleteShader(GLuint shaderId);
    void eraseShaders();

    bool useProgramCache(const char* programCachePath);
  }
}

#endif
