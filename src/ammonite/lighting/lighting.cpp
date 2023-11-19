#include <map>
#include <vector>
#include <cstring>
#include <cmath>

#include <GL/glew.h>
#include <glm/glm.hpp>

#include <omp.h>
#include <thread>

#include "internal/lightTypes.hpp"
#include "internal/lightTracker.hpp"
#include "../internal/internalSettings.hpp"

#include "../models/modelInterface.hpp"
#include "../models/internal/modelTracker.hpp"

namespace ammonite {
  namespace {
    //Lighting shader storage buffer IDs
    GLuint lightDataId = 0;

    //Default ambient light
    glm::vec3 ambientLight = glm::vec3(0.0f, 0.0f, 0.0f);

    //Track light sources
    std::map<int, lighting::internal::LightSource> lightTrackerMap;
    std::map<int, glm::mat4[6]> lightTransformMap;
    unsigned int prevLightCount = 0;

    //Track cumulative number of created light sources
    int totalLights = 0;
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

      std::map<int, glm::mat4[6]>* getLightTransformsPtr() {
        return &lightTransformMap;
      }

      //Unlink a light source from a model, using only the model ID (doesn't touch the model)
      void unlinkByModel(int modelId) {
        //Get the ID of the light attached to the model
        int lightId = ammonite::models::internal::getLightEmitterId(modelId);
        //Unlink if a light was attached
        if (lightId != -1) {
          lightTrackerMap[lightId].modelId = -1;
        }
      }
    }

    //Regular light handling methods

    void updateLightSources() {
      //Data structure to pass light sources into shader
      struct ShaderLightSource {
        glm::vec4 geometry;
        glm::vec4 diffuse;
        glm::vec4 specular;
        glm::vec4 power;
      } shaderData[lightTrackerMap.size()];

      //Use 1 thread per 20 light sources, up to hardware maximum
      unsigned int threadCount = std::ceil(lightTrackerMap.size() / 20);
      threadCount = std::min(threadCount, std::thread::hardware_concurrency());

      omp_set_dynamic(0);
      omp_set_num_threads(threadCount);

      //If no lights remain, unbind and return early
      if (lightTrackerMap.size() == 0) {
        glDeleteBuffers(1, &lightDataId);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
        lightDataId = 0;
        return;
      }

      static float* farPlanePtr = ammonite::settings::graphics::internal::getShadowFarPlanePtr();
      glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), 1.0f, 0.0f, *farPlanePtr);

      //Hold all calculated light / shadow transformation matrices
      glm::mat4 lightTransforms[lightTrackerMap.size()][6];
      int lightTransformIds[lightTrackerMap.size()];

      //Repack light sources into ShaderData (uses vec4s for OpenGL)
      #pragma omp parallel for
      for (unsigned int i = 0; i < lightTrackerMap.size(); i++) {
        //Repacking light sources
        auto lightIt = lightTrackerMap.begin();
        std::advance(lightIt, i);
        auto lightSource = &lightIt->second;

        //Override position for light emitting models, and add to tracker
        if (lightSource->modelId != -1) {
          //Override light position, using linked model
          lightSource->geometry = ammonite::models::position::getPosition(lightSource->modelId);

          //Update lightIndex for rendering light emitting models
          auto modelPtr = ammonite::models::internal::getModelPtr(lightSource->modelId);
          modelPtr->lightIndex = i;
        }

        //Calculate shadow transforms for shadows
        glm::vec3 lightPos = lightSource->geometry;
        lightTransformIds[i] = lightSource->lightId;
        lightTransforms[i][0] = shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0));
        lightTransforms[i][1] = shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0));
        lightTransforms[i][2] = shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0));
        lightTransforms[i][3] = shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0, 0.0, -1.0));
        lightTransforms[i][4] = shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.0, -1.0, 0.0));
        lightTransforms[i][5] = shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 0.0, -1.0), glm::vec3(0.0, -1.0, 0.0));

        //Repack lighting information
        shaderData[i].geometry = glm::vec4(lightSource->geometry, 0.0f);
        shaderData[i].diffuse = glm::vec4(lightSource->diffuse, 0.0f);
        shaderData[i].specular = glm::vec4(lightSource->specular, 0.0f);
        shaderData[i].power = glm::vec4(lightSource->power, 0.0f, 0.0f, 0.0f);
      }

      //Copy calculated tranforms to map
      for (unsigned int i = 0; i < lightTrackerMap.size(); i++) {
        std::memcpy(lightTransformMap[lightTransformIds[i]],
                    lightTransforms[i], sizeof(glm::mat4) * 6);
      }

      //If the light count hasn't changed, sub the data instead of recreating the buffer
      if (prevLightCount == lightTrackerMap.size()) {
        glNamedBufferSubData(lightDataId, 0, sizeof(shaderData), &shaderData);
      } else {
        //If the buffer already exists, destroy it
        if (lightDataId != 0) {
          glDeleteBuffers(1, &lightDataId);
        }

        //Add the shader data to a shader storage buffer object
        glCreateBuffers(1, &lightDataId);
        glNamedBufferData(lightDataId, sizeof(shaderData), &shaderData, GL_STATIC_DRAW);
      }

      //Use the lighting shader storage buffer
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, lightDataId);

      //Update previous light count for next run
      prevLightCount = lightTrackerMap.size();
    }

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
    }

    void unlinkModel(int lightId) {
      //Unlink the attached model from the light source
      ammonite::lighting::internal::LightSource* lightSource = ammonite::lighting::internal::getLightSourcePtr(lightId);
      ammonite::models::internal::setLightEmitterId(lightSource->modelId, -1);
      lightSource->modelId = -1;
    }

    void deleteLightSource(int lightId) {
      //Unlink any attached models
      ammonite::lighting::unlinkModel(lightId);

      //Check the light source exists
      if (lightTrackerMap.contains(lightId)) {
        //Remove the light source from the tracker
        lightTrackerMap.erase(lightId);
      }

      //Remove any light transform entry
      if (lightTransformMap.contains(lightId)) {
        //Remove the light source tranforms from the tracker
        lightTransformMap.erase(lightId);
      }
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
        ammonite::lighting::internal::LightSource* lightSource = ammonite::lighting::internal::getLightSourcePtr(lightId);
        if (lightSource == nullptr) {
          return glm::vec3(0.0f);
        }

        return lightSource->geometry;
      }

      glm::vec3 getColour(int lightId) {
        ammonite::lighting::internal::LightSource* lightSource = ammonite::lighting::internal::getLightSourcePtr(lightId);
        if (lightSource == nullptr) {
          return glm::vec3(0.0f);
        }

        return lightSource->diffuse;
      }

      float getPower(int lightId) {
        ammonite::lighting::internal::LightSource* lightSource = ammonite::lighting::internal::getLightSourcePtr(lightId);
        if (lightSource == nullptr) {
          return 0.0f;
        }

        return lightSource->power;
      }

      void setGeometry(int lightId, glm::vec3 geometry) {
        ammonite::lighting::internal::LightSource* lightSource = ammonite::lighting::internal::getLightSourcePtr(lightId);
        if (lightSource != nullptr) {
          lightSource->geometry = geometry;
        }
      }

      void setColour(int lightId, glm::vec3 colour) {
        ammonite::lighting::internal::LightSource* lightSource = ammonite::lighting::internal::getLightSourcePtr(lightId);
        if (lightSource != nullptr) {
          lightSource->diffuse = colour;
        }
      }

      void setPower(int lightId, float power) {
        ammonite::lighting::internal::LightSource* lightSource = ammonite::lighting::internal::getLightSourcePtr(lightId);
        if (lightSource != nullptr) {
          lightSource->power = power;
        }
      }
    }
  }
}
