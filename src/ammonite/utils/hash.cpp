#include <cstdint>
#include <string>

extern "C" {
  #if defined(__AVX512F__) && defined(__AVX512BW__) && defined(__AVX512VL__) && \
      defined(__VAES__) && defined(__AVX2__) && defined(__SSE2__) && defined(__BMI2__)
    #define USE_VAES_AVX512
    #include <immintrin.h>
    #include <emmintrin.h>
  #endif
}

#ifndef USE_VAES_AVX512
  #include <cstddef>
  #include <functional>
#endif

#include "hash.hpp"

namespace ammonite {
  namespace utils {
    namespace internal {
      /*
       - Hash together inputCount paths from inputs
       - See https://github.com/stuarthayhurst/bad-hash-museum for details
       - Don't use this for security, you'll lose your job
      */
#ifdef USE_VAES_AVX512
#pragma message("Using AVX512 + AVX2 + VAES + SSE2 optimised hashing")
      std::string hashStrings(const std::string* inputs, unsigned int inputCount) {
        __m512i hash = _mm512_setzero_epi32();
        for (unsigned int i = 0; i < inputCount; i++) {
          const uint8_t* input = (uint8_t*)inputs[i].data();
          unsigned int inputSize = inputs[i].length();

          //Chosen by a fair dice roll, guaranteed to be random
          const __m512i key = _mm512_setr_epi64(0x343B2F2F2063686F, 0x73656E2062792061,
            0x2066616972206469, 0x636520726F6C6C2E, 0x0A2F2F2067756172,
            0x616E746565642074, 0x6F2062652072616E, 0x646F6D2E0BAD1DEA);

          while (inputSize >= 64) {
            __m512i a = _mm512_loadu_epi8(input);
            a = _mm512_aesenc_epi128(a, key);
            hash = _mm512_xor_si512(_mm512_aesenc_epi128(a, key), hash);

            inputSize -= 64;
            input += 64;
          }

          if (inputSize > 0) {
            const __mmask64 mask = _bzhi_u64(0xFFFFFFFFFFFFFFFF, inputSize);
            __m512i a = _mm512_maskz_loadu_epi8(mask, input);
            a = _mm512_aesenc_epi128(a, key);
            hash = _mm512_xor_si512(_mm512_aesenc_epi128(a, key), hash);
          }
        }

        /*
         - Rotate and sum until every element contains the sum of all elements
         - Effectively, a horizontal add
        */
        const __m512i rotateOne = _mm512_alignr_epi64(hash, hash, 1);
        const __m512i sumRotateOne = _mm512_add_epi64(hash, rotateOne);
        const __m512i rotateTwo = _mm512_alignr_epi64(sumRotateOne, sumRotateOne, 2);
        const __m512i sumRotateTwo = _mm512_add_epi64(rotateTwo, sumRotateOne);
        const __m512i rotateFour = _mm512_alignr_epi64(sumRotateTwo, sumRotateTwo, 4);
        const __m512i sumRotateFour = _mm512_add_epi64(rotateFour, sumRotateTwo);

        //Spread the result out over 128 bits, adjust the range to ['A', 'P']
        std::string output(16, 0);

        //Load result into the vector
        const __m128i resultVec = _mm512_castsi512_si128(sumRotateFour);

        //Shift the result 4 bits right
        const __m128i shift = _mm_set_epi64((__m64)0ull, (__m64)4ull);
        const __m128i shiftedResult = _mm_srlv_epi64(resultVec, shift);

        //Interleave the result and shifted result every 8 bits, clear the high 4 bits
        const __m128i spacedResult = _mm_unpacklo_epi8(resultVec, shiftedResult);
        const __m128i clearedResult = _mm_and_si128(spacedResult, _mm_set1_epi8(0xF));

        //Adjust every element to start at 'A', save the result
        const __m128i addedResult = _mm_add_epi8(clearedResult, _mm_set1_epi8('A'));
        _mm_storeu_epi8((__m128i*)output.data(), addedResult);

        return output;
      }
#else
      std::string hashStrings(const std::string* inputs, unsigned int inputCount) {
        uintmax_t total = 0;
        for (unsigned int i = 0; i < inputCount; i++) {
          total += std::hash<std::string>{}(inputs[i]);
        }

        //Split upper and lower half of each byte, add to 'A' and store
        constexpr unsigned int outputSize = sizeof(total) * 2;
        std::string outputString(outputSize, 0);
        for (std::size_t i = 0; i < outputSize; i++) {
          //Map from output bits to elements, filling half a byte each time
          outputString[i] = (char)('A' + (char)((total >> (i * 4)) & 0xF));
        }

        return outputString;
      }
#endif
    }
  }
}
