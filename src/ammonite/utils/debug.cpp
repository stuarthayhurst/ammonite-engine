#ifdef DEBUG

#include <iostream>
#include <GL/glew.h>

#include "extension.hpp"
#include "logging.hpp"

#include "../internal/internalDebug.hpp"

namespace ammonite {
  namespace utils {
    namespace {
      static void GLAPIENTRY debugMessageCallback(GLenum, GLenum type, GLuint, GLenum severity, GLsizei, const GLchar* message, const void*) {
        std::cerr << "\nGL MESSAGE ";
        switch (severity) {
          case GL_DEBUG_SEVERITY_HIGH: std::cerr << "(High priority): "; break;
          case GL_DEBUG_SEVERITY_MEDIUM: std::cerr << "(Medium priority): "; break;
          case GL_DEBUG_SEVERITY_LOW: std::cerr << "(Low priority): "; break;
          case GL_DEBUG_SEVERITY_NOTIFICATION: std::cerr << "(Notification): "; break;
        }

        switch (type) {
          case GL_DEBUG_TYPE_ERROR: std::cerr << "** ERROR **"; break;
          case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::cerr << "DEPRECATED BEHAVIOUR"; break;
          case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: std::cerr << "UNDEFINED BEHAVIOUR"; break;
          case GL_DEBUG_TYPE_PORTABILITY: std::cerr << "PORTABILITY"; break;
          case GL_DEBUG_TYPE_PERFORMANCE: std::cerr << "PERFORMANCE"; break;
          case GL_DEBUG_TYPE_MARKER: std::cerr << "MARKER"; break;
          case GL_DEBUG_TYPE_PUSH_GROUP: std::cerr << "PUSH GROUP"; break;
          case GL_DEBUG_TYPE_POP_GROUP: std::cerr << "POP GROUP"; break;
          case GL_DEBUG_TYPE_OTHER: std::cerr << "OTHER"; break;
        }

        std::cerr << std::endl;
        std::cerr << "  Message: " << message << "\n" << std::endl;
      }
    }

    namespace debug {
      void enableDebug() {
        //Check support for OpenGL debugging
        if (!ammonite::utils::checkExtension("GL_KHR_debug", "GL_VERSION_4_3")) {
          std::cerr << ammonite::utils::error << "OpenGL debugging unsupported" << std::endl;
          return;
        }

        //Enable OpenGL debug output
        glEnable(GL_DEBUG_OUTPUT);
        glDebugMessageCallback(debugMessageCallback, 0);
      }
    }
  }
}

#endif
