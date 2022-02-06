#include <string>
#include <vector>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>

#include <GL/glew.h>

namespace ammonite {
  namespace {
    //Vector to store all existing shaders
    std::vector<int> shaderIds(0);
  }

  namespace shaders {
    void deleteShader(const GLuint shaderId) {
      //Search for and delete shaderId
      auto shaderPosition = std::find(shaderIds.begin(), shaderIds.end(), shaderId);
      if (shaderPosition != shaderIds.end()) {
        const int intShaderPosition = std::distance(shaderIds.begin(), shaderPosition);
        glDeleteShader(shaderIds[intShaderPosition]);
        shaderIds.erase(shaderPosition);
      }
    }

    //Delete every remaining shader
    void eraseShaders() {
      while (shaderIds.size() != 0) {
        deleteShader(shaderIds[0]);
      }
    }

    GLuint loadShader(const char* shaderPath, const GLenum shaderType, bool* externalSuccess) {
      //Check for compute shader support if needed
      if (shaderType == GL_COMPUTE_SHADER) {
        if (!glewIsSupported("GL_VERSION_4_3") and !GLEW_ARB_compute_shader) {
          std::cout << "Compute shaders unsupported" << std::endl;
          *externalSuccess = false;
          return -1;
        }
      }

      //Check for tessellation shader support if needed
      if (shaderType == GL_TESS_CONTROL_SHADER or shaderType == GL_TESS_EVALUATION_SHADER) {
        if (!glewIsSupported("GL_VERSION_4_0") and !GLEW_ARB_tessellation_shader) {
          std::cout << "Tessellation shaders unsupported" << std::endl;
          *externalSuccess = false;
          return -1;
        }
      }

      //Create the shader
      GLuint shaderId = glCreateShader(shaderType);

      std::string shaderCode;
      std::ifstream shaderCodeStream(shaderPath, std::ios::in);
      std::stringstream sstr;

      //Read the shader's source code
      if (shaderCodeStream.is_open()) {
        sstr << shaderCodeStream.rdbuf();
        shaderCode = sstr.str();
        shaderCodeStream.close();
      } else {
        std::cout << "Failed to open " << shaderPath << std::endl;
        *externalSuccess = false;
        return -1;
      }

      //Provide a shader source and compile the shader
      const char* shaderCodePointer = shaderCode.c_str();
      glShaderSource(shaderId, 1, &shaderCodePointer, NULL);
      glCompileShader(shaderId);

      //Test whether the shader compiled
      GLint success = GL_FALSE;
      glGetShaderiv(shaderId, GL_COMPILE_STATUS, &success);

      //If the shader failed to compile, print a log
      if (success == GL_FALSE) {
        GLint maxLength = 0;
        glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &maxLength);

        std::vector<GLchar> errorLog(maxLength);
        glGetShaderInfoLog(shaderId, maxLength, &maxLength, &errorLog[0]);
        std::cout << &errorLog[0] << std::endl;

        //Clean up and exit
        glDeleteShader(shaderId); //Use glDeleteShader, as the shader never made it to shaderIds
        *externalSuccess = false;
        return -1;
      }

      //Add the shader to the shader id store
      shaderIds.push_back(shaderId);
      return shaderId;
    }

    GLuint createProgram(const GLuint shaderIds[], const int shaderCount, bool* externalSuccess) {
      //Create the program
      GLuint programId = glCreateProgram();

      //Attach all passed shader ids
      for (int i = 0; i < shaderCount; i++) {
        glAttachShader(programId, shaderIds[i]);
      }

      glLinkProgram(programId);

      //Test whether the program linked
      GLint success = GL_FALSE;
      glGetProgramiv(programId, GL_LINK_STATUS, &success);

      //If the program failed to link, print a log
      if (success == GL_FALSE) {
        GLint maxLength = 0;
        glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &maxLength);

        std::vector<GLchar> errorLog(maxLength);
        glGetProgramInfoLog(programId, maxLength, &maxLength, &errorLog[0]);
        std::cout << &errorLog[0] << std::endl;

        *externalSuccess = false;
        return -1;
      }

      //Detach and remove all passed shader ids
      for (int i = 0; i < shaderCount; i++) {
        glDetachShader(programId, shaderIds[i]);
        deleteShader(shaderIds[i]);
      }

      return programId;
    }
  }
}
