#include <cmath>
#include <iostream>
#include <unordered_map>
#include <vector>

#include "camera.hpp"

#include "../maths/angle.hpp"
#include "../maths/vector.hpp"
#include "../utils/debug.hpp"
#include "../utils/id.hpp"
#include "../utils/logging.hpp"
#include "../utils/timer.hpp"

namespace ammonite {
  namespace camera {
    namespace {
      struct PathNode {
        ammonite::Vec<float, 3> position;
        double horizontalAngle;
        double verticalAngle;
        double time;
      };

      struct Path {
        AmmoniteId linkedCameraId = 0;
        ammonite::utils::Timer pathTimer;
        std::vector<PathNode> pathNodes;
        unsigned int selectedIndex = 0;
      };

      AmmoniteId lastPathId = 1;
      std::unordered_map<AmmoniteId, Path> pathTrackerMap;
    }

    namespace {
      double calculateVerticalAngle(const ammonite::Vec<float, 3>& direction) {
        ammonite::Vec<float, 3> normalisedDirection = {0};
        ammonite::normalise(direction, normalisedDirection);
        return std::asin(normalisedDirection[1]);
      }

      double calculateHorizontalAngle(const ammonite::Vec<float, 3>& direction) {
        ammonite::Vec<float, 3> normalisedDirection = {0};
        ammonite::copy(direction, normalisedDirection);
        normalisedDirection[1] = 0.0f;

        ammonite::normalise(normalisedDirection);
        return std::atan2(normalisedDirection[0], normalisedDirection[2]);
      }

      double smallestAngleDelta(double angleA, double angleB) {
        double angleDelta = angleA - angleB;
        if (angleDelta < -ammonite::pi<float>()) {
          angleDelta += ammonite::pi<float>() * 2.0f;
        } else if (angleDelta > ammonite::pi<float>()) {
          angleDelta -= ammonite::pi<float>() * 2.0f;
        }

        return angleDelta;
      }
    }

    namespace path {
      namespace internal {
        bool setLinkedCamera(AmmoniteId pathId, AmmoniteId cameraId) {
          if (!pathTrackerMap.contains(pathId)) {
            ammonite::utils::warning << "Couldn't find camera path with ID '" \
                                     << pathId << "'" << std::endl;
            return false;
          }

          pathTrackerMap[pathId].linkedCameraId = cameraId;
          return true;
        }

        void removeCameraLink(AmmoniteId pathId) {
          pathTrackerMap[pathId].linkedCameraId = 0;
        }

        void updateCamera(AmmoniteId pathId) {
          Path& cameraPath = pathTrackerMap[pathId];
          const double currentTime = cameraPath.pathTimer.getTime();

          //Check the node still exists
          if (cameraPath.selectedIndex >= cameraPath.pathNodes.size()) {
            ammoniteInternalDebug << "Selected camera path node no longer exists" << std::endl;
            return;
          }

          //Find the last reached node
          unsigned int selectedIndex = cameraPath.selectedIndex;
          while (true) {
            //Check for the end of the path
            const unsigned int nextSelectedIndex = selectedIndex + 1;
            if (nextSelectedIndex >= cameraPath.pathNodes.size()) {
              break;
            }

            //Store the new index or break
            const PathNode& nextNode = cameraPath.pathNodes[nextSelectedIndex];
            if (nextNode.time <= currentTime) {
              //Node has been reached, try the next
              selectedIndex = nextSelectedIndex;
            } else {
              //Node hasn't been reached yet
              break;
            }
          }

          //Store the selected index and fetch the node
          cameraPath.selectedIndex = selectedIndex;
          const PathNode& currentNode = cameraPath.pathNodes[selectedIndex];

          //Fetch the next node, reusing the current node if we're at the end
          const bool isEnd = ((unsigned int)(selectedIndex + 1) >= cameraPath.pathNodes.size());
          const unsigned int nextIndex = isEnd ? selectedIndex : selectedIndex + 1;
          const PathNode& nextNode = cameraPath.pathNodes[nextIndex];

          //Interpolate camera position between the node pair
          const double nodeTimeDelta = nextNode.time - currentNode.time;
          const double timeDelta = currentTime - currentNode.time;
          double nodeProgress = 0.0;
          if (nodeTimeDelta != 0.0) {
            nodeProgress = timeDelta / nodeTimeDelta;
          }

          //Find vector between the nodes
          ammonite::Vec<float, 3> nodePositionDelta;
          ammonite::sub(nextNode.position, currentNode.position, nodePositionDelta);

          //Find position between the nodes along the vector
          ammonite::Vec<float, 3> newPosition;
          ammonite::scale(nodePositionDelta, (float)nodeProgress, newPosition);
          ammonite::add(newPosition, currentNode.position);

          //Find smallest delta between node angles
          const double horizontalDelta = smallestAngleDelta(nextNode.horizontalAngle,
                                                            currentNode.horizontalAngle);
          const double verticalDelta = smallestAngleDelta(nextNode.verticalAngle,
                                                          currentNode.verticalAngle);

          //Apply the deltas
          const double newHorizontal = currentNode.horizontalAngle + (horizontalDelta * nodeProgress);
          const double newVertical = currentNode.verticalAngle + (verticalDelta * nodeProgress);

          //Apply the new position and direction
          const AmmoniteId cameraId = cameraPath.linkedCameraId;
          ammonite::camera::setPosition(cameraId, newPosition);
          ammonite::camera::setAngle(cameraId, newHorizontal, newVertical);
        }
      }

      AmmoniteId createCameraPath(unsigned int size) {
        //Get an ID for the new path
        const AmmoniteId pathId = utils::internal::setNextId(&lastPathId, pathTrackerMap);

        //Add a new path to the tracker
        pathTrackerMap[pathId] = {};

        if (size != 0) {
          pathTrackerMap[pathId].pathNodes.reserve(size);
        }

        return pathId;
      }

      AmmoniteId createCameraPath() {
        return createCameraPath(0);
      }

      //Destroy a camera path and all nodes contained
      void destroyCameraPath(AmmoniteId pathId) {
        //Check the path exists
        if (!pathTrackerMap.contains(pathId)) {
          ammonite::utils::warning << "Couldn't find camera path with ID '" \
                                   << pathId << "'" << std::endl;
          return;
        }

        //Unlink any linked camera
        const Path* const cameraPathPtr = &pathTrackerMap[pathId];
        if (cameraPathPtr->linkedCameraId != 0) {
          setLinkedPath(cameraPathPtr->linkedCameraId, 0);
        }

        //Delete the path
        pathTrackerMap.erase(pathId);
        ammoniteInternalDebug << "Deleted storage for camera path (ID " \
                              << pathId << ")" << std::endl;
      }

      //Reserve space for path nodes, for performance
      void reserveCameraPath(AmmoniteId pathId, unsigned int size) {
        if (!pathTrackerMap.contains(pathId)) {
          ammonite::utils::warning << "Couldn't find camera path with ID '" \
                                   << pathId << "'" << std::endl;
          return;
        }

        pathTrackerMap[pathId].pathNodes.reserve(size);
      }

      /*
       - Add a node to an existing path by angle pair, position and time
       - Returns the index of the node on success, 0 on failure
         - Since 0 is a valid index, use getPathNodeCount() to confirm
      */
      unsigned int addPathNode(AmmoniteId pathId,
                               const ammonite::Vec<float, 3>& position,
                               double horizontal, double vertical,
                               double time) {
        if (!pathTrackerMap.contains(pathId)) {
          ammonite::utils::warning << "Couldn't find camera path with ID '" \
                                   << pathId << "'" << std::endl;
          return 0;
        }

        //Add the camera path node
        pathTrackerMap[pathId].pathNodes.push_back(
          {{position[0], position[1], position[2]}, horizontal, vertical, time});

        //Return its index
        return pathTrackerMap[pathId].pathNodes.size() - 1;
      }

      //Same as the angle version of addPathNode(), using a vector instead
      unsigned int addPathNode(AmmoniteId pathId,
                               const ammonite::Vec<float, 3>& position,
                               const ammonite::Vec<float, 3>& direction,
                               double time) {
        const double horizontal = calculateHorizontalAngle(direction);
        const double vertical = calculateVerticalAngle(direction);
        return addPathNode(pathId, position, horizontal, vertical, time);
      }

      /*
       - Removes a node from a path by its index
       - Changes the index of each node following it
      */
      void removePathNode(AmmoniteId pathId, unsigned int nodeIndex) {
        if (!pathTrackerMap.contains(pathId)) {
          ammonite::utils::warning << "Couldn't find camera path with ID '" \
                                   << pathId << "'" << std::endl;
          return;
        }

        //Check the node exists in the path
        std::vector<PathNode>& pathNodes = pathTrackerMap[pathId].pathNodes;
        if (pathNodes.size() <= nodeIndex) {
          ammonite::utils::warning << "Can't remove node index " << nodeIndex \
                                   << " from a path of size " << pathNodes.size();
          return;
        }

        //Delete the path node
        pathNodes.erase(pathNodes.begin() + nodeIndex);
      }

      unsigned int getPathNodeCount(AmmoniteId pathId) {
        if (!pathTrackerMap.contains(pathId)) {
          ammonite::utils::warning << "Couldn't find camera path with ID '" \
                                   << pathId << "'" << std::endl;
          return 0;
        }

        return pathTrackerMap[pathId].pathNodes.size();
      }

      void playPath(AmmoniteId pathId) {
        if (!pathTrackerMap.contains(pathId)) {
          ammonite::utils::warning << "Couldn't find camera path with ID '" \
                                   << pathId << "'" << std::endl;
          return;
        }

         Path& cameraPath = pathTrackerMap[pathId];
         cameraPath.pathTimer.unpause();
      }

      void pausePath(AmmoniteId pathId) {
        if (!pathTrackerMap.contains(pathId)) {
          ammonite::utils::warning << "Couldn't find camera path with ID '" \
                                   << pathId << "'" << std::endl;
          return;
        }

         Path& cameraPath = pathTrackerMap[pathId];
         cameraPath.pathTimer.pause();
      }

      void restartPath(AmmoniteId pathId) {
        if (!pathTrackerMap.contains(pathId)) {
          ammonite::utils::warning << "Couldn't find camera path with ID '" \
                                   << pathId << "'" << std::endl;
          return;
        }

         Path& cameraPath = pathTrackerMap[pathId];
         cameraPath.pathTimer.reset();
         cameraPath.selectedIndex = 0;
      }
    }
  }
}
