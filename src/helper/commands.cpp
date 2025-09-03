#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include <ammonite/ammonite.hpp>

#include "commands.hpp"

namespace {
  enum ReturnAction : unsigned char {
    CONTINUE,
    EXIT_COMMANDS,
    EXIT_PROGRAM
  };

  constexpr std::string PROMPT_STRING = "> ";
  using CommandHandler = ReturnAction (*)(std::vector<std::string>* commandLine);
}

namespace {
  ReturnAction helpCommand(std::vector<std::string>*) {
    ammonite::utils::normal << "Command help:" << std::endl;
    ammonite::utils::normal << "  'help': Display this help page" << std::endl;
    ammonite::utils::normal << "  'exit': Exit the command system" << std::endl;
    ammonite::utils::normal << "  'stop': Stop the program" << std::endl;

    return CONTINUE;
  }

  ReturnAction exitCommand(std::vector<std::string>*) {
    return EXIT_COMMANDS;
  }

  ReturnAction stopCommand(std::vector<std::string>*) {
    return EXIT_PROGRAM;
  }
}

namespace commands {
  namespace {
    struct CommandInfo {
      CommandHandler handler;
    };

    const std::unordered_map<std::string, CommandInfo> commandMap = {
      {"help", {helpCommand}},
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
      const ReturnAction action = info.handler(&commandLineVec);
      if (action != CONTINUE) {
        return (action == EXIT_PROGRAM);
      }
    }
  }
}
