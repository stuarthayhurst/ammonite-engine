#include <glm/glm.hpp>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "modelManager.hpp"
#include "utils/timer.hpp"

#ifdef DEBUG
  #include <iostream>
#endif

namespace ammonite {
  namespace renderer {
    namespace {
      GLFWwindow* window;
      GLuint programId;

      //Shader uniform IDs
      GLuint matrixId;
      GLuint modelMatrixId;
      GLuint viewMatrixId;
      GLuint normalMatrixId;
      GLuint textureSamplerId;
      GLuint lightId;

      long totalFrames = 0;
      int frameCount = 0;
      double frameTime = 0.0f;
    }

    namespace setup {
      void setupRenderer(GLFWwindow* targetWindow, GLuint targetProgramId) {
        //Set window and shader to be used
        window = targetWindow;
        programId = targetProgramId;

        //Shader uniform locations
        matrixId = glGetUniformLocation(programId, "MVP");
        modelMatrixId = glGetUniformLocation(programId, "M");
        viewMatrixId = glGetUniformLocation(programId, "V");
        normalMatrixId = glGetUniformLocation(programId, "normalMatrix");
        textureSamplerId = glGetUniformLocation(programId, "textureSampler");
        lightId = glGetUniformLocation(programId, "LightPosition_worldspace");

        //Enable culling triangles and depth testing (only show fragments closer than the previous)
        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);

        //Use the shader
        glUseProgram(programId);
      }
    }

    namespace {
      static void setWireframe(bool enabled) {
        if (enabled) {
          glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        } else {
          glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
      }

      static void drawModel(ammonite::models::InternalModel *drawObject, const glm::mat4 viewProjectionMatrix) {
        //If the model is disabled, skip it
        if (!drawObject->active) {
          return;
        }

        ammonite::models::InternalModelData* drawObjectData = drawObject->data;
        //Bind texture in Texture Unit 0
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, drawObject->textureId);
        //Set texture sampler to use Texture Unit 0
        glUniform1i(textureSamplerId, 0);

        //Bind vertex attribute buffer
        glBindVertexArray(drawObjectData->vertexArrayId);

        //Calculate matrices
        glm::mat4 rotationMatrix = glm::toMat4(drawObject->positionData.rotationQuat);
        glm::mat4 modelMatrix = drawObject->positionData.translationMatrix * rotationMatrix * drawObject->positionData.scaleMatrix;
        glm::mat4 mvp = viewProjectionMatrix * modelMatrix;
        glm::mat3 normalMatrix = glm::transpose(glm::inverse(modelMatrix));

        //Send matrices to the shaders
        glUniformMatrix4fv(matrixId, 1, GL_FALSE, &mvp[0][0]);
        glUniformMatrix4fv(modelMatrixId, 1, GL_FALSE, &modelMatrix[0][0]);
        glUniformMatrix3fv(normalMatrixId, 1, GL_FALSE, &normalMatrix[0][0]);

        //Use wireframe if requested
        setWireframe(drawObject->wireframe);

        //Draw the triangles
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, drawObjectData->elementBufferId);
        glDrawElements(GL_TRIANGLES, drawObjectData->vertexCount, GL_UNSIGNED_INT, (void*)0);
      }
    }

    long getTotalFrames() {
      return totalFrames;
    }

    double getFrameTime() {
      return frameTime;
    }

    void drawFrame(const int modelIds[], const int modelCount, glm::mat4 projectionMatrix, glm::mat4 viewMatrix) {
      //Reset the canvas
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      //Increase frame counters
      totalFrames++;
      frameCount++;

      //Every half second, update the frame time
      static ammonite::utils::Timer frameTimer;
      double deltaTime = frameTimer.getTime();
      if (deltaTime >= 0.5) {
        frameTime = (deltaTime) / frameCount;
        frameTimer.reset();
        frameCount = 0;
      }

      glm::vec3 lightPos = glm::vec3(4, 4, 4);
      glUniform3f(lightId, lightPos.x, lightPos.y, lightPos.z);

      //Send view matrix to shader
      glUniformMatrix4fv(viewMatrixId, 1, GL_FALSE, &viewMatrix[0][0]);
      const glm::mat4 viewProjectionMatrix = projectionMatrix * viewMatrix;

      //Draw given model
      for (int i = 0; i < modelCount; i++) {
        drawModel(ammonite::models::getModelPtr(modelIds[i]), viewProjectionMatrix);
      }

      //Swap buffers
      glfwSwapBuffers(window);
      glfwPollEvents();
    }
  }
}
