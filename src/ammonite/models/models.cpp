#include <iostream>
#include <map>
#include <string>
#include <algorithm>

#include <GL/glew.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include "../types.hpp"

#include "internal/modelTypes.hpp"
#include "internal/modelLoader.hpp"

#include "../lighting/internal/lightTracker.hpp"
#include "../graphics/internal/internalTextures.hpp"
#include "../utils/logging.hpp"

//Constants for texture loading assumptions
#define ASSUME_FLIP_UVS true
#define ASSUME_SRGB_TEXTURES false

//Class definitions
namespace ammonite {
  typedef std::map<int, ammonite::models::internal::ModelInfo> ModelTrackerMap;
  typedef std::map<int, ammonite::models::internal::ModelInfo*> ModelPtrTrackerMap;
  typedef std::map<std::string, ammonite::models::internal::ModelData> ModelDataMap;

  bool haveModelsMoved = false;

  class ModelTracker {
  private:
    ModelTrackerMap modelTrackerMap;
    ModelTrackerMap lightTrackerMap;
    ModelPtrTrackerMap* modelIdPtrMapPtr;

    std::map<AmmoniteEnum, ModelTrackerMap*> modelSelector = {
      {AMMONITE_MODEL, &modelTrackerMap},
      {AMMONITE_LIGHT_EMITTER, &lightTrackerMap}
    };

  public:
    ModelTracker(ModelPtrTrackerMap* modelIdPtrMapAddr) {
      modelIdPtrMapPtr = modelIdPtrMapAddr;
    }

    int getModelCount(AmmoniteEnum modelType) {
      return modelSelector[modelType]->size();
    }

    void getModels(AmmoniteEnum modelType, int modelCount, ammonite::models::internal::ModelInfo* modelArr[]) {
      //Select the right model tracker
      ModelTrackerMap* modelMapPtr = modelSelector[modelType];

      //Fill modelArr with first number modelCount of items
      auto it = modelMapPtr->begin();
      modelCount = std::min(modelCount, int(modelMapPtr->size()));
      for (int i = 0; i < modelCount; i++) {
        modelArr[i] = &it->second;
        std::advance(it, 1);
      }
    }

    void addModel(int modelId, ammonite::models::internal::ModelInfo modelObject) {
      ModelTrackerMap* targetMapPtr = modelSelector[modelObject.modelType];
      (*targetMapPtr)[modelId] = modelObject;
      (*modelIdPtrMapPtr)[modelId] = &(*targetMapPtr)[modelId];
      haveModelsMoved = true;
    }

    void deleteModel(int modelId) {
      //Get the type of model, so the right tracker can be selected
      AmmoniteEnum modelType = (*modelIdPtrMapPtr)[modelId]->modelType;
      ModelTrackerMap* targetMapPtr = modelSelector[modelType];

      //Delete the model and id to pointer map entry
      targetMapPtr->erase(modelId);
      modelIdPtrMapPtr->erase(modelId);
      haveModelsMoved = true;
    }

    void changeModelType(int modelId, AmmoniteEnum targetType) {
      //Get the type of model, so the right tracker can be selected
      AmmoniteEnum modelType = (*modelIdPtrMapPtr)[modelId]->modelType;

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

    bool hasModel(int modelId) {
      //Return false if the model isn't tracked at all
      if (!modelIdPtrMapPtr->contains(modelId)) {
        return false;
      }

      //Get the type of model, so the right tracker can be selected
      AmmoniteEnum modelType = (*modelIdPtrMapPtr)[modelId]->modelType;
      ModelTrackerMap* targetMapPtr = modelSelector[modelType];

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

    //Track cumulative number of created models
    int totalModels = 0;

    ModelTracker activeModelTracker(&modelIdPtrMap);
    ModelTracker inactiveModelTracker(&modelIdPtrMap);
  }

  //Internally exposed model handling methods
  namespace models {
    namespace internal {
      int getModelCount(AmmoniteEnum modelType) {
        return activeModelTracker.getModelCount(modelType);
      }

      void getModels(AmmoniteEnum modelType, int modelCount, ammonite::models::internal::ModelInfo* modelArr[]) {
        activeModelTracker.getModels(modelType, modelCount, modelArr);
      }

      internal::ModelInfo* getModelPtr(int modelId) {
        //Check the model exists, and return a pointer
        if (modelIdPtrMap.contains(modelId)) {
          return modelIdPtrMap[modelId];
        } else {
          return nullptr;
        }
      }

      bool* getModelsMovedPtr() {
        return &haveModelsMoved;
      }

      void setLightEmitterId(int modelId, int lightEmitterId) {
        //Select the right tracker
        ModelTracker* selectedTracker = &inactiveModelTracker;
        internal::ModelInfo* modelPtr = modelIdPtrMap[modelId];
        if (modelPtr->isLoaded or modelPtr->drawMode != AMMONITE_DRAW_INACTIVE) {
          selectedTracker = &activeModelTracker;
        }

        //Move model to different sub-tracker and update pointer
        if (lightEmitterId != -1) {
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

      int getLightEmitterId(int modelId) {
        internal::ModelInfo* modelPtr = modelIdPtrMap[modelId];
        if (modelPtr != nullptr) {
          return modelPtr->lightEmitterId;
        }
        return false;
      }
    }
  }

  namespace {
    static void createBuffers(models::internal::ModelData* modelObjectData) {
      //Generate buffers for every mesh
      for (unsigned int i = 0; i < modelObjectData->meshes.size(); i++) {
        models::internal::MeshData* meshData = &modelObjectData->meshes[i];

        //Create vertex and index buffers
        glCreateBuffers(2, &meshData->vertexBufferId);

        //Fill interleaved vertex + normal + texture buffer and index buffer
        glNamedBufferData(meshData->vertexBufferId, meshData->meshData.size() * sizeof(models::internal::VertexData), &meshData->meshData[0], GL_STATIC_DRAW);
        glNamedBufferData(meshData->elementBufferId, meshData->indices.size() * sizeof(unsigned int), &meshData->indices[0], GL_STATIC_DRAW);

        //Create the vertex attribute buffer
        glCreateVertexArrays(1, &meshData->vertexArrayId);

        GLuint vaoId = meshData->vertexArrayId;
        GLuint vboId = meshData->vertexBufferId;
        int stride = 8 * sizeof(float); //(3 + 3 + 2) * bytes per float

        //Vertex attribute
        glEnableVertexArrayAttrib(vaoId, 0);
        glVertexArrayVertexBuffer(vaoId, 0, vboId, 0, stride);
        glVertexArrayAttribFormat(vaoId, 0, 3, GL_FLOAT, GL_FALSE, 0);
        glVertexArrayAttribBinding(vaoId, 0, 0);

        //Normal attribute
        glEnableVertexArrayAttrib(vaoId, 1);
        glVertexArrayVertexBuffer(vaoId, 1, vboId, 3 * sizeof(float), stride);
        glVertexArrayAttribFormat(vaoId, 1, 3, GL_FLOAT, GL_FALSE, 0);
        glVertexArrayAttribBinding(vaoId, 1, 1);

        //Texture attribute
        glEnableVertexArrayAttrib(vaoId, 2);
        glVertexArrayVertexBuffer(vaoId, 2, vboId, 6 * sizeof(float), stride);
        glVertexArrayAttribFormat(vaoId, 2, 2, GL_FLOAT, GL_FALSE, 0);
        glVertexArrayAttribBinding(vaoId, 2, 2);

        //Element buffer
        glVertexArrayElementBuffer(vaoId, meshData->elementBufferId);
      }
    }

    static void deleteBuffers(models::internal::ModelData* modelObjectData) {
      //Delete created buffers and the VAO
      for (unsigned int i = 0; i < modelObjectData->meshes.size(); i++) {
        models::internal::MeshData* meshData = &modelObjectData->meshes[i];

        glDeleteBuffers(1, &meshData->vertexBufferId);
        glDeleteBuffers(1, &meshData->elementBufferId);
        glDeleteVertexArrays(1, &meshData->vertexArrayId);
      }
    }

    static void moveModelToActive(int modelId, ammonite::models::internal::ModelInfo* modelPtr) {
      //Move from inactive to active tracker, update model pointer map
      ammonite::models::internal::ModelInfo modelObject = *modelPtr;
      inactiveModelTracker.deleteModel(modelId);
      activeModelTracker.addModel(modelId, modelObject);
    }

    static void moveModelToInactive(int modelId, ammonite::models::internal::ModelInfo* modelPtr) {
      //Move from active to inactive tracker, update model pointer map
      ammonite::models::internal::ModelInfo modelObject = *modelPtr;
      activeModelTracker.deleteModel(modelId);
      inactiveModelTracker.addModel(modelId, modelObject);
    }

    static void calcModelMatrices(models::internal::PositionData* positionData) {
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
    int createModel(const char* objectPath, bool flipTexCoords,
                    bool srgbTextures, bool* externalSuccess) {
      //Create the model
      internal::ModelInfo modelObject;
      modelObject.modelName = std::string(objectPath);

      //Reuse model data if it has already been loaded
      if (modelDataMap.contains(modelObject.modelName)) {
        //Link to existing model data
        modelObject.modelData = &modelDataMap[modelObject.modelName];
        modelObject.modelData->refCount++;
      } else {
        //Create empty ModelData object and add to tracker
        internal::ModelData newModelData;
        modelDataMap[modelObject.modelName] = newModelData;
        modelObject.modelData = &modelDataMap[modelObject.modelName];

        //Generate info required to load model
        std::string pathString = objectPath;
        ModelLoadInfo modelLoadInfo;
        modelLoadInfo.flipTexCoords = flipTexCoords;
        modelLoadInfo.srgbTextures = srgbTextures;
        modelLoadInfo.modelDirectory = pathString.substr(0, pathString.find_last_of('/'));

        //Fill the model data
        bool hasCreatedObject = true;
        internal::loadObject(objectPath, modelObject.modelData, modelLoadInfo, &hasCreatedObject);
        if (!hasCreatedObject) {
          modelDataMap.erase(modelObject.modelName);
          *externalSuccess = false;
          return -1;
        }

        //Create buffers from loaded data
        createBuffers(modelObject.modelData);
      }

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
      modelObject.modelId = ++totalModels;
      activeModelTracker.addModel(modelObject.modelId, modelObject);
      return modelObject.modelId;
    }

    int createModel(const char* objectPath, bool* externalSuccess) {
      return createModel(objectPath, ASSUME_FLIP_UVS, ASSUME_SRGB_TEXTURES, externalSuccess);
    }

    int copyModel(int modelId) {
      //Get the model and check it exists
      models::internal::ModelInfo* oldModelObject = modelIdPtrMap[modelId];
      if (oldModelObject == nullptr) {
        return -1;
      }

      //Copy model data
      internal::ModelInfo modelObject = *oldModelObject;
      modelObject.lightEmitterId = -1;
      modelObject.modelData->refCount++;

      //Add model to the tracker and return the ID
      modelObject.modelId = ++totalModels;
      activeModelTracker.addModel(modelObject.modelId, modelObject);
      return modelObject.modelId;
    }

    void unloadModel(int modelId) {
      internal::ModelInfo* modelPtr = modelIdPtrMap[modelId];
      if (modelPtr == nullptr) {
        return;
      }

      //If model is loaded and active, move from active to inactive tracker
      if (modelPtr->isLoaded and modelPtr->drawMode != AMMONITE_DRAW_INACTIVE) {
        moveModelToInactive(modelId, modelPtr);
        modelPtr = modelIdPtrMap[modelId];
      }

      //Check if model is loaded
      if (modelPtr->isLoaded) {
        //Set model as unloaded
        modelPtr->isLoaded = false;

        //Decrease refcount, add a new soft reference
        modelPtr->modelData->softRefCount++;
        modelPtr->modelData->refCount--;

        //Unload actual data if no models reference it
        if (modelPtr->modelData->refCount < 1) {
          deleteBuffers(modelPtr->modelData);
        }
      }
    }

    void reloadModel(int modelId) {
      internal::ModelInfo* modelPtr = modelIdPtrMap[modelId];
      if (modelPtr == nullptr) {
        return;
      }

      //If model is unloaded or inactive, move from inactive to active tracker
      if (!modelPtr->isLoaded or modelPtr->drawMode == AMMONITE_DRAW_INACTIVE) {
        moveModelToActive(modelId, modelPtr);
        modelPtr = modelIdPtrMap[modelId];
      }

      //Check if model is unloaded
      if (!modelPtr->isLoaded) {
        //Set model as loaded
        modelPtr->isLoaded = true;

        //Increase refcount, remove a soft reference
        modelPtr->modelData->softRefCount--;
        modelPtr->modelData->refCount++;

        //Upload actual data to the GPU, if it wasn't present
        if (modelPtr->modelData->refCount == 1) {
          createBuffers(modelPtr->modelData);
        }
      }
    }

    void deleteModel(int modelId) {
      //Check the model actually exists
      if (modelIdPtrMap.contains(modelId)) {
        internal::ModelInfo* modelObject = modelIdPtrMap[modelId];
        internal::ModelData* modelObjectData = modelObject->modelData;

        //Decrease the reference / soft reference count of the model data
        if (modelObject->isLoaded) {
          modelObjectData->refCount--;
        } else {
          modelObjectData->softRefCount--;
        }

        //If the model data is now unused, destroy it
        if (modelObjectData->refCount < 1 and modelObjectData->softRefCount < 1) {
          //Reduce reference count on textures
          for (unsigned int i = 0; i < modelObjectData->meshes.size(); i++) {
            ammonite::textures::internal::deleteTexture(modelObject->textureIds[i].diffuseId);
            ammonite::textures::internal::deleteTexture(modelObject->textureIds[i].specularId);
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

    void applyTexture(int modelId, AmmoniteEnum textureType, const char* texturePath,
                      bool srgbTexture, bool* externalSuccess) {
      internal::ModelInfo* modelPtr = modelIdPtrMap[modelId];
      if (modelPtr == nullptr) {
        *externalSuccess = false;
        return;
      }

      //Apply texture to every mesh on the model
      internal::ModelData* modelObjectData = modelPtr->modelData;
      for (unsigned int i = 0; i < modelObjectData->meshes.size(); i++) {
        int* textureIdPtr;
        if (textureType == AMMONITE_DIFFUSE_TEXTURE) {
          textureIdPtr = &modelPtr->textureIds[i].diffuseId;
        } else if (textureType == AMMONITE_SPECULAR_TEXTURE) {
          textureIdPtr = &modelPtr->textureIds[i].specularId;
        } else {
          ammonite::utils::warning << "Invalid texture type specified" << std::endl;
          *externalSuccess = false;
          return;
        }

        //If a texture is already applied, remove it
        if (*textureIdPtr != -1) {
          ammonite::textures::internal::deleteTexture(*textureIdPtr);
          *textureIdPtr = -1;
        }

        //Create new texture and apply to the mesh
        bool hasCreatedTexture = true;
        int textureId = ammonite::textures::internal::loadTexture(texturePath, srgbTexture, &hasCreatedTexture);
        if (!hasCreatedTexture) {
          *externalSuccess = false;
          return;
        }

        *textureIdPtr = textureId;
      }
    }

    void applyTexture(int modelId, AmmoniteEnum textureType, const char* texturePath, bool* externalSuccess) {
      applyTexture(modelId, textureType, texturePath, ASSUME_SRGB_TEXTURES, externalSuccess);
    }

    //Return the number of vertices on a model
    int getVertexCount(int modelId) {
      internal::ModelInfo* modelPtr = modelIdPtrMap[modelId];
      if (modelPtr == nullptr) {
        return 0;
      }

      int vertexCount = 0;
      models::internal::ModelData* modelData = modelPtr->modelData;
      for (unsigned int i = 0; i < modelData->meshes.size(); i++) {
        vertexCount += modelData->meshes[i].vertexCount;
      }

      return vertexCount;
    }

    namespace draw {
      void setDrawMode(int modelId, short drawMode) {
        internal::ModelInfo* modelPtr = modelIdPtrMap[modelId];
        if (modelPtr != nullptr) {
          if (modelPtr->drawMode == AMMONITE_DRAW_INACTIVE and drawMode != AMMONITE_DRAW_INACTIVE) {
            //Move from inactive to active tracker
            moveModelToActive(modelId, modelPtr);
          } else if (modelPtr->drawMode != AMMONITE_DRAW_INACTIVE and drawMode == AMMONITE_DRAW_INACTIVE) {
            //Move from active to inactive tracker
            moveModelToInactive(modelId, modelPtr);
          }

          //Update draw mode
          modelIdPtrMap[modelId]->drawMode = drawMode;
        }
      }
    }

    //Return position, scale and rotation of a model
    namespace position {
      glm::vec3 getPosition(int modelId) {
        //Get the model and check it exists
        models::internal::ModelInfo* modelObject = modelIdPtrMap[modelId];
        if (modelObject == nullptr) {
          return glm::vec3(0.0f);
        }

        return glm::vec3(modelObject->positionData.translationMatrix * glm::vec4(glm::vec3(0, 0, 0), 1));
      }

      glm::vec3 getScale(int modelId) {
        //Get the model and check it exists
        models::internal::ModelInfo* modelObject = modelIdPtrMap[modelId];
        if (modelObject == nullptr) {
          return glm::vec3(0.0f);
        }

        return glm::vec3(modelObject->positionData.scaleMatrix * glm::vec4(glm::vec3(1, 1, 1), 1));
      }

      //Return rotation, in radians
      glm::vec3 getRotation(int modelId) {
        //Get the model and check it exists
        models::internal::ModelInfo* modelObject = modelIdPtrMap[modelId];
        if (modelObject == nullptr) {
          return glm::vec3(0.0f);
        }

        return glm::eulerAngles(modelObject->positionData.rotationQuat);
      }
    }

    //Set absolute position, scale and rotation of models
    namespace position {
      void setPosition(int modelId, glm::vec3 position) {
        //Get the model and check it exists
        models::internal::ModelInfo* modelObject = modelIdPtrMap[modelId];
        if (modelObject == nullptr) {
          return;
        }

        //Set the position
        modelObject->positionData.translationMatrix = glm::translate(glm::mat4(1.0f), position);

        //Recalculate model and normal matrices
        calcModelMatrices(&modelObject->positionData);
      }

      void setScale(int modelId, glm::vec3 scale) {
        //Get the model and check it exists
        models::internal::ModelInfo* modelObject = modelIdPtrMap[modelId];
        if (modelObject == nullptr) {
          return;
        }

        //Set the scale
        modelObject->positionData.scaleMatrix = glm::scale(glm::mat4(1.0f), scale);

        //Recalculate model and normal matrices
        calcModelMatrices(&modelObject->positionData);
      }

      void setScale(int modelId, float scaleMultiplier) {
        setScale(modelId, glm::vec3(scaleMultiplier, scaleMultiplier, scaleMultiplier));
      }

      //Rotation, in radians
      void setRotation(int modelId, glm::vec3 rotation) {
        //Get the model and check it exists
        models::internal::ModelInfo* modelObject = modelIdPtrMap[modelId];
        if (modelObject == nullptr) {
          return;
        }

        //Set the rotation
        modelObject->positionData.rotationQuat = glm::quat(rotation) * glm::quat(glm::vec3(0, 0, 0));

        //Recalculate model and normal matrices
        calcModelMatrices(&modelObject->positionData);
      }
    }

    //Translate, scale and rotate models
    namespace position {
      void translateModel(int modelId, glm::vec3 translation) {
        //Get the model and check it exists
        models::internal::ModelInfo* modelObject = modelIdPtrMap[modelId];
        if (modelObject == nullptr) {
          return;
        }

        //Translate it
        modelObject->positionData.translationMatrix = glm::translate(
          modelObject->positionData.translationMatrix,
          translation);

        //Recalculate model and normal matrices
        calcModelMatrices(&modelObject->positionData);
      }

      void scaleModel(int modelId, glm::vec3 scaleVector) {
        //Get the model and check it exists
        models::internal::ModelInfo* modelObject = modelIdPtrMap[modelId];
        if (modelObject == nullptr) {
          return;
        }

        //Scale it
        modelObject->positionData.scaleMatrix = glm::scale(
          modelObject->positionData.scaleMatrix,
          scaleVector);

        //Recalculate model and normal matrices
        calcModelMatrices(&modelObject->positionData);
      }

      void scaleModel(int modelId, float scaleMultiplier) {
        scaleModel(modelId, glm::vec3(scaleMultiplier, scaleMultiplier, scaleMultiplier));
      }

      //Rotation, in radians
      void rotateModel(int modelId, glm::vec3 rotation) {
        //Get the model and check it exists
        models::internal::ModelInfo* modelObject = modelIdPtrMap[modelId];
        if (modelObject == nullptr) {
          return;
        }

        //Rotate it
        modelObject->positionData.rotationQuat = glm::quat(rotation) * modelObject->positionData.rotationQuat;

        //Recalculate model and normal matrices
        calcModelMatrices(&modelObject->positionData);
      }
    }
  }
}
