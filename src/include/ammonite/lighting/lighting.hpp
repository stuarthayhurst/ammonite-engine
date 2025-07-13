#ifndef AMMONITELIGHTING
#define AMMONITELIGHTING

#include "../maths/vectorTypes.hpp"
#include "../utils/id.hpp"

namespace ammonite {
  namespace lighting {
    namespace properties {
      void getGeometry(AmmoniteId lightId, ammonite::Vec<float, 3>& geometry);
      void getColour(AmmoniteId lightId, ammonite::Vec<float, 3>& colour);
      float getPower(AmmoniteId lightId);

      void setGeometry(AmmoniteId lightId, const ammonite::Vec<float, 3>& geometry);
      void setColour(AmmoniteId lightId, const ammonite::Vec<float, 3>& colour);
      void setPower(AmmoniteId lightId, float power);
    }

    void getAmbientLight(ammonite::Vec<float, 3>& ambient);
    void setAmbientLight(const ammonite::Vec<float, 3>& ambient);

    unsigned int getMaxLightCount();
    unsigned int getLightCount();
    AmmoniteId createLightSource();
    void deleteLightSource(AmmoniteId lightId);

    void linkModel(AmmoniteId lightId, AmmoniteId modelId);
    void unlinkModel(AmmoniteId lightId);
  }
}

#endif
