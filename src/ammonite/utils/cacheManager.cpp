#include <filesystem>
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <functional>

#include "../internal/fileManager.hpp"

namespace ammonite {
  namespace utils {
    namespace cache {
      namespace {
        bool cacheData = false;
        std::string dataCacheDir;
      }

      namespace {
        //Hash input filenames together to create a unique cache string
        static std::string generateCacheString(const char* inputNames[], const int inputCount) {
          std::string inputString = "";
          for (int i = 0; i < inputCount; i++) {
            inputString += std::string(inputNames[i]) + std::string(";");
          }

          return std::to_string(std::hash<std::string>{}(inputString));
        }
      }

      bool useDataCache(const char* dataCachePath) {
        //Attempt to create a cache directory
        try {
          std::filesystem::create_directory(dataCachePath);
        } catch (const std::filesystem::filesystem_error&) {
          std::cerr << "Failed to create cache directory: '" << dataCachePath << "'" << std::endl;
          cacheData = false;
          return false;
        }

        //If the cache directory doesn't exist, disable caching and exit
        if (!std::filesystem::is_directory(dataCachePath)) {
          std::cerr << "Couldn't find cache directory: '" << dataCachePath << "'" << std::endl;
          cacheData = false;
          return false;
        }

        //Enable cache and ensure path has a trailing slash
        cacheData = true;
        dataCacheDir = std::string(dataCachePath);
        if (dataCacheDir.back() != '/') {
          dataCacheDir.push_back('/');
        }

        std::cout << "Data caching enabled ('" << dataCacheDir << "')" << std::endl;
        return true;
      }

      bool getCacheEnabled() {
        return cacheData;
      }

      std::string requestNewCache(const char* filePaths[], const int fileCount) {
        return std::string(dataCacheDir + generateCacheString(filePaths, fileCount) + ".cache");
      }

      std::string requestCachedData(const char* filePaths[], const int fileCount, bool* found) {
        //Generate path to cache file from cache string
        std::string cacheFilePath = requestNewCache(filePaths, fileCount);
        std::string cacheInfoFilePath = cacheFilePath + "info";

        //Check cache and info file exist
        if (!std::filesystem::exists(cacheFilePath) or !std::filesystem::exists(cacheInfoFilePath)) {
          *found = false;
          return std::string("");
        }

        //Validate filesizes and timestamps for each input file
        bool cacheValid = true;
        std::string line;
        std::ifstream cacheInfoFile(cacheInfoFilePath);
        if (cacheInfoFile.is_open()) {
          for (int i = 0; i < fileCount; i++) {
            //Get expected filename, filesize and timestamp
            std::vector<std::string> strings;
            getline(cacheInfoFile, line);
            std::stringstream rawLine(line);

            while (getline(rawLine, line, ';')) {
              strings.push_back(line);
            }

            if (strings.size() == 4) {
              if (strings[0] != "input" or strings[1] != filePaths[i]) {
                //Cache made from different files, invalidate
                cacheValid = false;
                break;
              }

              //Get filesize and time of last modification of the shader source
              long long int filesize = 0, modificationTime = 0;
              if (!ammonite::utils::files::getFileMetadata(filePaths[i], &filesize, &modificationTime)) {
                //Failed to get the metadata
                cacheValid = false;
                break;
              }

              if (std::stoi(strings[2]) != filesize or std::stoi(strings[3]) != modificationTime) {
                //Shader source code has changed, invalidate
                cacheValid = false;
                break;
              }
            } else {
              //Cache info file broken, invalidate
              cacheValid = false;
              break;
            }
          }

          cacheInfoFile.close();
        } else {
          //Failed to open the cache info
          cacheValid = false;
        }


        //If cache failed to validate, set found and return nothing
        if (!cacheValid) {
          *found = false;
          return std::string("");
        }

        *found = true;
        return cacheFilePath;
      }
    }
  }
}
