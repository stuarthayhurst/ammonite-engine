#ifndef SHADER
#define SHADER

namespace ammonite {
  namespace shaders {
    int createProgram(const GLuint shaderIds[], const int shaderCount, bool* externalSuccess);
    int createProgram(const char* shaderPaths[], const int shaderTypes[], const int shaderCount, bool* externalSuccess);

    int loadShader(const char* shaderPath, const GLenum shaderType, bool* externalSuccess);
    void deleteShader(GLuint shaderId);
    void eraseShaders();

    int loadDirectory(const char* directoryPath, bool* externalSuccess);
  }
}

#endif
