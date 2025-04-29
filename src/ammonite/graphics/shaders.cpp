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
        shaderId = ammonite::shaders::internal::loadDirectory(shaderDirectory);
        if (shaderId == 0) {
          return false;
        }

        setUniformLocations();
        return true;
      }

      void Shader::destroyShader() {
        if (shaderId != 0) {
          glDeleteProgram(shaderId);
          shaderId = 0;
        }
      }

      void Shader::useShader() const {
        glUseProgram(shaderId);
      }

      void ModelShader::setUniformLocations() {
        matrixId = glGetUniformLocation(shaderId, "MVP");
        modelMatrixId = glGetUniformLocation(shaderId, "modelMatrix");
        normalMatrixId = glGetUniformLocation(shaderId, "normalMatrix");
        ambientLightId = glGetUniformLocation(shaderId, "ambientLight");
        cameraPosId = glGetUniformLocation(shaderId, "cameraPos");
        shadowFarPlaneId = glGetUniformLocation(shaderId, "shadowFarPlane");
        lightCountId = glGetUniformLocation(shaderId, "lightCount");
        diffuseSamplerId = glGetUniformLocation(shaderId, "diffuseSampler");
        specularSamplerId = glGetUniformLocation(shaderId, "specularSampler");
        shadowCubeMapId = glGetUniformLocation(shaderId, "shadowCubeMap");
      }

      void LightShader::setUniformLocations() {
        lightMatrixId = glGetUniformLocation(shaderId, "MVP");
        lightIndexId = glGetUniformLocation(shaderId, "lightIndex");
      }

      void DepthShader::setUniformLocations() {
        modelMatrixId = glGetUniformLocation(shaderId, "modelMatrix");
        shadowFarPlaneId = glGetUniformLocation(shaderId, "shadowFarPlane");
        shadowMatrixId = glGetUniformLocation(shaderId, "shadowMatrices");
        depthShadowIndexId = glGetUniformLocation(shaderId, "shadowMapIndex");
      }

      void SkyboxShader::setUniformLocations() {
        viewMatrixId = glGetUniformLocation(shaderId, "viewMatrix");
        projectionMatrixId = glGetUniformLocation(shaderId, "projectionMatrix");
        skyboxSamplerId = glGetUniformLocation(shaderId, "skyboxSampler");
      }

      void ScreenShader::setUniformLocations() {
        screenSamplerId = glGetUniformLocation(shaderId, "screenSampler");
        depthSamplerId = glGetUniformLocation(shaderId, "depthSampler");
        focalDepthId = glGetUniformLocation(shaderId, "focalDepth");
        focalDepthEnabledId = glGetUniformLocation(shaderId, "focalDepthEnabled");
        blurStrengthId = glGetUniformLocation(shaderId, "blurStrength");
        farPlaneId = glGetUniformLocation(shaderId, "farPlane");
      }

      void LoadingShader::setUniformLocations() {
        progressId = glGetUniformLocation(shaderId, "progress");
        widthId = glGetUniformLocation(shaderId, "width");
        heightId = glGetUniformLocation(shaderId, "height");
        heightOffsetId = glGetUniformLocation(shaderId, "heightOffset");
        progressColourId = glGetUniformLocation(shaderId, "progressColour");
      }
    }
  }
}
