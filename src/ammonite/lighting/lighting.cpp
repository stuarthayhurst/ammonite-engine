#include <iostream>
#include <iterator>
#include <unordered_map>

extern "C" {
  #include <epoxy/gl.h>
}

#include "lighting.hpp"

#include "../graphics/buffers.hpp"
#include "../graphics/renderer.hpp"
#include "../maths/angle.hpp"
#include "../maths/matrix.hpp"
#include "../maths/vector.hpp"
#include "../models/models.hpp"
#include "../utils/debug.hpp"
#include "../utils/id.hpp"

namespace ammonite {
  namespace lighting {
    namespace {
      //Default ambient light
      ammonite::Vec<float, 3> ambientLight {0.0f, 0.0f, 0.0f};

      //Track light sources
      std::unordered_map<AmmoniteId, lighting::internal::LightSource> lightTrackerMap;
      unsigned int prevLightCount = 0;
      bool lightSourcesChanged = false;
      AmmoniteId lastLightId = 0;

      //Data structures to match light sources in the shader
      using ShaderLightSource = ammonite::Vec<float, 4>[3];
      using ShaderShadowTransform = ammonite::Mat<float, 4>[6];
      ShaderLightSource* shaderLightData = nullptr;
      ShaderShadowTransform* shaderShadowData = nullptr;
    }

    namespace {
      void packLight(unsigned int index, const ammonite::Mat<float, 4>& shadowProj) {
        //Repacking light sources
        auto lightIt = lightTrackerMap.begin();
        std::advance(lightIt, index);
        internal::LightSource* const lightSource = &lightIt->second;
        lightSource->lightIndex = index;

        //Override position for light emitting models, and add to tracker
        if (lightSource->modelId != 0) {
          //Override light position, using linked model
          ammonite::models::position::getPosition(lightSource->modelId, lightSource->geometry);

          //Update lightIndex for rendering light emitting models
          auto* modelPtr = ammonite::models::internal::getModelPtr(lightSource->modelId);
          modelPtr->lightIndex = lightSource->lightIndex;
        }

        //Repack lighting information
        ammonite::set(shaderLightData[index][0], lightSource->geometry, 0.0f);
        ammonite::set(shaderLightData[index][1], lightSource->diffuse, 0.0f);
        ammonite::set(shaderLightData[index][2], lightSource->specular, lightSource->power);

        const ammonite::Vec<float, 3> targetVectors[6] = {
          {1.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f},
          {0.0f, 1.0f, 0.0f}, {0.0f, -1.0f, 0.0f},
          {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}
        };

        const ammonite::Vec<float, 3> upVectors[6] = {
          {0.0f, -1.0f, 0.0f}, {0.0f, -1.0f, 0.0f},
          {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, -1.0f},
          {0.0f, -1.0f, 0.0f}, {0.0f, -1.0f, 0.0f}
        };

        //Calculate shadow transform matrices
        for (int matIndex = 0; matIndex < 6; matIndex++) {
          ammonite::Vec<float, 3> targetPosition = {0};
          ammonite::add(lightSource->geometry, targetVectors[matIndex], targetPosition);

          ammonite::Mat<float, 4> transformMat = {{0}};
          ammonite::lookAt(lightSource->geometry, targetPosition, upVectors[matIndex], transformMat);
          ammonite::multiply(shadowProj, transformMat, shaderShadowData[lightSource->lightIndex][matIndex]);
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

      void freeLightBuffers() {
        if (shaderLightData != nullptr) {
          delete [] shaderLightData;
          delete [] shaderShadowData;
        }
      }
    }

    //Internally exposed light handling methods
    namespace internal {
      void destroyLightSystem() {
        //Destroy the GPU buffers
        graphics::internal::deleteLightBuffers();
        prevLightCount = 0;
        lightSourcesChanged = false;

        //Destroy the CPU buffers
        freeLightBuffers();
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

      //Rebuild the lighting buffers if required
      void updateLightSources() {
        //If lights haven't changed, skip
        if (!lightSourcesChanged) {
          return;
        }

        //If no lights remain, unbind and delete the buffers
        const unsigned int lightCount = lightTrackerMap.size();
        if (lightCount == 0) {
          destroyLightSystem();
          return;
        }

        ammonite::Mat<float, 4> shadowProj = {{0}};
        const float shadowFarPlane = renderer::settings::getShadowFarPlane();
        ammonite::perspective(ammonite::radians(90.0f), 1.0f, 0.0f, shadowFarPlane, shadowProj);

        //Resize CPU buffers if the size has changed
        const bool resizeBuffers = (prevLightCount != lightCount);
        if (resizeBuffers) {
          freeLightBuffers();
          shaderLightData = new ShaderLightSource[lightCount];
          shaderShadowData = new ShaderShadowTransform[lightCount];
        }

        //Repack light sources into buffers (uses vec4s for OpenGL)
        for (unsigned int i = 0; i < lightCount; i++) {
          packLight(i, shadowProj);
        }

        //Send the buffer to the GPU
        const unsigned int lightDataSize = sizeof(ShaderLightSource) * lightCount;
        const unsigned int shadowDataSize = sizeof(ShaderShadowTransform) * lightCount;
        graphics::internal::uploadLightBuffers(shaderLightData, lightDataSize,
                                               shaderShadowData, shadowDataSize);

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
      internal::LightSource* const lightSource = &lightTrackerMap[lightId];
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
      internal::LightSource* const lightSource = &lightTrackerMap[lightId];
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
        ammoniteInternalDebug << "Deleted storage for light source (ID " \
                              << lightId << ")" << std::endl;
      }

      if (lightTrackerMap.empty()) {
        freeLightBuffers();

        shaderLightData = nullptr;
        shaderShadowData = nullptr;
      }

      lightSourcesChanged = true;
    }

    void setAmbientLight(const ammonite::Vec<float, 3>& ambient) {
      ammonite::copy(ambient, ambientLight);
    }

    void getAmbientLight(ammonite::Vec<float, 3>& ambient) {
      ammonite::copy(ambientLight, ambient);
    }


    //Exposed functions to modify light properties
    namespace properties {
      void getGeometry(AmmoniteId lightId, ammonite::Vec<float, 3>& geometry) {
        const internal::LightSource* const lightSource = getLightSourcePtr(lightId);
        if (lightSource == nullptr) {
          ammonite::set(geometry, 0.0f);
          return;
        }

        ammonite::copy(lightSource->geometry, geometry);
      }

      void getColour(AmmoniteId lightId, ammonite::Vec<float, 3>& colour) {
        const internal::LightSource* const lightSource = getLightSourcePtr(lightId);
        if (lightSource == nullptr) {
          ammonite::set(colour, 0.0f);
          return;
        }

        ammonite::copy(lightSource->diffuse, colour);
      }

      float getPower(AmmoniteId lightId) {
        const internal::LightSource* const lightSource = getLightSourcePtr(lightId);
        if (lightSource == nullptr) {
          return 0.0f;
        }

        return lightSource->power;
      }

      void setGeometry(AmmoniteId lightId, const ammonite::Vec<float, 3>& geometry) {
        internal::LightSource* const lightSource = getLightSourcePtr(lightId);
        if (lightSource != nullptr) {
          ammonite::copy(geometry, lightSource->geometry);
          lightSourcesChanged = true;
        }
      }

      void setColour(AmmoniteId lightId, const ammonite::Vec<float, 3>& colour) {
        internal::LightSource* const lightSource = getLightSourcePtr(lightId);
        if (lightSource != nullptr) {
          ammonite::copy(colour, lightSource->diffuse);
          lightSourcesChanged = true;
        }
      }

      void setPower(AmmoniteId lightId, float power) {
        internal::LightSource* const lightSource = getLightSourcePtr(lightId);
        if (lightSource != nullptr) {
          lightSource->power = power;
          lightSourcesChanged = true;
        }
      }
    }
  }
}
