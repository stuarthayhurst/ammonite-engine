#ifndef AMMONITEMODELS
#define AMMONITEMODELS

#include <string>

#include "../maths/vectorTypes.hpp"
#include "../utils/id.hpp"

static constexpr bool ASSUME_FLIP_MODEL_UVS = true;

//Texture type enums
enum AmmoniteTextureEnum : unsigned char {
  AMMONITE_DIFFUSE_TEXTURE,
  AMMONITE_SPECULAR_TEXTURE
};

//Model drawing mode enums
enum AmmoniteDrawEnum : unsigned char {
  AMMONITE_DRAW_INACTIVE,
  AMMONITE_DRAW_ACTIVE,
  AMMONITE_DRAW_WIREFRAME,
  AMMONITE_DRAW_POINTS
};

namespace ammonite {
  namespace models {
    namespace position {
      void getPosition(AmmoniteId modelId, ammonite::Vec<float, 3>& position);
      void getScale(AmmoniteId modelId, ammonite::Vec<float, 3>& scale);
      void getRotation(AmmoniteId modelId, ammonite::Vec<float, 3>& rotation);

      //Absolute movements
      void setPosition(AmmoniteId modelId, const ammonite::Vec<float, 3>& position);
      void setScale(AmmoniteId modelId, const ammonite::Vec<float, 3>& scale);
      void setScale(AmmoniteId modelId, float scaleMultiplier);
      void setRotation(AmmoniteId modelId, const ammonite::Vec<float, 3>& rotation);

      //Relative adjustments
      void translateModel(AmmoniteId modelId, const ammonite::Vec<float, 3>& translation);
      void scaleModel(AmmoniteId modelId, const ammonite::Vec<float, 3>& scale);
      void scaleModel(AmmoniteId modelId, float scaleMultiplier);
      void rotateModel(AmmoniteId modelId, const ammonite::Vec<float, 3>& rotation);
    }

    AmmoniteId createModel(const std::string& objectPath);
    AmmoniteId createModel(const std::string& objectPath, bool flipTexCoords, bool srgbTextures);
    void deleteModel(AmmoniteId modelId);
    AmmoniteId copyModel(AmmoniteId modelId);

    bool applyTexture(AmmoniteId modelId, AmmoniteTextureEnum textureType, const std::string& texturePath);
    bool applyTexture(AmmoniteId modelId, AmmoniteTextureEnum textureType, const std::string& texturePath,
                      bool srgbTexture);
    unsigned int getIndexCount(AmmoniteId modelId);
    unsigned int getVertexCount(AmmoniteId modelId);
    void setDrawMode(AmmoniteId modelId, AmmoniteDrawEnum drawMode);
  }
}

#endif
