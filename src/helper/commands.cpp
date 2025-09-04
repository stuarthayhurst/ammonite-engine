#include <exception>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include <ammonite/ammonite.hpp>

#include "commands.hpp"

//Definitions for all commands
namespace {
  enum ReturnActionEnum : unsigned char {
    CONTINUE,
    EXIT_COMMANDS,
    EXIT_PROGRAM
  };

  constexpr std::string PROMPT_STRING = "> ";
  using CommandHandler = ReturnActionEnum (*)(const std::vector<std::string>& commandLine);
}

/*
 - Setting key enum conversion
 - Keys added here must have handlers added in getCommand() and setCommand()
*/
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
}

//Command helpers
namespace {
  /*
   - Return true if at least count arguments are passed, excluding the command
   - Return false and send a warning message otherwise
  */
  bool checkArgumentCount(const std::vector<std::string>& arguments, unsigned int count) {
    if (arguments.size() > count) {
      return true;
    }

    ammonite::utils::warning << "At least " << count << " argument(s) expected, " \
                             << arguments.size() - 1 << " received" << std::endl;

    return false;
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
}

//Command implementations
namespace {
  ReturnActionEnum helpCommand(const std::vector<std::string>&) {
    ammonite::utils::normal << "Command help:" << std::endl;
    ammonite::utils::normal << "  'help'              : Display this help page" << std::endl;
    ammonite::utils::normal << "  'get [key]'         : Get the value of a setting key" << std::endl;
    ammonite::utils::normal << "                        Leave [key] blank to list keys" << std::endl;
    ammonite::utils::normal << "  'set [key] [value]' : Set the value of a setting key" << std::endl;
    ammonite::utils::normal << "  'exit'              : Exit the command system" << std::endl;
    ammonite::utils::normal << "  'stop'              : Stop the program" << std::endl;

    return CONTINUE;
  }

  ReturnActionEnum getCommand(const std::vector<std::string>& arguments) {
    //Print the setting keys if none were given
    if (arguments.size() == 1) {
      dumpKeys(settingKeyMap);
      return CONTINUE;
    }

    //Filter out unknown keys
    if (!settingKeyMap.contains(arguments[1])) {
      ammonite::utils::warning << "'" << arguments[1] << "' isn't a valid key " << std::endl;
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
    //Validate argument count
    if (!checkArgumentCount(arguments, 2)) {
      return CONTINUE;
    }

    //Filter out unknown keys
    if (!settingKeyMap.contains(arguments[1])) {
      ammonite::utils::warning << "'" << arguments[1] << "' isn't a valid key " << std::endl;
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

  ReturnActionEnum exitCommand(const std::vector<std::string>&) {
    return EXIT_COMMANDS;
  }

  ReturnActionEnum stopCommand(const std::vector<std::string>&) {
    return EXIT_PROGRAM;
  }
}

namespace commands {
  namespace {
    struct CommandInfo {
      CommandHandler handler;
    };

    //Commands added here must have implementations written
    const std::unordered_map<std::string, CommandInfo> commandMap = {
      {"help", {helpCommand}},
      {"get", {getCommand}},
      {"set", {setCommand}},
      {"exit", {exitCommand}},
      {"stop", {stopCommand}}
    };
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
      const CommandInfo& info = commandMap.at(command);
      const ReturnActionEnum action = info.handler(commandLineVec);
      if (action != CONTINUE) {
        return (action == EXIT_PROGRAM);
      }
    }
  }
}
