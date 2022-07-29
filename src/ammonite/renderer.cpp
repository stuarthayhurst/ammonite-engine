#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <cmath>

#include <glm/glm.hpp>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "internal/internalSettings.hpp"
#include "internal/modelTracker.hpp"
#include "internal/lightTracker.hpp"
#include "internal/cameraMatrices.hpp"

#include "settings.hpp"
#include "shaders.hpp"
#include "camera.hpp"
#include "lightManager.hpp"
#include "environment.hpp"

#include "utils/timer.hpp"
#include "utils/extension.hpp"

namespace ammonite {
  namespace renderer {
    namespace {
      GLFWwindow* window;

      //Structures to store uniform IDs for the shaders
      struct {
        GLuint shaderId;
        GLuint matrixId;
        GLuint modelMatrixId;
        GLuint normalMatrixId;
        GLuint ambientLightId;
        GLuint cameraPosId;
        GLuint farPlaneId;
        GLuint lightCountId;
        GLuint textureSamplerId;
        GLuint shadowCubeMapId;
      } modelShader;

      struct {
        GLuint shaderId;
        GLuint lightMatrixId;
        GLuint lightIndexId;
      } lightShader;

      struct {
        GLuint shaderId;
        GLuint modelMatrixId;
        GLuint farPlaneId;
        GLuint depthLightPosId;
        GLuint depthShadowIndex;
      } depthShader;

      struct {
        GLuint shaderId;
        GLuint viewMatrixId;
        GLuint projectionMatrixId;
        GLuint skyboxSamplerId;
      } skyboxShader;

      GLuint skyboxVertexArrayId;

      GLuint depthCubeMapId = 0;
      GLuint depthMapFBO;

      long totalFrames = 0;
      int frameCount = 0;
      double frameTime = 0.0f;

      glm::mat4* viewMatrix = ammonite::camera::matrices::getViewMatrixPtr();
      glm::mat4* projectionMatrix = ammonite::camera::matrices::getProjectionMatrixPtr();

      //Get the light trackers
      std::map<int, ammonite::lighting::LightSource>* lightTrackerMap = ammonite::lighting::getLightTracker();
      std::map<int, glm::mat4[6]>* lightTransformMap = ammonite::lighting::getLightTransforms();
      unsigned int maxLightCount = 0;

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
          (*failureCount)++;
        }

        //Check SSBOs are supported
        if (!ammonite::utils::checkExtension("GL_ARB_shader_storage_buffer_object", "GL_VERSION_4_3")) {
          std::cerr << "Shader Storage Buffer Objects (SSBOs) unsupported" << std::endl;
          success = false;
          (*failureCount)++;
        }

        //Check texture storage is supported
        if (!ammonite::utils::checkExtension("GL_ARB_texture_storage", "GL_VERSION_4_2")) {
          std::cerr << "Texture storage unsupported" << std::endl;
          success = false;
          (*failureCount)++;
        }

        //Check cubemap arrays are supported
        if (!ammonite::utils::checkExtension("GL_ARB_texture_cube_map_array", "GL_VERSION_4_0")) {
          std::cerr << "Cubemap arrays unsupported" << std::endl;
          success = false;
          (*failureCount)++;
        }

        //Check minimum OpenGL version is supported
        if (!glewIsSupported("GL_VERSION_3_2")) {
          std::cerr << "OpenGL 3.2 unsupported" << std::endl;
          success = false;
          (*failureCount)++;
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
        bool hasCreatedShaders = true;
        std::string shaderLocation = std::string(shaderPath) + std::string("models/");
        modelShader.shaderId = ammonite::shaders::loadDirectory(shaderLocation.c_str(), &hasCreatedShaders);

        shaderLocation = std::string(shaderPath) + std::string("lights/");
        lightShader.shaderId = ammonite::shaders::loadDirectory(shaderLocation.c_str(), &hasCreatedShaders);

        shaderLocation = std::string(shaderPath) + std::string("depth/");
        depthShader.shaderId = ammonite::shaders::loadDirectory(shaderLocation.c_str(), &hasCreatedShaders);

        shaderLocation = std::string(shaderPath) + std::string("skybox/");
        skyboxShader.shaderId = ammonite::shaders::loadDirectory(shaderLocation.c_str(), &hasCreatedShaders);

        if (!hasCreatedShaders) {
          *externalSuccess = false;
          return;
        }

        //Shader uniform locations
        modelShader.matrixId = glGetUniformLocation(modelShader.shaderId, "MVP");
        modelShader.modelMatrixId = glGetUniformLocation(modelShader.shaderId, "modelMatrix");
        modelShader.normalMatrixId = glGetUniformLocation(modelShader.shaderId, "normalMatrix");
        modelShader.ambientLightId = glGetUniformLocation(modelShader.shaderId, "ambientLight");
        modelShader.cameraPosId = glGetUniformLocation(modelShader.shaderId, "cameraPos");
        modelShader.farPlaneId = glGetUniformLocation(modelShader.shaderId, "farPlane");
        modelShader.lightCountId = glGetUniformLocation(modelShader.shaderId, "lightCount");
        modelShader.textureSamplerId = glGetUniformLocation(modelShader.shaderId, "textureSampler");
        modelShader.shadowCubeMapId = glGetUniformLocation(modelShader.shaderId, "shadowCubeMap");

        lightShader.lightMatrixId = glGetUniformLocation(lightShader.shaderId, "MVP");
        lightShader.lightIndexId = glGetUniformLocation(lightShader.shaderId, "lightIndex");

        depthShader.modelMatrixId = glGetUniformLocation(depthShader.shaderId, "modelMatrix");
        depthShader.farPlaneId = glGetUniformLocation(depthShader.shaderId, "farPlane");
        depthShader.depthLightPosId = glGetUniformLocation(depthShader.shaderId, "lightPos");
        depthShader.depthShadowIndex = glGetUniformLocation(depthShader.shaderId, "shadowMapIndex");

        skyboxShader.viewMatrixId = glGetUniformLocation(skyboxShader.shaderId, "viewMatrix");
        skyboxShader.projectionMatrixId = glGetUniformLocation(skyboxShader.shaderId, "projectionMatrix");
        skyboxShader.skyboxSamplerId = glGetUniformLocation(skyboxShader.shaderId, "skyboxSampler");

        //Pass texture unit locations
        glUseProgram(modelShader.shaderId);
        glUniform1i(modelShader.textureSamplerId, 0);
        glUniform1i(modelShader.shadowCubeMapId, 1);

        glUseProgram(skyboxShader.shaderId);
        glUniform1i(skyboxShader.skyboxSamplerId, 2);

        //Setup depth map framebuffer
        glCreateFramebuffers(1, &depthMapFBO);
        glNamedFramebufferDrawBuffer(depthMapFBO, GL_NONE);
        glNamedFramebufferReadBuffer(depthMapFBO, GL_NONE);

        //Enable seamless cubemaps
        glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

        //Enable culling triangles and depth testing
        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);

        //Get the max number of lights supported
        maxLightCount = ammonite::lighting::getMaxLightCount();

        const char skyboxVertices[] = {
          -1,  1, -1,
          -1, -1, -1,
           1, -1, -1,
           1,  1, -1,
          -1, -1,  1,
          -1,  1,  1,
           1, -1,  1,
           1,  1,  1
        };

        const unsigned char skyboxIndices[36] = {
          0, 1, 2, 2, 3, 0,
          4, 1, 0, 0, 5, 4,
          2, 6, 7, 7, 3, 2,
          4, 5, 7, 7, 6, 4,
          0, 3, 7, 7, 5, 0,
          1, 4, 2, 2, 4, 6
        };


        //Create vertex and element buffers for the skybox
        GLuint skyboxBufferId, skyboxElementBufferId;
        glCreateBuffers(1, &skyboxBufferId);
        glCreateBuffers(1, &skyboxElementBufferId);
        glNamedBufferData(skyboxBufferId, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
        glNamedBufferData(skyboxElementBufferId, sizeof(skyboxIndices), &skyboxIndices, GL_STATIC_DRAW);

        //Create vertex array object for skybox
        glCreateVertexArrays(1, &skyboxVertexArrayId);
        glEnableVertexArrayAttrib(skyboxVertexArrayId, 0);
        glVertexArrayVertexBuffer(skyboxVertexArrayId, 0, skyboxBufferId, 0, 3 * sizeof(char));
        glVertexArrayAttribFormat(skyboxVertexArrayId, 0, 3, GL_BYTE, GL_FALSE, 0);
        glVertexArrayAttribBinding(skyboxVertexArrayId, 0, 0);
        glVertexArrayElementBuffer(skyboxVertexArrayId, skyboxElementBufferId);
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
        if (!drawObject->isActive or !drawObject->isLoaded) {
          return;
        }

        //Get model draw data
        ammonite::models::ModelData* drawObjectData = drawObject->modelData;

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

        //Calculate and obtain matrices
        glm::mat4 modelMatrix = drawObject->positionData.modelMatrix;

        //Calculate the MVP matrix for shading and light emitter passes
        glm::mat4 mvp;
        if (!depthPass) {
          mvp = viewProjectionMatrix * modelMatrix;
        }

        //Send uniforms to the shaders
        if (depthPass) { //Depth pass
          glUniformMatrix4fv(depthShader.modelMatrixId, 1, GL_FALSE, &modelMatrix[0][0]);
        } else if (lightIndex == -1) { //Regular pass
          glUniformMatrix4fv(modelShader.matrixId, 1, GL_FALSE, &mvp[0][0]);
          glUniformMatrix4fv(modelShader.modelMatrixId, 1, GL_FALSE, &modelMatrix[0][0]);
          glUniformMatrix3fv(modelShader.normalMatrixId, 1, GL_FALSE, &drawObject->positionData.normalMatrix[0][0]);
        } else { //Light emitter pass
          glUniformMatrix4fv(lightShader.lightMatrixId, 1, GL_FALSE, &mvp[0][0]);
          glUniform1i(lightShader.lightIndexId, lightIndex);
        }

        for (unsigned int i = 0; i < drawObjectData->meshes.size(); i++) {
          //Set texture for regular shading pass
          if (lightIndex == -1 and !depthPass) {
            glBindTextureUnit(0, drawObject->textureIds[i]);
          }

          //Bind vertex attribute buffer
          glBindVertexArray(drawObjectData->meshes[i].vertexArrayId);

          //Draw the triangles
          glDrawElements(mode, drawObjectData->meshes[i].vertexCount, GL_UNSIGNED_INT, nullptr);
        }
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
          if (!modelPtr->isLightEmitting) {
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
      static int* shadowResPtr = ammonite::settings::graphics::internal::getShadowResPtr();
      static int lastShadowRes = 0;
      unsigned int lightCount = lightTrackerMap->size();
      static unsigned int lastLightCount = -1;

      //If number of lights or shadow resolution changes, recreate cubemap
      if ((*shadowResPtr != lastShadowRes) or (lightCount != lastLightCount)) {
        //Delete the cubemap array if it already exists
        if (depthCubeMapId != 0) {
          glDeleteTextures(1, &depthCubeMapId);
        }

        //Create a cubemap for shadows
        glCreateTextures(GL_TEXTURE_CUBE_MAP_ARRAY, 1, &depthCubeMapId);

        //Create 6 faces for each light source
        int depthLayers = std::min(maxLightCount, lightCount) * 6;
        glTextureStorage3D(depthCubeMapId, 1, GL_DEPTH_COMPONENT32, *shadowResPtr, *shadowResPtr, depthLayers);

        //Set depth texture parameters
        glTextureParameteri(depthCubeMapId, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(depthCubeMapId, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(depthCubeMapId, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        glTextureParameteri(depthCubeMapId, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
        glTextureParameteri(depthCubeMapId, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(depthCubeMapId, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(depthCubeMapId, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        //Attach cubemap array to framebuffer
        glNamedFramebufferTexture(depthMapFBO, GL_DEPTH_ATTACHMENT, depthCubeMapId, 0);

        //Save for next time to avoid cubemap recreation
        lastShadowRes = *shadowResPtr;
        lastLightCount = lightCount;
      }

      //Swap to depth shader
      glUseProgram(depthShader.shaderId);
      glViewport(0, 0, *shadowResPtr, *shadowResPtr);
      glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);

      //Pass uniforms that don't change between light source
      static float* farPlanePtr = ammonite::settings::graphics::internal::getShadowFarPlanePtr();
      glUniform1f(depthShader.farPlaneId, *farPlanePtr);

      //Clear existing depths values
      glClear(GL_DEPTH_BUFFER_BIT);

      auto lightIt = lightTrackerMap->begin();
      unsigned int activeLights = std::min(lightCount, maxLightCount);
      for (unsigned int shadowCount = 0; shadowCount < activeLights; shadowCount++) {
        //Get light source and position from tracker
        auto lightSource = &lightIt->second;
        glm::vec3 lightPos = lightSource->geometry;

        //Check framebuffer status
        if (glCheckNamedFramebufferStatus(depthMapFBO, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
          std::cerr << "Warning: Incomplete depth framebuffer" << std::endl;
        }

        //Pass shadow transform matrices to depth shader
        for (int i = 0; i < 6; i++) {
          GLuint shadowMatrixId = glGetUniformLocation(depthShader.shaderId, std::string("shadowMatrices[" + std::to_string(i) + "]").c_str());
          //Fetch the transform from the tracker, and send to the shader
          glUniformMatrix4fv(shadowMatrixId, 1, GL_FALSE, &((*lightTransformMap)[lightSource->lightId][i])[0][0]);
        }

        //Pass light source specific uniforms
        glUniform3fv(depthShader.depthLightPosId, 1, &lightPos[0]);
        glUniform1i(depthShader.depthShadowIndex, shadowCount);

        //Render to depth buffer and move to the next light source
        drawModels(modelIds, modelCount, true);
        std::advance(lightIt, 1);
      }

      //Reset the framebuffer and viewport
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
      static int* widthPtr = ammonite::settings::runtime::internal::getWidthPtr();
      static int* heightPtr = ammonite::settings::runtime::internal::getHeightPtr();
      glViewport(0, 0, *widthPtr, *heightPtr);

      //CLear depth and colour (if no skybox is used)
      int activeSkybox = ammonite::environment::skybox::getActiveSkybox();
      if (activeSkybox == 0) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      } else {
        glClear(GL_DEPTH_BUFFER_BIT);
      }

      //Prepare model shader and depth cube map
      glUseProgram(modelShader.shaderId);
      glBindTextureUnit(1, depthCubeMapId);

      //Use gamma correction if enabled
      static bool* gammaPtr = ammonite::settings::graphics::internal::getGammaCorrectionPtr();
      if (*gammaPtr) {
        glEnable(GL_FRAMEBUFFER_SRGB);
      }

      //Calculate view projection matrix
      viewProjectionMatrix = *projectionMatrix * *viewMatrix;

      //Get ambient light and camera position
      glm::vec3 ambientLight = ammonite::lighting::getAmbientLight();
      glm::vec3 cameraPosition = ammonite::camera::getPosition(ammonite::camera::getActiveCamera());

      //Pass uniforms and render regular models
      glUniform3fv(modelShader.ambientLightId, 1, &ambientLight[0]);
      glUniform3fv(modelShader.cameraPosId, 1, &cameraPosition[0]);
      glUniform1f(modelShader.farPlaneId, *farPlanePtr);
      glUniform1i(modelShader.lightCountId, activeLights);
      drawModels(modelIds, modelCount, false);

      //Get information about light sources to be rendered
      int lightEmitterCount;
      std::vector<int> lightData;
      ammonite::lighting::getLightEmitters(&lightEmitterCount, &lightData);

      //Swap to the light emitting model shader
      if (lightEmitterCount > 0) {
        glUseProgram(lightShader.shaderId);

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

      //Draw the skybox
      if (activeSkybox != 0) {
        //Swap to skybox shader and pass uniforms
        glUseProgram(skyboxShader.shaderId);
        glUniformMatrix4fv(skyboxShader.viewMatrixId, 1, GL_FALSE, &(glm::mat4(glm::mat3(*viewMatrix)))[0][0]);
        glUniformMatrix4fv(skyboxShader.projectionMatrixId, 1, GL_FALSE, &(*projectionMatrix)[0][0]);

        //Prepare and draw the skybox
        setWireframe(false);
        glBindVertexArray(skyboxVertexArrayId);
        glBindTextureUnit(2, activeSkybox);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_BYTE, nullptr);
      }

      //Disable gamma correction for start of next pass
      glDisable(GL_FRAMEBUFFER_SRGB);

      //Swap buffers
      glfwSwapBuffers(window);

      //Figure out time budget remaining
      static ammonite::utils::Timer targetFrameTimer;

      //Wait until next frame should be prepared
      static float* frameLimitPtr = ammonite::settings::graphics::internal::getFrameLimitPtr();
      if (*frameLimitPtr != 0.0f) {
        //Length of microsleep and allowable error
        static const double sleepInterval = 1.0 / 100000;
        static const double maxError = (sleepInterval) * 1.1;
        static const auto sleepLength = std::chrono::nanoseconds(int(std::floor(sleepInterval * 1000000000.0)));

        double const targetFrameTime = 1.0 / *frameLimitPtr;
        double spareTime = targetFrameTime - targetFrameTimer.getTime();;

        //Sleep for short intervals until the frametime budget is gone
        while (spareTime > maxError) {
          std::this_thread::sleep_for(sleepLength);
          spareTime = targetFrameTime - targetFrameTimer.getTime();
        }
      }

      targetFrameTimer.reset();
    }
  }
}
