#ifndef SHADER
#define SHADER

namespace ammonite {
  namespace shaders {
    GLuint createProgram(const GLuint shaderIds[], const int shaderCount, bool* externalSuccess);
    GLuint createProgram(const GLuint shaderIds[], const int shaderCount, bool* externalSuccess, const char* programName);
    GLuint loadShader(const char* shaderPath, const GLenum shaderType, bool* externalSuccess);
    void deleteShader(GLuint shaderId);
    void eraseShaders();

    int useProgramCache(const char* programCachePath);
  }
}

#endif
