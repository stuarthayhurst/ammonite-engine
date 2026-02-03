#include <cstddef>
#include <vector>

extern "C" {
  #include <epoxy/gl.h>
}

#include "buffers.hpp"

#include "../models/models.hpp"

namespace ammonite {
  namespace graphics {
    namespace internal {
      void createModelBuffers(models::internal::ModelData* modelData,
                              std::vector<models::internal::RawMeshData>* rawMeshDataVec) {
        //Generate buffers for every mesh
        for (const models::internal::RawMeshData& rawMeshData : *rawMeshDataVec) {
          models::internal::MeshInfoGroup& meshInfo = modelData->meshInfo.emplace_back();

          //Copy point count information
          meshInfo.vertexCount = rawMeshData.vertexCount;
          meshInfo.indexCount = rawMeshData.indexCount;

          //Create vertex and index buffers
          glCreateBuffers(1, &meshInfo.vertexBufferId);
          glCreateBuffers(1, &meshInfo.elementBufferId);

          //Fill interleaved vertex + normal + texture buffer and index buffer
          glNamedBufferData(meshInfo.vertexBufferId,
                            meshInfo.vertexCount * (long)sizeof(models::AmmoniteVertex),
                            &rawMeshData.vertexData[0], GL_STATIC_DRAW);
          glNamedBufferData(meshInfo.elementBufferId,
                            meshInfo.indexCount * (long)sizeof(unsigned int),
                            &rawMeshData.indices[0], GL_STATIC_DRAW);

          //Destroy mesh data once uploaded
          delete [] rawMeshData.vertexData;
          delete [] rawMeshData.indices;

          //Create the vertex attribute buffer
          glCreateVertexArrays(1, &meshInfo.vertexArrayId);

          const GLuint vaoId = meshInfo.vertexArrayId;
          const GLuint vboId = meshInfo.vertexBufferId;
          const int stride = sizeof(models::AmmoniteVertex);

          //Vertex attribute
          glEnableVertexArrayAttrib(vaoId, 0);
          glVertexArrayVertexBuffer(vaoId, 0, vboId,
                                    offsetof(models::AmmoniteVertex, vertex), stride);
          glVertexArrayAttribFormat(vaoId, 0, 3, GL_FLOAT, GL_FALSE, 0);
          glVertexArrayAttribBinding(vaoId, 0, 0);

          //Normal attribute
          glEnableVertexArrayAttrib(vaoId, 1);
          glVertexArrayVertexBuffer(vaoId, 1, vboId,
                                    offsetof(models::AmmoniteVertex, normal), stride);
          glVertexArrayAttribFormat(vaoId, 1, 3, GL_FLOAT, GL_FALSE, 0);
          glVertexArrayAttribBinding(vaoId, 1, 1);

          //Texture attribute
          glEnableVertexArrayAttrib(vaoId, 2);
          glVertexArrayVertexBuffer(vaoId, 2, vboId,
                                    offsetof(models::AmmoniteVertex, texturePoint), stride);
          glVertexArrayAttribFormat(vaoId, 2, 2, GL_FLOAT, GL_FALSE, 0);
          glVertexArrayAttribBinding(vaoId, 2, 2);

          //Element buffer
          glVertexArrayElementBuffer(vaoId, meshInfo.elementBufferId);
        }
      }

      void deleteModelBuffers(models::internal::ModelData* modelData) {
        //Delete created buffers and the VAO
        for (const models::internal::MeshInfoGroup& meshInfo : modelData->meshInfo) {
          glDeleteBuffers(1, &meshInfo.vertexBufferId);
          glDeleteBuffers(1, &meshInfo.elementBufferId);
          glDeleteVertexArrays(1, &meshInfo.vertexArrayId);
        }
      }
    }
  }
}
