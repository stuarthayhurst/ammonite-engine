#ifndef AMMONITEMODELS
#define AMMONITEMODELS

#include <string>

#include "../maths/vectorTypes.hpp"
#include "../utils/id.hpp"
#include "../enums.hpp"
#include "../visibility.hpp"

static constexpr bool ASSUME_FLIP_MODEL_UVS = true;

//Model drawing mode enums
enum AmmoniteDrawEnum : unsigned char {
  AMMONITE_DRAW_INACTIVE,
  AMMONITE_DRAW_ACTIVE,
  AMMONITE_DRAW_WIREFRAME,
  AMMONITE_DRAW_POINTS
};

namespace AMMONITE_EXPOSED ammonite {
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

    /*
     - Store data for a single vertex
     - Changes to this structure require changes to VertexCompare
    */
    struct AmmoniteVertex {
      ammonite::Vec<float, 3> vertex;
      ammonite::Vec<float, 3> normal;
      ammonite::Vec<float, 2> texturePoint;
    };

    /*
     - Store data for a single material
     - Set each union to the desired material component source
       - isSrgbTexture determines whether to treat the texture as sRGB or not
     - *isTexture determines whether to treat the corresponding union as a texture
       or a colour
    */
    //NOLINTBEGIN(cppcoreguidelines-avoid-const-or-ref-data-members)
    union AmmoniteMaterialComponent {
      const ammonite::Vec<float, 3> colour;
      struct AmmoniteTextureInfo {
        const std::string* texturePath;
        bool isSrgbTexture = ASSUME_SRGB_TEXTURES;
      } textureInfo;
    };

    struct AmmoniteMaterial {
      AmmoniteMaterialComponent diffuse;
      AmmoniteMaterialComponent specular;

      const bool diffuseIsTexture;
      const bool specularIsTexture;
    };
    //NOLINTEND(cppcoreguidelines-avoid-const-or-ref-data-members)

    //File-based object creation
    AmmoniteId createModel(const std::string& objectPath, bool flipTexCoords,
                           bool srgbTextures);
    AmmoniteId createModel(const std::string& objectPath);

    //Multiple mesh object creation
    AmmoniteId createModel(const AmmoniteVertex* meshArray[],
                           const unsigned int* indicesArray[],
                           const AmmoniteMaterial* materials,
                           unsigned int meshCount, const unsigned int* vertexCounts,
                           const unsigned int* indexCounts);
    AmmoniteId createModel(const AmmoniteVertex* meshArray[],
                           const AmmoniteMaterial* materials,
                           unsigned int meshCount, const unsigned int* vertexCounts);

    //Single mesh model creation
    AmmoniteId createModel(const AmmoniteVertex* mesh,
                           const unsigned int* indices,
                           const AmmoniteMaterial& material,
                           unsigned int vertexCount, unsigned int indexCount);
    AmmoniteId createModel(const AmmoniteVertex* mesh, const AmmoniteMaterial& material,
                           unsigned int vertexCount);

    void deleteModel(AmmoniteId modelId);
    AmmoniteId copyModel(AmmoniteId modelId, bool preserveDrawMode);

    bool applyMaterial(AmmoniteId modelId, const AmmoniteMaterial& material);
    AmmoniteMaterial createMaterial(const std::string& diffusePath,
                                    const std::string& specularPath);
    AmmoniteMaterial createMaterial(const ammonite::Vec<float, 3>& diffuseColour,
                                    const ammonite::Vec<float, 3>& specularColour);
    AmmoniteMaterial createMaterial(const std::string& diffusePath,
                                    const ammonite::Vec<float, 3>& specularColour);
    AmmoniteMaterial createMaterial(const ammonite::Vec<float, 3>& diffuseColour,
                                    const std::string& specularPath);
    void deleteMaterial(const AmmoniteMaterial& material);

    unsigned int getIndexCount(AmmoniteId modelId);
    unsigned int getVertexCount(AmmoniteId modelId);
    void setDrawMode(AmmoniteId modelId, AmmoniteDrawEnum drawMode);

    bool dumpModelStorageDebug();
  }
}

#endif
