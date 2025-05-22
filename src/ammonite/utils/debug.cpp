#include <iostream>

extern "C" {
  #include <epoxy/gl.h>
}

#include "debug.hpp"

#include "logging.hpp"
#include "../graphics/extensions.hpp"

#ifdef AMMONITE_DEBUG
//NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables, cppcoreguidelines-interfaces-global-init)
ammonite::utils::OutputHelper ammoniteInternalDebug(std::cout, "DEBUG: ",
                                                    ammonite::utils::colour::magenta);
#endif

namespace ammonite {
  namespace utils {
    namespace {
      //NOLINTNEXTLINE(cppcoreguidelines-interfaces-global-init)
      ammonite::utils::OutputHelper glDebugLogger(std::cerr, "\nGL MESSAGE: ",
                                                  ammonite::utils::colour::cyan);

      void GLAPIENTRY debugMessageCallback(GLenum, GLenum type, GLuint, GLenum severity,
                                           GLsizei, const GLchar* message, const void*) {
        switch (severity) {
          case GL_DEBUG_SEVERITY_HIGH: glDebugLogger << "(High priority): "; break;
          case GL_DEBUG_SEVERITY_MEDIUM: glDebugLogger << "(Medium priority): "; break;
          case GL_DEBUG_SEVERITY_LOW: glDebugLogger << "(Low priority): "; break;
          case GL_DEBUG_SEVERITY_NOTIFICATION: glDebugLogger << "(Notification): "; break;
          default: glDebugLogger << "(Unknown severity): "; break;
        }

        switch (type) {
          case GL_DEBUG_TYPE_ERROR: glDebugLogger << "** ERROR **"; break;
          case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: glDebugLogger << "DEPRECATED BEHAVIOUR"; break;
          case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: glDebugLogger << "UNDEFINED BEHAVIOUR"; break;
          case GL_DEBUG_TYPE_PORTABILITY: glDebugLogger << "PORTABILITY"; break;
          case GL_DEBUG_TYPE_PERFORMANCE: glDebugLogger << "PERFORMANCE"; break;
          case GL_DEBUG_TYPE_MARKER: glDebugLogger << "MARKER"; break;
          case GL_DEBUG_TYPE_PUSH_GROUP: glDebugLogger << "PUSH GROUP"; break;
          case GL_DEBUG_TYPE_POP_GROUP: glDebugLogger << "POP GROUP"; break;
          case GL_DEBUG_TYPE_OTHER: glDebugLogger << "OTHER"; break;
          default: glDebugLogger << "UNKNOWN"; break;
        }

        glDebugLogger << "\n  Message: " << message << "\n" << std::endl;
      }
    }

    namespace debug {
      void enableDebug() {
        //Check support for OpenGL debugging
        if (!graphics::internal::checkExtension("GL_KHR_debug", 4, 3)) {
          ammonite::utils::error << "OpenGL debugging unsupported" << std::endl;
          return;
        }

        /*
         - This isn't used for debugging, but won't be explicitly checked otherwise
         - Handled before engine init, so no output would be shown
        */
        graphics::internal::checkExtension("GL_KHR_no_error", 4, 6);

        //Enable OpenGL debug output
        glEnable(GL_DEBUG_OUTPUT);
        glDebugMessageCallback(debugMessageCallback, nullptr);
      }

      void printDriverInfo() {
        GLint majorVersion = 0;
        GLint minorVersion = 0;

        glGetIntegerv(GL_MAJOR_VERSION, &majorVersion);
        glGetIntegerv(GL_MAJOR_VERSION, &minorVersion);

        ammonite::utils::status << "OpenGL version: " << majorVersion << "." << minorVersion << std::endl;
        ammonite::utils::status << "OpenGL renderer: " << glGetString(GL_RENDERER) << std::endl;
        ammonite::utils::status << "OpenGL vendor: " << glGetString(GL_VENDOR) << "\n" << std::endl;
      }
    }
  }
}
