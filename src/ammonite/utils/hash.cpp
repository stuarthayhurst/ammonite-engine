#include <cstdint>
#include <string>

extern "C" {
  #if defined(__AVX512F__) && defined(__AVX512BW__) && defined(__VAES__) && \
      defined(__AVX2__) && defined(__SSE2__) && defined(__BMI2__) && defined(UINT64_MAX)
    #define USE_VAES_AVX512
    #include <immintrin.h>
    #include <emmintrin.h>
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
           - On a Ryzen 7 7700X, the AVX-512 + VAES accelerated version is around 40x faster
       - Don't use this for security, you'll lose your job
      */
#ifdef USE_VAES_AVX512
      std::string hashStrings(std::string* filePaths, unsigned int fileCount) {
        __m512i last = _mm512_setzero_epi32();
        for (unsigned int i = 0; i < fileCount; i++) {
          uint8_t* filePath = (uint8_t*)filePaths[i].c_str();
          unsigned int pathSize = filePaths[i].length();

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

        //Spread the result out over 128 bits, adjust the range to ['A', 'P']
        std::string output(16, 0);

        //Load result into the vector
        __m128i resultVec = _mm_set1_epi64((__m64)result);

        //Shift the result 4 bits right
        __m128i shift = _mm_set_epi64((__m64)0ull, (__m64)4ull);
        __m128i shiftedResult = _mm_srlv_epi64(resultVec, shift);

        //Interleave the result and shifted result every 8 bits, clear the high 4 bits
        __m128i spacedResult = _mm_unpacklo_epi8(resultVec, shiftedResult);
        __m128i clearedResult = _mm_and_si128(spacedResult, _mm_set1_epi8(0xF));

        //Adjust every element to start at 'A', save the result
        __m128i addedResult = _mm_add_epi8(clearedResult, _mm_set1_epi8('A'));
        _mm_storeu_epi8((__m128i*)output.data(), addedResult);

        return output;
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
          unsigned int pathLength = filePaths[i].length();
          for (unsigned int i = 0; i < pathLength; i++) {
            output[0] ^= filePath[i];
            for (unsigned int j = 0; j < sizeof(uintmax_t); j++) {
              output[j] ^= prev;
              prev = output[j];
            }
          }
        }

        std::string outputString(sizeof(uintmax_t) * 2, 0);
        uintmax_t result = *(uintmax_t*)output;
        for (uintmax_t i = 0; i < sizeof(uintmax_t) * 2; i++) {
          outputString[i] = (char)('A' + (char)((result >> (i * 4)) & 0xFull));
        }

        return outputString;
      }
#endif
    }
  }
}
