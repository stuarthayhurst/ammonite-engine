#include "lighting.hpp"

#include "../maths/vector.hpp"
#include "../utils/id.hpp"

namespace ammonite {
  namespace lighting {
    namespace properties {
      namespace {
        //Default ambient light
        ammonite::Vec<float, 3> ambientLight {0.0f, 0.0f, 0.0f};
      }

      void setAmbientLight(const ammonite::Vec<float, 3>& ambient) {
        ammonite::copy(ambient, ambientLight);
      }

      void getAmbientLight(ammonite::Vec<float, 3>& ambient) {
        ammonite::copy(ambientLight, ambient);
      }

      /*
       - Returned position will be the internal light position,
         not necessarily where it's being rendered
       - This only matters when the light is linked to a model
      */
      void getPosition(AmmoniteId lightId, ammonite::Vec<float, 3>& position) {
        const internal::LightSource* const lightSource = lighting::internal::getLightSourcePtr(lightId);
        if (lightSource == nullptr) {
          ammonite::set(position, 0.0f);
          return;
        }

        ammonite::copy(lightSource->position, position);
      }

      void getColour(AmmoniteId lightId, ammonite::Vec<float, 3>& colour) {
        const internal::LightSource* const lightSource = lighting::internal::getLightSourcePtr(lightId);
        if (lightSource == nullptr) {
          ammonite::set(colour, 0.0f);
          return;
        }

        ammonite::copy(lightSource->diffuse, colour);
      }

      float getPower(AmmoniteId lightId) {
        const internal::LightSource* const lightSource = lighting::internal::getLightSourcePtr(lightId);
        if (lightSource == nullptr) {
          return 0.0f;
        }

        return lightSource->power;
      }

      //Changes to light positions are recorded but not rendered while linked to a model
      void setPosition(AmmoniteId lightId, const ammonite::Vec<float, 3>& position) {
        internal::LightSource* const lightSource = lighting::internal::getLightSourcePtr(lightId);
        if (lightSource != nullptr) {
          ammonite::copy(position, lightSource->position);
          lighting::internal::setLightSourcesChanged();
        }
      }

      void setColour(AmmoniteId lightId, const ammonite::Vec<float, 3>& colour) {
        internal::LightSource* const lightSource = lighting::internal::getLightSourcePtr(lightId);
        if (lightSource != nullptr) {
          ammonite::copy(colour, lightSource->diffuse);
          lighting::internal::setLightSourcesChanged();
        }
      }

      void setPower(AmmoniteId lightId, float power) {
        internal::LightSource* const lightSource = lighting::internal::getLightSourcePtr(lightId);
        if (lightSource != nullptr) {
          lightSource->power = power;
          lighting::internal::setLightSourcesChanged();
        }
      }
    }
  }
}
