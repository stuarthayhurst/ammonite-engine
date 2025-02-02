#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "internal/internalExtensions.hpp"
#include "internal/internalRenderHelper.hpp"
#include "internal/internalShaders.hpp"

#include "../camera.hpp"
#include "../enums.hpp"
#include "../interface.hpp"
#include "../lighting/lighting.hpp"
#include "../models/models.hpp"
#include "../skybox.hpp"
#include "../types.hpp"
#include "../utils/logging.hpp"
#include "../utils/debug.hpp"
#include "../window/window.hpp"

/*
 - Store and expose controls settings
*/

namespace ammonite {
  namespace renderer {
    namespace {
      struct PostSettings {
        bool focalDepthEnabled = false;
        float focalDepth = 0.0f;
        float blurStrength = 1.0f;
      } postSettings;

      struct GraphicsSettings {
        bool vsyncEnabled = true;
        float frameLimit = 0.0f;
        unsigned int shadowRes = 1024;
        float renderResMultiplier = 1.0f;
        unsigned int antialiasingSamples = 0;
        float renderFarPlane = 100.0f;
        float shadowFarPlane = 25.0f;
        bool gammaCorrection = false;
      } graphicsSettings;
    }

    namespace settings {
      namespace post {
        namespace internal {
          bool* getFocalDepthEnabledPtr() {
            return &postSettings.focalDepthEnabled;
          }

          float* getFocalDepthPtr() {
            return &postSettings.focalDepth;
          }

          float* getBlurStrengthPtr() {
            return &postSettings.blurStrength;
          }
        }

        void setFocalDepthEnabled(bool enabled) {
          postSettings.focalDepthEnabled = enabled;
        }

        bool getFocalDepthEnabled() {
          return postSettings.focalDepthEnabled;
        }

        void setFocalDepth(float depth) {
          postSettings.focalDepth = depth;
        }

        float getFocalDepth() {
          return postSettings.focalDepth;
        }

        void setBlurStrength(float strength) {
          postSettings.blurStrength = strength;
        }

        float getBlurStrength() {
          return postSettings.blurStrength;
        }
      }

      //Exposed internally only
      namespace internal {
        float* getFrameLimitPtr() {
          return &graphicsSettings.frameLimit;
        }

        unsigned int* getShadowResPtr() {
          return &graphicsSettings.shadowRes;
        }

        float* getRenderResMultiplierPtr() {
          return &graphicsSettings.renderResMultiplier;
        }

        unsigned int* getAntialiasingSamplesPtr() {
          return &graphicsSettings.antialiasingSamples;
        }

        float* getRenderFarPlanePtr() {
          return &graphicsSettings.renderFarPlane;
        }

        float* getShadowFarPlanePtr() {
          return &graphicsSettings.shadowFarPlane;
        }

        bool* getGammaCorrectionPtr() {
          return &graphicsSettings.gammaCorrection;
        }
      }

      void setVsync(bool enabled) {
        graphicsSettings.vsyncEnabled = enabled;
      }

      bool getVsync() {
        return graphicsSettings.vsyncEnabled;
      }

      void setFrameLimit(float frameLimit) {
        //Override with 0 if given a negative
        graphicsSettings.frameLimit = frameLimit > 0.0 ? frameLimit : 0;
      }

      float getFrameLimit() {
        return graphicsSettings.frameLimit;
      }

      void setShadowRes(unsigned int shadowRes) {
        graphicsSettings.shadowRes = shadowRes;
      }

      unsigned int getShadowRes() {
        return graphicsSettings.shadowRes;
      }

      void setRenderResMultiplier(float renderResMultiplier) {
        graphicsSettings.renderResMultiplier = renderResMultiplier;
      }

      float getRenderResMultiplier() {
        return graphicsSettings.renderResMultiplier;
      }

      void setAntialiasingSamples(unsigned int samples) {
        graphicsSettings.antialiasingSamples = samples;
      }

      unsigned int getAntialiasingSamples() {
        return graphicsSettings.antialiasingSamples;
      }

      void setRenderFarPlane(float renderFarPlane) {
        graphicsSettings.renderFarPlane = renderFarPlane;
      }

      float getRenderFarPlane() {
        return graphicsSettings.renderFarPlane;
      }

      void setShadowFarPlane(float shadowFarPlane) {
        graphicsSettings.shadowFarPlane = shadowFarPlane;
      }

      float getShadowFarPlane() {
        return graphicsSettings.shadowFarPlane;
      }

      void setGammaCorrection(bool gammaCorrection) {
        graphicsSettings.gammaCorrection = gammaCorrection;
      }

      bool getGammaCorrection() {
        return graphicsSettings.gammaCorrection;
      }
    }
  }
}

/*
 - Implement core rendering for 3D graphics
*/

namespace ammonite {
  namespace renderer {
    namespace {
      //Structures to store uniform IDs for the shaders
      struct {
        GLuint shaderId;
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
      } modelShader;

      struct {
        GLuint shaderId;
        GLint lightMatrixId;
        GLint lightIndexId;
      } lightShader;

      struct {
        GLuint shaderId;
        GLint modelMatrixId;
        GLint shadowFarPlaneId;
        GLint depthLightPosId;
        GLint depthShadowIndex;
      } depthShader;

      struct {
        GLuint shaderId;
        GLint viewMatrixId;
        GLint projectionMatrixId;
        GLint skyboxSamplerId;
      } skyboxShader;

      struct {
        GLuint shaderId;
        GLint screenSamplerId;
        GLint depthSamplerId;
        GLint focalDepthId;
        GLint focalDepthEnabledId;
        GLint blurStrengthId;
        GLint farPlaneId;
      } screenShader;

      struct {
        GLuint shaderId;
        GLint progressId;
        GLint widthId;
        GLint heightId;
        GLint heightOffsetId;
        GLint progressColourId;
      } loadingShader;

      struct {
        GLuint skybox;
        GLuint skyboxElement;
        GLuint screenQuad;
        GLuint screenQuadElement;
      } bufferIds;

      GLuint skyboxVertexArrayId;
      GLuint screenQuadVertexArrayId;

      GLuint depthCubeMapId = 0;
      GLuint depthMapFBO;

      GLuint screenQuadTextureId = 0;
      GLuint screenQuadDepthTextureId = 0;
      GLuint screenQuadFBO;
      GLuint depthRenderBufferId;
      GLuint colourRenderBufferId = 0;
      GLuint colourBufferMultisampleFBO;

      glm::mat4* viewMatrix = ammonite::camera::internal::getViewMatrixPtr();
      glm::mat4* projectionMatrix = ammonite::camera::internal::getProjectionMatrixPtr();

      //Get the light trackers
      std::map<AmmoniteId, ammonite::lighting::internal::LightSource>* lightTrackerMap =
        ammonite::lighting::internal::getLightTrackerPtr();
      glm::mat4** lightTransformsPtr = ammonite::lighting::internal::getLightTransformsPtr();
      unsigned int maxLightCount = 0;

      //Store model data pointers for regular models and light models
      ammonite::models::internal::ModelInfo** modelPtrs = nullptr;
      ammonite::models::internal::ModelInfo** lightModelPtrs = nullptr;

      GLint maxSampleCount = 0;

      //View projection combined matrix
      glm::mat4 viewProjectionMatrix;

      //Render modes for drawModels()
      enum AmmoniteRenderMode {
        AMMONITE_RENDER_PASS,
        AMMONITE_DEPTH_PASS,
        AMMONITE_EMISSION_PASS,
        AMMONITE_DATA_REFRESH
      };
    }

    namespace setup {
      namespace internal {
        //Load required shaders from a path
        bool createShaders(std::string shaderPath) {
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
          unsigned int shaderCount = sizeof(shaderInfo) / sizeof(shaderInfo[0]);

          //Load shaders
          bool hasCreatedShaders = true;
          for (unsigned int i = 0; i < shaderCount; i++) {
            std::string shaderLocation = shaderPath + shaderInfo[i].shaderDir;
            *shaderInfo[i].shaderId = ammonite::shaders::internal::loadDirectory(shaderLocation);
            if (*shaderInfo[i].shaderId == 0) {
              hasCreatedShaders = false;
            }
          }

          return hasCreatedShaders;
        }

        void deleteShaders() {
          glDeleteProgram(modelShader.shaderId);
          glDeleteProgram(lightShader.shaderId);
          glDeleteProgram(depthShader.shaderId);
          glDeleteProgram(skyboxShader.shaderId);
          glDeleteProgram(screenShader.shaderId);
          glDeleteProgram(loadingShader.shaderId);
        }

        //Check for essential GPU capabilities
        bool checkGPUCapabilities(unsigned int* failureCount) {
          const char* extensions[5][3] = {
            {"GL_ARB_direct_state_access", "GL_VERSION_4_5", "Direct state access"},
            {"GL_ARB_shader_storage_buffer_object", "GL_VERSION_4_3", "Shader Storage Buffer Objects (SSBOs)"},
            {"GL_ARB_texture_storage", "GL_VERSION_4_2", "Texture storage"},
            {"GL_ARB_shading_language_420pack", "GL_VERSION_4_2", "GLSL shader version 4.20"},
            {"GL_ARB_texture_cube_map_array", "GL_VERSION_4_0", "Cubemap arrays"}
          };
          unsigned int extensionCount = sizeof(extensions) / sizeof(extensions[0]);

          bool success = true;
          for (unsigned int i = 0; i < extensionCount; i++) {
            if (!graphics::internal::checkExtension(extensions[i][0], extensions[i][1])) {
              ammonite::utils::error << extensions[i][2] << " unsupported" << std::endl;
              success = false;
              (*failureCount)++;
            }
          }

          //Check minimum OpenGL version is supported
          if (!glewIsSupported("GL_VERSION_3_2")) {
            ammonite::utils::error << "OpenGL 3.2 unsupported" << std::endl;
            success = false;
            (*failureCount)++;
          }

          //Check for shader caching support
          ammonite::shaders::internal::updateCacheSupport();

          return success;
        }

        //Prepare required objects for rendering
        void setupOpenGLObjects() {
          //Shader uniform locations
          modelShader.matrixId = glGetUniformLocation(modelShader.shaderId, "MVP");
          modelShader.modelMatrixId = glGetUniformLocation(modelShader.shaderId, "modelMatrix");
          modelShader.normalMatrixId = glGetUniformLocation(modelShader.shaderId, "normalMatrix");
          modelShader.ambientLightId = glGetUniformLocation(modelShader.shaderId, "ambientLight");
          modelShader.cameraPosId = glGetUniformLocation(modelShader.shaderId, "cameraPos");
          modelShader.shadowFarPlaneId = glGetUniformLocation(modelShader.shaderId, "shadowFarPlane");
          modelShader.lightCountId = glGetUniformLocation(modelShader.shaderId, "lightCount");
          modelShader.diffuseSamplerId = glGetUniformLocation(modelShader.shaderId, "diffuseSampler");
          modelShader.specularSamplerId = glGetUniformLocation(modelShader.shaderId, "specularSampler");
          modelShader.shadowCubeMapId = glGetUniformLocation(modelShader.shaderId, "shadowCubeMap");

          lightShader.lightMatrixId = glGetUniformLocation(lightShader.shaderId, "MVP");
          lightShader.lightIndexId = glGetUniformLocation(lightShader.shaderId, "lightIndex");

          depthShader.modelMatrixId = glGetUniformLocation(depthShader.shaderId, "modelMatrix");
          depthShader.shadowFarPlaneId = glGetUniformLocation(depthShader.shaderId, "shadowFarPlane");
          depthShader.depthLightPosId = glGetUniformLocation(depthShader.shaderId, "lightPos");
          depthShader.depthShadowIndex = glGetUniformLocation(depthShader.shaderId, "shadowMapIndex");

          skyboxShader.viewMatrixId = glGetUniformLocation(skyboxShader.shaderId, "viewMatrix");
          skyboxShader.projectionMatrixId = glGetUniformLocation(skyboxShader.shaderId, "projectionMatrix");
          skyboxShader.skyboxSamplerId = glGetUniformLocation(skyboxShader.shaderId, "skyboxSampler");

          screenShader.screenSamplerId = glGetUniformLocation(screenShader.shaderId, "screenSampler");
          screenShader.depthSamplerId = glGetUniformLocation(screenShader.shaderId, "depthSampler");
          screenShader.focalDepthId = glGetUniformLocation(screenShader.shaderId, "focalDepth");
          screenShader.focalDepthEnabledId = glGetUniformLocation(screenShader.shaderId, "focalDepthEnabled");
          screenShader.blurStrengthId = glGetUniformLocation(screenShader.shaderId, "blurStrength");
          screenShader.farPlaneId = glGetUniformLocation(screenShader.shaderId, "farPlane");

          loadingShader.progressId = glGetUniformLocation(loadingShader.shaderId, "progress");
          loadingShader.widthId = glGetUniformLocation(loadingShader.shaderId, "width");
          loadingShader.heightId = glGetUniformLocation(loadingShader.shaderId, "height");
          loadingShader.heightOffsetId = glGetUniformLocation(loadingShader.shaderId, "heightOffset");
          loadingShader.progressColourId = glGetUniformLocation(loadingShader.shaderId, "progressColour");

          //Pass texture unit locations
          glUseProgram(modelShader.shaderId);
          glUniform1i(modelShader.diffuseSamplerId, 0);
          glUniform1i(modelShader.specularSamplerId, 1);
          glUniform1i(modelShader.shadowCubeMapId, 2);

          glUseProgram(skyboxShader.shaderId);
          glUniform1i(skyboxShader.skyboxSamplerId, 3);

          glUseProgram(screenShader.shaderId);
          glUniform1i(screenShader.screenSamplerId, 4);
          glUniform1i(screenShader.depthSamplerId, 5);

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
          glCullFace(GL_BACK);
          glDepthFunc(GL_LEQUAL);

          //Find multisampling limits
          glGetIntegerv(GL_MAX_SAMPLES, &maxSampleCount);

          //Get the max number of lights supported
          maxLightCount = ammonite::lighting::getMaxLightCount();

          const signed char skyboxVertices[] = {
            -1,  1, -1,
            -1, -1, -1,
             1, -1, -1,
             1,  1, -1,
            -1, -1,  1,
            -1,  1,  1,
             1, -1,  1,
             1,  1,  1
          };

          const signed char skyboxIndices[36] = {
            0, 1, 2, 2, 3, 0,
            4, 1, 0, 0, 5, 4,
            2, 6, 7, 7, 3, 2,
            4, 5, 7, 7, 6, 4,
            0, 3, 7, 7, 5, 0,
            1, 4, 2, 2, 4, 6
          };

          //Position and texture coord of screen quad
          const signed char screenVertices[16] = {
            -1,  1,  0,  1,
            -1, -1,  0,  0,
             1, -1,  1,  0,
             1,  1,  1,  1
          };

          const signed char screenIndices[6] = {
            0, 1, 2,
            0, 2, 3
          };

          //Create vertex and element buffers for the skybox and screen quad
          glCreateBuffers(4, &bufferIds.skybox);

          //Fill vertex and element buffers for the skybox
          glNamedBufferData(bufferIds.skybox, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
          glNamedBufferData(bufferIds.skyboxElement, sizeof(skyboxIndices), &skyboxIndices, GL_STATIC_DRAW);

          //Create vertex array object for skybox
          glCreateVertexArrays(1, &skyboxVertexArrayId);
          glEnableVertexArrayAttrib(skyboxVertexArrayId, 0);
          glVertexArrayVertexBuffer(skyboxVertexArrayId, 0, bufferIds.skybox, 0, 3 * sizeof(signed char));
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
          glVertexArrayVertexBuffer(screenQuadVertexArrayId, 0, bufferIds.screenQuad, 0, 4 * sizeof(signed char));
          glVertexArrayAttribFormat(screenQuadVertexArrayId, 0, 2, GL_BYTE, GL_FALSE, 0);
          glVertexArrayAttribBinding(screenQuadVertexArrayId, 0, 0);

          //Texture coords
          glEnableVertexArrayAttrib(screenQuadVertexArrayId, 1);
          glVertexArrayVertexBuffer(screenQuadVertexArrayId, 1, bufferIds.screenQuad, 2 * sizeof(signed char), 4 * sizeof(signed char));
          glVertexArrayAttribFormat(screenQuadVertexArrayId, 1, 2, GL_BYTE, GL_FALSE, 0);
          glVertexArrayAttribBinding(screenQuadVertexArrayId, 1, 1);

          glVertexArrayElementBuffer(screenQuadVertexArrayId, bufferIds.screenQuadElement);
        }

        void destroyOpenGLObjects() {
          glDeleteFramebuffers(1, &depthMapFBO);
          glDeleteFramebuffers(1, &colourBufferMultisampleFBO);
          glDeleteFramebuffers(1, &screenQuadFBO);
          glDeleteRenderbuffers(1, &depthRenderBufferId);

          glDeleteBuffers(4, &bufferIds.skybox);
          glDeleteVertexArrays(1, &skyboxVertexArrayId);
          glDeleteVertexArrays(1, &screenQuadVertexArrayId);

          if (screenQuadTextureId != 0) {
            glDeleteTextures(1, &screenQuadTextureId);
            glDeleteTextures(1, &screenQuadDepthTextureId);
          }

          if (colourRenderBufferId != 0) {
            glDeleteRenderbuffers(1, &colourRenderBufferId);
          }

          if (depthCubeMapId != 0) {
            glDeleteTextures(1, &depthCubeMapId);
          }
        }

        void deleteModelCache() {
          if (modelPtrs != nullptr) {
            delete [] modelPtrs;
          }

          if (lightModelPtrs != nullptr) {
            delete [] lightModelPtrs;
          }
        }
      }
    }

    namespace {
      /*
       - Functions to setup and validate render objects
      */

      //Setup framebuffers for rendering
      static void recreateFramebuffers(GLuint* targetBufferIdPtr, unsigned int sampleCount,
                                       unsigned int renderWidth, unsigned int renderHeight) {
        //Delete regular colour and depth storage textures
        if (screenQuadTextureId != 0) {
          glDeleteTextures(1, &screenQuadTextureId);
          glDeleteTextures(1, &screenQuadDepthTextureId);
          screenQuadTextureId = 0;
          screenQuadDepthTextureId = 0;
        }

        //Delete multisampled colour storage if it exists
        if (colourRenderBufferId != 0) {
          glDeleteRenderbuffers(1, &colourRenderBufferId);
          colourRenderBufferId = 0;
        }

        //Create texture for whole screen
        glCreateTextures(GL_TEXTURE_2D, 1, &screenQuadTextureId);
        glCreateTextures(GL_TEXTURE_2D, 1, &screenQuadDepthTextureId);

        //Decide which framebuffer to render to and create multisampled renderbuffer, if needed
        if (sampleCount != 0) {
          *targetBufferIdPtr = colourBufferMultisampleFBO;
          glCreateRenderbuffers(1, &colourRenderBufferId);
        } else {
          *targetBufferIdPtr = screenQuadFBO;
        }

        if (sampleCount != 0) {
          //Create multisampled renderbuffers for colour and depth
          glNamedRenderbufferStorageMultisample(colourRenderBufferId, (GLsizei)sampleCount,
            GL_SRGB8, (GLsizei)renderWidth, (GLsizei)renderHeight);
          glNamedRenderbufferStorageMultisample(depthRenderBufferId, (GLsizei)sampleCount,
            GL_DEPTH_COMPONENT32, (GLsizei)renderWidth, (GLsizei)renderHeight);

          //Attach colour and depth renderbuffers to multisampled framebuffer
          glNamedFramebufferRenderbuffer(colourBufferMultisampleFBO, GL_COLOR_ATTACHMENT0,
                                         GL_RENDERBUFFER, colourRenderBufferId);
          glNamedFramebufferRenderbuffer(colourBufferMultisampleFBO, GL_DEPTH_ATTACHMENT,
                                         GL_RENDERBUFFER, depthRenderBufferId);
        }

        //Create texture to store colour data and bind to framebuffer
        glTextureStorage2D(screenQuadTextureId, 1, GL_SRGB8, (GLsizei)renderWidth, (GLsizei)renderHeight);
        glTextureParameteri(screenQuadTextureId, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(screenQuadTextureId, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(screenQuadTextureId, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(screenQuadTextureId, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glNamedFramebufferTexture(screenQuadFBO, GL_COLOR_ATTACHMENT0, screenQuadTextureId, 0);

        glTextureStorage2D(screenQuadDepthTextureId, 1, GL_DEPTH_COMPONENT32,
                           (GLsizei)renderWidth, (GLsizei)renderHeight);
        glTextureParameteri(screenQuadDepthTextureId, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(screenQuadDepthTextureId, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(screenQuadDepthTextureId, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(screenQuadDepthTextureId, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glNamedFramebufferTexture(screenQuadFBO, GL_DEPTH_ATTACHMENT, screenQuadDepthTextureId, 0);
      }

      static void checkFramebuffers(unsigned int renderWidth, unsigned int renderHeight,
                                    unsigned int sampleCount) {
        //Check multisampled framebuffer
        if (sampleCount != 0) {
          if (glCheckNamedFramebufferStatus(colourBufferMultisampleFBO, GL_FRAMEBUFFER) !=
              GL_FRAMEBUFFER_COMPLETE) {
            ammonite::utils::warning << "Incomplete multisampled render framebuffer" << std::endl;
          } else {
            ammoniteInternalDebug << "Created new multisampled render framebuffer (" << renderWidth \
                                  << " x " << renderHeight << "), samples: x" << sampleCount << std::endl;
          }
        }

        //Check regular framebuffer
        if (glCheckNamedFramebufferStatus(screenQuadFBO, GL_FRAMEBUFFER) !=
            GL_FRAMEBUFFER_COMPLETE) {
          ammonite::utils::warning << "Incomplete render framebuffer" << std::endl;
        } else {
          ammoniteInternalDebug << "Created new render framebuffer (" << renderWidth << " x " \
                                << renderHeight << ")" << std::endl;
        }
      }

      //Create, configure and bind depth cubemap for shadows
      static void setupDepthMap(unsigned int lightCount, unsigned int shadowRes) {
        //Delete the cubemap array if it already exists
        if (depthCubeMapId != 0) {
          glDeleteTextures(1, &depthCubeMapId);
        }

        //Create a cubemap for shadows
        glCreateTextures(GL_TEXTURE_CUBE_MAP_ARRAY, 1, &depthCubeMapId);

        //Workaround for no lights causing a depth of 0
        if (lightCount == 0) {
          lightCount = 1;
        }

        //Create 6 faces for each light source
        GLsizei depthLayers = (GLsizei)std::min(maxLightCount, lightCount) * 6;
        glTextureStorage3D(depthCubeMapId, 1, GL_DEPTH_COMPONENT32, (GLsizei)shadowRes,
                           (GLsizei)shadowRes, depthLayers);

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

      /*
       - Helper functions to draw / wrap components
      */
      static void drawModel(ammonite::models::internal::ModelInfo *drawObject,
                            AmmoniteRenderMode renderMode) {
        //Get model draw data
        ammonite::models::internal::ModelData* drawObjectData = drawObject->modelData;

        //Set the requested draw mode (normal, wireframe, points)
        GLenum mode = GL_TRIANGLES;
        if (drawObject->drawMode == AMMONITE_DRAW_WIREFRAME) {
          //Use wireframe if requested
          internal::setWireframe(true);
        } else {
          //Draw points if requested
          if (drawObject->drawMode == AMMONITE_DRAW_POINTS) {
            mode = GL_POINTS;
          }
          internal::setWireframe(false);
        }

        //Fetch the model matrix
        glm::mat4 modelMatrix = drawObject->positionData.modelMatrix;

        //Handle pass-specific matrices and uniforms
        glm::mat4 mvp;
        switch (renderMode) {
        case AMMONITE_DEPTH_PASS:
          glUniformMatrix4fv(depthShader.modelMatrixId, 1, GL_FALSE, glm::value_ptr(modelMatrix));
          break;
        case AMMONITE_RENDER_PASS:
          mvp = viewProjectionMatrix * modelMatrix;
          glUniformMatrix4fv(modelShader.matrixId, 1, GL_FALSE, glm::value_ptr(mvp));
          glUniformMatrix4fv(modelShader.modelMatrixId, 1, GL_FALSE, glm::value_ptr(modelMatrix));
          glUniformMatrix3fv(modelShader.normalMatrixId, 1, GL_FALSE,
                             glm::value_ptr(drawObject->positionData.normalMatrix));
          break;
        case AMMONITE_EMISSION_PASS:
          mvp = viewProjectionMatrix * modelMatrix;
          glUniformMatrix4fv(lightShader.lightMatrixId, 1, GL_FALSE, glm::value_ptr(mvp));
          glUniform1ui(lightShader.lightIndexId, drawObject->lightIndex);
          break;
        case AMMONITE_DATA_REFRESH:
          //How did we get here?
          ammonite::utils::error << "drawModel() called with AMMONITE_DATA_REFRESH" << std::endl;
          std::unreachable();
          break;
        }

        for (unsigned int i = 0; i < drawObjectData->meshes.size(); i++) {
          //Set texture for regular shading pass
          if (renderMode == AMMONITE_RENDER_PASS) {
            if (drawObject->textureIds[i].diffuseId != 0) {
              glBindTextureUnit(0, drawObject->textureIds[i].diffuseId);
            } else {
              ammoniteInternalDebug << "No diffuse texture supplied, skipping" << std::endl;
            }

            if (drawObject->textureIds[i].specularId != 0) {
              glBindTextureUnit(1, drawObject->textureIds[i].specularId);
            }
          }

          //Bind vertex attribute buffer
          glBindVertexArray(drawObjectData->meshes[i].vertexArrayId);

          //Draw the triangles
          glDrawElements(mode, (GLsizei)drawObjectData->meshes[i].indexCount,
                         GL_UNSIGNED_INT, nullptr);
        }
      }
    }

    /*
     - Draw models of a given type, from a cache
     - Update the cache when given AMMONITE_DATA_REFRESH or a pointer to a null pointer
    */
    static void drawModelsCached(ammonite::models::internal::ModelInfo*** modelPtrsPtr,
                                AmmoniteEnum modelType, AmmoniteRenderMode renderMode) {
      unsigned int modelCount = ammonite::models::internal::getModelCount(modelType);

      //Create / update cache for model pointers
      if (renderMode == AMMONITE_DATA_REFRESH || *modelPtrsPtr == nullptr) {
        //Free old model cache
        if (*modelPtrsPtr != nullptr) {
          delete [] *modelPtrsPtr;
        }

        //Create and fill / update model pointers cache
        *modelPtrsPtr = new ammonite::models::internal::ModelInfo*[modelCount];
        ammonite::models::internal::getModels(modelType, modelCount, *modelPtrsPtr);

        //Return if only refreshing
        if (renderMode == AMMONITE_DATA_REFRESH) {
          return;
        }
      }

      //Draw the model pointers
      for (unsigned int i = 0; i < modelCount; i++) {
        drawModel((*modelPtrsPtr)[i], renderMode);
      }

      return;
    }

    static void drawSkybox(AmmoniteId activeSkyboxId) {
      //Swap to skybox shader and pass uniforms
      glUseProgram(skyboxShader.shaderId);
      glUniformMatrix4fv(skyboxShader.viewMatrixId, 1, GL_FALSE,
                         glm::value_ptr(glm::mat4(glm::mat3(*viewMatrix))));
      glUniformMatrix4fv(skyboxShader.projectionMatrixId, 1, GL_FALSE,
                         glm::value_ptr(*projectionMatrix));

      //Prepare and draw the skybox
      glBindVertexArray(skyboxVertexArrayId);
      glBindTextureUnit(3, activeSkyboxId);
      glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_BYTE, nullptr);
    }

    static void drawLoadingScreen(AmmoniteId loadingScreenId, unsigned int width,
                                  unsigned int height) {
        //Swap to loading screen shader
        glUseProgram(loadingShader.shaderId);

        //Pass drawing parameters
        interface::internal::LoadingScreen* loadingScreen =
          interface::internal::getLoadingScreenPtr(loadingScreenId);
        glUniform1f(loadingShader.widthId, loadingScreen->width);
        glUniform1f(loadingShader.heightId, loadingScreen->height);
        glUniform1f(loadingShader.heightOffsetId, loadingScreen->heightOffset);

        //Prepare viewport and framebuffer
        internal::prepareScreen(0, width, height, false);

        //Prepare to draw the screen
        glm::vec3 backgroundColour = loadingScreen->backgroundColour;
        glClearColor(backgroundColour.x, backgroundColour.y, backgroundColour.z, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glBindVertexArray(screenQuadVertexArrayId);

        //Draw the track
        glUniform1f(loadingShader.progressId, 1.0f);
        glUniform3fv(loadingShader.progressColourId, 1, glm::value_ptr(loadingScreen->trackColour));
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, nullptr);

        //Fill in the bar
        glUniform1f(loadingShader.progressId, loadingScreen->progress);
        glUniform3fv(loadingShader.progressColourId, 1, glm::value_ptr(loadingScreen->progressColour));
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, nullptr);
      }

    namespace internal {
      void internalDrawLoadingScreen(AmmoniteId loadingScreenId) {
        unsigned int width = ammonite::window::internal::getGraphicsWidth();
        unsigned int height = ammonite::window::internal::getGraphicsHeight();

        drawLoadingScreen(loadingScreenId, width, height);

        //Prepare for next frame
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        ammonite::window::internal::showFrame(window::internal::getWindowPtr(),
          settings::getVsync(), settings::getFrameLimit());
      }

      void internalDrawFrame() {
        static unsigned int lastWidth = 0, lastHeight = 0;
        unsigned int width = ammonite::window::internal::getGraphicsWidth();
        unsigned int height = ammonite::window::internal::getGraphicsHeight();

        static float lastRenderResMultiplier = 0.0f;
        static float* renderResMultiplierPtr = settings::internal::getRenderResMultiplierPtr();

        static unsigned int lastSamples = 0;
        static unsigned int* samplesPtr = settings::internal::getAntialiasingSamplesPtr();
        static unsigned int sampleCount = *samplesPtr;

        static unsigned int renderWidth = 0, renderHeight = 0;

        //Recreate the framebuffer if the width, height, resolution multiplier or sample count changes
        static GLuint targetBufferId = 0;
        if ((lastWidth != width) or (lastHeight != height) or
            (lastRenderResMultiplier != *renderResMultiplierPtr) or
            (lastSamples != *samplesPtr)) {
          //Update values used to determine when to recreate framebuffer
          lastWidth = width;
          lastHeight = height;
          lastRenderResMultiplier = *renderResMultiplierPtr;
          lastSamples = *samplesPtr;

          //Limit sample count to implementation limit
          unsigned int requestedSamples = *samplesPtr;
          sampleCount = std::min(requestedSamples, (unsigned int)maxSampleCount);

          if (sampleCount < requestedSamples) {
            ammonite::utils::warning << "Ignoring request for " << requestedSamples \
                                     << " samples, using implementation limit of " \
                                     << maxSampleCount << std::endl;
            *samplesPtr = sampleCount;
          }

          //Calculate render resolution
          renderWidth = (unsigned int)((float)width * *renderResMultiplierPtr);
          renderHeight = (unsigned int)((float)height * *renderResMultiplierPtr);

          //Create or recreate the framebuffers for rendering
          recreateFramebuffers(&targetBufferId, sampleCount, renderWidth, renderHeight);
          checkFramebuffers(renderWidth, renderHeight, sampleCount);

          ammoniteInternalDebug << "Output resolution: " << width << " x " << height << std::endl;
        }

        //Get shadow resolution and light count, save for next time to avoid cubemap recreation
        static unsigned int* shadowResPtr = settings::internal::getShadowResPtr();
        static unsigned int lastShadowRes = 0;
        unsigned int lightCount = lightTrackerMap->size();
        static unsigned int lastLightCount = -1;

        //If number of lights or shadow resolution changes, recreate cubemap
        if ((*shadowResPtr != lastShadowRes) or (lightCount != lastLightCount)) {
          setupDepthMap(lightCount, *shadowResPtr);

          //Save for next time to avoid cubemap recreation
          lastShadowRes = *shadowResPtr;
          lastLightCount = lightCount;
        }

        //Swap to depth shader and enable depth testing
        glUseProgram(depthShader.shaderId);
        internal::prepareScreen(depthMapFBO, *shadowResPtr, *shadowResPtr, true);

        //Pass uniforms that don't change between light sources
        static float* shadowFarPlanePtr = settings::internal::getShadowFarPlanePtr();
        glUniform1f(depthShader.shadowFarPlaneId, *shadowFarPlanePtr);

        //Clear existing depth values
        glClear(GL_DEPTH_BUFFER_BIT);

        //Update cached model pointers, if the models have changed trackers
        static bool* modelsMovedPtr = ammonite::models::internal::getModelsMovedPtr();
        if (*modelsMovedPtr) {
          drawModelsCached(&modelPtrs, AMMONITE_MODEL, AMMONITE_DATA_REFRESH);
          drawModelsCached(&lightModelPtrs, AMMONITE_LIGHT_EMITTER, AMMONITE_DATA_REFRESH);
          *modelsMovedPtr = false;
        }

        //Use gamma correction if enabled
        static bool* gammaPtr = settings::internal::getGammaCorrectionPtr();
        if (*gammaPtr) {
          glEnable(GL_FRAMEBUFFER_SRGB);
        } else {
          glDisable(GL_FRAMEBUFFER_SRGB);
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
            ammonite::utils::warning << "Incomplete depth framebuffer" << std::endl;
          }

          //Pass shadow transform matrices to depth shader
          glm::mat4* lightTransformStart = *lightTransformsPtr + (std::size_t)(lightSource->lightIndex) * 6;
          for (int i = 0; i < 6; i++) {
            std::string identifier = std::string("shadowMatrices[") + std::to_string(i) + std::string("]");
            GLint shadowMatrixId = glGetUniformLocation(depthShader.shaderId, identifier.c_str());
            //Fetch the transform from the tracker, and send to the shader
            glUniformMatrix4fv(shadowMatrixId, 1, GL_FALSE,
                               glm::value_ptr(lightTransformStart[i]));
          }

          //Pass light source specific uniforms
          glUniform3fv(depthShader.depthLightPosId, 1, glm::value_ptr(lightPos));
          glUniform1ui(depthShader.depthShadowIndex, shadowCount);

          //Render to depth buffer and move to the next light source
          drawModelsCached(&modelPtrs, AMMONITE_MODEL, AMMONITE_DEPTH_PASS);
          std::advance(lightIt, 1);
        }

        //Reset the framebuffer and viewport
        internal::prepareScreen(targetBufferId, renderWidth, renderHeight, true);

        //Clear depth and colour (if no skybox is used)
        AmmoniteId activeSkybox = ammonite::skybox::getActiveSkybox();
        if (activeSkybox == 0) {
          glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        } else {
          glClear(GL_DEPTH_BUFFER_BIT);
        }

        //Prepare model shader and depth cube map
        glUseProgram(modelShader.shaderId);
        glBindTextureUnit(2, depthCubeMapId);

        //Calculate view projection matrix
        viewProjectionMatrix = *projectionMatrix * *viewMatrix;

        //Get ambient light and camera position
        glm::vec3 ambientLight = ammonite::lighting::getAmbientLight();
        glm::vec3 cameraPosition = ammonite::camera::getPosition(ammonite::camera::getActiveCamera());

        //Pass uniforms and render regular models
        glUniform3fv(modelShader.ambientLightId, 1, glm::value_ptr(ambientLight));
        glUniform3fv(modelShader.cameraPosId, 1, glm::value_ptr(cameraPosition));
        glUniform1f(modelShader.shadowFarPlaneId, *shadowFarPlanePtr);
        glUniform1ui(modelShader.lightCountId, activeLights);
        drawModelsCached(&modelPtrs, AMMONITE_MODEL, AMMONITE_RENDER_PASS);

        //Render light emitting models
        unsigned int lightModelCount = ammonite::models::internal::getModelCount(AMMONITE_LIGHT_EMITTER);
        if (lightModelCount > 0) {
          //Swap to the light emitter shader and render cached light model pointers
          glUseProgram(lightShader.shaderId);
          drawModelsCached(&lightModelPtrs, AMMONITE_LIGHT_EMITTER, AMMONITE_EMISSION_PASS);
        }

        //Ensure wireframe is disabled
        internal::setWireframe(false);

        //Draw the skybox
        if (activeSkybox != 0) {
          drawSkybox(activeSkybox);
        }

        //Get focal depth status, used to conditionally post-process
        static bool* focalDepthEnabledPtr = settings::post::internal::getFocalDepthEnabledPtr();

        //Enable post-processor when required, or blit would fail
        bool isPostRequired = *focalDepthEnabledPtr;
        if (sampleCount == 0 || *renderResMultiplierPtr != 1.0f) {
          /*
           - Workaround until non-multisampled rendering is done to an offscreen framebuffer
             - sampleCount == 0
           - Workaround INVALID_OPERATION when scaling a multisampled buffer with a blit
             - sampleCount != 0 && *renderResMultiplierPtr != 1.0f
          */
          isPostRequired = true;
        }

        /*
          If post-processing is required, blit offscreen framebuffer to texture
          Use a post-processing fragment shader with this texture to blur and scale

          If post-processing isn't required or can be avoided, render directly to screen
        */
        internal::prepareScreen(0, width, height, false);
        if (isPostRequired) {
          //Resolve multisampling into regular texture
          if (sampleCount != 0) {
            GLbitfield blitBits = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;
            glBlitNamedFramebuffer(colourBufferMultisampleFBO, screenQuadFBO, 0, 0,
                                   (GLint)renderWidth, (GLint)renderHeight, 0, 0,
                                   (GLint)renderWidth, (GLint)renderHeight,
                                   blitBits, GL_NEAREST);
          }

          //Swap to correct shaders
          glUseProgram(screenShader.shaderId);

          //Conditionally send data for blur
          glUniform1i(screenShader.focalDepthEnabledId, *focalDepthEnabledPtr);
          if (*focalDepthEnabledPtr) {
            static float* focalDepthPtr = settings::post::internal::getFocalDepthPtr();
            static float* blurStrengthPtr = settings::post::internal::getBlurStrengthPtr();
            static float* farPlanePtr = settings::internal::getRenderFarPlanePtr();

            glUniform1f(screenShader.focalDepthId, *focalDepthPtr);
            glUniform1f(screenShader.blurStrengthId, *blurStrengthPtr);
            glUniform1f(screenShader.farPlaneId, *farPlanePtr);
            glBindTextureUnit(5, screenQuadDepthTextureId);
          }

          //Display the rendered frame
          glBindVertexArray(screenQuadVertexArrayId);
          glBindTextureUnit(4, screenQuadTextureId);
          glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, nullptr);
        } else {
          //Resolve multisampling into default framebuffer
          if (sampleCount != 0) {
            GLbitfield blitBits = GL_COLOR_BUFFER_BIT;
            glBlitNamedFramebuffer(colourBufferMultisampleFBO, 0, 0, 0,
                                   (GLint)renderWidth, (GLint)renderHeight, 0, 0,
                                   (GLint)width, (GLint)height, blitBits, GL_NEAREST);
          }
        }

        //Display frame and handle any sleeping required
        ammonite::window::internal::showFrame(window::internal::getWindowPtr(),
          settings::getVsync(), settings::getFrameLimit());
      }
    }
  }
}
