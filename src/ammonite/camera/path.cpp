#include <algorithm>
#include <cmath>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "camera.hpp"

#include "../engine.hpp"
#include "../maths/angle.hpp"
#include "../maths/vector.hpp"
#include "../utils/debug.hpp"
#include "../utils/id.hpp"
#include "../utils/logging.hpp"

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
        double currentTime = 0.0;
        bool isPathPlaying = false;
        std::vector<PathNode> pathNodes;
        unsigned int selectedIndex = 0; //Used for performance, not config
        AmmonitePathMode pathMode = AMMONITE_PATH_FORWARD;
      };

      AmmoniteId lastPathId = 1;
      std::unordered_map<AmmoniteId, Path> pathTrackerMap;

      //Store the IDs of paths with cameras updated since the last automatic update
      std::unordered_set<AmmoniteId> updatedPaths;
    }

    namespace path {
      namespace {
        /*
         - Reset the time and node if looping and restarting
         - Return the time written to the path
        */
        double loopPathTime(Path& cameraPath, double currentTime) {
          const unsigned int nodeCount = cameraPath.pathNodes.size();
          const double maxNodeTime = cameraPath.pathNodes[nodeCount - 1].time;

          //Don't do anything if the path is paused
          if (!cameraPath.isPathPlaying) {
            return currentTime;
          }

          /*
           - If the path is a loop and needs to restart:
             - Reset the index to the start
             - Set the time to however far the loop overshot
          */
          double newTime = currentTime;
          if (cameraPath.pathMode == AMMONITE_PATH_LOOP) {
            if (newTime >= maxNodeTime) {
              //Reset the time, preserving the overshoot
              cameraPath.currentTime = std::fmod(newTime, maxNodeTime);
              newTime = cameraPath.currentTime;

              //Return to the first node
              cameraPath.selectedIndex = 0;
            }
          }

          return newTime;
        }

        void updatePathTime(AmmoniteId pathId, double timeDelta) {
          Path& cameraPath = pathTrackerMap[pathId];
          double newTime = cameraPath.currentTime + timeDelta;

          //Don't modify the time if the path is paused
          if (!cameraPath.isPathPlaying) {
            return;
          }

          /*
           - Handle time resets for looping paths
           - This already updates the time if it changed, but set it again
             to keep the code path simple
          */
          newTime = loopPathTime(cameraPath, newTime);

          //Save the time
          cameraPath.currentTime = newTime;
        }
      }

      namespace internal {
        //Update the stored link for pathId, optionally unlink the existing camera
        bool setLinkedCamera(AmmoniteId pathId, AmmoniteId cameraId,
                             bool unlinkExisting) {
          //Ignore reset requests for path 0
          if (pathId == 0 && cameraId == 0) {
            ammoniteInternalDebug << "Ignored camera reset request for path ID 0" << std::endl;
            return true;
          }

          //Check the camera path exists
          if (!pathTrackerMap.contains(pathId)) {
            ammonite::utils::warning << "Can't find camera path (ID " \
                                     << cameraId << ") to unlink" << std::endl;
            return false;
          }

          //Reset the linked path on any already linked camera, if requested
          if (unlinkExisting) {
            if (!camera::internal::setLinkedPath(pathTrackerMap[pathId].linkedCameraId, 0, false)) {
              ammonite::utils::warning << "Failed to unlink camera (ID " \
                                       << pathTrackerMap[pathId].linkedCameraId \
                                       << ") from path (ID " << pathId << ")" << std::endl;
              return false;
            }
          }

          //If the path was marked as updated, forget it
          if (updatedPaths.contains(pathId)) {
            updatedPaths.erase(pathId);
          }

          //Set the camera on the path
          pathTrackerMap[pathId].linkedCameraId = cameraId;
          return true;
        }

        /*
         - Update the camera for the path, if it hasn't been done manually
         - Called once per frame
        */
        void updateCamera(AmmoniteId pathId) {
          if (!updatedPaths.contains(pathId)) {
            ammonite::camera::path::updateCameraForPath(pathId);
          }

          updatedPaths.clear();
        }
      }

      /*
       - Update the time for all registered paths
       - Only call this once per frame
      */
      void updatePathProgress() {
        //Get the time since the last frame
        const double frameTime = ammonite::getFrameTime();

        //Apply the time to every path
        for (auto& pathData : pathTrackerMap) {
          updatePathTime(pathData.first, frameTime);
        }
      }

      /*
       - Call updateCameraForPath() with the path ID of the active camera
       - If the active camera isn't on a path, do nothing
      */
      void updateActiveCameraOnPath() {
        const AmmoniteId activeCameraId = camera::getActiveCamera();
        const AmmoniteId pathId = camera::getLinkedPath(activeCameraId);
        if (pathId != 0) {
          updateCameraForPath(pathId);
        }
      }

      /*
       - Calculate the new position, direction and path state for the mode
         - Write it to the linked camera
         - This may modify the path time for looped paths
       - This function will automatically be called during rendering, only if
         it hasn't already been manually called for that frame
       - Call this early if the camera's values need to be queried,
         after path movements have been applied
      */
      void updateCameraForPath(AmmoniteId pathId) {
        Path& cameraPath = pathTrackerMap[pathId];
        const unsigned int nodeCount = cameraPath.pathNodes.size();
        const double maxNodeTime = cameraPath.pathNodes[nodeCount - 1].time;

        //Record that the camera was updated
        updatedPaths.insert(pathId);

        //Skip 0 length paths
        if (nodeCount == 0) {
          return;
        }

        /*
         - Handle time resets for looping paths
         - Generally, this would be handled by updatePathProgress(), but
           the path mode may have changed since the call
        */
        const double currentTime = loopPathTime(cameraPath, cameraPath.currentTime);

        //Check the node still exists
        if (cameraPath.selectedIndex >= nodeCount) {
          ammoniteInternalDebug << "Selected camera path node no longer exists, resetting" << std::endl;
          cameraPath.selectedIndex = 0;
        }

        //Find the last reached node
        unsigned int selectedIndex = cameraPath.selectedIndex;
        while (true) {
          //Select the next node, according to the mode
          unsigned int nextSelectedIndex = 0;
          bool isLastNode = false;
          switch (cameraPath.pathMode) {
          case AMMONITE_PATH_FORWARD:
          case AMMONITE_PATH_REVERSE:
            nextSelectedIndex = selectedIndex + 1;
            if (nextSelectedIndex >= nodeCount) {
              isLastNode = true;
            }
            break;
          case AMMONITE_PATH_LOOP:
            nextSelectedIndex = (selectedIndex + 1) % nodeCount;
            break;
          }

          //No more nodes to try for linear modes
          if (isLastNode) {
            break;
          }

          //Use corresponding index from the other end in reverse mode
          unsigned int realNextIndex = nextSelectedIndex;
          if (cameraPath.pathMode == AMMONITE_PATH_REVERSE) {
            realNextIndex = nodeCount - (nextSelectedIndex + 1);
          }

          //Determine the node's time
          const PathNode& nextNode = cameraPath.pathNodes[realNextIndex];

          //Find the time this node occurs at
          double nodeTime = 0.0;
          switch (cameraPath.pathMode) {
          case AMMONITE_PATH_FORWARD:
          case AMMONITE_PATH_LOOP:
            nodeTime = nextNode.time;
            break;
          case AMMONITE_PATH_REVERSE:
            //Subtract the node time from the final node's time
            nodeTime = maxNodeTime - nextNode.time;
            break;
          }

          //Store the new index or break
          if (nodeTime <= currentTime) {
            //Node has been reached, try the next
            selectedIndex = nextSelectedIndex;
          } else {
            //Node hasn't been reached yet
            break;
          }
        }

        //Use corresponding index from the other end in reverse mode
        unsigned int realSelectedIndex = selectedIndex;
        if (cameraPath.pathMode == AMMONITE_PATH_REVERSE) {
          realSelectedIndex = nodeCount - (selectedIndex + 1);
        }

        //Store the selected index and fetch the node
        cameraPath.selectedIndex = selectedIndex;
        const PathNode& currentNode = cameraPath.pathNodes[realSelectedIndex];

        //Select the next node and detect the end
        bool isEnd = false;
        unsigned int nextIndex = 0;
        switch (cameraPath.pathMode) {
        case AMMONITE_PATH_FORWARD:
        case AMMONITE_PATH_REVERSE:
          nextIndex = selectedIndex + 1;
          isEnd = (nextIndex >= nodeCount);
          break;
        case AMMONITE_PATH_LOOP:
          nextIndex = (selectedIndex + 1) % nodeCount;
          break;
        }

        //Reuse the current node if the next node is past the end
        nextIndex = isEnd ? selectedIndex : nextIndex;

        //Use corresponding index from the other end in reverse mode
        unsigned int realNextIndex = nextIndex;
        if (cameraPath.pathMode == AMMONITE_PATH_REVERSE) {
          realNextIndex = nodeCount - (nextIndex + 1);
        }

        //Fetch the next node
        const PathNode& nextNode = cameraPath.pathNodes[realNextIndex];

        //Find the node time delta and the time between now and the current node
        double nodeTimeDelta = 0.0;
        double timeDelta = 0.0;
        if (cameraPath.pathMode != AMMONITE_PATH_REVERSE) {
          nodeTimeDelta = nextNode.time - currentNode.time;
          timeDelta = currentTime - currentNode.time;
        } else {
          nodeTimeDelta = currentNode.time - nextNode.time;
          timeDelta = currentNode.time - (maxNodeTime - currentTime);
        }

        //Override time deltas for loop end
        if (cameraPath.pathMode == AMMONITE_PATH_LOOP) {
          if (nextIndex == 0) {
            nodeTimeDelta = 0.0;
            timeDelta = 0.0;
          }
        }

        //Find the progress between the nodes
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
        const double horizontalDelta =
          ammonite::smallestAngleDelta(nextNode.horizontalAngle, currentNode.horizontalAngle);
        const double verticalDelta =
          ammonite::smallestAngleDelta(nextNode.verticalAngle, currentNode.verticalAngle);

        //Apply the deltas
        const double newHorizontal = currentNode.horizontalAngle + (horizontalDelta * nodeProgress);
        const double newVertical = currentNode.verticalAngle + (verticalDelta * nodeProgress);

        //Apply the new position and direction
        ammonite::camera::setPosition(cameraPath.linkedCameraId, newPosition);
        ammonite::camera::setAngle(cameraPath.linkedCameraId, newHorizontal, newVertical);
      }

      //Create a new camera, reserving 'size' nodes
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

      //Create a new camera, without reserving nodes
      AmmoniteId createCameraPath() {
        return createCameraPath(0);
      }

      //Delete a camera path and all nodes contained
      void deleteCameraPath(AmmoniteId pathId) {
        //Check the path exists
        if (!pathTrackerMap.contains(pathId)) {
          ammonite::utils::warning << "Couldn't find camera path with ID '" \
                                   << pathId << "'" << std::endl;
          return;
        }

        //Unlink any linked camera
        const Path* const cameraPathPtr = &pathTrackerMap[pathId];
        camera::internal::setLinkedPath(cameraPathPtr->linkedCameraId, 0, false);

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
        pathTrackerMap[pathId].pathNodes.push_back({
          .position = {position[0], position[1], position[2]},
          .horizontalAngle = horizontal, .verticalAngle = vertical, .time = time});

        //Return its index
        return pathTrackerMap[pathId].pathNodes.size() - 1;
      }

      //Same as the angle version of addPathNode(), using a vector instead
      unsigned int addPathNode(AmmoniteId pathId,
                               const ammonite::Vec<float, 3>& position,
                               const ammonite::Vec<float, 3>& direction,
                               double time) {
        const double horizontal = ammonite::calculateHorizontalAngle(direction);
        const double vertical = ammonite::calculateVerticalAngle(direction);
        return addPathNode(pathId, position, horizontal, vertical, time);
      }

      /*
       - Removes a node from a path by its index
       - Changes the index of each node following it
       - If the path shrinks below the currently reached node, it'll restart
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

      //Set the node traversal mode
      void setPathMode(AmmoniteId pathId, AmmonitePathMode pathMode) {
        if (!pathTrackerMap.contains(pathId)) {
          ammonite::utils::warning << "Couldn't find camera path with ID '" \
                                   << pathId << "'" << std::endl;
          return;
        }

         Path& cameraPath = pathTrackerMap[pathId];
         cameraPath.pathMode = pathMode;
      }

      AmmonitePathMode getPathMode(AmmoniteId pathId) {
        if (!pathTrackerMap.contains(pathId)) {
          ammonite::utils::warning << "Couldn't find camera path with ID '" \
                                   << pathId << "'" << std::endl;
          return AMMONITE_PATH_FORWARD;
        }

        return pathTrackerMap[pathId].pathMode;
      }

      void playPath(AmmoniteId pathId) {
        if (!pathTrackerMap.contains(pathId)) {
          ammonite::utils::warning << "Couldn't find camera path with ID '" \
                                   << pathId << "'" << std::endl;
          return;
        }

        Path& cameraPath = pathTrackerMap[pathId];
        cameraPath.isPathPlaying = true;
      }

      void pausePath(AmmoniteId pathId) {
        if (!pathTrackerMap.contains(pathId)) {
          ammonite::utils::warning << "Couldn't find camera path with ID '" \
                                   << pathId << "'" << std::endl;
          return;
        }

        Path& cameraPath = pathTrackerMap[pathId];
        cameraPath.isPathPlaying = false;
      }

      bool getPathPaused(AmmoniteId pathId) {
        if (!pathTrackerMap.contains(pathId)) {
          ammonite::utils::warning << "Couldn't find camera path with ID '" \
                                   << pathId << "'" << std::endl;
          return false;
        }

        return pathTrackerMap[pathId].isPathPlaying;
      }

      //Jump to a given node
      void setNode(AmmoniteId pathId, unsigned int nodeIndex) {
        if (!pathTrackerMap.contains(pathId)) {
          ammonite::utils::warning << "Couldn't find camera path with ID '" \
                                   << pathId << "'" << std::endl;
          return;
        }

        Path& cameraPath = pathTrackerMap[pathId];
        nodeIndex = std::min(nodeIndex, (unsigned int)cameraPath.pathNodes.size() - 1);
        cameraPath.currentTime = cameraPath.pathNodes[nodeIndex].time;
      }

      //Jump to a given point in time
      void setTime(AmmoniteId pathId, double time) {
        if (!pathTrackerMap.contains(pathId)) {
          ammonite::utils::warning << "Couldn't find camera path with ID '" \
                                   << pathId << "'" << std::endl;
          return;
        }

        Path& cameraPath = pathTrackerMap[pathId];
        cameraPath.currentTime = time;
      }

      //Jump to a point in time, relative to 1.0 as the end
      void setProgress(AmmoniteId pathId, double progress) {
        if (!pathTrackerMap.contains(pathId)) {
          ammonite::utils::warning << "Couldn't find camera path with ID '" \
                                   << pathId << "'" << std::endl;
          return;
        }

        Path& cameraPath = pathTrackerMap[pathId];
        const PathNode& maxNode = cameraPath.pathNodes[(cameraPath.pathNodes.size() - 1)];
        cameraPath.currentTime = maxNode.time * progress;
      }

      //Return the current point in time
      double getTime(AmmoniteId pathId) {
        if (!pathTrackerMap.contains(pathId)) {
          ammonite::utils::warning << "Couldn't find camera path with ID '" \
                                   << pathId << "'" << std::endl;
          return 0.0;
        }

        const Path& cameraPath = pathTrackerMap[pathId];
        const PathNode& maxNode = cameraPath.pathNodes[(cameraPath.pathNodes.size() - 1)];
        return std::min(cameraPath.currentTime, maxNode.time);
      }

      //Return the current progress, relative to 1.0 as the end
      double getProgress(AmmoniteId pathId) {
        if (!pathTrackerMap.contains(pathId)) {
          ammonite::utils::warning << "Couldn't find camera path with ID '" \
                                   << pathId << "'" << std::endl;
          return 0.0;
        }

        const Path& cameraPath = pathTrackerMap[pathId];
        const PathNode& maxNode = cameraPath.pathNodes[(cameraPath.pathNodes.size() - 1)];
        const double progress = cameraPath.currentTime / maxNode.time;
        return std::min(progress, 1.0);
      }

      bool getPathComplete(AmmoniteId pathId) {
        return (getProgress(pathId) == 1.0);
      }

      //Reset a path back to the start
      void restartPath(AmmoniteId pathId) {
        if (!pathTrackerMap.contains(pathId)) {
          ammonite::utils::warning << "Couldn't find camera path with ID '" \
                                   << pathId << "'" << std::endl;
          return;
        }

        Path& cameraPath = pathTrackerMap[pathId];
        cameraPath.currentTime = 0.0;
        cameraPath.selectedIndex = 0;
      }
    }
  }
}
