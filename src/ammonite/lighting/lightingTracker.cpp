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
#include "../utils/logging.hpp"

namespace ammonite {
  namespace lighting {
    namespace {
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
          ammonite::models::position::getPosition(lightSource->modelId, lightSource->position);

          //Update lightIndex for rendering light emitting models
          auto* modelPtr = ammonite::models::internal::getModelPtr(lightSource->modelId);
          modelPtr->lightIndex = lightSource->lightIndex;
        }

        //Repack lighting information
        ammonite::set(shaderLightData[index][0], lightSource->position, 0.0f);
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
          ammonite::add(lightSource->position, targetVectors[matIndex], targetPosition);

          ammonite::Mat<float, 4> transformMat = {{0}};
          ammonite::lookAt(lightSource->position, targetPosition, upVectors[matIndex], transformMat);
          ammonite::multiply(shadowProj, transformMat, shaderShadowData[lightSource->lightIndex][matIndex]);
        }
      }

      void freeLightBuffers() {
        if (shaderLightData != nullptr) {
          delete [] shaderLightData;
          delete [] shaderShadowData;

          shaderLightData = nullptr;
          shaderShadowData = nullptr;
        }
      }
    }

    //Internally exposed light handling methods
    namespace internal {
      //Pointer is only valid until lightTrackerMap is modified
      LightSource* getLightSourcePtr(AmmoniteId lightId) {
        //Check the light source exists, and return a pointer
        if (lightTrackerMap.contains(lightId)) {
          return &lightTrackerMap[lightId];
        }

        return nullptr;
      }

      void destroyLightSystem() {
        //Destroy the GPU buffers
        graphics::internal::deleteLightBuffers();
        prevLightCount = 0;
        lightSourcesChanged = false;

        //Destroy the CPU buffers
        freeLightBuffers();
      }

      /*
       - Unlink a light source from a model, using only the model ID
       - This doesn't touch the model's structures
         - Only use this when the model will be immediately destroyed after,
           or when a new light will be manually linked
         - This is behaviour useful for model destruction, since the model's data
           won't be shuffled around to match its state change
      */
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
      //Check that the light source exists
      if (!lightTrackerMap.contains(lightId)) {
        ammonite::utils::warning << "Can't link light ID '" << lightId \
                                 << "', it doesn't exist" << std::endl;
      }

      //Check that the model exists
      if (models::internal::getModelPtr(modelId) == nullptr) {
        ammonite::utils::warning << "Can't link to model ID '" << modelId \
                                 << "', it doesn't exist" << std::endl;
      }

      //Remove any light source's attachment to the model
      ammonite::lighting::internal::unlinkByModel(modelId);

      //Remove any model attached to the light source
      unlinkModel(lightId);

      //Link the light source and model together
      lightTrackerMap[lightId].modelId = modelId;
      ammonite::models::internal::setLightEmitterId(modelId, lightId);
      lightSourcesChanged = true;
    }

    void unlinkModel(AmmoniteId lightId) {
      //Check that the light source exists, then fetch it
      if (!lightTrackerMap.contains(lightId)) {
        ammonite::utils::warning << "Can't unlink light ID '" << lightId \
                                 << "', it doesn't exist" << std::endl;
      }
      internal::LightSource* const lightSource = &lightTrackerMap[lightId];

      //Unlink any attached model from the light source
      if (lightSource->modelId != 0) {
        ammonite::models::internal::setLightEmitterId(lightSource->modelId, 0);
        lightSource->modelId = 0;
        lightSourcesChanged = true;
      }
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
  }
}
