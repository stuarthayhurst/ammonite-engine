#include <cstdint>
#include <string>

extern "C" {
  #if defined(__AVX512F__) && defined(__AVX512BW__) && defined(__VAES__) && defined(__BMI2__) && \
      defined(UINT64_MAX)
    #define USE_VAES_AVX512
    #include <immintrin.h>
  #endif
}

#include "hash.hpp"

namespace ammonite {
  namespace utils {
    namespace internal {
      /*
       - Hash together fileCount paths from filePaths
         - Use an AVX-512 + VAES implementation, if supported
         - The AVX-512 + VAES and generic versions produce different results, but both work
           - On a Ryzen 7 7700X, the AVX-512 + VAES accelerated version is around 64x faster
       - Don't use this for security, you'll lose your job
      */
#ifdef USE_VAES_AVX512
      std::string hashStrings(std::string* filePaths, unsigned int fileCount) {
        __m512i last = _mm512_setzero_epi32();
        for (unsigned int i = 0; i < fileCount; i++) {
          uint8_t* filePath = (uint8_t*)filePaths[i].c_str();
          int pathSize = (int)filePaths[i].length();

          while (pathSize >= 64) {
            __m512i a = _mm512_loadu_epi8(filePath);
            last = _mm512_aesenc_epi128(last, a);

            pathSize -= 64;
            filePath += 64;
          }

          if (pathSize > 0) {
            __mmask64 mask = _bzhi_u64(0xFFFFFFFF, pathSize);
            __m512i a = _mm512_maskz_loadu_epi8(mask, filePath);
            last = _mm512_aesenc_epi128(last, a);
          }
        }

        uint64_t* values = (uint64_t*)&last;
        uint64_t result = 0;
        for (int i = 0; i < 8; i++) {
          result += values[i];
        }

        return std::to_string(result);
      }
#else
      std::string hashStrings(std::string* filePaths, unsigned int fileCount) {
        alignas(uintmax_t) uint8_t output[sizeof(uintmax_t)] = {0};
        uint8_t prev = 0;

        /*
         - XOR the first byte of the hash with the first character of the first path
         - Sequentially XOR every byte of the hash with the result of the previous
           operation of this stage
         - Repeat this process for every character of every path
        */
        for (unsigned int i = 0; i < fileCount; i++) {
          uint8_t* filePath = (uint8_t*)filePaths[i].c_str();
          int pathLength = (int)filePaths[i].length();
          for (int i = 0; i < pathLength; i++) {
            output[0] ^= filePath[i];
            for (unsigned int j = 0; j < sizeof(uintmax_t); j++) {
              output[j] ^= prev;
              prev = output[j];
            }
          }
        }

        return std::to_string(*(uintmax_t*)output);
      }
#endif
    }
  }
}
