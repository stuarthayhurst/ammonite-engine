#include <map>
#include <cmath>
#include <vector>

#include <GL/glew.h>
#include <glm/glm.hpp>

#include <omp.h>
#include <thread>

#include "internal/lightTracker.hpp"
#include "internal/modelTracker.hpp"
#include "modelManager.hpp"

#ifdef DEBUG
  #include <iostream>
#endif

namespace ammonite {
  namespace {
    //Lighting shader storage buffer IDs
    GLuint lightDataId = 0;

    //Default ambient light
    glm::vec3 ambientLight = glm::vec3(0.0f, 0.0f, 0.0f);

    //Track light sources
    std::map<int, lighting::LightSource> lightTrackerMap;
    unsigned int prevLightCount = 0;

    //Track cumulative number of created light sources
    int totalLights = 0;

    //Track light emitting models
    std::vector<int> lightEmitterData;
  }

  //Internally exposed light handling methods
  namespace lighting {
    //Return data on light emitting models
    void getLightEmitters(int* lightCount, std::vector<int>* lightData) {
      *lightCount = lightEmitterData.size() / 2;

      //Fill passed vector with data
      for (unsigned int i = 0; i < lightEmitterData.size(); i++) {
        lightData->push_back(lightEmitterData[i]);
      }
    }

    LightSource* getLightSourcePtr(int lightId) {
      //Check the light source exists, and return a pointer
      auto it = lightTrackerMap.find(lightId);
      if (it != lightTrackerMap.end()) {
        return &it->second;
      } else {
        return nullptr;
      }
    }

    //Unlink a light source from a model, using only the model ID (doesn't touch the model)
    void unlinkByModel(int modelId) {
      //Check if the model has already been linked to
      if (ammonite::models::getLightEmitting(modelId)) {
        //Find the light source responsible and unlink
        auto lightIt = lightTrackerMap.begin();
        for(unsigned int i = 0; i < lightTrackerMap.size(); i++) {
          //Reset the modelId on the previously linked light source
          if (lightIt->second.modelId == modelId) {
            lightIt->second.modelId = -1;
            return;
          }

          lightIt++;
        }
      }
    }
  }

  //Exposed light handling methods
  namespace lighting {
    void updateLightSources() {
      //Data structure to pass light sources into shader
      struct ShaderLightSource {
        glm::vec4 geometry;
        glm::vec4 colour;
        glm::vec4 diffuse;
        glm::vec4 specular;
        float power[4];
      } shaderData[lightTrackerMap.size()];

      //Use 1 thread per 25 light sources, up to hardware maximum
      unsigned int threadCount = std::ceil(lightTrackerMap.size() / 25);
      if (threadCount > std::thread::hardware_concurrency()) {
        threadCount = std::thread::hardware_concurrency();
      }
      omp_set_dynamic(0);
      omp_set_num_threads(threadCount);

      //Clear saved data on light emitting models
      lightEmitterData.clear();

      //Repack light sources into ShaderData (uses vec4s for OpenGL)
      #pragma omp parallel for
      for(unsigned int i = 0; i < lightTrackerMap.size(); i++) {
        //Repacking light sources
        auto lightIt = lightTrackerMap.begin();
        std::advance(lightIt, i);
        auto lightSource = lightIt->second;

        shaderData[i].colour = glm::vec4(lightSource.colour, 0);
        shaderData[i].diffuse = glm::vec4(lightSource.diffuse, 0);
        shaderData[i].specular = glm::vec4(lightSource.specular, 0);
        shaderData[i].power[0] = lightSource.power;

        //Override position for light emitting models, and add to tracker
        if (lightSource.modelId != -1) {
          //Override light position
          glm::vec3 modelPosition = ammonite::models::position::getPosition(lightSource.modelId);
          shaderData[i].geometry = glm::vec4(modelPosition, 0);

          //Add to tracker
          lightEmitterData.push_back(lightSource.modelId);
          lightEmitterData.push_back(i);
        } else {
          shaderData[i].geometry = glm::vec4(lightSource.geometry, 0);
        }
      }

      //If no lights remain, unbind and return early
      if (lightTrackerMap.size() == 0) {
        glDeleteBuffers(1, &lightDataId);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
        lightDataId = 0;
        return;
      }

      //If the light count hasn't changed, sub the data instead of recreating the buffer
      if (prevLightCount == lightTrackerMap.size()) {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightDataId);
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(shaderData), &shaderData);
      } else {
        //If the buffer already exists, destroy it
        if (lightDataId != 0) {
          glDeleteBuffers(1, &lightDataId);
        }

        //Add the shader data to a shader storage buffer object
        glGenBuffers(1, &lightDataId);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightDataId);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(shaderData), &shaderData, GL_STATIC_DRAW);
      }

      //Use the lighting shader storage buffer
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, lightDataId);
      glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

      //Update previous light count for next run
      prevLightCount = lightTrackerMap.size();
    }

    int createLightSource() {
      LightSource lightSource;

      //Add light source to the tracker
      lightSource.lightId = ++totalLights;
      lightTrackerMap[lightSource.lightId] = lightSource;

      //Return the light source's ID
      return lightSource.lightId;
    }

    void linkModel(int lightId, int modelId) {
      //Remove the light source's attachment to any model
      ammonite::lighting::unlinkByModel(modelId);

      //If the light source is already linked to another model, reset the linked model
      ammonite::lighting::LightSource* lightSource = ammonite::lighting::getLightSourcePtr(lightId);
      if (lightSource->modelId != -1) {
        ammonite::models::setLightEmitting(lightSource->modelId, false);
      }

      //Link the light source and model together
      lightSource->modelId = modelId;
      ammonite::models::setLightEmitting(modelId, true);
    }

    void unlinkModel(int lightId) {
      //Unlink the attached model from the light source
      ammonite::lighting::LightSource* lightSource = ammonite::lighting::getLightSourcePtr(lightId);
      ammonite::models::setLightEmitting(lightSource->modelId, false);
      lightSource->modelId = -1;
    }

    void deleteLightSource(int lightId) {
      //Unlink any attached models
      ammonite::lighting::unlinkModel(lightId);

      //Check the light source exists
      auto it = lightTrackerMap.find(lightId);
      if (it != lightTrackerMap.end()) {
        //Remove the light source from the tracker
        lightTrackerMap.erase(lightId);
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
        ammonite::lighting::LightSource* lightSource = ammonite::lighting::getLightSourcePtr(lightId);
        if (lightSource == nullptr) {
          return glm::vec3(0.0f);
        }

        return lightSource->geometry;
      }

      glm::vec3 getColour(int lightId) {
        ammonite::lighting::LightSource* lightSource = ammonite::lighting::getLightSourcePtr(lightId);
        if (lightSource == nullptr) {
          return glm::vec3(0.0f);
        }

        return lightSource->colour;
      }

      float getPower(int lightId) {
        ammonite::lighting::LightSource* lightSource = ammonite::lighting::getLightSourcePtr(lightId);
        if (lightSource == nullptr) {
          return 0.0f;
        }

        return lightSource->power;
      }

      void setGeometry(int lightId, glm::vec3 geometry) {
        ammonite::lighting::LightSource* lightSource = ammonite::lighting::getLightSourcePtr(lightId);
        if (lightSource == nullptr) {
          return;
        }

        lightSource->geometry = geometry;
      }

      void setColour(int lightId, glm::vec3 colour) {
        ammonite::lighting::LightSource* lightSource = ammonite::lighting::getLightSourcePtr(lightId);
        if (lightSource == nullptr) {
          return;
        }

        lightSource->colour = colour;
      }

      void setPower(int lightId, float power) {
        ammonite::lighting::LightSource* lightSource = ammonite::lighting::getLightSourcePtr(lightId);
        if (lightSource == nullptr) {
          return;
        }

        lightSource->power = power;
      }
    }
  }
}
