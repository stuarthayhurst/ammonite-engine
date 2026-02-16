#ifndef INTERNALSHADERS
#define INTERNALSHADERS

#include <string>

extern "C" {
  #include <epoxy/gl.h>
}

#include "../visibility.hpp"

namespace ammonite {
  namespace renderer {
    namespace AMMONITE_INTERNAL internal {
      class Shader {
      protected:
        GLuint shaderId = 0;
        virtual void setUniformLocations() {};

      public:
        virtual ~Shader() = default;
        bool loadShader(const std::string& shaderDirectory);
        void destroyShader();
        void useShader() const;
      };

      class ModelShader : public Shader {
      private:
        void setUniformLocations() override;
      public:
        GLint matrixId;
        GLint modelMatrixId;
        GLint normalMatrixId;
        GLint ambientLightId;
        GLint cameraPosId;
        GLint shadowFarPlaneId;
        GLint lightCountId;
        GLint diffuseSamplerId;
        GLint specularSamplerId;
        GLint shadowCubeMapId;
      };

      class LightShader : public Shader {
      private:
        void setUniformLocations() override;
      public:
        GLint lightMatrixId;
        GLint lightIndexId;
      };

      class DepthShader : public Shader {
      private:
        void setUniformLocations() override;
      public:
        GLint modelMatrixId;
        GLint shadowFarPlaneId;
        GLint shadowMatrixId;
        GLint depthShadowIndexId;
      };

      class SkyboxShader : public Shader {
      private:
        void setUniformLocations() override;
      public:
        GLint viewMatrixId;
        GLint projectionMatrixId;
        GLint skyboxSamplerId;
      };

      class ScreenShader : public Shader {
      private:
        void setUniformLocations() override;
      public:
        GLint screenSamplerId;
        GLint depthSamplerId;
        GLint focalDepthId;
        GLint focalDepthEnabledId;
        GLint blurStrengthId;
        GLint farPlaneId;
      };

      class SplashShader : public Shader {
      private:
        void setUniformLocations() override;
      public:
        GLint progressId;
        GLint widthId;
        GLint heightId;
        GLint heightOffsetId;
        GLint progressColourId;
      };
    }
  }
}

#endif
