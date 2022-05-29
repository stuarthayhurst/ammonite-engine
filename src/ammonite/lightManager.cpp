#include <map>
#include <cmath>
#include <vector>

#include <GL/glew.h>
#include <glm/glm.hpp>

#include <omp.h>
#include <thread>

#include "internal/modelTracker.hpp"

#ifdef DEBUG
  #include <iostream>
#endif

namespace ammonite {
  namespace lighting {
    struct LightSource {
      glm::vec3 geometry = glm::vec3(0.0f, 0.0f, 0.0f);
      glm::vec3 colour = glm::vec3(1.0f, 1.0f, 1.0f);
      glm::vec3 diffuse = glm::vec3(1.0f, 1.0f, 1.0f);
      glm::vec3 specular = glm::vec3(0.3f, 0.3f, 0.3f);
      float power = 1.0f;
      int lightId;
      int modelId = -1;
    };
  }

  namespace {
    //Lighting shader storage buffer IDs
    GLuint lightDataId = 0;

    //Default ambient light
    glm::vec3 ambientLight = glm::vec3(0.0f, 0.0f, 0.0f);

    //Track light sources
    std::map<int, lighting::LightSource> lightTrackerMap;
    unsigned int prevLightCount = 0;

    //Track light emitting models
    std::vector<int> lightEmitterData;
  }

  namespace lighting {
    //Return data on light emitting models
    void getLightEmitters(int* lightCount, std::vector<int>* lightData) {
      *lightCount = lightEmitterData.size() / 2;

      //Fill passed vector with data
      for (unsigned int i = 0; i < lightEmitterData.size(); i++) {
        lightData->push_back(lightEmitterData[i]);
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

        shaderData[i].geometry = glm::vec4(lightSource.geometry, 0);
        shaderData[i].colour = glm::vec4(lightSource.colour, 0);
        shaderData[i].diffuse = glm::vec4(lightSource.diffuse, 0);
        shaderData[i].specular = glm::vec4(lightSource.specular, 0);
        shaderData[i].power[0] = lightSource.power;

        //Track all light emitting models
        if (lightSource.modelId != -1) {
          lightEmitterData.push_back(lightSource.modelId);
          lightEmitterData.push_back(i);
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
      lightSource.lightId = lightTrackerMap.size() + 1;
      lightTrackerMap[lightSource.lightId] = lightSource;

      //Return the light source's ID
      return lightSource.lightId;
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

    void deleteLightSource(int lightId) {
      //Check the light source exists
      auto it = lightTrackerMap.find(lightId);
      if (it != lightTrackerMap.end()) {
        //Remove the light source from the tracker
        lightTrackerMap.erase(lightId);
      }
    }

    void linkModel(int lightId, int modelId) {
      ammonite::lighting::LightSource* lightSource = ammonite::lighting::getLightSourcePtr(lightId);
      lightSource->modelId = modelId;
      ammonite::models::setLightEmitting(modelId, true);
    }

    void unlinkModel(int lightId, int modelId) {
      ammonite::lighting::LightSource* lightSource = ammonite::lighting::getLightSourcePtr(lightId);
      lightSource->modelId = -1;
      ammonite::models::setLightEmitting(modelId, false);
    }

    void setAmbientLight(glm::vec3 newAmbientLight) {
      ambientLight = newAmbientLight;
    }

    glm::vec3 getAmbientLight() {
      return ambientLight;
    }
  }
}
