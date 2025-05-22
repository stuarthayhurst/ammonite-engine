#include <cstddef>
#include <cstring>
#include <iterator>
#include <unordered_map>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

extern "C" {
  #include <epoxy/gl.h>
}

#include "lighting.hpp"

#include "../graphics/renderer.hpp"
#include "../models/models.hpp"
#include "../utils/id.hpp"
#include "../utils/thread.hpp"

namespace ammonite {
  namespace lighting {
    namespace {
      //Lighting shader storage buffer IDs
      GLuint lightDataId = 0;
      GLuint shadowDataId = 0;

      //Default ambient light
      glm::vec3 ambientLight = glm::vec3(0.0f, 0.0f, 0.0f);

      //Track light sources
      std::unordered_map<AmmoniteId, lighting::internal::LightSource> lightTrackerMap;
      unsigned int prevLightCount = 0;
      bool lightSourcesChanged = false;
      AmmoniteId lastLightId = 0;

      //Data structure to match shader light sources in shader
      using ShaderLightSource = glm::vec4[3];
      using ShaderShadowTransform = GLfloat[4z * 4z * 6z];

      //Data used by the light worker
      struct LightWorkerData {
        unsigned int i;
      };
      glm::mat4 shadowProj;

      ShaderLightSource* shaderLightData = nullptr;
      ShaderShadowTransform* shaderShadowData = nullptr;
      LightWorkerData* workerData = nullptr;
      AmmoniteGroup group{0};
    }

    namespace {
      void lightWork(void* userPtr) {
        const unsigned int i = ((LightWorkerData*)userPtr)->i;

        //Repacking light sources
        auto lightIt = lightTrackerMap.begin();
        std::advance(lightIt, i);
        lighting::internal::LightSource* lightSource = &lightIt->second;
        lightSource->lightIndex = i;

        //Override position for light emitting models, and add to tracker
        if (lightSource->modelId != 0) {
          //Override light position, using linked model
          lightSource->geometry = ammonite::models::position::getPosition(lightSource->modelId);

          //Update lightIndex for rendering light emitting models
          auto modelPtr = ammonite::models::internal::getModelPtr(lightSource->modelId);
          modelPtr->lightIndex = lightSource->lightIndex;
        }

        //Repack lighting information
        shaderLightData[i][0] = glm::vec4(lightSource->geometry, 0.0f);
        shaderLightData[i][1] = glm::vec4(lightSource->diffuse, 0.0f);
        shaderLightData[i][2] = glm::vec4(lightSource->specular, lightSource->power);

        //Calculate shadow transforms
        const glm::vec3 lightPos = lightSource->geometry;
        glm::mat4 transforms[6] = {
          shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0)),
          shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0)),
          shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0)),
          shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0, 0.0, -1.0)),
          shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.0, -1.0, 0.0)),
          shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 0.0, -1.0), glm::vec3(0.0, -1.0, 0.0))
        };

        //Pack shadow transforms
        const std::size_t matSize = 4z * 4z;
        GLfloat* transformBlockStart = shaderShadowData[lightSource->lightIndex];
        for (int face = 0; face < 6; face++) {
          std::memcpy(transformBlockStart + (matSize * face), glm::value_ptr(transforms[face]),
                      matSize * sizeof(GLfloat));
        }
      }

      //Pointer is only valid until lightTrackerMap is modified
      internal::LightSource* getLightSourcePtr(AmmoniteId lightId) {
        //Check the light source exists, and return a pointer
        if (lightTrackerMap.contains(lightId)) {
          return &lightTrackerMap[lightId];
        }

        return nullptr;
      }
    }


    //Internally exposed light handling methods
    namespace internal {
      void destroyLightSystem() {
        if (lightDataId != 0) {
          glDeleteBuffers(1, &lightDataId);
          glDeleteBuffers(1, &shadowDataId);
          prevLightCount = 0;
        }

        if (shaderLightData != nullptr) {
          delete [] shaderLightData;
          delete [] shaderShadowData;
          delete [] workerData;
        }
      }

      //Unlink a light source from a model, using only the model ID (doesn't touch the model)
      void unlinkByModel(AmmoniteId modelId) {
        //Get the ID of the light attached to the model
        const AmmoniteId lightId = ammonite::models::internal::getLightEmitterId(modelId);
        //Unlink if a light was attached
        if (lightId != 0) {
          lightTrackerMap[lightId].modelId = 0;
          lightSourcesChanged = true;
        }
      }

      void setLightSourcesChanged() {
        lightSourcesChanged = true;
      }

      //Dispatch workers if the lighting buffers need rebuilding
      void startUpdateLightSources() {
        //If lights haven't changed, skip
        if (!lightSourcesChanged) {
          return;
        }

        //If no lights remain, unbind and return early
        const unsigned int lightCount = lightTrackerMap.size();
        if (lightCount == 0) {
          glDeleteBuffers(1, &lightDataId);
          glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
          lightDataId = 0;

          glDeleteBuffers(1, &shadowDataId);
          glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, 0);
          shadowDataId = 0;

          prevLightCount = 0;
          lightSourcesChanged = false;
          return;
        }

        const float shadowFarPlane = renderer::settings::getShadowFarPlane();
        shadowProj = glm::perspective(glm::radians(90.0f), 1.0f, 0.0f, shadowFarPlane);

        //Resize buffers
        if (prevLightCount != lightCount) {
          if (shaderLightData != nullptr) {
            delete [] shaderLightData;
            delete [] shaderShadowData;
            delete [] workerData;
          }

          shaderLightData = new ShaderLightSource[lightCount];
          shaderShadowData = new ShaderShadowTransform[lightCount];
          workerData = new LightWorkerData[lightCount];
        }

        //Repack light sources into buffers (uses vec4s for OpenGL)
        for (unsigned int i = 0; i < lightCount; i++) {
          workerData[i].i = i;
        }

        ammonite::utils::thread::submitMultiple(lightWork, &workerData[0],
          sizeof(LightWorkerData), &group, lightCount, nullptr);
      }

      //Wait for the workers to finish and upload the lighting buffers
      void finishUpdateLightSources() {
        //If lights haven't changed, skip
        if (!lightSourcesChanged) {
          return;
        }

        const unsigned int lightCount = lightTrackerMap.size();
        const std::size_t shaderLightDataSize = sizeof(ShaderLightSource) * lightCount;
        const std::size_t shaderShadowDataSize = sizeof(ShaderShadowTransform) * lightCount;

        ammonite::utils::thread::waitGroupComplete(&group, lightCount);

        //If the light count hasn't changed, sub the data instead of recreating the buffer
        if (prevLightCount == lightTrackerMap.size()) {
          glNamedBufferSubData(lightDataId, 0, (GLsizeiptr)shaderLightDataSize, shaderLightData);
          glNamedBufferSubData(shadowDataId, 0, (GLsizeiptr)shaderShadowDataSize, shaderShadowData);
        } else {
          //If the buffers already exist, destroy them
          if (lightDataId != 0) {
            glDeleteBuffers(1, &lightDataId);
            glDeleteBuffers(1, &shadowDataId);
            prevLightCount = 0;
          }

          //Add the shader data to a shader storage buffer object
          glCreateBuffers(1, &lightDataId);
          glNamedBufferData(lightDataId, (GLsizeiptr)shaderLightDataSize, shaderLightData, GL_DYNAMIC_DRAW);

          glCreateBuffers(1, &shadowDataId);
          glNamedBufferData(shadowDataId, (GLsizeiptr)shaderShadowDataSize, shaderShadowData, GL_DYNAMIC_DRAW);
        }

        //Use the lighting and shadow shader storage buffer
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, lightDataId);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, shadowDataId);

        //Update previous light count for next run
        prevLightCount = lightTrackerMap.size();
        lightSourcesChanged = false;
      }
    }


    //Exposed light handling functions
    unsigned int getLightCount() {
      return lightTrackerMap.size();
    }

    unsigned int getMaxLightCount() {
      //Get the max number of lights supported, from the max layers on a cubemap
      int maxArrayLayers = 0;
      glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &maxArrayLayers);
      return maxArrayLayers / 6;
    }

    AmmoniteId createLightSource() {
      internal::LightSource lightSource;

      //Add light source to the tracker
      lightSource.lightId = utils::internal::setNextId(&lastLightId, lightTrackerMap);
      lightTrackerMap[lightSource.lightId] = lightSource;

      //Return the light source's ID
      lightSourcesChanged = true;
      return lightSource.lightId;
    }

    void linkModel(AmmoniteId lightId, AmmoniteId modelId) {
      //Remove any light source's attachment to the model
      ammonite::lighting::internal::unlinkByModel(modelId);

      //If the light source is already linked to another model, reset the linked model
      internal::LightSource* lightSource = &lightTrackerMap[lightId];
      if (lightSource->modelId != 0) {
        ammonite::models::internal::setLightEmitterId(lightSource->modelId, 0);
      }

      //Link the light source and model together
      lightSource->modelId = modelId;
      ammonite::models::internal::setLightEmitterId(modelId, lightId);
      lightSourcesChanged = true;
    }

    void unlinkModel(AmmoniteId lightId) {
      //Unlink the attached model from the light source
      internal::LightSource* lightSource = &lightTrackerMap[lightId];
      ammonite::models::internal::setLightEmitterId(lightSource->modelId, 0);
      lightSource->modelId = 0;
      lightSourcesChanged = true;
    }

    void deleteLightSource(AmmoniteId lightId) {
      //Unlink any attached models
      ammonite::lighting::unlinkModel(lightId);

      //Check the light source exists
      if (lightTrackerMap.contains(lightId)) {
        //Remove the light source from the tracker
        lightTrackerMap.erase(lightId);
      }

      if (lightTrackerMap.empty()) {
        if (shaderLightData != nullptr) {
          delete [] shaderLightData;
          delete [] shaderShadowData;
          delete [] workerData;
        }

        shaderLightData = nullptr;
        shaderShadowData = nullptr;
        workerData = nullptr;
      }

      lightSourcesChanged = true;
    }

    void setAmbientLight(glm::vec3 newAmbientLight) {
      ambientLight = newAmbientLight;
    }

    glm::vec3 getAmbientLight() {
      return ambientLight;
    }


    //Exposed functions to modify light properties
    namespace properties {
      glm::vec3 getGeometry(AmmoniteId lightId) {
        const internal::LightSource* lightSource = getLightSourcePtr(lightId);
        if (lightSource == nullptr) {
          return glm::vec3(0.0f);
        }

        return lightSource->geometry;
      }

      glm::vec3 getColour(AmmoniteId lightId) {
        const internal::LightSource* lightSource = getLightSourcePtr(lightId);
        if (lightSource == nullptr) {
          return glm::vec3(0.0f);
        }

        return lightSource->diffuse;
      }

      float getPower(AmmoniteId lightId) {
        const internal::LightSource* lightSource = getLightSourcePtr(lightId);
        if (lightSource == nullptr) {
          return 0.0f;
        }

        return lightSource->power;
      }

      void setGeometry(AmmoniteId lightId, glm::vec3 geometry) {
        internal::LightSource* lightSource = getLightSourcePtr(lightId);
        if (lightSource != nullptr) {
          lightSource->geometry = geometry;
          lightSourcesChanged = true;
        }
      }

      void setColour(AmmoniteId lightId, glm::vec3 colour) {
        internal::LightSource* lightSource = getLightSourcePtr(lightId);
        if (lightSource != nullptr) {
          lightSource->diffuse = colour;
          lightSourcesChanged = true;
        }
      }

      void setPower(AmmoniteId lightId, float power) {
        internal::LightSource* lightSource = getLightSourcePtr(lightId);
        if (lightSource != nullptr) {
          lightSource->power = power;
          lightSourcesChanged = true;
        }
      }
    }
  }
}
