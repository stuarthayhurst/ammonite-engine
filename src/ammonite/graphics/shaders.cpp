#include <string>

extern "C" {
  #include <epoxy/gl.h>
}

#include "shaders.hpp"

#include "shaderLoader.hpp"

namespace ammonite {
  namespace renderer {
    namespace internal {
      bool Shader::loadShader(const std::string& shaderDirectory) {
        this->shaderId = ammonite::shaders::internal::loadDirectory(shaderDirectory);
        if (this->shaderId == 0) {
          return false;
        }

        this->setUniformLocations();
        return true;
      }

      void Shader::destroyShader() {
        if (this->shaderId != 0) {
          glDeleteProgram(this->shaderId);
          this->shaderId = 0;
        }
      }

      void Shader::useShader() const {
        glUseProgram(this->shaderId);
      }

      void ModelShader::setUniformLocations() {
        this->matrixId = glGetUniformLocation(this->shaderId, "MVP");
        this->modelMatrixId = glGetUniformLocation(this->shaderId, "modelMatrix");
        this->normalMatrixId = glGetUniformLocation(this->shaderId, "normalMatrix");
        this->ambientLightId = glGetUniformLocation(this->shaderId, "ambientLight");
        this->cameraPosId = glGetUniformLocation(this->shaderId, "cameraPos");
        this->shadowFarPlaneId = glGetUniformLocation(this->shaderId, "shadowFarPlane");
        this->lightCountId = glGetUniformLocation(this->shaderId, "lightCount");
        this->diffuseSamplerId = glGetUniformLocation(this->shaderId, "diffuseSampler");
        this->specularSamplerId = glGetUniformLocation(this->shaderId, "specularSampler");
        this->shadowCubeMapId = glGetUniformLocation(this->shaderId, "shadowCubeMap");
      }

      void LightShader::setUniformLocations() {
        this->lightMatrixId = glGetUniformLocation(this->shaderId, "MVP");
        this->lightIndexId = glGetUniformLocation(this->shaderId, "lightIndex");
      }

      void DepthShader::setUniformLocations() {
        this->modelMatrixId = glGetUniformLocation(this->shaderId, "modelMatrix");
        this->shadowFarPlaneId = glGetUniformLocation(this->shaderId, "shadowFarPlane");
        this->shadowMatrixId = glGetUniformLocation(this->shaderId, "shadowMatrices");
        this->depthShadowIndexId = glGetUniformLocation(this->shaderId, "shadowMapIndex");
      }

      void SkyboxShader::setUniformLocations() {
        this->viewMatrixId = glGetUniformLocation(this->shaderId, "viewMatrix");
        this->projectionMatrixId = glGetUniformLocation(this->shaderId, "projectionMatrix");
        this->skyboxSamplerId = glGetUniformLocation(this->shaderId, "skyboxSampler");
      }

      void ScreenShader::setUniformLocations() {
        this->screenSamplerId = glGetUniformLocation(this->shaderId, "screenSampler");
        this->depthSamplerId = glGetUniformLocation(this->shaderId, "depthSampler");
        this->focalDepthId = glGetUniformLocation(this->shaderId, "focalDepth");
        this->focalDepthEnabledId = glGetUniformLocation(this->shaderId, "focalDepthEnabled");
        this->blurStrengthId = glGetUniformLocation(this->shaderId, "blurStrength");
        this->farPlaneId = glGetUniformLocation(this->shaderId, "farPlane");
      }

      void SplashShader::setUniformLocations() {
        this->progressId = glGetUniformLocation(this->shaderId, "progress");
        this->widthId = glGetUniformLocation(this->shaderId, "width");
        this->heightId = glGetUniformLocation(this->shaderId, "height");
        this->heightOffsetId = glGetUniformLocation(this->shaderId, "heightOffset");
        this->progressColourId = glGetUniformLocation(this->shaderId, "progressColour");
      }
    }
  }
}
