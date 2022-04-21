#include <iostream>
#include <GL/glew.h>

namespace ammonite {
  namespace {
    void GLAPIENTRY debugMessageCallback(GLenum, GLenum type, GLuint, GLenum severity, GLsizei, const GLchar* message, const void*) {
      std::cerr << "\nGL CALLBACK: " << (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "") << std::endl;
      std::cerr << "  Type: 0x" << type << std::endl;
      std::cerr << "  Severity: 0x" << severity << std::endl;
      std::cerr << "  Message: " << message << "\n" << std::endl;
    }
  }

  namespace debug {
    void enableDebug() {
      //Enable OpenGL debug output
      glEnable(GL_DEBUG_OUTPUT);
      glDebugMessageCallback(debugMessageCallback, 0);
    }
  }
}
