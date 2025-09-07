#include <exception>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include <ammonite/ammonite.hpp>

#include "commands.hpp"

//Command helpers
namespace {
  /*
   - Return true if at least count arguments are passed, excluding the command
   - Otherwise, send a warning message if requested and return false
  */
  bool checkArgumentCount(const std::vector<std::string>& arguments,
                          unsigned int count, bool showMessage) {
    if (arguments.size() > count) {
      return true;
    }

    if (showMessage) {
      ammonite::utils::warning << "At least " << count << " argument(s) expected, " \
                               << arguments.size() - 1 << " received" << std::endl;
    }

    return false;
  }

  /*
   - Return true if the key is present
   - Return false and send a warning message otherwise
  */
  template <typename T>
  bool checkKeyValid(const std::unordered_map<std::string, T>& map,
                     const std::string& key) {
    if (!map.contains(key)) {
      ammonite::utils::warning << "'" << key << "' isn't a valid key " << std::endl;
      return false;
    }

    return true;
  }

  //Dump the keys of an unordered_map with string keys
  template <typename T>
  void dumpKeys(const std::unordered_map<std::string, T>& map) {
    ammonite::utils::normal << "Supported keys: ";
    bool isStart = true;
    for (const auto& entry : map) {
      if (isStart) {
        isStart = false;
      } else {
        ammonite::utils::normal << ", ";
      }

      ammonite::utils::normal << entry.first;
    }

    ammonite::utils::normal << std::endl;
  }

  std::string boolToString(bool x) {
    return x ? "true" : "false";
  }

  /*
   - Convert a string to a boolean in *value
   - Return true on success, otherwise return false and send a warning
  */
  bool stringToBool(const std::string& string, bool* value) {
    if (string == "true") {
      *value = true;
    } else if (string == "false") {
      *value = false;
    } else {
      ammonite::utils::warning << "Expected a boolean, got '" << string << "'" << std::endl;
      return false;
    }

    return true;
  }

  /*
   - Convert a string to a float in *value
   - Return true on success, otherwise return false and send a warning
  */
  bool stringToFloat(const std::string& string, float* value) {
    try {
      *value = std::stof(string);
    } catch (const std::exception&) {
      ammonite::utils::warning << "Expected a float, got '" << string << "'" << std::endl;
      return false;
    }

    return true;
  }

  /*
   - Convert a string to an unsigned int in *value
   - Return true on success, otherwise return false and send a warning
  */
  bool stringToUInt(const std::string& string, unsigned int* value) {
    try {
      *value = (unsigned int)std::stoul(string);
    } catch (const std::exception&) {
      ammonite::utils::warning << "Expected an unsigned int, got '" << string << "'" << std::endl;
      return false;
    }

    return true;
  }

  /*
   - Convert an array of strings to a vector of floats in value
   - Return true on success, otherwise return false and send a warning
  */
  bool stringToFloatVector(const std::string* string, ammonite::Vec<float, 3>& value) {
    bool success = true;
    for (unsigned int i = 0; i < 3; i++) {
      success &= stringToFloat(string[i], &value[i]);
    }

    return success;
  }
}

//Definitions for commands handlers
namespace {
  enum ReturnActionEnum : unsigned char {
    CONTINUE,
    EXIT_COMMANDS,
    EXIT_PROGRAM
  };

  using CommandHandler = ReturnActionEnum (*)(const std::vector<std::string>& commandLine);
}

/*
 - Command implementations
 - Each command is self-contained and anonymous
 - Commands must be added to commandMap, and have an entry in helpCommand
*/

namespace {
  ReturnActionEnum helpCommand(const std::vector<std::string>&) {
    ammonite::utils::normal << "Command help:" << std::endl;
    ammonite::utils::normal << "  'help'                        : Display this help page" << std::endl;
    ammonite::utils::normal << "  'get [key]'                   : Get the value of a setting key" << std::endl;
    ammonite::utils::normal << "  'set [key] [value]'           : Set the value of a setting key" << std::endl;
    ammonite::utils::normal << "  'camera' [mode] [key] [value] : Get / set camera properties" << std::endl;
    ammonite::utils::normal << "  'models'                      : Dump model system data (debug mode)" << std::endl;
    ammonite::utils::normal << "  'exit'                        : Exit the command system" << std::endl;
    ammonite::utils::normal << "  'stop'                        : Stop the program" << std::endl;
    ammonite::utils::normal << " - Leave [key] blank to list keys" << std::endl;

    return CONTINUE;
  }
}

namespace {
  enum SettingKeyEnum : unsigned char {
    FocalDepthEnabledKey,
    FocalDepthKey,
    BlurStrengthKey,
    VsyncKey,
    FrameLimitKey,
    ShadowResolutionKey,
    RenderResolutionMultiplierKey,
    AntialiasingSamplesKey,
    RenderFarPlaneKey,
    ShadowFarPlaneKey,
    GammaCorrectionEnabledKey
  };

  /*
   - Convert a setting key to an enum for matching
   - Keys added here must be handled in getCommand() and setCommand()
  */
  const std::unordered_map<std::string, SettingKeyEnum> settingKeyMap = {
    {"focalDepthEnabled", FocalDepthEnabledKey},
    {"focalDepth", FocalDepthKey},
    {"blurStrength", BlurStrengthKey},
    {"vsync", VsyncKey},
    {"frameLimit", FrameLimitKey},
    {"shadowRes", ShadowResolutionKey},
    {"renderResMul", RenderResolutionMultiplierKey},
    {"aaSamples", AntialiasingSamplesKey},
    {"renderFarPlane", RenderFarPlaneKey},
    {"shadowFarPlane", ShadowFarPlaneKey},
    {"gammaCorrection", GammaCorrectionEnabledKey}
  };

  ReturnActionEnum getCommand(const std::vector<std::string>& arguments) {
    //Print the setting keys if none were given
    if (!checkArgumentCount(arguments, 1, false)) {
      dumpKeys(settingKeyMap);
      return CONTINUE;
    }

    //Filter out unknown keys
    if (!checkKeyValid(settingKeyMap, arguments[1])) {
      return CONTINUE;
    }

    //Match the key against handlers
    std::string result;
    switch (settingKeyMap.at(arguments[1])) {
    case FocalDepthEnabledKey:
      result = boolToString(ammonite::renderer::settings::post::getFocalDepthEnabled());
      break;
    case FocalDepthKey:
      result = std::to_string(ammonite::renderer::settings::post::getFocalDepth());
      break;
    case BlurStrengthKey:
      result = std::to_string(ammonite::renderer::settings::post::getBlurStrength());
      break;
    case VsyncKey:
      result = boolToString(ammonite::renderer::settings::getVsync());
      break;
    case FrameLimitKey:
      result = std::to_string(ammonite::renderer::settings::getFrameLimit());
      break;
    case ShadowResolutionKey:
      result = std::to_string(ammonite::renderer::settings::getShadowRes());
      break;
    case RenderResolutionMultiplierKey:
      result = std::to_string(ammonite::renderer::settings::getRenderResMultiplier());
      break;
    case AntialiasingSamplesKey:
      result = std::to_string(ammonite::renderer::settings::getAntialiasingSamples());
      break;
    case RenderFarPlaneKey:
      result = std::to_string(ammonite::renderer::settings::getRenderFarPlane());
      break;
    case ShadowFarPlaneKey:
      result = std::to_string(ammonite::renderer::settings::getShadowFarPlane());
      break;
    case GammaCorrectionEnabledKey:
      result = boolToString(ammonite::renderer::settings::getGammaCorrection());
      break;
    }

    //Print the key and return
    ammonite::utils::normal << result << std::endl;
    return CONTINUE;
  }

  ReturnActionEnum setCommand(const std::vector<std::string>& arguments) {
    //Print the setting keys if none were given
    if (!checkArgumentCount(arguments, 1, false)) {
      dumpKeys(settingKeyMap);
      return CONTINUE;
    }

    //Validate argument count
    if (!checkArgumentCount(arguments, 2, true)) {
      return CONTINUE;
    }

    //Filter out unknown keys
    if (!checkKeyValid(settingKeyMap, arguments[1])) {
      return CONTINUE;
    }

    //Convert the value to the correct type in memory
    void* valuePtr = nullptr;
    switch (settingKeyMap.at(arguments[1])) {
    case FocalDepthEnabledKey:
    case VsyncKey:
    case GammaCorrectionEnabledKey:
      valuePtr = new bool;
      if (!stringToBool(arguments[2], (bool*)valuePtr)) {
        delete (bool*)valuePtr;
        return CONTINUE;
      };
      break;
    case FocalDepthKey:
    case BlurStrengthKey:
    case FrameLimitKey:
    case RenderResolutionMultiplierKey:
    case RenderFarPlaneKey:
    case ShadowFarPlaneKey:
      valuePtr = new float;
      if (!stringToFloat(arguments[2], (float*)valuePtr)) {
        delete (float*)valuePtr;
        return CONTINUE;
      };
      break;
    case ShadowResolutionKey:
    case AntialiasingSamplesKey:
      valuePtr = new unsigned int;
      if (!stringToUInt(arguments[2], (unsigned int*)valuePtr)) {
        delete (unsigned int*)valuePtr;
        return CONTINUE;
      };
      break;
    }

    //Match the key against handlers to set the value
    switch (settingKeyMap.at(arguments[1])) {
    case FocalDepthEnabledKey:
      ammonite::renderer::settings::post::setFocalDepthEnabled(*((bool*)valuePtr));
      break;
    case FocalDepthKey:
      ammonite::renderer::settings::post::setFocalDepth(*((float*)valuePtr));
      break;
    case BlurStrengthKey:
      ammonite::renderer::settings::post::setBlurStrength(*((float*)valuePtr));
      break;
    case VsyncKey:
      ammonite::renderer::settings::setVsync(*((bool*)valuePtr));
      break;
    case FrameLimitKey:
      ammonite::renderer::settings::setFrameLimit(*((float*)valuePtr));
      break;
    case ShadowResolutionKey:
      ammonite::renderer::settings::setShadowRes(*((unsigned int*)valuePtr));
      break;
    case RenderResolutionMultiplierKey:
      ammonite::renderer::settings::setRenderResMultiplier(*((float*)valuePtr));
      break;
    case AntialiasingSamplesKey:
      ammonite::renderer::settings::setAntialiasingSamples(*((unsigned int*)valuePtr));
      break;
    case RenderFarPlaneKey:
      ammonite::renderer::settings::setRenderFarPlane(*((float*)valuePtr));
      break;
    case ShadowFarPlaneKey:
      ammonite::renderer::settings::setShadowFarPlane(*((float*)valuePtr));
      break;
    case GammaCorrectionEnabledKey:
      ammonite::renderer::settings::setGammaCorrection(*((bool*)valuePtr));
      break;
    }

    //Delete the value's memory
    switch (settingKeyMap.at(arguments[1])) {
    case FocalDepthEnabledKey:
    case VsyncKey:
    case GammaCorrectionEnabledKey:
      delete (bool*)valuePtr;
      break;
    case FocalDepthKey:
    case BlurStrengthKey:
    case FrameLimitKey:
    case RenderResolutionMultiplierKey:
    case RenderFarPlaneKey:
    case ShadowFarPlaneKey:
      delete (float*)valuePtr;
      break;
    case ShadowResolutionKey:
    case AntialiasingSamplesKey:
      delete (unsigned int*)valuePtr;
      break;
    }

    return CONTINUE;
  }
}

namespace {
  enum CameraKeyEnum : unsigned char {
    FieldOfViewKey,
    PositionKey,
    DirectionKey,
    HorizontalKey,
    VerticalKey
  };

  const std::unordered_map<std::string, CameraKeyEnum> cameraKeyMap = {
    {"fov", FieldOfViewKey},
    {"position", PositionKey},
    {"direction", DirectionKey},
    {"horizontal", HorizontalKey},
    {"vertical", VerticalKey}
  };

  void cameraGetCommand(const std::vector<std::string>& arguments) {
    //List keys when no key is given
    if (!checkArgumentCount(arguments, 2, false)) {
      dumpKeys(cameraKeyMap);
      return;
    }

    //Filter out unknown keys
    if (!checkKeyValid(cameraKeyMap, arguments[2])) {
      return;
    }

    //Query the key and print it
    const AmmoniteId cameraId = ammonite::camera::getActiveCamera();
    switch (cameraKeyMap.at(arguments[2])) {
    case FieldOfViewKey:
      ammonite::utils::normal << ammonite::camera::getFieldOfView(cameraId) << std::endl;
      break;
    case PositionKey:
      {
        ammonite::Vec<float, 3> positionVec = {0};
        ammonite::camera::getPosition(cameraId, positionVec);
        ammonite::utils::normal << ammonite::formatVector(positionVec) << std::endl;
      }
      break;
    case DirectionKey:
      {
        ammonite::Vec<float, 3> directionVec = {0};
        ammonite::camera::getDirection(cameraId, directionVec);
        ammonite::utils::normal << ammonite::formatVector(directionVec) << std::endl;
      }
      break;
    case HorizontalKey:
      ammonite::utils::normal << ammonite::camera::getHorizontal(cameraId) << std::endl;
      break;
    case VerticalKey:
      ammonite::utils::normal << ammonite::camera::getVertical(cameraId) << std::endl;
      break;
    }
  }

  void cameraSetCommand(const std::vector<std::string>& arguments) {
    //List keys when no key is given
    if (!checkArgumentCount(arguments, 2, false)) {
      dumpKeys(cameraKeyMap);
      return;
    }

    //Filter out unknown keys
    if (!checkKeyValid(cameraKeyMap, arguments[2])) {
      return;
    }

    //Decide whether to search for a scalar or a vector
    unsigned int valueArgCount = 0;
    switch (cameraKeyMap.at(arguments[2])) {
    case FieldOfViewKey:
    case HorizontalKey:
    case VerticalKey:
      valueArgCount = 1;
      break;
    case PositionKey:
    case DirectionKey:
      valueArgCount = 3;
      break;
    }

    //Check that enough values were passed
    if (!checkArgumentCount(arguments, valueArgCount + 2, true)) {
      return;
    }

    //Read the value in
    bool success = true;
    ammonite::Vec<float, 3> floatVector = {0};
    if (valueArgCount == 1) {
      success = stringToFloat(arguments[3], &floatVector[0]);
    } else {
      success = stringToFloatVector(&arguments[3], floatVector);
    }

    //Bail if argument conversion failed
    if (!success) {
      return;
    }

    //Set the key
    const AmmoniteId cameraId = ammonite::camera::getActiveCamera();
    switch (cameraKeyMap.at(arguments[2])) {
    case FieldOfViewKey:
      ammonite::camera::setFieldOfView(cameraId, floatVector[0]);
      break;
    case PositionKey:
      ammonite::camera::setPosition(cameraId, floatVector);
      break;
    case DirectionKey:
      ammonite::camera::setDirection(cameraId, floatVector);
      break;
    case HorizontalKey:
      ammonite::camera::setAngle(cameraId, floatVector[0],
                                 ammonite::camera::getVertical(cameraId));
      break;
    case VerticalKey:
      ammonite::camera::setAngle(cameraId, ammonite::camera::getHorizontal(cameraId),
                                 floatVector[0]);
      break;
    }
  }

  ReturnActionEnum cameraCommand(const std::vector<std::string>& arguments) {
    //Ignore empty commands
    if (!checkArgumentCount(arguments, 1, false)) {
      ammonite::utils::warning << "No mode specified, use 'get' or 'set'" << std::endl;
      return CONTINUE;
    }

    //Handle get and set modes
    if (arguments[1] == "get") {
      cameraGetCommand(arguments);
    } else if (arguments[1] == "set") {
      cameraSetCommand(arguments);
    } else {
      ammonite::utils::warning << "'" << arguments[1] \
                               << "' isn't a valid mode, use 'get' or 'set'" << std::endl;
    }


    return CONTINUE;
  }
}

namespace {
  ReturnActionEnum modelDumpCommand(const std::vector<std::string>&) {
    if (!ammonite::models::dumpModelStorageDebug()) {
      ammonite::utils::warning << "Model storage querying is unavailable" << std::endl;
    }

    return CONTINUE;
  }
}

namespace {
  ReturnActionEnum exitCommand(const std::vector<std::string>&) {
    return EXIT_COMMANDS;
  }
}

namespace {
  ReturnActionEnum stopCommand(const std::vector<std::string>&) {
    return EXIT_PROGRAM;
  }
}

/*
 - Public command management
 - Since the commands are self-contained, this just enables the command mode
*/

namespace commands {
  namespace {
    const std::unordered_map<std::string, CommandHandler> commandMap = {
      {"help", {helpCommand}},
      {"get", {getCommand}},
      {"set", {setCommand}},
      {"camera", {cameraCommand}},
      {"models", {modelDumpCommand}},
      {"exit", {exitCommand}},
      {"stop", {stopCommand}}
    };

    constexpr std::string PROMPT_STRING = "> ";
  }

  /*
   - Take commands from the terminal until told to exit the command system
   - Returns true if the program has been told to close
  */
  bool commandPrompt() {
    while (true) {
      ammonite::utils::normal << PROMPT_STRING << std::flush;

      //Take a command input
      std::string commandLine;
      std::getline(std::cin, commandLine);

      //Split the command by word
      std::vector<std::string> commandLineVec;
      std::stringstream commandLineStream(commandLine);
      std::string word;
      while (commandLineStream >> word) {
        commandLineVec.push_back(word);
      }

      //Skip empty commands
      if (commandLineVec.empty()) {
        continue;
      }

      //Check the command exists
      const std::string& command = commandLineVec[0];
      if (!commandMap.contains(command)) {
        ammonite::utils::warning << "'" << command << "' isn't a valid command" << std::endl;
        continue;
      }

      //Call the handler with the command, then continue or return
      const ReturnActionEnum action = (commandMap.at(command))(commandLineVec);
      if (action != CONTINUE) {
        return (action == EXIT_PROGRAM);
      }
    }
  }
}
