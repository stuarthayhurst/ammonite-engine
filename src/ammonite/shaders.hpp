#ifndef SHADER
#define SHADER

namespace ammonite {
  namespace shaders {
    int createProgram(const char* shaderPaths[], const int shaderTypes[], const int shaderCount, bool* externalSuccess);
    int loadDirectory(const char* directoryPath, bool* externalSuccess);
  }
}

#endif
