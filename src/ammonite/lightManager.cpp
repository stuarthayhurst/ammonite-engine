#include <map>

#include <GL/glew.h>
#include <glm/glm.hpp>

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
    };
  }

  namespace {
    //Lighting shader storage buffer IDs
    GLuint lightDataId = 0;

    //Default ambient light
    glm::vec3 ambientLight = glm::vec3(0.0f, 0.0f, 0.0f);

    //Track light sources
    std::map<int, lighting::LightSource> lightTrackerMap;
  }

  namespace {
    static void repackLightStorage() {
      //Data structure to pass light sources into shader
      struct ShaderLightSource {
        glm::vec4 geometry;
        glm::vec4 colour;
        glm::vec4 diffuse;
        glm::vec4 specular;
        float power[4];
      } shaderData[lightTrackerMap.size()];

      //Repack light sources into ShaderData as vec4s / float[4], without their ID
      //Packing them into 4 components is required for shader storage
      int currentLight = 0;
      for (auto const& [key, lightSource] : lightTrackerMap) {
        shaderData[currentLight].geometry = glm::vec4(lightSource.geometry, 0);
        shaderData[currentLight].colour = glm::vec4(lightSource.colour, 0);
        shaderData[currentLight].diffuse = glm::vec4(lightSource.diffuse, 0);
        shaderData[currentLight].specular = glm::vec4(lightSource.specular, 0);
        shaderData[currentLight].power[0] = lightSource.power;

        currentLight++;
      }

      //If the buffer already exists, destroy it
      if (lightDataId != 0) {
        glDeleteBuffers(1, &lightDataId);
      }

      //If no lights remain, unbind and return early
      if (lightTrackerMap.size() == 0) {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
        return;
      }

      //Add the shader data to a shader storage buffer object
      glGenBuffers(1, &lightDataId);
      glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightDataId);
      glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(shaderData), &shaderData, GL_STATIC_DRAW);
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, lightDataId);
      glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }
  }

  //Exposed light handling methods
  namespace lighting {

    int createLightSource() {
      LightSource lightSource;

      //Add light source to the tracker
      lightSource.lightId = lightTrackerMap.size() + 1;
      lightTrackerMap[lightSource.lightId] = lightSource;

      //Create new lighting buffers
      repackLightStorage();

      //Return the light source's ID
      return lightSource.lightId;
    }

    void updateLightSources() {
      repackLightStorage();
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

        //Create new lighting buffers
        repackLightStorage();
      }
    }

    void setAmbientLight(glm::vec3 newAmbientLight) {
      ambientLight = newAmbientLight;
    }

    glm::vec3 getAmbientLight() {
      return ambientLight;
    }
  }
}