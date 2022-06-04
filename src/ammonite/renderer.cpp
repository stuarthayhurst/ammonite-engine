#include <iostream>
#include <vector>

#include <glm/glm.hpp>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "internal/modelTracker.hpp"
#include "internal/lightTracker.hpp"
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
      GLuint textureSamplerId;
      GLuint ambientLightId;
      GLuint lightSpaceMatrixId;
      GLuint shadowMapId;

      GLuint lightMatrixId;
      GLuint lightIndexId;

      GLuint depthLightSpaceMatrixId;
      GLuint depthModelMatrixId;

      GLuint depthMapId;
      GLuint depthMapFBO;
      unsigned int shadowWidth = 1024, shadowHeight = 1024;

      long totalFrames = 0;
      int frameCount = 0;
      double frameTime = 0.0f;

      glm::mat4* projectionMatrix;
      glm::mat4* viewMatrix;
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
      void setupRenderer(GLFWwindow* targetWindow, GLuint targetModelId, GLuint targetLightId, GLuint targetDepthId, bool* externalSuccess) {
        //Check GPU supported required extensions
        int failureCount = 0;
        if (!checkGPUCapabilities(&failureCount)) {
          std::cerr << failureCount << " required extensions are unsupported" << std::endl;
          *externalSuccess = false;
          return;
        }

        //Set window and shader to be used
        window = targetWindow;
        modelShaderId = targetModelId;
        lightShaderId = targetLightId;
        depthShaderId = targetDepthId;

        //Shader uniform locations
        matrixId = glGetUniformLocation(modelShaderId, "MVP");
        modelMatrixId = glGetUniformLocation(modelShaderId, "M");
        viewMatrixId = glGetUniformLocation(modelShaderId, "V");
        normalMatrixId = glGetUniformLocation(modelShaderId, "normalMatrix");
        textureSamplerId = glGetUniformLocation(modelShaderId, "textureSampler");
        ambientLightId = glGetUniformLocation(modelShaderId, "ambientLight");
        lightSpaceMatrixId = glGetUniformLocation(modelShaderId, "lightSpaceMatrix");
        shadowMapId = glGetUniformLocation(modelShaderId, "shadowMap");

        lightMatrixId = glGetUniformLocation(lightShaderId, "MVP");
        lightIndexId = glGetUniformLocation(lightShaderId, "lightIndex");

        depthLightSpaceMatrixId = glGetUniformLocation(depthShaderId, "lightSpaceMatrix");
        depthModelMatrixId = glGetUniformLocation(depthShaderId, "modelMatrix");

        //Create depth map
        glGenTextures(1, &depthMapId);
        glBindTexture(GL_TEXTURE_2D, depthMapId);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, shadowWidth, shadowHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        const float borderColour[] = {1.0f, 1.0f, 1.0f, 1.0f};
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColour);

        //Setup depth map framebuffer
        glGenFramebuffers(1, &depthMapFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapId, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        //Enable culling triangles and depth testing (only show fragments closer than the previous)
        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
      }

      void setupMatrices(glm::mat4* newProjectionMatrix, glm::mat4* newViewMatrix) {
        //Accept pointers to the view and projection matrices
        projectionMatrix = newProjectionMatrix;
        viewMatrix = newViewMatrix;
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

      static void drawModel(ammonite::models::InternalModel *drawObject, const glm::mat4 viewProjectionMatrix, int lightIndex, bool depthPass) {
        //If the model is disabled, skip it
        if (!drawObject->active) {
          return;
        }

        ammonite::models::InternalModelData* drawObjectData = drawObject->data;
        if (lightIndex == -1) {
          //Bind texture in Texture Unit 0
          glActiveTexture(GL_TEXTURE0);
          glBindTexture(GL_TEXTURE_2D, drawObject->textureId);
          //Set texture sampler to use Texture Unit 0
          glUniform1i(textureSamplerId, 0);
        }

        //Bind vertex attribute buffer
        glBindVertexArray(drawObjectData->vertexArrayId);

        //Calculate and obtain matrices
        glm::mat4 modelMatrix = drawObject->positionData.modelMatrix;
        glm::mat4 mvp = viewProjectionMatrix * modelMatrix;

        //Send uniforms to the shaders
        if (depthPass) {
          glUniformMatrix4fv(depthModelMatrixId, 1, GL_FALSE, &modelMatrix[0][0]);
        } else if (lightIndex == -1) {
          glUniformMatrix4fv(matrixId, 1, GL_FALSE, &mvp[0][0]);
          glUniformMatrix4fv(modelMatrixId, 1, GL_FALSE, &modelMatrix[0][0]);
          glUniformMatrix3fv(normalMatrixId, 1, GL_FALSE, &drawObject->positionData.normalMatrix[0][0]);
        } else {
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

    static void drawModels(const int modelIds[], const int modelCount, glm::mat4 viewProjectionMatrix, bool depthPass) {
      //Draw given models
      for (int i = 0; i < modelCount; i++) {
        ammonite::models::InternalModel* modelPtr = ammonite::models::getModelPtr(modelIds[i]);
        //Only draw non-light emitting models that exist
        if (modelPtr != nullptr) {
          if (!modelPtr->lightEmitting) {
            drawModel(modelPtr, viewProjectionMatrix, -1, depthPass);
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

      static const float nearPlane = 0.0f, farPlane = 100.0f;
      static const glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, nearPlane, farPlane);

      glm::mat4 lightView = glm::lookAt(glm::vec3(4.0f, 4.0f, 4.0f),
                                  glm::vec3(0.0f),
                                  glm::vec3(0.0f, 1.0f, 0.0f));

      //Calculate matrices used later
      glm::mat4 lightSpaceMatrix = lightProjection * lightView;
      const glm::mat4 viewProjectionMatrix = *projectionMatrix * *viewMatrix;

      //Swap to depth shader and pass light space matrix
      glUseProgram(depthShaderId);
      glUniformMatrix4fv(depthLightSpaceMatrixId, 1, GL_FALSE, &lightSpaceMatrix[0][0]);

      //Prepare to fill depth buffer
      glViewport(0, 0, shadowWidth, shadowHeight);
      glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
      glClear(GL_DEPTH_BUFFER_BIT);

      //Render to depth buffer (doesn't really need viewProjectionMatrix)
      drawModels(modelIds, modelCount, viewProjectionMatrix, true);

      //Reset the framebuffer, viewport and canvas
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
      glViewport(0, 0, 1024, 768);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      //Swap to the model shader
      glUseProgram(modelShaderId);

      //Bind the depth map in the shader
      glActiveTexture(GL_TEXTURE1);
      glBindTexture(GL_TEXTURE_2D, depthMapId);
      glUniform1i(shadowMapId, 1);

      //Pass ambient light to shader
      glm::vec3 ambientLight = ammonite::lighting::getAmbientLight();
      glUniform3f(ambientLightId, ambientLight.x, ambientLight.y, ambientLight.z);

      //Pass matrices and render regular models
      glUniformMatrix4fv(viewMatrixId, 1, GL_FALSE, &(*viewMatrix)[0][0]);
      glUniformMatrix4fv(lightSpaceMatrixId, 1, GL_FALSE, &lightSpaceMatrix[0][0]);
      drawModels(modelIds, modelCount, viewProjectionMatrix, false);

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
            drawModel(modelPtr, viewProjectionMatrix, lightIndex, false);
          }
        }
      }

      //Swap buffers
      glfwSwapBuffers(window);
    }
  }
}
