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
#endif

#include "hash.hpp"

namespace ammonite {
  namespace utils {
    namespace internal {
      /*
       - Hash together inputCount paths from inputs
         - Use an AVX-512 + VAES implementation, if supported
         - The AVX-512 + VAES and generic versions produce different results, but both work
           - On a Ryzen 7 7700X, the AVX-512 + VAES accelerated version is around 40x faster
       - Don't use this for security, you'll lose your job
      */
#ifdef USE_VAES_AVX512
#pragma message("Using AVX512 + AVX2 + VAES + SSE2 optimised hashing")
      std::string hashStrings(const std::string* inputs, unsigned int inputCount) {
        __m512i hash = _mm512_setzero_epi32();
        for (unsigned int i = 0; i < inputCount; i++) {
          const uint8_t* input = (uint8_t*)inputs[i].data();
          unsigned int inputSize = inputs[i].length();

          while (inputSize >= 64) {
            const __m512i a = _mm512_loadu_epi8(input);
            hash = _mm512_aesenc_epi128(hash, a);

            inputSize -= 64;
            input += 64;
          }

          if (inputSize > 0) {
            const __mmask64 mask = _bzhi_u64(0xFFFFFFFFFFFFFFFF, inputSize);
            const __m512i a = _mm512_maskz_loadu_epi8(mask, input);
            hash = _mm512_aesenc_epi128(hash, a);
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
        constexpr unsigned int hashWidth = 8;
        uint8_t output[hashWidth] = {0};
        uint8_t prev = 0;

        /*
         - XOR the first byte of the hash with the first character of the first input
         - Sequentially XOR every byte of the hash with the result of the previous
           operation of this stage
         - Repeat this process for every character of every input
        */
        for (unsigned int i = 0; i < inputCount; i++) {
          for (const char& character : inputs[i]) {
            output[0] ^= character;
            for (unsigned char& outputByte : output) {
              outputByte ^= prev;
              prev = outputByte;
            }
          }
        }

        //Split upper and lower half of each byte, add to 'A' and store
        std::string outputString(sizeof(output) * 2, 0);
        for (std::size_t i = 0; i < sizeof(output); i++) {
          outputString[(i * 2) + 0] = (char)('A' + (char)(output[i] & 0xF));
          outputString[(i * 2) + 1] = (char)('A' + (char)((output[i] >> 4) & 0xF));
        }

        return outputString;
      }
#endif
    }
  }
}
