#include "models.hpp"

#include "../lighting/lighting.hpp"
#include "../maths/matrix.hpp"
#include "../maths/quaternion.hpp"
#include "../maths/vector.hpp"
#include "../utils/id.hpp"

/*
 - Modify / query the position, scale or rotation of a model by ID
*/

namespace ammonite {
  namespace models {
    namespace internal {
      void calcModelMatrices(PositionData* positionData) {
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

    //Return position, scale and rotation of a model
    namespace position {
      void getPosition(AmmoniteId modelId, ammonite::Vec<float, 3>& position) {
        //Get the model and check it exists
        const internal::ModelInfo* const modelInfo = internal::getModelPtr(modelId);
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
        const internal::ModelInfo* const modelInfo = internal::getModelPtr(modelId);
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
        const internal::ModelInfo* const modelInfo = internal::getModelPtr(modelId);
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
        internal::ModelInfo* const modelInfo = internal::getModelPtr(modelId);
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
        internal::ModelInfo* const modelInfo = internal::getModelPtr(modelId);
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
        internal::ModelInfo* const modelInfo = internal::getModelPtr(modelId);
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
        internal::ModelInfo* const modelInfo = internal::getModelPtr(modelId);
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
        internal::ModelInfo* const modelInfo = internal::getModelPtr(modelId);
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
        internal::ModelInfo* const modelInfo = internal::getModelPtr(modelId);
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
