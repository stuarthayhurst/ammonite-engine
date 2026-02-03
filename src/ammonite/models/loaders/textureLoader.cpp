#include <iostream>
#include <list>
#include <string>

#include "../models.hpp"

#include "../../graphics/textures.hpp"
#include "../../utils/logging.hpp"
#include "../../utils/thread.hpp"

namespace ammonite {
  namespace models {
    namespace internal {
      namespace {
        //Data required by the worker thread to load the texture
        struct TextureThreadData {
          std::string texturePath;
          bool flipTextures;
          bool srgbTextures;
          textures::internal::TextureData textureData;
          bool loadedTexture;
        };

        //Hold data to load a texture and the sync for its worker
        struct TextureLoadData {
          TextureThreadData threadData;
          GLuint textureId;
          AmmoniteGroup sync{0};
        };

        //Store the outstanding texture loads
        std::list<TextureLoadData> textureQueue;
      }

      //Thread workers
      namespace {
        void textureLoadWorker(void* userPtr) {
          TextureThreadData* const threadData = (TextureThreadData*)userPtr;

          const bool prepared = textures::internal::prepareTextureData(
            threadData->texturePath, threadData->flipTextures,
            threadData->srgbTextures, &threadData->textureData);

          threadData->loadedTexture = prepared;
        }
      }

      /*
       - Queue a texture load on the thread pool
       - Must call uploadQueuedTextures() before it can be used
       - All queued textures share the same queue
      */
      GLuint queueTextureLoad(const std::string& texturePath, bool flipTexture,
                              bool srgbTexture) {
        //Calculate the texture's key
        std::string textureKey;
        textures::internal::calculateTextureKey(texturePath, flipTexture, srgbTexture, &textureKey);

        //Use texture cache, if already loaded / reserved
        if (textures::internal::checkTextureKey(textureKey)) {
          return textures::internal::acquireTextureKeyId(textureKey);
        }

        //Reserve the texture key before loading
        const GLuint textureId = textures::internal::reserveTextureKey(textureKey);
        if (textureId == 0) {
          ammonite::utils::warning << "Failed to reserve texture" << std::endl;
          return 0;
        }

        //Prepare data for the worker thread
        TextureLoadData& textureLoadData = textureQueue.emplace_back();
        textureLoadData.threadData.texturePath = texturePath;
        textureLoadData.threadData.flipTextures = flipTexture;
        textureLoadData.threadData.srgbTextures = srgbTexture;
        textureLoadData.threadData.loadedTexture = false;

        //Prepare data for texture upload after sync
        textureLoadData.textureId = textureId;

        //Submit the texture load to the thread pool
        ammonite::utils::thread::submitWork(textureLoadWorker,
          &textureLoadData.threadData, &textureLoadData.sync);

        return textureId;
      }

      /*
       - Upload the queued textures
         - All textures share the same queue, so this will upload all queued textures
       - Returns true if the load and upload was successful, otherwise false
      */
      bool uploadQueuedTextures() {
        //Wait for the texture loads to complete, and upload their data
        bool success = true;
        for (TextureLoadData& textureLoadData : textureQueue) {
          ammonite::utils::thread::waitGroupComplete(&textureLoadData.sync, 1);

          //Attempt to upload the texture data if the load was successful
          success &= textureLoadData.threadData.loadedTexture;
          if (textureLoadData.threadData.loadedTexture) {
            success &= textures::internal::uploadTextureData(
              textureLoadData.textureId, textureLoadData.threadData.textureData);
          }
        }

        //Clear the texture queue
        textureQueue.clear();

        return success;
      }
    }
  }
}
