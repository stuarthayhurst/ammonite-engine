#include <algorithm>
#include <iostream>
#include <iterator>
#include <map>
#include <string>
#include <unordered_map>

extern "C" {
  #include <epoxy/gl.h>
}

#include "models.hpp"

#include "../enums.hpp"
#include "../graphics/textures.hpp"
#include "../lighting/lighting.hpp"
#include "../maths/matrix.hpp"
#include "../maths/quaternion.hpp"
#include "../maths/vector.hpp"
#include "../utils/debug.hpp"
#include "../utils/id.hpp"
#include "../utils/logging.hpp"


namespace ammonite {
  namespace models {
    //Model tracker class and instances
    namespace {
      using ModelInfoMap = std::map<AmmoniteId, internal::ModelInfo>;
      using ModelPtrTrackerMap = std::unordered_map<AmmoniteId, internal::ModelInfo*>;

      ModelPtrTrackerMap modelIdPtrMap;
      bool haveModelsMoved = false;

      class ModelTracker {
      private:
        ModelInfoMap modelInfoMapSelector[2];

      public:
        unsigned int getModelCount(ModelTypeEnum modelType) {
          return modelInfoMapSelector[modelType].size();
        }

        void getModels(ModelTypeEnum modelType, unsigned int modelCount,
                       internal::ModelInfo* modelInfoArray[]) {
          //Select the right model tracker
          ModelInfoMap* modelMapPtr = &modelInfoMapSelector[modelType];

          //Fill model info array with first modelCount of items
          auto it = modelMapPtr->begin();
          modelCount = std::min(modelCount, (unsigned int)modelMapPtr->size());
          for (unsigned int i = 0; i < modelCount; i++) {
            modelInfoArray[i] = &it->second;
            std::advance(it, 1);
          }
        }

        void addModelInfo(AmmoniteId modelId, const internal::ModelInfo& modelInfo) {
          ModelInfoMap* const targetMapPtr = &modelInfoMapSelector[modelInfo.modelType];
          (*targetMapPtr)[modelId] = modelInfo;
          modelIdPtrMap[modelId] = &(*targetMapPtr)[modelId];
          haveModelsMoved = true;
        }

        void copyModelInfoFromPtr(AmmoniteId modelId, internal::ModelInfo* modelInfo) {
          ModelInfoMap* const targetMapPtr = &modelInfoMapSelector[modelInfo->modelType];
          (*targetMapPtr)[modelId] = *modelInfo;
          modelIdPtrMap[modelId] = &(*targetMapPtr)[modelId];
          haveModelsMoved = true;
        }

        void deleteModelInfo(AmmoniteId modelId) {
          //Get the type of model, so the right tracker can be selected
          const ModelTypeEnum modelType = modelIdPtrMap[modelId]->modelType;
          ModelInfoMap* targetMapPtr = &modelInfoMapSelector[modelType];

          ammoniteInternalDebug << "Deleted storage for model info (ID " \
                                << modelId << ", '" \
                                << modelIdPtrMap[modelId]->modelData->modelName \
                                << "')" << std::endl;

          //Delete the model info and id to pointer map entry
          targetMapPtr->erase(modelId);
          modelIdPtrMap.erase(modelId);
          haveModelsMoved = true;
        }

        void changeModelType(AmmoniteId modelId, ModelTypeEnum targetType) {
          //Get the type of model, so the right tracker can be selected
          const ModelTypeEnum modelType = modelIdPtrMap[modelId]->modelType;

          //Return early if no work needs to be done
          if (modelType == targetType) {
            return;
          }

          //Select current and target trackers
          ModelInfoMap* currentMapPtr = &modelInfoMapSelector[modelType];
          ModelInfoMap* const targetMapPtr = &modelInfoMapSelector[targetType];

          if (currentMapPtr->contains(modelId)) {
            (*targetMapPtr)[modelId] = (*currentMapPtr)[modelId];
            currentMapPtr->erase(modelId);

            //Update the id to pointer map
            internal::ModelInfo* const modelPtr = &(*targetMapPtr)[modelId];
            modelIdPtrMap[modelId] = modelPtr;

            //Update the type saved on the model
            modelPtr->modelType = targetType;
          }

          haveModelsMoved = true;
        }

        bool hasModel(AmmoniteId modelId) {
          //Return false if the model isn't tracked at all
          if (!modelIdPtrMap.contains(modelId)) {
            return false;
          }

          //Get the type of model, so the right tracker can be selected
          const ModelTypeEnum modelType = modelIdPtrMap[modelId]->modelType;
          const ModelInfoMap* const targetMapPtr = &modelInfoMapSelector[modelType];

          //Return whether the selected tracker holds the model
          return targetMapPtr->contains(modelId);
        }
      };

      AmmoniteId lastModelId = 0;
      ModelTracker activeModelTracker;
      ModelTracker inactiveModelTracker;
    }

    //Model movement and modification helpers
    namespace {
      void moveModelToActive(AmmoniteId modelId, internal::ModelInfo* modelPtr) {
        //Move from inactive to active tracker, update model pointer map
        const internal::ModelInfo modelInfo = *modelPtr;
        inactiveModelTracker.deleteModelInfo(modelId);
        activeModelTracker.addModelInfo(modelId, modelInfo);
        internal::setModelInfoActive(modelId, true);
      }

      void moveModelToInactive(AmmoniteId modelId, internal::ModelInfo* modelPtr) {
        //Move from active to inactive tracker, update model pointer map
        const internal::ModelInfo modelInfo = *modelPtr;
        activeModelTracker.deleteModelInfo(modelId);
        inactiveModelTracker.addModelInfo(modelId, modelInfo);
        internal::setModelInfoActive(modelId, false);
      }

      void calcModelMatrices(internal::PositionData* positionData) {
        //Recalculate the model matrix when a component changes
        ammonite::Mat<float, 4> rotationScaleMatrix = {{0}};
        ammonite::multiply(positionData->rotationMatrix, positionData->scaleMatrix,
                           rotationScaleMatrix);
        ammonite::multiply(positionData->translationMatrix, rotationScaleMatrix,
                           positionData->modelMatrix);

        //Normal matrix
        ammonite::Mat<float, 4> inverseModelMatrix = {{0}};
        ammonite::inverse(positionData->modelMatrix, inverseModelMatrix);
        ammonite::transpose(inverseModelMatrix);
        ammonite::copy(inverseModelMatrix, positionData->normalMatrix);
      }
    }

    //Internally exposed model handling functions
    namespace internal {
      unsigned int getModelCount(ModelTypeEnum modelType) {
        return activeModelTracker.getModelCount(modelType);
      }

      void getModels(ModelTypeEnum modelType, unsigned int modelCount,
                     ModelInfo* modelInfoArray[]) {
        activeModelTracker.getModels(modelType, modelCount, modelInfoArray);
      }

      ModelInfo* getModelPtr(AmmoniteId modelId) {
        //Check the model exists, and return a pointer
        if (modelIdPtrMap.contains(modelId)) {
          return modelIdPtrMap[modelId];
        }

        return nullptr;
      }

      bool* getModelsMovedPtr() {
        return &haveModelsMoved;
      }

      void setLightEmitterId(AmmoniteId modelId, AmmoniteId lightEmitterId) {
        //Select the right tracker
        ModelTracker* selectedTracker = &inactiveModelTracker;
        ModelInfo* modelPtr = modelIdPtrMap[modelId];
        if (modelPtr->drawMode != AMMONITE_DRAW_INACTIVE) {
          selectedTracker = &activeModelTracker;
        }

        //Move model to different sub-tracker and update pointer
        if (lightEmitterId != 0) {
          selectedTracker->changeModelType(modelId, AMMONITE_LIGHT_EMITTER);
        } else {
          selectedTracker->changeModelType(modelId, AMMONITE_MODEL);
        }
        modelPtr = modelIdPtrMap[modelId];

        //Set light emitter ID property
        if (modelPtr != nullptr) {
          modelPtr->lightEmitterId = lightEmitterId;
        }
      }

      AmmoniteId getLightEmitterId(AmmoniteId modelId) {
        const ModelInfo* const modelPtr = modelIdPtrMap[modelId];
        if (modelPtr != nullptr) {
          return modelPtr->lightEmitterId;
        }
        return 0;
      }
    }

    //Exposed model handling functions
    AmmoniteId createModel(const std::string& objectPath, bool flipTexCoords,
                           bool srgbTextures) {
      //Generate info required to load model
      internal::ModelLoadInfo modelLoadInfo;
      modelLoadInfo.flipTexCoords = flipTexCoords;
      modelLoadInfo.srgbTextures = srgbTextures;
      modelLoadInfo.modelDirectory = objectPath.substr(0, objectPath.find_last_of('/'));

      //Create the model info entry
      internal::ModelInfo modelInfo;
      modelInfo.modelId = utils::internal::setNextId(&lastModelId, modelIdPtrMap);

      //Either reuse or load model from scratch
      modelInfo.modelData = internal::addModelData(objectPath, modelLoadInfo,
                                                   modelInfo.modelId);
      if (modelInfo.modelData == nullptr) {
        return 0;
      }

      //Apply default texture IDs per mesh
      modelInfo.textureIds = modelInfo.modelData->textureIds;
      for (const internal::TextureIdGroup& textureGroup : modelInfo.textureIds) {
        if (textureGroup.diffuseId != 0) {
          ammonite::textures::internal::copyTexture(textureGroup.diffuseId);
        }

        if (textureGroup.specularId != 0) {
          ammonite::textures::internal::copyTexture(textureGroup.specularId);
        }
      }

      //Initialise position data
      ammonite::identity(modelInfo.positionData.translationMatrix);
      ammonite::identity(modelInfo.positionData.scaleMatrix);
      ammonite::identity(modelInfo.positionData.rotationMatrix);
      ammonite::fromEuler(modelInfo.positionData.rotationQuat, 0.0f, 0.0f, 0.0f);

      //Calculate model and normal matrices
      calcModelMatrices(&modelInfo.positionData);

      //Add model to the tracker and return the ID
      activeModelTracker.addModelInfo(modelInfo.modelId, modelInfo);
      return modelInfo.modelId;
    }

    AmmoniteId createModel(const std::string& objectPath) {
      return createModel(objectPath, ASSUME_FLIP_MODEL_UVS, ASSUME_SRGB_TEXTURES);
    }

    AmmoniteId copyModel(AmmoniteId modelId, bool preserveDrawMode) {
      //Get the model and check it exists
      internal::ModelInfo* const existingModelInfo = modelIdPtrMap[modelId];
      if (existingModelInfo == nullptr) {
        return 0;
      }

      //Add model info to the correct tracker via pointer
      const AmmoniteId newModelId = utils::internal::setNextId(&lastModelId, modelIdPtrMap);
      if (!preserveDrawMode || existingModelInfo->drawMode != AMMONITE_DRAW_INACTIVE) {
        //Active tracker for reset mode or an already active mode
        activeModelTracker.copyModelInfoFromPtr(newModelId, existingModelInfo);
      } else {
        //Inactive tracker for preserving the inactive mode
        inactiveModelTracker.copyModelInfoFromPtr(newModelId, existingModelInfo);
      }

      //Update model data storage
      internal::ModelInfo* const newModelInfo = modelIdPtrMap[newModelId];
      internal::copyModelData(existingModelInfo->modelData->modelName, newModelId);

      //Reset the draw mode unless asked to preserve it
      if (!preserveDrawMode) {
        newModelInfo->drawMode = AMMONITE_DRAW_ACTIVE;
      }

      //Increase texture reference counters
      for (const internal::TextureIdGroup& textureGroup : newModelInfo->textureIds) {
        if (textureGroup.diffuseId != 0) {
          ammonite::textures::internal::copyTexture(textureGroup.diffuseId);
        }

        if (textureGroup.specularId != 0) {
          ammonite::textures::internal::copyTexture(textureGroup.specularId);
        }
      }

      //Correct its ID and light linking
      newModelInfo->modelId = newModelId;
      newModelInfo->lightEmitterId = 0;

      //Return the new ID
      return newModelId;
    }

    void deleteModel(AmmoniteId modelId) {
      //Check the model actually exists
      if (modelIdPtrMap.contains(modelId)) {
        internal::ModelInfo* const modelInfo = modelIdPtrMap[modelId];

        //Release textures
        for (const internal::TextureIdGroup& textureGroup : modelInfo->textureIds) {
          if (textureGroup.diffuseId != 0) {
            ammonite::textures::internal::deleteTexture(textureGroup.diffuseId);
          }

          if (textureGroup.specularId != 0) {
            ammonite::textures::internal::deleteTexture(textureGroup.specularId);
          }
        }

        //Unlink any attached light source
        ammonite::lighting::internal::unlinkByModel(modelId);

        /*
         - Remove the model info from the tracker, before the data is deleted
         - Copy the modelName for future use
        */
        const std::string modelName = modelInfo->modelData->modelName;
        if (activeModelTracker.hasModel(modelId)) {
          activeModelTracker.deleteModelInfo(modelId);
        } else if (inactiveModelTracker.hasModel(modelId)) {
          inactiveModelTracker.deleteModelInfo(modelId);
        } else {
          ammonite::utils::warning << "Failed to delete model info (ID " \
                                   << modelId << ")" << std::endl;
        }

        //Reduce reference count and possibly delete model data
        if (!internal::deleteModelData(modelName, modelId)) {
          ammonite::utils::warning << "Failed to delete model data (ID " \
                                   << modelId << ")" << std::endl;
        }
      }
    }

    bool applyTexture(AmmoniteId modelId, AmmoniteTextureEnum textureType,
                      const std::string& texturePath, bool srgbTexture) {
      internal::ModelInfo* const modelInfoPtr = modelIdPtrMap[modelId];
      if (modelInfoPtr == nullptr) {
        return false;
      }

      //Apply texture to every mesh on the model
      for (internal::TextureIdGroup& textureIdGroup : modelInfoPtr->textureIds) {
        GLuint* textureIdPtr = nullptr;
        if (textureType == AMMONITE_DIFFUSE_TEXTURE) {
          textureIdPtr = &textureIdGroup.diffuseId;
        } else if (textureType == AMMONITE_SPECULAR_TEXTURE) {
          textureIdPtr = &textureIdGroup.specularId;
        } else {
          ammonite::utils::warning << "Invalid texture type specified" << std::endl;
          return false;
        }

        //If a texture is already applied, remove it
        if (*textureIdPtr != 0) {
          ammonite::textures::internal::deleteTexture(*textureIdPtr);
          *textureIdPtr = 0;
        }

        //Create new texture and apply to the mesh
        const GLuint textureId = ammonite::textures::internal::loadTexture(texturePath,
          false, srgbTexture);
        if (textureId == 0) {
          return false;
        }

        *textureIdPtr = textureId;
      }

      return true;
    }

    bool applyTexture(AmmoniteId modelId, AmmoniteTextureEnum textureType,
                      const std::string& texturePath) {
      return applyTexture(modelId, textureType, texturePath, ASSUME_SRGB_TEXTURES);
    }

    //Return the number of indices on a model
    unsigned int getIndexCount(AmmoniteId modelId) {
      const internal::ModelInfo* const modelPtr = modelIdPtrMap[modelId];
      if (modelPtr == nullptr) {
        return 0;
      }

      //Sum indices between all meshes
      unsigned int indexCount = 0;
      for (const internal::MeshInfoGroup& meshInfo : modelPtr->modelData->meshInfo) {
        indexCount += meshInfo.indexCount;
      }

      return indexCount;
    }

    //Return the number of vertices on a model
    unsigned int getVertexCount(AmmoniteId modelId) {
      const internal::ModelInfo* const modelPtr = modelIdPtrMap[modelId];
      if (modelPtr == nullptr) {
        return 0;
      }

      //Sum vertices between all meshes
      unsigned int vertexCount = 0;
      for (const internal::MeshInfoGroup& meshInfo : modelPtr->modelData->meshInfo) {
        vertexCount += meshInfo.vertexCount;
      }

      return vertexCount;
    }

    void setDrawMode(AmmoniteId modelId, AmmoniteDrawEnum drawMode) {
      internal::ModelInfo* const modelPtr = modelIdPtrMap[modelId];
      if (modelPtr != nullptr) {
        if (modelPtr->drawMode == AMMONITE_DRAW_INACTIVE &&
            drawMode != AMMONITE_DRAW_INACTIVE) {
          //Move from inactive to active tracker
          moveModelToActive(modelId, modelPtr);
        } else if (modelPtr->drawMode != AMMONITE_DRAW_INACTIVE &&
                   drawMode == AMMONITE_DRAW_INACTIVE) {
          //Move from active to inactive tracker
          moveModelToInactive(modelId, modelPtr);
        }

        //Update draw mode
        modelIdPtrMap[modelId]->drawMode = drawMode;
      }
    }

    //Return position, scale and rotation of a model
    namespace position {
      void getPosition(AmmoniteId modelId, ammonite::Vec<float, 3>& position) {
        //Get the model and check it exists
        const models::internal::ModelInfo* const modelInfo = modelIdPtrMap[modelId];
        if (modelInfo == nullptr) {
          ammonite::set(position, 0.0f);
          return;
        }

        const ammonite::Vec<float, 4> origin = {0.0f, 0.0f, 0.0f, 1.0f};
        ammonite::Vec<float, 4> rawPosition = {0};
        ammonite::multiply(modelInfo->positionData.translationMatrix, origin, rawPosition);
        ammonite::copy(rawPosition, position);
      }

      void getScale(AmmoniteId modelId, ammonite::Vec<float, 3>& scale) {
        //Get the model and check it exists
        const models::internal::ModelInfo* const modelInfo = modelIdPtrMap[modelId];
        if (modelInfo == nullptr) {
          ammonite::set(scale, 0.0f);
          return;
        }

        const ammonite::Vec<float, 4> ones = {1.0f, 1.0f, 1.0f, 1.0f};
        ammonite::Vec<float, 4> rawScale = {0};
        ammonite::multiply(modelInfo->positionData.scaleMatrix, ones, rawScale);
        ammonite::copy(rawScale, scale);
      }

      //Return rotation, in radians
      void getRotation(AmmoniteId modelId, ammonite::Vec<float, 3>& rotation) {
        //Get the model and check it exists
        const models::internal::ModelInfo* const modelInfo = modelIdPtrMap[modelId];
        if (modelInfo == nullptr) {
          ammonite::set(rotation, 0.0f);
          return;
        }

        ammonite::toEuler(modelInfo->positionData.rotationQuat, rotation);
      }
    }

    //Set absolute position, scale and rotation of models
    namespace position {
      void setPosition(AmmoniteId modelId, const ammonite::Vec<float, 3>& position) {
        //Get the model and check it exists
        models::internal::ModelInfo* const modelInfo = modelIdPtrMap[modelId];
        if (modelInfo == nullptr) {
          return;
        }

        //Set the position
        ammonite::Mat<float, 4> identityMat = {{0}};
        ammonite::identity(identityMat);
        ammonite::translate(identityMat, position, modelInfo->positionData.translationMatrix);

        if (modelInfo->lightEmitterId != 0) {
          ammonite::lighting::internal::setLightSourcesChanged();
        }

        //Recalculate model and normal matrices
        calcModelMatrices(&modelInfo->positionData);
      }

      void setScale(AmmoniteId modelId, const ammonite::Vec<float, 3>& scale) {
        //Get the model and check it exists
        models::internal::ModelInfo* const modelInfo = modelIdPtrMap[modelId];
        if (modelInfo == nullptr) {
          return;
        }

        //Set the scale
        ammonite::Mat<float, 4> identityMat = {{0}};
        ammonite::identity(identityMat);
        ammonite::scale(identityMat, scale, modelInfo->positionData.scaleMatrix);

        if (modelInfo->lightEmitterId != 0) {
          ammonite::lighting::internal::setLightSourcesChanged();
        }

        //Recalculate model and normal matrices
        calcModelMatrices(&modelInfo->positionData);
      }

      void setScale(AmmoniteId modelId, float scaleMultiplier) {
        const ammonite::Vec<float, 3> scale = {scaleMultiplier, scaleMultiplier, scaleMultiplier};
        setScale(modelId, scale);
      }

      //Rotation, in radians
      void setRotation(AmmoniteId modelId, const ammonite::Vec<float, 3>& rotation) {
        //Get the model and check it exists
        models::internal::ModelInfo* const modelInfo = modelIdPtrMap[modelId];
        if (modelInfo == nullptr) {
          return;
        }

        //Set the rotation
        ammonite::fromEuler(modelInfo->positionData.rotationQuat, rotation);
        ammonite::toMatrix(modelInfo->positionData.rotationQuat,
                           modelInfo->positionData.rotationMatrix);

        if (modelInfo->lightEmitterId != 0) {
          ammonite::lighting::internal::setLightSourcesChanged();
        }

        //Recalculate model and normal matrices
        calcModelMatrices(&modelInfo->positionData);
      }
    }

    //Translate, scale and rotate models
    namespace position {
      void translateModel(AmmoniteId modelId, const ammonite::Vec<float, 3>& translation) {
        //Get the model and check it exists
        models::internal::ModelInfo* const modelInfo = modelIdPtrMap[modelId];
        if (modelInfo == nullptr) {
          return;
        }

        //Translate the matrix
        ammonite::translate(modelInfo->positionData.translationMatrix, translation);

        if (modelInfo->lightEmitterId != 0) {
          ammonite::lighting::internal::setLightSourcesChanged();
        }

        //Recalculate model and normal matrices
        calcModelMatrices(&modelInfo->positionData);
      }

      void scaleModel(AmmoniteId modelId, const ammonite::Vec<float, 3>& scale) {
        //Get the model and check it exists
        models::internal::ModelInfo* const modelInfo = modelIdPtrMap[modelId];
        if (modelInfo == nullptr) {
          return;
        }

        //Scale the matrix
        ammonite::scale(modelInfo->positionData.scaleMatrix, scale);

        if (modelInfo->lightEmitterId != 0) {
          ammonite::lighting::internal::setLightSourcesChanged();
        }

        //Recalculate model and normal matrices
        calcModelMatrices(&modelInfo->positionData);
      }

      void scaleModel(AmmoniteId modelId, float scaleMultiplier) {
        const ammonite::Vec<float, 3> scale = {scaleMultiplier, scaleMultiplier, scaleMultiplier};
        scaleModel(modelId, scale);
      }

      //Rotation, in radians
      void rotateModel(AmmoniteId modelId, const ammonite::Vec<float, 3>& rotation) {
        //Get the model and check it exists
        models::internal::ModelInfo* const modelInfo = modelIdPtrMap[modelId];
        if (modelInfo == nullptr) {
          return;
        }

        //Rotate the matrix
        ammonite::Quat<float> newRotation = {{0}};
        ammonite::fromEuler(newRotation, rotation);
        ammonite::multiply(newRotation, modelInfo->positionData.rotationQuat,
                           modelInfo->positionData.rotationQuat);
        ammonite::toMatrix(modelInfo->positionData.rotationQuat,
                           modelInfo->positionData.rotationMatrix);

        if (modelInfo->lightEmitterId != 0) {
          ammonite::lighting::internal::setLightSourcesChanged();
        }

        //Recalculate model and normal matrices
        calcModelMatrices(&modelInfo->positionData);
      }
    }
  }
}
