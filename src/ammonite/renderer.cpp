#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <chrono>
#include <thread>

#include <glm/glm.hpp>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "internal/runtimeSettings.hpp"
#include "internal/modelTracker.hpp"
#include "internal/lightTracker.hpp"
#include "internal/cameraMatrices.hpp"

#include "settings.hpp"
#include "shaders.hpp"
#include "camera.hpp"
#include "lightManager.hpp"

#include "utils/timer.hpp"
#include "utils/extension.hpp"

namespace ammonite {
  namespace renderer {
    namespace {
      GLFWwindow* window;
      GLuint modelShaderId;
      GLuint lightShaderId;
      GLuint depthShaderId;

      //Shader uniform IDs
      GLuint matrixId;
      GLuint modelMatrixId;
      GLuint normalMatrixId;
      GLuint ambientLightId;
      GLuint cameraPosId;
      GLuint modelFarPlaneId;
      GLuint textureSamplerId;
      GLuint shadowCubeMapId;

      GLuint lightMatrixId;
      GLuint lightIndexId;

      GLuint depthModelMatrixId;
      GLuint depthFarPlaneId;
      GLuint depthLightPosId;
      GLuint depthShadowIndex;

      GLuint depthCubeMapId = 0;
      GLuint depthMapFBO;

      long totalFrames = 0;
      int frameCount = 0;
      double frameTime = 0.0f;

      glm::mat4* viewMatrix = ammonite::camera::matrices::getViewMatrixPtr();
      glm::mat4* projectionMatrix = ammonite::camera::matrices::getProjectionMatrixPtr();

      //Get the light tracker
      std::map<int, ammonite::lighting::LightSource>* lightTrackerMap = ammonite::lighting::getLightTracker();

      //View projection combined matrix
      glm::mat4 viewProjectionMatrix;
    }

    namespace {
      //Check for essential GPU capabilities
      static bool checkGPUCapabilities(int* failureCount) {
        bool success = true;
        //Check DSA is supported
        if (!ammonite::utils::checkExtension("GL_ARB_direct_state_access", "GL_VERSION_4_5")) {
          std::cerr << "Direct state access unsupported" << std::endl;
          success = false;
          *failureCount += 1;
        }

        //Check SSBOs are supported
        if (!ammonite::utils::checkExtension("GL_ARB_shader_storage_buffer_object", "GL_VERSION_4_3")) {
          std::cerr << "Shader Storage Buffer Objects (SSBOs) unsupported" << std::endl;
          success = false;
          *failureCount += 1;
        }

        //Check texture storage is supported
        if (!ammonite::utils::checkExtension("GL_ARB_texture_storage", "GL_VERSION_4_2")) {
          std::cerr << "Texture storage unsupported" << std::endl;
          success = false;
          *failureCount += 1;
        }

        //Check cubemap arrays are supported
        if (!ammonite::utils::checkExtension("GL_ARB_texture_cube_map_array", "GL_VERSION_4_0")) {
          std::cerr << "Cubemap arrays unsupported" << std::endl;
          success = false;
          *failureCount += 1;
        }

        //Check minimum OpenGL version is supported
        if (!glewIsSupported("GL_VERSION_3_2")) {
          std::cerr << "OpenGL 3.2 unsupported" << std::endl;
          success = false;
          *failureCount += 1;
        }

        return success;
      }
    }

    namespace setup {
      void setupRenderer(GLFWwindow* targetWindow, const char* shaderPath, bool* externalSuccess) {
        //Check GPU supported required extensions
        int failureCount = 0;
        if (!checkGPUCapabilities(&failureCount)) {
          std::cerr << failureCount << " required extensions are unsupported" << std::endl;
          *externalSuccess = false;
          return;
        }

        //Set window to be used
        window = targetWindow;

        //Create shaders
        std::string shaderLocation;
        shaderLocation = std::string(shaderPath) + std::string("models/");
        modelShaderId = ammonite::shaders::loadDirectory(shaderLocation.c_str(), externalSuccess);

        shaderLocation = std::string(shaderPath) + std::string("lights/");
        lightShaderId = ammonite::shaders::loadDirectory(shaderLocation.c_str(), externalSuccess);

        shaderLocation = std::string(shaderPath) + std::string("depth/");
        depthShaderId = ammonite::shaders::loadDirectory(shaderLocation.c_str(), externalSuccess);

        if (!*externalSuccess) {
          return;
        }

        //Shader uniform locations
        matrixId = glGetUniformLocation(modelShaderId, "MVP");
        modelMatrixId = glGetUniformLocation(modelShaderId, "M");
        normalMatrixId = glGetUniformLocation(modelShaderId, "normalMatrix");
        ambientLightId = glGetUniformLocation(modelShaderId, "ambientLight");
        cameraPosId = glGetUniformLocation(modelShaderId, "cameraPos");
        modelFarPlaneId = glGetUniformLocation(modelShaderId, "farPlane");
        textureSamplerId = glGetUniformLocation(modelShaderId, "textureSampler");
        shadowCubeMapId = glGetUniformLocation(modelShaderId, "shadowCubeMap");

        lightMatrixId = glGetUniformLocation(lightShaderId, "MVP");
        lightIndexId = glGetUniformLocation(lightShaderId, "lightIndex");

        depthModelMatrixId = glGetUniformLocation(depthShaderId, "modelMatrix");
        depthFarPlaneId = glGetUniformLocation(depthShaderId, "farPlane");
        depthLightPosId = glGetUniformLocation(depthShaderId, "lightPos");
        depthShadowIndex = glGetUniformLocation(depthShaderId, "shadowMapIndex");

        //Pass texture unit locations
        glUseProgram(modelShaderId);
        glUniform1i(textureSamplerId, 0);
        glUniform1i(shadowCubeMapId, 1);

        //Setup depth map framebuffer
        glCreateFramebuffers(1, &depthMapFBO);
        glNamedFramebufferDrawBuffer(depthMapFBO, GL_NONE);
        glNamedFramebufferReadBuffer(depthMapFBO, GL_NONE);

        //Enable culling triangles and depth testing (only show fragments closer than the previous)
        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
      }
    }

    namespace {
      static void setWireframe(bool enabled) {
        if (enabled) {
          glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        } else {
          glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
      }

      static void drawModel(ammonite::models::ModelInfo *drawObject, int lightIndex, bool depthPass) {
        //If the model is disabled, skip it
        if (!drawObject->active) {
          return;
        }

        //Set texture for regular shading pass
        if (lightIndex == -1 and !depthPass) {
          glBindTextureUnit(0, drawObject->textureId);
        }

        //Get draw data and bind vertex attribute buffer
        ammonite::models::MeshData* drawObjectData = drawObject->data;
        glBindVertexArray(drawObjectData->vertexArrayId);

        //Calculate and obtain matrices
        glm::mat4 modelMatrix = drawObject->positionData.modelMatrix;

        //Calculate the MVP matrix for shading and light emitter passes
        glm::mat4 mvp;
        if (!depthPass) {
          mvp = viewProjectionMatrix * modelMatrix;
        }

        //Send uniforms to the shaders
        if (depthPass) { //Depth pass
          glUniformMatrix4fv(depthModelMatrixId, 1, GL_FALSE, &modelMatrix[0][0]);
        } else if (lightIndex == -1) { //Regular pass
          glUniformMatrix4fv(matrixId, 1, GL_FALSE, &mvp[0][0]);
          glUniformMatrix4fv(modelMatrixId, 1, GL_FALSE, &modelMatrix[0][0]);
          glUniformMatrix3fv(normalMatrixId, 1, GL_FALSE, &drawObject->positionData.normalMatrix[0][0]);
        } else { //Light emitter pass
          glUniformMatrix4fv(lightMatrixId, 1, GL_FALSE, &mvp[0][0]);
          glUniform1i(lightIndexId, lightIndex);
        }

        //Set the requested draw mode (normal, wireframe, points)
        GLenum mode = GL_TRIANGLES;
        if (drawObject->drawMode == 1) {
          //Use wireframe if requested
          setWireframe(true);
        } else {
          //Draw points if requested
          if (drawObject->drawMode == 2) {
            mode = GL_POINTS;
          }
          setWireframe(false);
        }

        //Draw the triangles
        glDrawElements(mode, drawObjectData->vertexCount, GL_UNSIGNED_INT, nullptr);
      }
    }

    long getTotalFrames() {
      return totalFrames;
    }

    double getFrameTime() {
      return frameTime;
    }

    static void drawModels(const int modelIds[], const int modelCount, bool depthPass) {
      //Draw given models
      for (int i = 0; i < modelCount; i++) {
        ammonite::models::ModelInfo* modelPtr = ammonite::models::getModelPtr(modelIds[i]);
        //Only draw non-light emitting models that exist
        if (modelPtr != nullptr) {
          if (!modelPtr->lightEmitting) {
            drawModel(modelPtr, -1, depthPass);
          }
        }
      }
    }

    void drawFrame(const int modelIds[], const int modelCount) {
      //Increase frame counters
      totalFrames++;
      frameCount++;

      //Every tenth of a second, update the frame time
      static ammonite::utils::Timer frameTimer;
      double deltaTime = frameTimer.getTime();
      if (deltaTime >= 0.1f) {
        frameTime = deltaTime / frameCount;
        frameTimer.reset();
        frameCount = 0;
      }

      //Get shadow resolution and light count, save for next time to avoid cubemap recreation
      int shadowRes = ammonite::settings::graphics::getShadowRes();
      static int lastShadowRes = 0;
      unsigned int lightCount = lightTrackerMap->size();
      static unsigned int lastLightCount = 0;

      //If number of lights or shadow resolution changes, recreate cubemap
      if ((shadowRes != lastShadowRes) or (lightCount != lastLightCount)) {
        //Delete the cubemap array if it already exists
        if (depthCubeMapId != 0) {
          glDeleteTextures(1, &depthCubeMapId);
        }

        //Create a cubemap for shadows
        glCreateTextures(GL_TEXTURE_CUBE_MAP_ARRAY, 1, &depthCubeMapId);

        //Create 6 faces for each light source
        glTextureStorage3D(depthCubeMapId, 1, GL_DEPTH_COMPONENT24, shadowRes, shadowRes, lightCount * 6);

        //Set depth texture parameters
        glTextureParameteri(depthCubeMapId, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTextureParameteri(depthCubeMapId, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(depthCubeMapId, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(depthCubeMapId, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(depthCubeMapId, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        //Save for next time to avoid cubemap recreation
        lastShadowRes = shadowRes;
        lastLightCount = lightCount;
      }

      //Swap to depth shader
      glUseProgram(depthShaderId);
      glViewport(0, 0, shadowRes, shadowRes);
      glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);

      const float nearPlane = 0.0f, farPlane = 25.0f;
      glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), 1.0f, nearPlane, farPlane);

      //Pass uniforms that don't change between light source
      glUniform1f(depthFarPlaneId, farPlane);

      //Attach cubemap array to framebuffer and clear existing depths
      glNamedFramebufferTexture(depthMapFBO, GL_DEPTH_ATTACHMENT, depthCubeMapId, 0);
      glClear(GL_DEPTH_BUFFER_BIT);

      auto lightIt = lightTrackerMap->begin();
      for (unsigned int shadowCount = 0; shadowCount < lightCount; shadowCount++) {
        //Get light source and position from tracker
        auto lightSource = lightIt->second;
        glm::vec3 lightPos = lightSource.geometry;

        //Check framebuffer status
        if (glCheckNamedFramebufferStatus(depthMapFBO, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
          std::cerr << "Incomplete framebuffer" << std::endl;
        }

        //Transformations to cube map faces
        std::vector<glm::mat4> shadowTransforms;
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0, 0.0, -1.0)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.0, -1.0, 0.0)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 0.0, -1.0), glm::vec3(0.0, -1.0, 0.0)));

        //Pass shadow matrices to depth shader
        for (int i = 0; i < 6; i++) {
          GLuint shadowMatrixId = glGetUniformLocation(depthShaderId, std::string("shadowMatrices[" + std::to_string(i) + "]").c_str());
          glUniformMatrix4fv(shadowMatrixId, 1, GL_FALSE, &(shadowTransforms[i])[0][0]);
        }

        //Pass light source specific uniforms
        glUniform3fv(depthLightPosId, 1, &lightPos[0]);
        glUniform1i(depthShadowIndex, shadowCount);

        //Render to depth buffer and move to the next light source
        drawModels(modelIds, modelCount, true);
        std::advance(lightIt, 1);
      }

      //Reset the framebuffer, viewport and canvas
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
      glViewport(0, 0, ammonite::settings::runtime::getWidth(), ammonite::settings::runtime::getHeight());
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      //Prepare model shader, gamma correction and depth cube map
      glUseProgram(modelShaderId);
      glEnable(GL_FRAMEBUFFER_SRGB);
      glBindTextureUnit(1, depthCubeMapId);

      //Calculate view projection matrix
      viewProjectionMatrix = *projectionMatrix * *viewMatrix;

      //Get ambient light and camera position
      glm::vec3 ambientLight = ammonite::lighting::getAmbientLight();
      glm::vec3 cameraPosition = ammonite::camera::getPosition(ammonite::camera::getActiveCamera());

      //Pass uniforms and render regular models
      glUniform3fv(ambientLightId, 1, &ambientLight[0]);
      glUniform3fv(cameraPosId, 1, &cameraPosition[0]);
      glUniform1f(modelFarPlaneId, farPlane);
      drawModels(modelIds, modelCount, false);

      //Get information about light sources to be rendered
      int lightEmitterCount;
      std::vector<int> lightData;
      ammonite::lighting::getLightEmitters(&lightEmitterCount, &lightData);

      //Swap to the light emitting model shader
      if (lightEmitterCount > 0) {
        glUseProgram(lightShaderId);

        //Draw light sources with models attached
        for (int i = 0; i < lightEmitterCount; i++) {
          int modelId = lightData[(i * 2)];
          int lightIndex = lightData[(i * 2) + 1];
          ammonite::models::ModelInfo* modelPtr = ammonite::models::getModelPtr(modelId);

          if (modelPtr != nullptr) {
            drawModel(modelPtr, lightIndex, false);
          }
        }
      }

      //Disable gamma correction
      glDisable(GL_FRAMEBUFFER_SRGB);

      //Swap buffers
      glfwSwapBuffers(window);

      //Sleep until target frame time is met, if not 0
      static ammonite::utils::Timer targetFrameTimer;
      if (ammonite::settings::graphics::getFrameLimit() != 0.0f) {
        double targetFrameTime = 1.0 / ammonite::settings::graphics::getFrameLimit();
        double spareTime = targetFrameTime - targetFrameTimer.getTime();
        if (spareTime > 0.0) {
          std::this_thread::sleep_for(std::chrono::nanoseconds(int(std::floor(spareTime * 1000000000.0))));
        }
      }
      targetFrameTimer.reset();
    }
  }
}
