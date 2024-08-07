#include <atomic>
#include <map>
#include <vector>
#include <cstring>
#include <cmath>

#include <GL/glew.h>
#include <glm/glm.hpp>

#include "internal/lightTypes.hpp"
#include "internal/internalLighting.hpp"
#include "../graphics/internal/internalRenderCore.hpp"

#include "../core/threadManager.hpp"
#include "../models/modelInterface.hpp"
#include "../models/internal/modelTracker.hpp"
#include "../types.hpp"

namespace ammonite {
  namespace {
    //Lighting shader storage buffer IDs
    GLuint lightDataId = 0;

    //Default ambient light
    glm::vec3 ambientLight = glm::vec3(0.0f, 0.0f, 0.0f);

    //Track light sources
    std::map<int, lighting::internal::LightSource> lightTrackerMap;
    glm::mat4* lightTransforms = nullptr;
    unsigned int prevLightCount = 0;
    bool lightSourcesChanged = false;

    //Track cumulative number of created light sources
    int totalLights = 0;

    //Data structure to pass light sources into shader
    struct ShaderLightSource {
      glm::vec4 geometry;
      glm::vec4 diffuse;
      glm::vec4 specular;
      glm::vec4 power;
    };

    //Data used by the light worker
    struct LightWorkerData {
      ShaderLightSource* shaderData;
      glm::mat4* shadowProj;
      int i;
    };
  }

  namespace {
    static void lightWork(void* userPtr) {
      int i = ((LightWorkerData*)userPtr)->i;
      ShaderLightSource* shaderData = ((LightWorkerData*)userPtr)->shaderData;
      glm::mat4* shadowProj = ((LightWorkerData*)userPtr)->shadowProj;

      //Repacking light sources
      auto lightIt = lightTrackerMap.begin();
      std::advance(lightIt, i);
      auto lightSource = &lightIt->second;
      lightSource->lightIndex = i;

      //Override position for light emitting models, and add to tracker
      if (lightSource->modelId != -1) {
        //Override light position, using linked model
        lightSource->geometry = ammonite::models::position::getPosition(lightSource->modelId);

        //Update lightIndex for rendering light emitting models
        auto modelPtr = ammonite::models::internal::getModelPtr(lightSource->modelId);
        modelPtr->lightIndex = lightSource->lightIndex;
      }

      //Calculate shadow transforms for shadows
      glm::vec3 lightPos = lightSource->geometry;
      glm::mat4* transformStart = lightTransforms + (lightSource->lightIndex * 6);
      transformStart[0] = *shadowProj *
        glm::lookAt(lightPos, lightPos + glm::vec3(1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0));
      transformStart[1] = *shadowProj *
        glm::lookAt(lightPos, lightPos + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0));
      transformStart[2] = *shadowProj *
        glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0));
      transformStart[3] = *shadowProj *
        glm::lookAt(lightPos, lightPos + glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0, 0.0, -1.0));
      transformStart[4] = *shadowProj *
        glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.0, -1.0, 0.0));
      transformStart[5] = *shadowProj *
        glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 0.0, -1.0), glm::vec3(0.0, -1.0, 0.0));

      //Repack lighting information
      shaderData[i].geometry = glm::vec4(lightSource->geometry, 0.0f);
      shaderData[i].diffuse = glm::vec4(lightSource->diffuse, 0.0f);
      shaderData[i].specular = glm::vec4(lightSource->specular, 0.0f);
      shaderData[i].power = glm::vec4(lightSource->power, 0.0f, 0.0f, 0.0f);
    }
  }

  namespace lighting {
    //Internally exposed light handling methods
    namespace internal {
      LightSource* getLightSourcePtr(int lightId) {
        //Check the light source exists, and return a pointer
        if (lightTrackerMap.contains(lightId)) {
          return &lightTrackerMap[lightId];
        } else {
          return nullptr;
        }
      }

      std::map<int, LightSource>* getLightTrackerPtr() {
        return &lightTrackerMap;
      }

      glm::mat4** getLightTransformsPtr() {
        return &lightTransforms;
      }

      //Unlink a light source from a model, using only the model ID (doesn't touch the model)
      void unlinkByModel(int modelId) {
        //Get the ID of the light attached to the model
        int lightId = ammonite::models::internal::getLightEmitterId(modelId);
        //Unlink if a light was attached
        if (lightId != -1) {
          lightTrackerMap[lightId].modelId = -1;
          lightSourcesChanged = true;
        }
      }

      void setLightSourcesChanged() {
        lightSourcesChanged = true;
      }

      void updateLightSources() {
        //If lights haven't changed, skip
        if (!lightSourcesChanged) {
          return;
        }

        //If no lights remain, unbind and return early
        unsigned int lightCount = lightTrackerMap.size();
        if (lightCount == 0) {
          glDeleteBuffers(1, &lightDataId);
          glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
          lightDataId = 0;
          lightSourcesChanged = false;
          return;
        }

        static float* shadowFarPlanePtr = renderer::settings::internal::getShadowFarPlanePtr();
        glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f),
                                                1.0f, 0.0f, *shadowFarPlanePtr);

        //Resize packed light buffer
        if (prevLightCount != lightCount) {
          if (lightTransforms != nullptr) {
            delete [] lightTransforms;
          }
          lightTransforms = new glm::mat4[lightCount * 6];
        }

        //Repack light sources into ShaderData (uses vec4s for OpenGL)
        ShaderLightSource* shaderData = new ShaderLightSource[lightCount];
        int shaderDataSize = sizeof(ShaderLightSource) * lightCount;
        AmmoniteCompletion* syncs = new AmmoniteCompletion[lightCount]{ATOMIC_FLAG_INIT};
        LightWorkerData* workerData = new LightWorkerData[lightCount];
        for (unsigned int i = 0; i < lightCount; i++) {
          workerData[i].shaderData = shaderData;
          workerData[i].shadowProj = &shadowProj;
          workerData[i].i = i;
        }
        ammonite::thread::internal::submitMultiple(lightWork, (void*)&workerData[0],
                                                   sizeof(LightWorkerData),
                                                   &syncs[0], lightCount);

        for (unsigned int i = 0; i < lightCount; i++) {
          syncs[i].wait(false);
        }
        delete [] syncs;
        delete [] workerData;

        //If the light count hasn't changed, sub the data instead of recreating the buffer
        if (prevLightCount == lightTrackerMap.size()) {
          glNamedBufferSubData(lightDataId, 0, shaderDataSize, shaderData);
        } else {
          //If the buffer already exists, destroy it
          if (lightDataId != 0) {
            glDeleteBuffers(1, &lightDataId);
          }

          //Add the shader data to a shader storage buffer object
          glCreateBuffers(1, &lightDataId);
          glNamedBufferData(lightDataId, shaderDataSize, shaderData, GL_DYNAMIC_DRAW);
        }
        delete [] shaderData;

        //Use the lighting shader storage buffer
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, lightDataId);

        //Update previous light count for next run
        prevLightCount = lightTrackerMap.size();
        lightSourcesChanged = false;
      }
    }

    //Regular light handling methods

    int getMaxLightCount() {
      //Get the max number of lights supported, from the max layers on a cubemap
      int maxArrayLayers = 0;
      glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &maxArrayLayers);
      return std::floor(maxArrayLayers / 6);
    }

    int createLightSource() {
      internal::LightSource lightSource;

      //Add light source to the tracker
      lightSource.lightId = ++totalLights;
      lightTrackerMap[lightSource.lightId] = lightSource;

      //Return the light source's ID
      lightSourcesChanged = true;
      return lightSource.lightId;
    }

    void linkModel(int lightId, int modelId) {
      //Remove any light source's attachment to the model
      ammonite::lighting::internal::unlinkByModel(modelId);

      //If the light source is already linked to another model, reset the linked model
      ammonite::lighting::internal::LightSource* lightSource = ammonite::lighting::internal::getLightSourcePtr(lightId);
      if (lightSource->modelId != -1) {
        ammonite::models::internal::setLightEmitterId(lightSource->modelId, -1);
      }

      //Link the light source and model together
      lightSource->modelId = modelId;
      ammonite::models::internal::setLightEmitterId(modelId, lightId);
      lightSourcesChanged = true;
    }

    void unlinkModel(int lightId) {
      //Unlink the attached model from the light source
      ammonite::lighting::internal::LightSource* lightSource = ammonite::lighting::internal::getLightSourcePtr(lightId);
      ammonite::models::internal::setLightEmitterId(lightSource->modelId, -1);
      lightSource->modelId = -1;
      lightSourcesChanged = true;
    }

    void deleteLightSource(int lightId) {
      //Unlink any attached models
      ammonite::lighting::unlinkModel(lightId);

      //Check the light source exists
      if (lightTrackerMap.contains(lightId)) {
        //Remove the light source from the tracker
        lightTrackerMap.erase(lightId);
      }

      if (lightTrackerMap.size() == 0) {
        if (lightTransforms != nullptr) {
          delete [] lightTransforms;
        }
        lightTransforms = nullptr;
      }

      lightSourcesChanged = true;
    }

    void setAmbientLight(glm::vec3 newAmbientLight) {
      ambientLight = newAmbientLight;
    }

    glm::vec3 getAmbientLight() {
      return ambientLight;
    }
  }

  //Exposed methods to modify light properties
  namespace lighting {
    namespace properties {
      glm::vec3 getGeometry(int lightId) {
        ammonite::lighting::internal::LightSource* lightSource =
          ammonite::lighting::internal::getLightSourcePtr(lightId);
        if (lightSource == nullptr) {
          return glm::vec3(0.0f);
        }

        return lightSource->geometry;
      }

      glm::vec3 getColour(int lightId) {
        ammonite::lighting::internal::LightSource* lightSource =
          ammonite::lighting::internal::getLightSourcePtr(lightId);
        if (lightSource == nullptr) {
          return glm::vec3(0.0f);
        }

        return lightSource->diffuse;
      }

      float getPower(int lightId) {
        ammonite::lighting::internal::LightSource* lightSource =
          ammonite::lighting::internal::getLightSourcePtr(lightId);
        if (lightSource == nullptr) {
          return 0.0f;
        }

        return lightSource->power;
      }

      void setGeometry(int lightId, glm::vec3 geometry) {
        ammonite::lighting::internal::LightSource* lightSource =
          ammonite::lighting::internal::getLightSourcePtr(lightId);
        if (lightSource != nullptr) {
          lightSource->geometry = geometry;
          lightSourcesChanged = true;
        }
      }

      void setColour(int lightId, glm::vec3 colour) {
        ammonite::lighting::internal::LightSource* lightSource =
          ammonite::lighting::internal::getLightSourcePtr(lightId);
        if (lightSource != nullptr) {
          lightSource->diffuse = colour;
          lightSourcesChanged = true;
        }
      }

      void setPower(int lightId, float power) {
        ammonite::lighting::internal::LightSource* lightSource =
          ammonite::lighting::internal::getLightSourcePtr(lightId);
        if (lightSource != nullptr) {
          lightSource->power = power;
          lightSourcesChanged = true;
        }
      }
    }
  }
}
