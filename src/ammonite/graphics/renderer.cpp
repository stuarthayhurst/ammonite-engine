#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "../internal/internalSettings.hpp"
#include "../internal/cameraMatrices.hpp"
#include "../internal/interfaceTracker.hpp"

#include "../models/internal/modelTracker.hpp"

#include "../lighting/internal/lightTypes.hpp"
#include "../lighting/internal/lightTracker.hpp"
#include "../lighting/lightStorage.hpp"

#include "shaders.hpp"
#include "../constants.hpp"
#include "../settings.hpp"
#include "../camera.hpp"
#include "../environment.hpp"

#include "../utils/timer.hpp"
#include "../utils/extension.hpp"
#include "../utils/logging.hpp"

#include "../internal/internalDebug.hpp"

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

      struct {
        GLuint shaderId;
        GLuint screenSamplerId;
      } screenShader;

      struct {
        GLuint shaderId;
        GLuint progressId;
        GLuint widthId;
        GLuint heightId;
        GLuint heightOffsetId;
        GLuint progressColourId;
      } loadingShader;

      GLuint skyboxVertexArrayId;
      GLuint screenQuadVertexArrayId;

      GLuint depthCubeMapId = 0;
      GLuint depthMapFBO;

      GLuint screenQuadTextureId = 0;
      GLuint screenQuadFBO;
      GLuint depthRenderBufferId;
      GLuint colourRenderBufferId = 0;
      GLuint colourBufferMultisampleFBO;

      long totalFrames = 0;
      int frameCount = 0;
      double frameTime = 0.0f;

      glm::mat4* viewMatrix = ammonite::camera::matrices::getViewMatrixPtr();
      glm::mat4* projectionMatrix = ammonite::camera::matrices::getProjectionMatrixPtr();

      //Get the light trackers
      std::map<int, ammonite::lighting::internal::LightSource>* lightTrackerMap = ammonite::lighting::internal::getLightTracker();
      std::map<int, glm::mat4[6]>* lightTransformMap = ammonite::lighting::internal::getLightTransforms();
      unsigned int maxLightCount = 0;

      //Get the loading screen tracker
      std::map<int, ammonite::interface::LoadingScreen>* loadingScreenTracker = ammonite::interface::internal::getLoadingScreenTracker();

      int maxSampleCount = 0;

      //View projection combined matrix
      glm::mat4 viewProjectionMatrix;

      //Render modes for drawModels()
      enum AmmoniteRenderMode : unsigned char {
        AMMONITE_RENDER_PASS,
        AMMONITE_DEPTH_PASS,
        AMMONITE_DATA_REFRESH
      };
    }

    namespace {
      //Check for essential GPU capabilities
      static bool checkGPUCapabilities(int* failureCount) {
        const char* extensions[5][3] = {
          {"GL_ARB_direct_state_access", "GL_VERSION_4_5", "Direct state access"},
          {"GL_ARB_shader_storage_buffer_object", "GL_VERSION_4_3", "Shader Storage Buffer Objects (SSBOs)"},
          {"GL_ARB_texture_storage", "GL_VERSION_4_2", "Texture storage"},
          {"GL_ARB_shading_language_420pack", "GL_VERSION_4_2", "GLSL shader version 4.20"},
          {"GL_ARB_texture_cube_map_array", "GL_VERSION_4_0", "Cubemap arrays"}
        };
        const int extensionCount = sizeof(extensions) / sizeof(extensions[0]);

        bool success = true;
        for (int i = 0; i < extensionCount; i++) {
          if (!ammonite::utils::checkExtension(extensions[i][0], extensions[i][1])) {
            std::cerr << ammonite::utils::error << extensions[i][2] << " unsupported" << std::endl;
            success = false;
            (*failureCount)++;
          }
        }

        //Check minimum OpenGL version is supported
        if (!glewIsSupported("GL_VERSION_3_2")) {
          std::cerr << ammonite::utils::error << "OpenGL 3.2 unsupported" << std::endl;
          success = false;
          (*failureCount)++;
        }

        return success;
      }
    }

    namespace setup {
      void setupRenderer(GLFWwindow* targetWindow, const char* shaderPath, bool* externalSuccess) {
        //Start a timer to measure load time
        ammonite::utils::Timer loadTimer;

        //Check GPU supported required extensions
        int failureCount = 0;
        if (!checkGPUCapabilities(&failureCount)) {
          std::cerr << ammonite::utils::error << failureCount << " required extension(s) unsupported" << std::endl;
          *externalSuccess = false;
          return;
        }

        //Set window to be used
        window = targetWindow;

        //Directory and pointer to ID of each shader
        struct {
          std::string shaderDir;
          GLuint* shaderId;
        } shaderInfo[6] = {
          {"models/", &modelShader.shaderId},
          {"lights/", &lightShader.shaderId},
          {"depth/", &depthShader.shaderId},
          {"skybox/", &skyboxShader.shaderId},
          {"screen/", &screenShader.shaderId},
          {"loading/", &loadingShader.shaderId}
        };

        //Load shaders
        bool hasCreatedShaders = true;
        const int shaderCount = sizeof(shaderInfo) / sizeof(shaderInfo[0]);
        for (int i = 0; i < shaderCount; i++) {
          std::string shaderLocation = std::string(shaderPath) + shaderInfo[i].shaderDir;
          *shaderInfo[i].shaderId = ammonite::shaders::loadDirectory(shaderLocation.c_str(), &hasCreatedShaders);
        }

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

        screenShader.screenSamplerId = glGetUniformLocation(screenShader.shaderId, "screenSampler");

        loadingShader.progressId = glGetUniformLocation(loadingShader.shaderId, "progress");
        loadingShader.widthId = glGetUniformLocation(loadingShader.shaderId, "width");
        loadingShader.heightId = glGetUniformLocation(loadingShader.shaderId, "height");
        loadingShader.heightOffsetId = glGetUniformLocation(loadingShader.shaderId, "heightOffset");
        loadingShader.progressColourId = glGetUniformLocation(loadingShader.shaderId, "progressColour");

        //Pass texture unit locations
        glUseProgram(modelShader.shaderId);
        glUniform1i(modelShader.textureSamplerId, 0);
        glUniform1i(modelShader.shadowCubeMapId, 1);

        glUseProgram(skyboxShader.shaderId);
        glUniform1i(skyboxShader.skyboxSamplerId, 2);

        glUseProgram(screenShader.shaderId);
        glUniform1i(screenShader.screenSamplerId, 3);

        //Setup depth map framebuffer
        glCreateFramebuffers(1, &depthMapFBO);
        glNamedFramebufferDrawBuffer(depthMapFBO, GL_NONE);
        glNamedFramebufferReadBuffer(depthMapFBO, GL_NONE);

        //Create multisampled framebuffer and depthbuffer to draw to
        glCreateFramebuffers(1, &colourBufferMultisampleFBO);
        glCreateFramebuffers(1, &screenQuadFBO);
        glCreateRenderbuffers(1, &depthRenderBufferId);

        //Enable seamless cubemaps
        glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

        //Enable multisampling
        glEnable(GL_MULTISAMPLE);

        //Enable culling triangles setup depth testing function
        glEnable(GL_CULL_FACE);
        glDepthFunc(GL_LEQUAL);

        //Find multisampling limits
        glGetIntegerv(GL_MAX_SAMPLES, &maxSampleCount);

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

        //Position and texture coord of screen quad
        const char screenVertices[16] = {
          -1,  1,  0,  1,
          -1, -1,  0,  0,
           1, -1,  1,  0,
           1,  1,  1,  1
        };

        const unsigned char screenIndices[6] = {
          0, 1, 2,
          0, 2, 3
        };

        struct {
          GLuint skybox;
          GLuint skyboxElement;
          GLuint screenQuad;
          GLuint screenQuadElement;
        } bufferIds;

        //Create vertex and element buffers for the skybox and screen quad
        glCreateBuffers(4, &bufferIds.skybox);

        //Fill vertex and element buffers for the skybox
        glNamedBufferData(bufferIds.skybox, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
        glNamedBufferData(bufferIds.skyboxElement, sizeof(skyboxIndices), &skyboxIndices, GL_STATIC_DRAW);

        //Create vertex array object for skybox
        glCreateVertexArrays(1, &skyboxVertexArrayId);
        glEnableVertexArrayAttrib(skyboxVertexArrayId, 0);
        glVertexArrayVertexBuffer(skyboxVertexArrayId, 0, bufferIds.skybox, 0, 3 * sizeof(char));
        glVertexArrayAttribFormat(skyboxVertexArrayId, 0, 3, GL_BYTE, GL_FALSE, 0);
        glVertexArrayAttribBinding(skyboxVertexArrayId, 0, 0);
        glVertexArrayElementBuffer(skyboxVertexArrayId, bufferIds.skyboxElement);

        //Fill vertex and element buffers for the screen quad
        glNamedBufferData(bufferIds.screenQuad, sizeof(screenVertices), &screenVertices, GL_STATIC_DRAW);
        glNamedBufferData(bufferIds.screenQuadElement, sizeof(screenIndices), &screenIndices, GL_STATIC_DRAW);

        //Create vertex array object for screen quad
        glCreateVertexArrays(1, &screenQuadVertexArrayId);
        //Vertex positions
        glEnableVertexArrayAttrib(screenQuadVertexArrayId, 0);
        glVertexArrayVertexBuffer(screenQuadVertexArrayId, 0, bufferIds.screenQuad, 0, 4 * sizeof(char));
        glVertexArrayAttribFormat(screenQuadVertexArrayId, 0, 2, GL_BYTE, GL_FALSE, 0);
        glVertexArrayAttribBinding(screenQuadVertexArrayId, 0, 0);

        //Texture coords
        glEnableVertexArrayAttrib(screenQuadVertexArrayId, 1);
        glVertexArrayVertexBuffer(screenQuadVertexArrayId, 1, bufferIds.screenQuad, 2 * sizeof(char), 4 * sizeof(char));
        glVertexArrayAttribFormat(screenQuadVertexArrayId, 1, 2, GL_BYTE, GL_FALSE, 0);
        glVertexArrayAttribBinding(screenQuadVertexArrayId, 1, 1);

        glVertexArrayElementBuffer(screenQuadVertexArrayId, bufferIds.screenQuadElement);

        //Output time taken to load renderer
        std::cout << ammonite::utils::status << "Loaded renderer in: " << loadTimer.getTime() << "s" << std::endl;
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
        //Get model draw data
        ammonite::models::ModelData* drawObjectData = drawObject->modelData;

        //Set the requested draw mode (normal, wireframe, points)
        GLenum mode = GL_TRIANGLES;
        if (drawObject->drawMode == AMMONITE_DRAW_WIREFRAME) {
          //Use wireframe if requested
          setWireframe(true);
        } else {
          //Draw points if requested
          if (drawObject->drawMode == AMMONITE_DRAW_POINTS) {
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
          glUniformMatrix4fv(depthShader.modelMatrixId, 1, GL_FALSE, glm::value_ptr(modelMatrix));
        } else if (lightIndex == -1) { //Regular pass
          glUniformMatrix4fv(modelShader.matrixId, 1, GL_FALSE, glm::value_ptr(mvp));
          glUniformMatrix4fv(modelShader.modelMatrixId, 1, GL_FALSE, glm::value_ptr(modelMatrix));
          glUniformMatrix3fv(modelShader.normalMatrixId, 1, GL_FALSE, glm::value_ptr(drawObject->positionData.normalMatrix));
        } else { //Light emitter pass
          glUniformMatrix4fv(lightShader.lightMatrixId, 1, GL_FALSE, glm::value_ptr(mvp));
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

    static void drawModels(AmmoniteRenderMode renderMode) {
      //Create initial array for model pointers
      static int modelCount = ammonite::models::getModelCount(AMMONITE_MODEL);
      static ammonite::models::ModelInfo** modelPtrs = new ammonite::models::ModelInfo* [modelCount];

      //If requested, create a new array for model pointers
      if (renderMode == AMMONITE_DATA_REFRESH) {
        //Replace array with one of the correct size
        modelCount = ammonite::models::getModelCount(AMMONITE_MODEL);
        ammonite::models::ModelInfo** newArr = new ammonite::models::ModelInfo* [modelCount];
        delete [] modelPtrs;
        modelPtrs = newArr;

        //Update saved model pointers and return
        ammonite::models::getModels(AMMONITE_MODEL, modelCount, modelPtrs);
        return;
      }

      //Draw the model pointers
      for (int i = 0; i < modelCount; i++) {
        drawModel(modelPtrs[i], -1, (renderMode == AMMONITE_DEPTH_PASS));
      }
    }

    //Create, configure and bind depth cubemap for shadows
    static void setupDepthMap(unsigned int lightCount, int shadowRes) {
      //Delete the cubemap array if it already exists
      if (depthCubeMapId != 0) {
        glDeleteTextures(1, &depthCubeMapId);
      }

      //Create a cubemap for shadows
      glCreateTextures(GL_TEXTURE_CUBE_MAP_ARRAY, 1, &depthCubeMapId);

      //Create 6 faces for each light source
      int depthLayers = std::min(maxLightCount, lightCount) * 6;
      glTextureStorage3D(depthCubeMapId, 1, GL_DEPTH_COMPONENT32, shadowRes, shadowRes, depthLayers);

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
    }

    static void drawSkybox(int activeSkyboxId) {
      //Swap to skybox shader and pass uniforms
      glUseProgram(skyboxShader.shaderId);
      glUniformMatrix4fv(skyboxShader.viewMatrixId, 1, GL_FALSE, glm::value_ptr(glm::mat4(glm::mat3(*viewMatrix))));
      glUniformMatrix4fv(skyboxShader.projectionMatrixId, 1, GL_FALSE, glm::value_ptr(*projectionMatrix));

      //Prepare and draw the skybox
      glBindVertexArray(skyboxVertexArrayId);
      glBindTextureUnit(2, activeSkyboxId);
      glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_BYTE, nullptr);
    }

    void finishFrame() {
      //Swap buffers
      glfwSwapBuffers(window);

      //Figure out time budget remaining
      static ammonite::utils::Timer targetFrameTimer;

      //Wait until next frame should be prepared
      static float* frameLimitPtr = ammonite::settings::graphics::internal::getFrameLimitPtr();
      if (*frameLimitPtr != 0.0f) {
        //Length of microsleep and allowable error
        static const double sleepInterval = 1.0 / 100000;
        static const double maxError = (sleepInterval) * 2.0f;
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

    void drawFrame() {
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

      static int lastWidth = 0, lastHeight = 0;
      static int* widthPtr = ammonite::settings::runtime::internal::getWidthPtr();
      static int* heightPtr = ammonite::settings::runtime::internal::getHeightPtr();

      static float lastRenderResMultiplier = 0.0f;
      static float* renderResMultiplierPtr = ammonite::settings::graphics::internal::getRenderResMultiplierPtr();

      static int lastSamples = 0;
      static int* samplesPtr = ammonite::settings::graphics::internal::getAntialiasingSamplesPtr();
      static int sampleCount = *samplesPtr;

      static int renderWidth = 0, renderHeight = 0;

      //Recreate the framebuffer if the width, height, resolution multiplier or sample count changes
      static GLuint targetBufferId = 0;
      if ((lastWidth != *widthPtr) or (lastHeight != *heightPtr) or (lastRenderResMultiplier != *renderResMultiplierPtr) or (lastSamples != *samplesPtr)) {
        //Update values used to determine when to recreate framebuffer
        lastWidth = *widthPtr;
        lastHeight = *heightPtr;
        lastRenderResMultiplier = *renderResMultiplierPtr;
        lastSamples = *samplesPtr;

        //Limit sample count to implementation limit
        const int requestedSamples = *samplesPtr;
        sampleCount = std::min(requestedSamples, maxSampleCount);

        if (sampleCount < requestedSamples) {
          std::cerr << ammonite::utils::warning << "Ignoring request for " << requestedSamples << " samples, using implementation limit of " << maxSampleCount << std::endl;
          *samplesPtr = sampleCount;
        }

        //Delete regular colour storage texture
        if (screenQuadTextureId != 0) {
          glDeleteTextures(1, &screenQuadTextureId);
        }

        //Delete multisampled colour storage if it exists
        if (colourRenderBufferId != 0) {
          glDeleteRenderbuffers(1, &colourRenderBufferId);
        }

        //Create texture for whole screen (and multisampled renderbuffer, if needed
        glCreateTextures(GL_TEXTURE_2D, 1, &screenQuadTextureId);

        //Deicde which framebuffer to render to and create multisampled renderbuffer, if needed
        if (sampleCount != 0) {
          targetBufferId = colourBufferMultisampleFBO;
          glCreateRenderbuffers(1, &colourRenderBufferId);
        } else {
          targetBufferId = screenQuadFBO;
          colourRenderBufferId = 0;
        }

        //Calculate render resolution
        renderWidth = std::floor(*widthPtr * *renderResMultiplierPtr);
        renderHeight = std::floor(*heightPtr * *renderResMultiplierPtr);

        //Create multisampled renderbuffer to store colour data and bind to framebuffer
        if (sampleCount != 0) {
          glNamedRenderbufferStorageMultisample(colourRenderBufferId, sampleCount, GL_RGB8, renderWidth, renderHeight);
          glNamedFramebufferRenderbuffer(colourBufferMultisampleFBO, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, colourRenderBufferId);
        }

        //Create texture to store colour data and bind to framebuffer
        glTextureStorage2D(screenQuadTextureId, 1, GL_RGB8, renderWidth, renderHeight);
        glTextureParameteri(screenQuadTextureId, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(screenQuadTextureId, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(screenQuadTextureId, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(screenQuadTextureId, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glNamedFramebufferTexture(screenQuadFBO, GL_COLOR_ATTACHMENT0, screenQuadTextureId, 0);

        //Attach the depth buffer
        if (sampleCount != 0) {
          glNamedRenderbufferStorageMultisample(depthRenderBufferId, sampleCount, GL_DEPTH_COMPONENT, renderWidth, renderHeight);
          //Detach from other framebuffer, in case it was bound
          glNamedFramebufferRenderbuffer(screenQuadFBO, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
        } else {
          glNamedRenderbufferStorage(depthRenderBufferId, GL_DEPTH_COMPONENT, renderWidth, renderHeight);
          //Detach from other framebuffer, in case it was bound
          glNamedFramebufferRenderbuffer(colourBufferMultisampleFBO, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
        }
        glNamedFramebufferRenderbuffer(targetBufferId, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderBufferId);

        //Check multisampled framebuffer
        if (sampleCount != 0) {
          if (glCheckNamedFramebufferStatus(colourBufferMultisampleFBO, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << ammonite::utils::warning << "Incomplete multisampled render framebuffer" << std::endl;
          } else {
            ammoniteInternalDebug << "Created new multisampled render framebuffer (" << renderWidth << " x " << renderHeight << "), samples: x" << sampleCount << std::endl;
          }
        }

        //Check regular framebuffer
        if (glCheckNamedFramebufferStatus(screenQuadFBO, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
          std::cerr << ammonite::utils::warning << "Incomplete render framebuffer" << std::endl;
        } else {
          ammoniteInternalDebug << "Created new render framebuffer (" << renderWidth << " x " << renderHeight << ")" << std::endl;
        }
        ammoniteInternalDebug << "Output resolution: " << *widthPtr << " x " << *heightPtr << std::endl;
      }

      //Get shadow resolution and light count, save for next time to avoid cubemap recreation
      static int* shadowResPtr = ammonite::settings::graphics::internal::getShadowResPtr();
      static int lastShadowRes = 0;
      unsigned int lightCount = lightTrackerMap->size();
      static unsigned int lastLightCount = -1;

      //If number of lights or shadow resolution changes, recreate cubemap
      if ((*shadowResPtr != lastShadowRes) or (lightCount != lastLightCount)) {
        setupDepthMap(lightCount, *shadowResPtr);

        //Save for next time to avoid cubemap recreation
        lastShadowRes = *shadowResPtr;
        lastLightCount = lightCount;
      }

      //If a loading screen is active, draw it and return
      int loadingScreenId = ammonite::interface::internal::getActiveLoadingScreenId();
      if (loadingScreenId != 0) {
        //Swap to loading screen shader
        glUseProgram(loadingShader.shaderId);

        //Pass drawing parameters
        ammonite::interface::LoadingScreen loadingScreen = (*loadingScreenTracker)[loadingScreenId];
        glUniform1f(loadingShader.widthId, loadingScreen.width);
        glUniform1f(loadingShader.heightId, loadingScreen.height);
        glUniform1f(loadingShader.heightOffsetId, loadingScreen.heightOffset);

        //Prepare viewport and framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, *widthPtr, *heightPtr);
        glDisable(GL_DEPTH_TEST);

        //Prepare to draw the screen
        glm::vec3 backgroundColour = loadingScreen.backgroundColour;
        glClearColor(backgroundColour.x, backgroundColour.y, backgroundColour.z, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glBindVertexArray(screenQuadVertexArrayId);

        //Draw the track
        glUniform1f(loadingShader.progressId, 1.0f);
        glUniform3fv(loadingShader.progressColourId, 1, glm::value_ptr(loadingScreen.trackColour));
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, nullptr);

        //Fill in the bar
        glUniform1f(loadingShader.progressId, loadingScreen.progress);
        glUniform3fv(loadingShader.progressColourId, 1, glm::value_ptr(loadingScreen.progressColour));
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, nullptr);

        //Prepare for next frame
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        finishFrame();
        return;
      }

      //Swap to depth shader and enable depth testing
      glUseProgram(depthShader.shaderId);
      glViewport(0, 0, *shadowResPtr, *shadowResPtr);
      glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
      glEnable(GL_DEPTH_TEST);

      //Pass uniforms that don't change between light sources
      static float* farPlanePtr = ammonite::settings::graphics::internal::getShadowFarPlanePtr();
      glUniform1f(depthShader.farPlaneId, *farPlanePtr);

      //Clear existing depth values
      glClear(GL_DEPTH_BUFFER_BIT);

      //Update cached model pointers, if the models have changed trackers
      static bool* modelsMovedPtr = ammonite::models::getModelsMovedPtr();
      if (*modelsMovedPtr) {
        drawModels(AMMONITE_DATA_REFRESH);
        *modelsMovedPtr = false;
      }

      //Depth mapping render passes
      auto lightIt = lightTrackerMap->begin();
      unsigned int activeLights = std::min(lightCount, maxLightCount);
      for (unsigned int shadowCount = 0; shadowCount < activeLights; shadowCount++) {
        //Get light source and position from tracker
        auto lightSource = &lightIt->second;
        glm::vec3 lightPos = lightSource->geometry;

        //Check framebuffer status
        if (glCheckNamedFramebufferStatus(depthMapFBO, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
          std::cerr << ammonite::utils::warning << "Incomplete depth framebuffer" << std::endl;
        }

        //Pass shadow transform matrices to depth shader
        for (int i = 0; i < 6; i++) {
          GLuint shadowMatrixId = glGetUniformLocation(depthShader.shaderId, std::string("shadowMatrices[" + std::to_string(i) + "]").c_str());
          //Fetch the transform from the tracker, and send to the shader
          glUniformMatrix4fv(shadowMatrixId, 1, GL_FALSE, glm::value_ptr((*lightTransformMap)[lightSource->lightId][i]));
        }

        //Pass light source specific uniforms
        glUniform3fv(depthShader.depthLightPosId, 1, glm::value_ptr(lightPos));
        glUniform1i(depthShader.depthShadowIndex, shadowCount);

        //Render to depth buffer and move to the next light source
        drawModels(AMMONITE_DEPTH_PASS);
        std::advance(lightIt, 1);
      }

      //Reset the framebuffer and viewport
      glBindFramebuffer(GL_FRAMEBUFFER, targetBufferId);
      glViewport(0, 0, renderWidth, renderHeight);

      //Clear depth and colour (if no skybox is used)
      int activeSkybox = ammonite::environment::skybox::getActiveSkybox();
      if (activeSkybox == 0) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      } else {
        glClear(GL_DEPTH_BUFFER_BIT);
      }

      //Prepare model shader and depth cube map
      glUseProgram(modelShader.shaderId);
      glBindTextureUnit(1, depthCubeMapId);

      //Calculate view projection matrix
      viewProjectionMatrix = *projectionMatrix * *viewMatrix;

      //Get ambient light and camera position
      glm::vec3 ambientLight = ammonite::lighting::getAmbientLight();
      glm::vec3 cameraPosition = ammonite::camera::getPosition(ammonite::camera::getActiveCamera());

      //Pass uniforms and render regular models
      glUniform3fv(modelShader.ambientLightId, 1, glm::value_ptr(ambientLight));
      glUniform3fv(modelShader.cameraPosId, 1, glm::value_ptr(cameraPosition));
      glUniform1f(modelShader.farPlaneId, *farPlanePtr);
      glUniform1i(modelShader.lightCountId, activeLights);
      drawModels(AMMONITE_RENDER_PASS);

      //Get information about light sources to be rendered
      int lightEmitterCount = ammonite::lighting::internal::getLightEmitterCount();
      int lightData[lightEmitterCount * 2];
      ammonite::lighting::internal::getLightEmitters(lightData);

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

      //Ensure wireframe is disabled
      setWireframe(false);

      //Draw the skybox
      if (activeSkybox != 0) {
        drawSkybox(activeSkybox);
      }

      //Use gamma correction if enabled
      static bool* gammaPtr = ammonite::settings::graphics::internal::getGammaCorrectionPtr();
      if (*gammaPtr) {
        glEnable(GL_FRAMEBUFFER_SRGB);
      }

      //Resolve multisampling into regular texture
      if (sampleCount != 0) {
        glBlitNamedFramebuffer(colourBufferMultisampleFBO, screenQuadFBO, 0, 0, renderWidth, renderHeight, 0, 0, renderWidth, renderHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);
      }

      //Swap to default framebuffer and correct shaders
      glUseProgram(screenShader.shaderId);
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
      glViewport(0, 0, *widthPtr, *heightPtr);
      glDisable(GL_DEPTH_TEST);

      //Display the rendered frame
      glBindVertexArray(screenQuadVertexArrayId);
      glBindTextureUnit(3, screenQuadTextureId);
      glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, nullptr);

      //Disable gamma correction for start of next pass
      glDisable(GL_FRAMEBUFFER_SRGB);

      //Display frame and handle any sleeping required
      finishFrame();
    }
  }
}
