#ifndef INTERNALLIGHTS
#define INTERNALLIGHTS

#include <map>
#include <vector>
#include <glm/glm.hpp>

/* Internally exposed header:
 - Allow access to light tracker internally
 - Allow access to light transforms internally
*/

namespace ammonite {
  namespace lighting {
    struct LightSource {
      glm::vec3 geometry = glm::vec3(0.0f, 0.0f, 0.0f);
      glm::vec3 diffuse = glm::vec3(1.0f, 1.0f, 1.0f);
      glm::vec3 specular = glm::vec3(0.3f, 0.3f, 0.3f);
      float power = 1.0f;
      int lightId;
      int modelId = -1;
    };

    void unlinkByModel(int modelId);
    LightSource* getLightSourcePtr(int lightId);
    void getLightEmitters(int* lightCount, std::vector<int>* lightData);

    std::map<int, LightSource>* getLightTracker();
    std::map<int, glm::mat4[6]>* getLightTransforms();
  }
}

#endif
