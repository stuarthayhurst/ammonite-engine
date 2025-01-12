#include <iostream>

#include <GL/glew.h>

#include "../graphics/internal/internalExtensions.hpp"
#include "logging.hpp"

#ifdef DEBUG
/*NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables,
              cppcoreguidelines-interfaces-global-init)*/
ammonite::utils::OutputHelper ammoniteInternalDebug(std::cout, "DEBUG: ");
/*NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables,
            cppcoreguidelines-interfaces-global-init)*/
#endif

namespace ammonite {
  namespace utils {
    namespace {
      static void GLAPIENTRY debugMessageCallback(GLenum, GLenum type, GLuint, GLenum severity,
                                                  GLsizei, const GLchar* message, const void*) {
        std::cerr << "\nGL MESSAGE ";
        switch (severity) {
          case GL_DEBUG_SEVERITY_HIGH: std::cerr << "(High priority): "; break;
          case GL_DEBUG_SEVERITY_MEDIUM: std::cerr << "(Medium priority): "; break;
          case GL_DEBUG_SEVERITY_LOW: std::cerr << "(Low priority): "; break;
          case GL_DEBUG_SEVERITY_NOTIFICATION: std::cerr << "(Notification): "; break;
          default: std::cerr << "(Unknown severity): "; break;
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
          default: std::cerr << "UNKNOWN"; break;
        }

        std::cerr << std::endl;
        std::cerr << "  Message: " << message << "\n" << std::endl;
      }
    }

    namespace debug {
      void enableDebug() {
        //Check support for OpenGL debugging
        if (!graphics::internal::checkExtension("GL_KHR_debug", "GL_VERSION_4_3")) {
          ammonite::utils::error << "OpenGL debugging unsupported" << std::endl;
          return;
        }

        /*
         - This isn't used for debugging, but won't be explicitly checked otherwise
         - Handled before engine init, so no output would be shown
        */
        graphics::internal::checkExtension("GL_KHR_no_error", "GL_VERSION_4_6");

        //Enable OpenGL debug output
        glEnable(GL_DEBUG_OUTPUT);
        glDebugMessageCallback(debugMessageCallback, 0);
      }

      void printDriverInfo() {
        GLint majorVersion = 0;
        GLint minorVersion = 0;

        glGetIntegerv(GL_MAJOR_VERSION, &majorVersion);
        glGetIntegerv(GL_MAJOR_VERSION, &minorVersion);

        ammonite::utils::status << "OpenGL version: " << majorVersion << "." << minorVersion << std::endl;
        ammonite::utils::status << "OpenGL renderer: " << glGetString(GL_RENDERER) << std::endl;
        ammonite::utils::status << "OpenGL vendor: " << glGetString(GL_VENDOR) << std::endl;
      }
    }
  }
}
