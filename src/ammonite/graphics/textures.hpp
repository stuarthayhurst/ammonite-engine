#ifndef INTERNALTEXTURES
#define INTERNALTEXTURES

#include <string>

extern "C" {
  #include <epoxy/gl.h>
}

#include "../visibility.hpp"
#include "../maths/vector.hpp"

namespace AMMONITE_INTERNAL ammonite {
  namespace textures {
    namespace internal {
      GLuint createTexture(int width, int height, unsigned char* data, GLenum dataFormat,
                           GLenum textureFormat, GLint textureLevels);
      GLuint loadTexture(const std::string& texturePath, bool flipTexture, bool srgbTexture);

      GLuint loadSolidTexture(const ammonite::Vec<float, 3>& colour);
      GLuint loadSolidTexture(const ammonite::Vec<float, 4>& colour);

      void copyTexture(GLuint textureId);
      void deleteTexture(GLuint textureId);

      struct TextureData {
        int width;
        int height;
        int numChannels;
        bool srgbTexture;
        unsigned char* data;
      };

      void calculateTextureKey(const std::string& texturePath, bool flipTexture,
                               bool srgbTexture, std::string* textureKey);
      void calculateTextureKey(const ammonite::Vec<float, 3>& colour, std::string* textureKey);
      void calculateTextureKey(const ammonite::Vec<float, 4>& colour, std::string* textureKey);
      bool checkTextureKey(const std::string& textureKey);
      GLuint acquireTextureKeyId(const std::string& textureKey);
      GLuint reserveTextureKey(const std::string& textureKey);
      bool prepareTextureData(const std::string& texturePath, bool flipTexture,
                              bool srgbTexture, TextureData* textureData);
      bool uploadTextureData(GLuint textureId, const TextureData& textureData);

      GLuint loadCubemap(std::string texturePaths[6], bool flipTextures, bool srgbTextures);

      unsigned int calculateMipmapLevels(int width, int height);
    }
  }
}

#endif
