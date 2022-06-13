#include <iostream>
#include <vector>
#include <string>

#include <glm/glm.hpp>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "internal/sharedSettings.hpp"
#include "internal/modelTracker.hpp"
#include "internal/lightTracker.hpp"
#include "internal/cameraMatrices.hpp"
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
      GLuint viewMatrixId;
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

      GLuint depthCubeMapId;
      GLuint depthMapFBO;
      unsigned int shadowWidth = 1024, shadowHeight = 1024;

      long totalFrames = 0;
      int frameCount = 0;
      double frameTime = 0.0f;

      glm::mat4* viewMatrix = ammonite::camera::matrices::getViewMatrixPtr();
      glm::mat4* projectionMatrix = ammonite::camera::matrices::getProjectionMatrixPtr();

      //View projection combined matrix
      glm::mat4 viewProjectionMatrix;
    }

    namespace {
      //Check for essential GPU capabilities
      static bool checkGPUCapabilities(int* failureCount) {
        bool success = true;
        //Check SSBOs are supported
        if (!ammonite::utils::checkExtension("GL_ARB_shader_storage_buffer_object", "GL_VERSION_4_3")) {
          std::cerr << "Shader Storage Buffer Objects (SSBOs) unsupported" << std::endl;
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
        shaderLocation = shaderPath;
        shaderLocation += "models/";
        modelShaderId = ammonite::shaders::loadDirectory(shaderLocation.c_str(), externalSuccess);

        shaderLocation = shaderPath;
        shaderLocation += "lights/";
        lightShaderId = ammonite::shaders::loadDirectory(shaderLocation.c_str(), externalSuccess);

        shaderLocation = shaderPath;
        shaderLocation += "depth/";
        depthShaderId = ammonite::shaders::loadDirectory(shaderLocation.c_str(), externalSuccess);

        if (!*externalSuccess) {
          return;
        }

        //Shader uniform locations
        matrixId = glGetUniformLocation(modelShaderId, "MVP");
        modelMatrixId = glGetUniformLocation(modelShaderId, "M");
        viewMatrixId = glGetUniformLocation(modelShaderId, "V");
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

        //Pass texture unit locations
        glUseProgram(modelShaderId);
        glUniform1i(textureSamplerId, 0);
        glUniform1i(shadowCubeMapId, 1);

        //Create depth cube map
        glGenTextures(1, &depthCubeMapId);
        glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubeMapId);
        for (unsigned int i = 0; i < 6; i++) {
          glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT,
                       shadowWidth, shadowHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        }

        //Set depth texture parameters
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        //Setup depth map framebuffer
        glGenFramebuffers(1, &depthMapFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubeMapId, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

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

      static void drawModel(ammonite::models::InternalModel *drawObject, int lightIndex, bool depthPass) {
        //If the model is disabled, skip it
        if (!drawObject->active) {
          return;
        }

        //Set texture for regular shading pass
        if (lightIndex == -1 and !depthPass) {
          glActiveTexture(GL_TEXTURE0);
          glBindTexture(GL_TEXTURE_2D, drawObject->textureId);
        }

        //Get draw data and bind vertex attribute buffer
        ammonite::models::InternalModelData* drawObjectData = drawObject->data;
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
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, drawObjectData->elementBufferId);
        glDrawElements(mode, drawObjectData->vertexCount, GL_UNSIGNED_INT, (void*)0);
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
        ammonite::models::InternalModel* modelPtr = ammonite::models::getModelPtr(modelIds[i]);
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
        frameTime = (deltaTime) / frameCount;
        frameTimer.reset();
        frameCount = 0;
      }

      glm::vec3 lightPos = glm::vec3(4.0f, 4.0f, 4.0f);

      float const aspectRatio = float(shadowWidth) / float(shadowHeight);
      const float nearPlane = 0.0f, farPlane = 25.0f;
      glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), aspectRatio, nearPlane, farPlane);

      //Transformations to cube map faces
      std::vector<glm::mat4> shadowTransforms;
      shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0)));
      shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0)));
      shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0)));
      shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0, 0.0, -1.0)));
      shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.0, -1.0, 0.0)));
      shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 0.0, -1.0), glm::vec3(0.0, -1.0, 0.0)));

      //Calculate matrices used later
      viewProjectionMatrix = *projectionMatrix * *viewMatrix;

      //Swap to depth shader and pass light space matrix
      glUseProgram(depthShaderId);

      //Prepare to fill depth buffer
      glViewport(0, 0, shadowWidth, shadowHeight);
      glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
      glClear(GL_DEPTH_BUFFER_BIT);

      //Pass shadow matrices to depth shader
      for (int i = 0; i < 6; i++) {
        GLuint shadowMatrixId = glGetUniformLocation(depthShaderId, std::string("shadowMatrices[" + std::to_string(i) + "]").c_str());
        glUniformMatrix4fv(shadowMatrixId, 1, GL_FALSE, &(shadowTransforms[i])[0][0]);
      }

      //Pass depth shader uniforms
      glUniform1f(depthFarPlaneId, farPlane);
      glUniform3fv(depthLightPosId, 1, &lightPos[0]);

      //Render to depth buffer
      drawModels(modelIds, modelCount, true);

      //Reset the framebuffer, viewport and canvas
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
      glViewport(0, 0, ammonite::settings::getWidth(), ammonite::settings::getHeight());
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      //Prepare model shader, gamma correction and depth cube map
      glUseProgram(modelShaderId);
      glEnable(GL_FRAMEBUFFER_SRGB);
      glActiveTexture(GL_TEXTURE1);
      glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubeMapId);

      //Pass ambient light and camera position to shader
      glm::vec3 ambientLight = ammonite::lighting::getAmbientLight();
      glm::vec3 cameraPosition = ammonite::camera::getPosition(ammonite::camera::getActiveCamera());
      glUniform3fv(ambientLightId, 1, &ambientLight[0]);
      glUniform3fv(cameraPosId, 1, &cameraPosition[0]);
      glUniform1f(modelFarPlaneId, farPlane);

      //Pass matrices and render regular models
      glUniformMatrix4fv(viewMatrixId, 1, GL_FALSE, &(*viewMatrix)[0][0]);
      drawModels(modelIds, modelCount, false);

      //Get information about light sources to be rendered
      int lightCount;
      std::vector<int> lightData;
      ammonite::lighting::getLightEmitters(&lightCount, &lightData);

      //Swap to the light emitting model shader
      if (lightCount > 0) {
        glUseProgram(lightShaderId);

        //Draw light sources with models attached
        for (int i = 0; i < lightCount; i++) {
          int modelId = lightData[(i * 2)];
          int lightIndex = lightData[(i * 2) + 1];
          ammonite::models::InternalModel* modelPtr = ammonite::models::getModelPtr(modelId);

          if (modelPtr != nullptr) {
            drawModel(modelPtr, lightIndex, false);
          }
        }
      }

      //Disable gamma correction
      glDisable(GL_FRAMEBUFFER_SRGB);

      //Swap buffers
      glfwSwapBuffers(window);
    }
  }
}
