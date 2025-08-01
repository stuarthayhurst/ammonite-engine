#include <algorithm>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

extern "C" {
  #include <epoxy/gl.h>
}

#include "renderer.hpp"

#include "extensions.hpp"
#include "shaderLoader.hpp"
#include "shaders.hpp"
#include "../camera.hpp"
#include "../lighting/lighting.hpp"
#include "../maths/matrix.hpp"
#include "../maths/vector.hpp"
#include "../models/models.hpp"
#include "../skybox.hpp"
#include "../splash.hpp"
#include "../utils/debug.hpp"
#include "../utils/logging.hpp"
#include "../utils/id.hpp"
#include "../window/window.hpp"

/*
 - Implement core rendering for 3D graphics
*/

namespace ammonite {
  namespace renderer {
    namespace {
      //Structures to store uniform IDs for the shaders
      internal::ModelShader modelShader;
      internal::LightShader lightShader;
      internal::DepthShader depthShader;
      internal::SkyboxShader skyboxShader;
      internal::ScreenShader screenShader;
      internal::SplashShader splashShader;

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

      ammonite::Mat<float, 4>* viewMatrixPtr = ammonite::camera::internal::getViewMatrixPtr();
      ammonite::Mat<float, 4>* projectionMatrixPtr = ammonite::camera::internal::getProjectionMatrixPtr();

      //Store model data pointers for regular models and light models
      ammonite::models::internal::ModelInfo** modelPtrs = nullptr;
      ammonite::models::internal::ModelInfo** lightModelPtrs = nullptr;

      unsigned int maxLightCount = 0;
      GLint maxSampleCount = 0;

      //Render modes for drawModels()
      enum AmmoniteRenderMode : unsigned char {
        AMMONITE_RENDER_PASS,
        AMMONITE_DEPTH_PASS,
        AMMONITE_EMISSION_PASS,
        AMMONITE_DATA_REFRESH
      };
    }

    namespace setup {
      namespace internal {
        //Load required shaders from a path
        bool createShaders(const std::string& shaderPath) {
          //Load the shader classes
          bool hasCreatedShaders = modelShader.loadShader(shaderPath + "models/");
          hasCreatedShaders &= lightShader.loadShader(shaderPath + "lights/");
          hasCreatedShaders &= depthShader.loadShader(shaderPath + "depth/");
          hasCreatedShaders &= skyboxShader.loadShader(shaderPath + "skybox/");
          hasCreatedShaders &= screenShader.loadShader(shaderPath + "screen/");
          hasCreatedShaders &= splashShader.loadShader(shaderPath + "splash/");

          return hasCreatedShaders;
        }

        void deleteShaders() {
          modelShader.destroyShader();
          lightShader.destroyShader();
          depthShader.destroyShader();
          skyboxShader.destroyShader();
          screenShader.destroyShader();
          splashShader.destroyShader();
        }

        //Check for essential GPU capabilities
        bool checkGPUCapabilities(unsigned int* failureCount) {
          const struct {
            const char* extension;
            int major;
            int minor;
            const char* prettyName;
          } extensions[5] {
            {"GL_ARB_direct_state_access", 4, 5, "Direct state access"},
            {"GL_ARB_shader_storage_buffer_object", 4, 3, "Shader Storage Buffer Objects (SSBOs)"},
            {"GL_ARB_texture_storage", 4, 2, "Texture storage"},
            {"GL_ARB_shading_language_420pack", 4, 2, "GLSL shader version 4.20"},
            {"GL_ARB_texture_cube_map_array", 4, 2, "Cubemap arrays"}
          };
          const unsigned int extensionCount = sizeof(extensions) / sizeof(extensions[0]);

          bool success = true;
          for (unsigned int i = 0; i < extensionCount; i++) {
            if (!graphics::internal::checkExtension(extensions[i].extension,
                extensions[i].major, extensions[i].minor)) {
              ammonite::utils::error << extensions[i].prettyName << " unsupported" << std::endl;
              success = false;
              (*failureCount)++;
            }
          }

          //Check minimum OpenGL version is supported
          if (!graphics::internal::checkGlVersion(3, 2)) {
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
          //Pass texture unit locations
          modelShader.useShader();
          glUniform1i(modelShader.diffuseSamplerId, 0);
          glUniform1i(modelShader.specularSamplerId, 1);
          glUniform1i(modelShader.shadowCubeMapId, 2);

          skyboxShader.useShader();
          glUniform1i(skyboxShader.skyboxSamplerId, 3);

          screenShader.useShader();
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
          glEnableVertexArrayAttrib(screenQuadVertexArrayId, 0);
          glVertexArrayVertexBuffer(screenQuadVertexArrayId, 0, bufferIds.screenQuad, 0, 4 * sizeof(signed char));
          glVertexArrayAttribFormat(screenQuadVertexArrayId, 0, 2, GL_BYTE, GL_FALSE, 0);
          glVertexArrayAttribBinding(screenQuadVertexArrayId, 0, 0);

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
          delete [] modelPtrs;
          delete [] lightModelPtrs;
        }
      }
    }

    namespace {
      /*
       - Functions to setup and validate render objects
      */

      //Setup framebuffers for rendering
      void recreateFramebuffers(GLuint* targetBufferIdPtr, unsigned int sampleCount,
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

      void checkFramebuffers(unsigned int renderWidth, unsigned int renderHeight,
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
      void setupDepthMap(unsigned int lightCount, unsigned int shadowRes) {
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
        const GLsizei depthLayers = (GLsizei)std::min(maxLightCount, lightCount) * 6;
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
      void drawModel(ammonite::models::internal::ModelInfo *drawObject,
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

        //Handle pass-specific matrices and uniforms
        ammonite::Mat<float, 4> mvp = {{0}};
        ammonite::Mat<float, 4> vp = {{0}};
        switch (renderMode) {
        case AMMONITE_DEPTH_PASS:
          glUniformMatrix4fv(depthShader.modelMatrixId, 1, GL_FALSE,
                             &drawObject->positionData.modelMatrix[0][0]);
          break;
        case AMMONITE_RENDER_PASS:
          //Calculate model view projection matrix
          ammonite::multiply(*projectionMatrixPtr, *viewMatrixPtr, vp);
          ammonite::multiply(vp, drawObject->positionData.modelMatrix, mvp);

          glUniformMatrix4fv(modelShader.matrixId, 1, GL_FALSE, &mvp[0][0]);
          glUniformMatrix4fv(modelShader.modelMatrixId, 1, GL_FALSE,
                             &drawObject->positionData.modelMatrix[0][0]);
          glUniformMatrix3fv(modelShader.normalMatrixId, 1, GL_FALSE,
                             &drawObject->positionData.normalMatrix[0][0]);
          break;
        case AMMONITE_EMISSION_PASS:
          //Calculate model view projection matrix
          ammonite::multiply(*projectionMatrixPtr, *viewMatrixPtr, vp);
          ammonite::multiply(vp, drawObject->positionData.modelMatrix, mvp);

          glUniformMatrix4fv(lightShader.lightMatrixId, 1, GL_FALSE, &mvp[0][0]);
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

    namespace {
      /*
       - Draw models of a given type, from a cache
       - Update the cache when given AMMONITE_DATA_REFRESH or a pointer to a null pointer
      */
      void drawModelsCached(ammonite::models::internal::ModelInfo*** modelPtrsPtr,
                            ModelTypeEnum modelType, AmmoniteRenderMode renderMode) {
        const unsigned int modelCount = ammonite::models::internal::getModelCount(modelType);

        //Create / update cache for model pointers
        if (renderMode == AMMONITE_DATA_REFRESH || *modelPtrsPtr == nullptr) {
          //Free old model cache
          delete [] *modelPtrsPtr;

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
      }

      void drawSkybox(AmmoniteId activeSkyboxId) {
        //Swap to skybox shader and pass uniforms
        skyboxShader.useShader();

        ammonite::Mat<float, 3> viewMatrixSmall = {{0}};
        ammonite::Mat<float, 4> viewMatrixCut = {{0}};
        ammonite::copy(*viewMatrixPtr, viewMatrixSmall);
        ammonite::copy(viewMatrixSmall, viewMatrixCut);

        glUniformMatrix4fv(skyboxShader.viewMatrixId, 1, GL_FALSE,
                           &viewMatrixCut[0][0]);
        glUniformMatrix4fv(skyboxShader.projectionMatrixId, 1, GL_FALSE,
                           &(*projectionMatrixPtr)[0][0]);

        //Prepare and draw the skybox
        glBindVertexArray(skyboxVertexArrayId);
        glBindTextureUnit(3, activeSkyboxId);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_BYTE, nullptr);
      }

      void drawSplashScreen(AmmoniteId splashScreenId, unsigned int width,
                            unsigned int height) {
        //Swap to splash screen shader
        splashShader.useShader();

        //Pass drawing parameters, pointer is only valid for this frame
        splash::internal::SplashScreen* splashScreen =
          splash::internal::getSplashScreenPtr(splashScreenId);
        glUniform1f(splashShader.widthId, splashScreen->width);
        glUniform1f(splashShader.heightId, splashScreen->height);
        glUniform1f(splashShader.heightOffsetId, splashScreen->heightOffset);

        //Prepare viewport and framebuffer
        internal::prepareScreen(0, width, height, false);

        //Prepare to draw the screen
        glClearColor(splashScreen->backgroundColour[0],
                     splashScreen->backgroundColour[1],
                     splashScreen->backgroundColour[2], 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glBindVertexArray(screenQuadVertexArrayId);

        //Draw the track
        glUniform1f(splashShader.progressId, 1.0f);
        glUniform3fv(splashShader.progressColourId, 1, &splashScreen->trackColour[0]);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, nullptr);

        //Fill in the bar
        glUniform1f(splashShader.progressId, splashScreen->progress);
        glUniform3fv(splashShader.progressColourId, 1, &splashScreen->progressColour[0]);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, nullptr);
      }
    }

    namespace internal {
      void internalDrawSplashScreen(AmmoniteId splashScreenId) {
        const unsigned int width = ammonite::window::internal::getGraphicsWidth();
        const unsigned int height = ammonite::window::internal::getGraphicsHeight();

        drawSplashScreen(splashScreenId, width, height);

        //Prepare for next frame
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        ammonite::window::internal::showFrame(window::internal::getWindowPtr(), false, 0);
      }

      void internalDrawFrame() {
        const unsigned int width = ammonite::window::internal::getGraphicsWidth();
        const unsigned int height = ammonite::window::internal::getGraphicsHeight();
        static unsigned int lastWidth = 0, lastHeight = 0;

        static float lastRenderResMultiplier = 0.0f;
        const float renderResMultiplier = settings::getRenderResMultiplier();

        static unsigned int lastSamples = 0;
        unsigned int sampleCount = settings::getAntialiasingSamples();

        static unsigned int renderWidth = 0, renderHeight = 0;

        //Recreate the framebuffer if the width, height, resolution multiplier or sample count changes
        static GLuint targetBufferId = 0;
        if ((lastWidth != width) || (lastHeight != height) ||
            (lastRenderResMultiplier != renderResMultiplier) ||
            (lastSamples != sampleCount)) {
          //Update values used to determine when to recreate framebuffer
          lastWidth = width;
          lastHeight = height;
          lastRenderResMultiplier = renderResMultiplier;
          lastSamples = sampleCount;

          //Limit sample count to implementation limit
          const unsigned int requestedSamples = sampleCount;
          sampleCount = std::min(requestedSamples, (unsigned int)maxSampleCount);

          if (sampleCount < requestedSamples) {
            ammonite::utils::warning << "Ignoring request for " << requestedSamples \
                                     << " samples, using implementation limit of " \
                                     << maxSampleCount << std::endl;
            settings::setAntialiasingSamples(sampleCount);
          }

          //Calculate render resolution
          renderWidth = (unsigned int)((float)width * renderResMultiplier);
          renderHeight = (unsigned int)((float)height * renderResMultiplier);

          //Create or recreate the framebuffers for rendering
          recreateFramebuffers(&targetBufferId, sampleCount, renderWidth, renderHeight);
          checkFramebuffers(renderWidth, renderHeight, sampleCount);

          ammoniteInternalDebug << "Output resolution: " << width << " x " << height << std::endl;
        }

        //Get shadow resolution and light count, save for next time to avoid cubemap recreation
        const unsigned int shadowRes = settings::getShadowRes();
        const unsigned int lightCount = lighting::getLightCount();
        static unsigned int lastShadowRes = 0;
        static unsigned int lastLightCount = -1;

        //If number of lights or shadow resolution changes, recreate cubemap
        if ((shadowRes != lastShadowRes) || (lightCount != lastLightCount)) {
          setupDepthMap(lightCount, shadowRes);

          //Save for next time to avoid cubemap recreation
          lastShadowRes = shadowRes;
          lastLightCount = lightCount;
        }

        //Swap to depth shader and enable depth testing
        depthShader.useShader();
        internal::prepareScreen(depthMapFBO, shadowRes, shadowRes, true);

        //Pass uniforms that don't change between light sources
        const float shadowFarPlane = settings::getShadowFarPlane();
        glUniform1f(depthShader.shadowFarPlaneId, shadowFarPlane);

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
        if (settings::getGammaCorrection()) {
          glEnable(GL_FRAMEBUFFER_SRGB);
        } else {
          glDisable(GL_FRAMEBUFFER_SRGB);
        }

        //Depth mapping render passes
        ammonite::lighting::internal::finishUpdateLightSources();
        const unsigned int activeLights = std::min(lightCount, maxLightCount);
        for (unsigned int shadowCount = 0; shadowCount < activeLights; shadowCount++) {
          //Check framebuffer status
          if (glCheckNamedFramebufferStatus(depthMapFBO, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            ammonite::utils::warning << "Incomplete depth framebuffer" << std::endl;
          }

          //Pass light source specific uniforms
          glUniform1ui(depthShader.depthShadowIndexId, shadowCount);

          //Render to depth buffer and move to the next light source
          drawModelsCached(&modelPtrs, AMMONITE_MODEL, AMMONITE_DEPTH_PASS);
        }

        //Reset the framebuffer and viewport
        internal::prepareScreen(targetBufferId, renderWidth, renderHeight, true);

        //Clear depth and colour (if no skybox is used)
        const AmmoniteId activeSkybox = ammonite::skybox::getActiveSkybox();
        if (activeSkybox == 0) {
          glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        } else {
          glClear(GL_DEPTH_BUFFER_BIT);
        }

        //Prepare model shader and depth cube map
        modelShader.useShader();
        glBindTextureUnit(2, depthCubeMapId);

        //Get ambient light and camera position
        ammonite::Vec<float, 3> ambientLight = {0};
        ammonite::Vec<float, 3> cameraPosition = {0};
        ammonite::lighting::getAmbientLight(ambientLight);
        ammonite::camera::getPosition(ammonite::camera::getActiveCamera(), cameraPosition);

        //Pass uniforms and render regular models
        glUniform3fv(modelShader.ambientLightId, 1, &ambientLight[0]);
        glUniform3fv(modelShader.cameraPosId, 1, &cameraPosition[0]);
        glUniform1f(modelShader.shadowFarPlaneId, shadowFarPlane);
        glUniform1ui(modelShader.lightCountId, activeLights);
        drawModelsCached(&modelPtrs, AMMONITE_MODEL, AMMONITE_RENDER_PASS);

        //Render light emitting models
        const unsigned int lightModelCount =
          ammonite::models::internal::getModelCount(AMMONITE_LIGHT_EMITTER);
        if (lightModelCount > 0) {
          //Swap to the light emitter shader and render cached light model pointers
          lightShader.useShader();
          drawModelsCached(&lightModelPtrs, AMMONITE_LIGHT_EMITTER, AMMONITE_EMISSION_PASS);
        }

        //Ensure wireframe is disabled
        internal::setWireframe(false);

        //Draw the skybox
        if (activeSkybox != 0) {
          drawSkybox(activeSkybox);
        }

        //Get focal depth status, used to conditionally post-process
        const bool focalDepthEnabled = settings::post::getFocalDepthEnabled();

        //Enable post-processor when required, or blit would fail
        bool isPostRequired = focalDepthEnabled;
        if (sampleCount == 0 || renderResMultiplier != 1.0f) {
          /*
           - Workaround until non-multisampled rendering is done to an offscreen framebuffer
             - sampleCount == 0
           - Workaround INVALID_OPERATION when scaling a multisampled buffer with a blit
             - sampleCount != 0 && renderResMultiplier != 1.0f
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
            const GLbitfield blitBits = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;
            glBlitNamedFramebuffer(colourBufferMultisampleFBO, screenQuadFBO, 0, 0,
                                   (GLint)renderWidth, (GLint)renderHeight, 0, 0,
                                   (GLint)renderWidth, (GLint)renderHeight,
                                   blitBits, GL_NEAREST);
          }

          //Swap to correct shaders
          screenShader.useShader();

          //Conditionally send data for blur
          glUniform1i(screenShader.focalDepthEnabledId, (GLint)focalDepthEnabled);
          if (focalDepthEnabled) {
            const float focalDepth = settings::post::getFocalDepth();
            const float blurStrength = settings::post::getBlurStrength();
            const float farPlane = settings::getRenderFarPlane();

            glUniform1f(screenShader.focalDepthId, focalDepth);
            glUniform1f(screenShader.blurStrengthId, blurStrength);
            glUniform1f(screenShader.farPlaneId, farPlane);
            glBindTextureUnit(5, screenQuadDepthTextureId);
          }

          //Display the rendered frame
          glBindVertexArray(screenQuadVertexArrayId);
          glBindTextureUnit(4, screenQuadTextureId);
          glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, nullptr);
        } else {
          //Resolve multisampling into default framebuffer
          if (sampleCount != 0) {
            const GLbitfield blitBits = GL_COLOR_BUFFER_BIT;
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

