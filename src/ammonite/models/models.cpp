#include <cstddef>
#include <iostream>
#include <iterator>
#include <map>
#include <string>
#include <algorithm>

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include "models.hpp"

#include "../enums.hpp"
#include "../graphics/textures.hpp"
#include "../lighting/lighting.hpp"
#include "../types.hpp"
#include "../utils/id.hpp"
#include "../utils/logging.hpp"

//Class definitions
namespace {
  using ModelTrackerMap = std::map<AmmoniteId, ammonite::models::internal::ModelInfo>;
  using ModelPtrTrackerMap = std::map<AmmoniteId, ammonite::models::internal::ModelInfo*>;
  using ModelDataMap = std::map<std::string, ammonite::models::internal::ModelData>;

  bool haveModelsMoved = false;

  class ModelTracker {
  private:
    ModelTrackerMap modelTrackerMap;
    ModelTrackerMap lightTrackerMap;
    ModelPtrTrackerMap* modelIdPtrMapPtr;

    std::map<AmmoniteModelEnum, ModelTrackerMap*> modelSelector = {
      {AMMONITE_MODEL, &modelTrackerMap},
      {AMMONITE_LIGHT_EMITTER, &lightTrackerMap}
    };

  public:
    ModelTracker(ModelPtrTrackerMap* modelIdPtrMapAddr): modelIdPtrMapPtr(modelIdPtrMapAddr) {}

    unsigned int getModelCount(AmmoniteModelEnum modelType) {
      return modelSelector[modelType]->size();
    }

    void getModels(AmmoniteModelEnum modelType, unsigned int modelCount,
                   ammonite::models::internal::ModelInfo* modelArr[]) {
      //Select the right model tracker
      ModelTrackerMap* modelMapPtr = modelSelector[modelType];

      //Fill modelArr with first number modelCount of items
      auto it = modelMapPtr->begin();
      modelCount = std::min(modelCount, (unsigned int)modelMapPtr->size());
      for (unsigned int i = 0; i < modelCount; i++) {
        modelArr[i] = &it->second;
        std::advance(it, 1);
      }
    }

    void addModel(AmmoniteId modelId, const ammonite::models::internal::ModelInfo& modelObject) {
      ModelTrackerMap* targetMapPtr = modelSelector[modelObject.modelType];
      (*targetMapPtr)[modelId] = modelObject;
      (*modelIdPtrMapPtr)[modelId] = &(*targetMapPtr)[modelId];
      haveModelsMoved = true;
    }

    void copyModelFromPtr(AmmoniteId modelId, ammonite::models::internal::ModelInfo* modelObject) {
      ModelTrackerMap* targetMapPtr = modelSelector[modelObject->modelType];
      (*targetMapPtr)[modelId] = *modelObject;
      (*modelIdPtrMapPtr)[modelId] = &(*targetMapPtr)[modelId];
      haveModelsMoved = true;
    }

    void deleteModel(AmmoniteId modelId) {
      //Get the type of model, so the right tracker can be selected
      const AmmoniteModelEnum modelType = (*modelIdPtrMapPtr)[modelId]->modelType;
      ModelTrackerMap* targetMapPtr = modelSelector[modelType];

      //Delete the model and id to pointer map entry
      targetMapPtr->erase(modelId);
      modelIdPtrMapPtr->erase(modelId);
      haveModelsMoved = true;
    }

    void changeModelType(AmmoniteId modelId, AmmoniteModelEnum targetType) {
      //Get the type of model, so the right tracker can be selected
      const AmmoniteModelEnum modelType = (*modelIdPtrMapPtr)[modelId]->modelType;

      //Return early if no work needs to be done
      if (modelType == targetType) {
        return;
      }

      //Select current and target trackers
      ModelTrackerMap* currentMapPtr = modelSelector[modelType];
      ModelTrackerMap* targetMapPtr = modelSelector[targetType];

      if (currentMapPtr->contains(modelId)) {
        (*targetMapPtr)[modelId] = (*currentMapPtr)[modelId];
        currentMapPtr->erase(modelId);

        //Update the id to pointer map
        ammonite::models::internal::ModelInfo* modelPtr = &(*targetMapPtr)[modelId];
        (*modelIdPtrMapPtr)[modelId] = modelPtr;

        //Update the type saved on the model
        modelPtr->modelType = targetType;
      }

      haveModelsMoved = true;
    }

    bool hasModel(AmmoniteId modelId) {
      //Return false if the model isn't tracked at all
      if (!modelIdPtrMapPtr->contains(modelId)) {
        return false;
      }

      //Get the type of model, so the right tracker can be selected
      const AmmoniteModelEnum modelType = (*modelIdPtrMapPtr)[modelId]->modelType;
      const ModelTrackerMap* targetMapPtr = modelSelector[modelType];

      //Return whether the selected tracker holds the model
      return targetMapPtr->contains(modelId);
    }
  };
}

namespace ammonite {
  namespace {
    //Track all loaded models
    ModelDataMap modelDataMap;
    ModelPtrTrackerMap modelIdPtrMap;
    AmmoniteId lastModelId = 0;

    ModelTracker activeModelTracker(&modelIdPtrMap);
    ModelTracker inactiveModelTracker(&modelIdPtrMap);
  }

  //Internally exposed model handling methods
  namespace models {
    namespace internal {
      unsigned int getModelCount(AmmoniteModelEnum modelType) {
        return activeModelTracker.getModelCount(modelType);
      }

      void getModels(AmmoniteModelEnum modelType, unsigned int modelCount,
                     ammonite::models::internal::ModelInfo* modelArr[]) {
        activeModelTracker.getModels(modelType, modelCount, modelArr);
      }

      internal::ModelInfo* getModelPtr(AmmoniteId modelId) {
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
        internal::ModelInfo* modelPtr = modelIdPtrMap[modelId];
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
        const internal::ModelInfo* modelPtr = modelIdPtrMap[modelId];
        if (modelPtr != nullptr) {
          return modelPtr->lightEmitterId;
        }
        return 0;
      }
    }
  }

  namespace {
    void createBuffers(models::internal::ModelData* modelObjectData) {
      //Generate buffers for every mesh
      for (unsigned int i = 0; i < modelObjectData->meshes.size(); i++) {
        models::internal::MeshData* meshData = &modelObjectData->meshes[i];

        //Create vertex and index buffers
        glCreateBuffers(1, &meshData->vertexBufferId);
        glCreateBuffers(1, &meshData->elementBufferId);

        //Fill interleaved vertex + normal + texture buffer and index buffer
        glNamedBufferData(meshData->vertexBufferId,
                          meshData->vertexCount * (long)sizeof(models::internal::VertexData),
                          &meshData->meshData[0], GL_STATIC_DRAW);
        glNamedBufferData(meshData->elementBufferId,
                          meshData->indexCount * (long)sizeof(unsigned int),
                          &meshData->indices[0], GL_STATIC_DRAW);

        //Destroy mesh data early
        delete [] meshData->meshData;
        delete [] meshData->indices;
        meshData->meshData = nullptr;
        meshData->indices = nullptr;

        //Create the vertex attribute buffer
        glCreateVertexArrays(1, &meshData->vertexArrayId);

        const GLuint vaoId = meshData->vertexArrayId;
        const GLuint vboId = meshData->vertexBufferId;
        const int stride = sizeof(models::internal::VertexData);

        //Vertex attribute
        glEnableVertexArrayAttrib(vaoId, 0);
        glVertexArrayVertexBuffer(vaoId, 0, vboId,
                                  offsetof(models::internal::VertexData, vertex), stride);
        glVertexArrayAttribFormat(vaoId, 0, 3, GL_FLOAT, GL_FALSE, 0);
        glVertexArrayAttribBinding(vaoId, 0, 0);

        //Normal attribute
        glEnableVertexArrayAttrib(vaoId, 1);
        glVertexArrayVertexBuffer(vaoId, 1, vboId,
                                  offsetof(models::internal::VertexData, normal), stride);
        glVertexArrayAttribFormat(vaoId, 1, 3, GL_FLOAT, GL_FALSE, 0);
        glVertexArrayAttribBinding(vaoId, 1, 1);

        //Texture attribute
        glEnableVertexArrayAttrib(vaoId, 2);
        glVertexArrayVertexBuffer(vaoId, 2, vboId,
                                  offsetof(models::internal::VertexData, texturePoint), stride);
        glVertexArrayAttribFormat(vaoId, 2, 2, GL_FLOAT, GL_FALSE, 0);
        glVertexArrayAttribBinding(vaoId, 2, 2);

        //Element buffer
        glVertexArrayElementBuffer(vaoId, meshData->elementBufferId);
      }
    }

    void deleteBuffers(models::internal::ModelData* modelObjectData) {
      //Delete created buffers and the VAO
      for (unsigned int i = 0; i < modelObjectData->meshes.size(); i++) {
        const models::internal::MeshData* meshData = &modelObjectData->meshes[i];

        glDeleteBuffers(1, &meshData->vertexBufferId);
        glDeleteBuffers(1, &meshData->elementBufferId);
        glDeleteVertexArrays(1, &meshData->vertexArrayId);
      }
    }

    void moveModelToActive(AmmoniteId modelId, ammonite::models::internal::ModelInfo* modelPtr) {
      //Move from inactive to active tracker, update model pointer map
      const ammonite::models::internal::ModelInfo modelObject = *modelPtr;
      inactiveModelTracker.deleteModel(modelId);
      activeModelTracker.addModel(modelId, modelObject);
    }

    void moveModelToInactive(AmmoniteId modelId, ammonite::models::internal::ModelInfo* modelPtr) {
      //Move from active to inactive tracker, update model pointer map
      const ammonite::models::internal::ModelInfo modelObject = *modelPtr;
      activeModelTracker.deleteModel(modelId);
      inactiveModelTracker.addModel(modelId, modelObject);
    }

    void calcModelMatrices(models::internal::PositionData* positionData) {
      //Recalculate the model matrix when a component changes
      positionData->modelMatrix = positionData->translationMatrix
                                * glm::toMat4(positionData->rotationQuat)
                                * positionData->scaleMatrix;

      //Normal matrix
      positionData->normalMatrix = glm::transpose(glm::inverse(positionData->modelMatrix));
    }
  }

  //Exposed model handling methods
  namespace models {
    AmmoniteId createModel(const std::string& objectPath, bool flipTexCoords,
                           bool srgbTextures) {
      //Create the model
      internal::ModelInfo modelObject;
      modelObject.modelName = objectPath;

      //Reuse model data if it has already been loaded
      if (modelDataMap.contains(modelObject.modelName)) {
        //Link to existing model data
        modelObject.modelData = &modelDataMap[modelObject.modelName];
      } else {
        //Create empty ModelData object and add to tracker
        const internal::ModelData newModelData;
        modelDataMap[modelObject.modelName] = newModelData;
        modelObject.modelData = &modelDataMap[modelObject.modelName];

        //Generate info required to load model
        internal::ModelLoadInfo modelLoadInfo;
        modelLoadInfo.flipTexCoords = flipTexCoords;
        modelLoadInfo.srgbTextures = srgbTextures;
        modelLoadInfo.modelDirectory = objectPath.substr(0, objectPath.find_last_of('/'));

        //Fill the model data
        if (!internal::loadObject(objectPath, modelObject.modelData, modelLoadInfo)) {
          modelDataMap.erase(modelObject.modelName);
          return 0;
        }

        //Create buffers from loaded data
        createBuffers(modelObject.modelData);
      }
      modelObject.modelData->refCount++;

      //Load default texture IDs per mesh
      modelObject.textureIds = modelObject.modelData->textureIds;

      internal::PositionData positionData;
      positionData.translationMatrix = glm::mat4(1.0f);
      positionData.scaleMatrix = glm::mat4(1.0f);
      positionData.rotationQuat = glm::quat(glm::vec3(0, 0, 0));

      modelObject.positionData = positionData;

      //Calculate model and normal matrices
      calcModelMatrices(&modelObject.positionData);

      //Add model to the tracker and return the ID
      modelObject.modelId = utils::internal::setNextId(&lastModelId, modelIdPtrMap);
      activeModelTracker.addModel(modelObject.modelId, modelObject);
      return modelObject.modelId;
    }

    AmmoniteId createModel(const std::string& objectPath) {
      return createModel(objectPath, ASSUME_FLIP_MODEL_UVS, ASSUME_SRGB_TEXTURES);
    }

    AmmoniteId copyModel(AmmoniteId modelId) {
      //Get the model and check it exists
      models::internal::ModelInfo* oldModelObject = modelIdPtrMap[modelId];
      if (oldModelObject == nullptr) {
        return 0;
      }

      //Add model to the tracker via pointer
      const AmmoniteId newModelId = utils::internal::setNextId(&lastModelId, modelIdPtrMap);
      activeModelTracker.copyModelFromPtr(newModelId, oldModelObject);

      //Correct its ID and reference counter
      modelIdPtrMap[newModelId]->modelId = newModelId;
      modelIdPtrMap[newModelId]->lightEmitterId = 0;
      oldModelObject->modelData->refCount++;

      //Return the new ID
      return newModelId;
    }

    void deleteModel(AmmoniteId modelId) {
      //Check the model actually exists
      if (modelIdPtrMap.contains(modelId)) {
        internal::ModelInfo* modelObject = modelIdPtrMap[modelId];
        internal::ModelData* modelObjectData = modelObject->modelData;

        //Decrease the reference count of the model data
        modelObjectData->refCount--;

        //If the model data is now unused, destroy it
        if (modelObjectData->refCount == 0) {
          //Reduce reference count on textures
          for (unsigned int i = 0; i < modelObjectData->meshes.size(); i++) {
            if (modelObject->textureIds[i].diffuseId != 0) {
              ammonite::textures::internal::deleteTexture(modelObject->textureIds[i].diffuseId);
            }

            if (modelObject->textureIds[i].specularId != 0) {
              ammonite::textures::internal::deleteTexture(modelObject->textureIds[i].specularId);
            }
          }

          //Free the data if it hasn't been already
          for (unsigned int i = 0; i < modelObjectData->meshes.size(); i++) {
            if (modelObjectData->meshes[i].meshData != nullptr) {
              delete [] modelObjectData->meshes[i].meshData;
              modelObjectData->meshes[i].meshData = nullptr;
            }

            if (modelObjectData->meshes[i].indices != nullptr) {
              delete [] modelObjectData->meshes[i].indices;
              modelObjectData->meshes[i].indices = nullptr;
            }
          }

          //Destroy the model buffers and position in second tracker layer
          deleteBuffers(modelObjectData);
          modelDataMap.erase(modelObject->modelName);
        }

        //Unlink any attached light source
        ammonite::lighting::internal::unlinkByModel(modelId);

        //Remove the model from the tracker
        if (activeModelTracker.hasModel(modelId)) {
          activeModelTracker.deleteModel(modelId);
        } else if (inactiveModelTracker.hasModel(modelId)) {
          inactiveModelTracker.deleteModel(modelId);
        } else {
          ammonite::utils::warning << "Potential memory leak, couldn't delete model" << std::endl;
        }
      }
    }

    bool applyTexture(AmmoniteId modelId, AmmoniteEnum textureType,
                      const std::string& texturePath, bool srgbTexture) {
      internal::ModelInfo* modelPtr = modelIdPtrMap[modelId];
      if (modelPtr == nullptr) {
        return false;
      }

      //Apply texture to every mesh on the model
      const internal::ModelData* modelObjectData = modelPtr->modelData;
      for (unsigned int i = 0; i < modelObjectData->meshes.size(); i++) {
        GLuint* textureIdPtr = nullptr;
        if (textureType == AMMONITE_DIFFUSE_TEXTURE) {
          textureIdPtr = &modelPtr->textureIds[i].diffuseId;
        } else if (textureType == AMMONITE_SPECULAR_TEXTURE) {
          textureIdPtr = &modelPtr->textureIds[i].specularId;
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

    bool applyTexture(AmmoniteId modelId, AmmoniteEnum textureType,
                      const std::string& texturePath) {
      return applyTexture(modelId, textureType, texturePath, ASSUME_SRGB_TEXTURES);
    }

    //Return the number of indices on a model
    unsigned int getIndexCount(AmmoniteId modelId) {
      const internal::ModelInfo* modelPtr = modelIdPtrMap[modelId];
      if (modelPtr == nullptr) {
        return 0;
      }

      unsigned int indexCount = 0;
      models::internal::ModelData* modelData = modelPtr->modelData;
      for (unsigned int i = 0; i < modelData->meshes.size(); i++) {
        indexCount += modelData->meshes[i].indexCount;
      }

      return indexCount;
    }

    //Return the number of vertices on a model
    unsigned int getVertexCount(AmmoniteId modelId) {
      const internal::ModelInfo* modelPtr = modelIdPtrMap[modelId];
      if (modelPtr == nullptr) {
        return 0;
      }

      unsigned int vertexCount = 0;
      models::internal::ModelData* modelData = modelPtr->modelData;
      for (unsigned int i = 0; i < modelData->meshes.size(); i++) {
        vertexCount += modelData->meshes[i].vertexCount;
      }

      return vertexCount;
    }

    void setDrawMode(AmmoniteId modelId, AmmoniteEnum drawMode) {
      internal::ModelInfo* modelPtr = modelIdPtrMap[modelId];
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
      glm::vec3 getPosition(AmmoniteId modelId) {
        //Get the model and check it exists
        const models::internal::ModelInfo* modelObject = modelIdPtrMap[modelId];
        if (modelObject == nullptr) {
          return glm::vec3(0.0f);
        }

        return glm::vec3(modelObject->positionData.translationMatrix * \
          glm::vec4(glm::vec3(0, 0, 0), 1));
      }

      glm::vec3 getScale(AmmoniteId modelId) {
        //Get the model and check it exists
        const models::internal::ModelInfo* modelObject = modelIdPtrMap[modelId];
        if (modelObject == nullptr) {
          return glm::vec3(0.0f);
        }

        return glm::vec3(modelObject->positionData.scaleMatrix * glm::vec4(glm::vec3(1, 1, 1), 1));
      }

      //Return rotation, in radians
      glm::vec3 getRotation(AmmoniteId modelId) {
        //Get the model and check it exists
        const models::internal::ModelInfo* modelObject = modelIdPtrMap[modelId];
        if (modelObject == nullptr) {
          return glm::vec3(0.0f);
        }

        return glm::eulerAngles(modelObject->positionData.rotationQuat);
      }
    }

    //Set absolute position, scale and rotation of models
    namespace position {
      void setPosition(AmmoniteId modelId, glm::vec3 position) {
        //Get the model and check it exists
        models::internal::ModelInfo* modelObject = modelIdPtrMap[modelId];
        if (modelObject == nullptr) {
          return;
        }

        //Set the position
        modelObject->positionData.translationMatrix = glm::translate(glm::mat4(1.0f), position);

        if (modelObject->lightEmitterId != 0) {
          ammonite::lighting::internal::setLightSourcesChanged();
        }

        //Recalculate model and normal matrices
        calcModelMatrices(&modelObject->positionData);
      }

      void setScale(AmmoniteId modelId, glm::vec3 scale) {
        //Get the model and check it exists
        models::internal::ModelInfo* modelObject = modelIdPtrMap[modelId];
        if (modelObject == nullptr) {
          return;
        }

        //Set the scale
        modelObject->positionData.scaleMatrix = glm::scale(glm::mat4(1.0f), scale);

        if (modelObject->lightEmitterId != 0) {
          ammonite::lighting::internal::setLightSourcesChanged();
        }

        //Recalculate model and normal matrices
        calcModelMatrices(&modelObject->positionData);
      }

      void setScale(AmmoniteId modelId, float scaleMultiplier) {
        setScale(modelId, glm::vec3(scaleMultiplier, scaleMultiplier, scaleMultiplier));
      }

      //Rotation, in radians
      void setRotation(AmmoniteId modelId, glm::vec3 rotation) {
        //Get the model and check it exists
        models::internal::ModelInfo* modelObject = modelIdPtrMap[modelId];
        if (modelObject == nullptr) {
          return;
        }

        //Set the rotation
        modelObject->positionData.rotationQuat = glm::quat(rotation) * \
          glm::quat(glm::vec3(0, 0, 0));

        if (modelObject->lightEmitterId != 0) {
          ammonite::lighting::internal::setLightSourcesChanged();
        }

        //Recalculate model and normal matrices
        calcModelMatrices(&modelObject->positionData);
      }
    }

    //Translate, scale and rotate models
    namespace position {
      void translateModel(AmmoniteId modelId, glm::vec3 translation) {
        //Get the model and check it exists
        models::internal::ModelInfo* modelObject = modelIdPtrMap[modelId];
        if (modelObject == nullptr) {
          return;
        }

        //Translate it
        modelObject->positionData.translationMatrix = glm::translate(
          modelObject->positionData.translationMatrix,
          translation);

        if (modelObject->lightEmitterId != 0) {
          ammonite::lighting::internal::setLightSourcesChanged();
        }

        //Recalculate model and normal matrices
        calcModelMatrices(&modelObject->positionData);
      }

      void scaleModel(AmmoniteId modelId, glm::vec3 scaleVector) {
        //Get the model and check it exists
        models::internal::ModelInfo* modelObject = modelIdPtrMap[modelId];
        if (modelObject == nullptr) {
          return;
        }

        //Scale it
        modelObject->positionData.scaleMatrix = glm::scale(
          modelObject->positionData.scaleMatrix,
          scaleVector);

        if (modelObject->lightEmitterId != 0) {
          ammonite::lighting::internal::setLightSourcesChanged();
        }

        //Recalculate model and normal matrices
        calcModelMatrices(&modelObject->positionData);
      }

      void scaleModel(AmmoniteId modelId, float scaleMultiplier) {
        scaleModel(modelId, glm::vec3(scaleMultiplier, scaleMultiplier, scaleMultiplier));
      }

      //Rotation, in radians
      void rotateModel(AmmoniteId modelId, glm::vec3 rotation) {
        //Get the model and check it exists
        models::internal::ModelInfo* modelObject = modelIdPtrMap[modelId];
        if (modelObject == nullptr) {
          return;
        }

        //Rotate it
        modelObject->positionData.rotationQuat = glm::quat(rotation) * \
          modelObject->positionData.rotationQuat;

        if (modelObject->lightEmitterId != 0) {
          ammonite::lighting::internal::setLightSourcesChanged();
        }

        //Recalculate model and normal matrices
        calcModelMatrices(&modelObject->positionData);
      }
    }
  }
}
