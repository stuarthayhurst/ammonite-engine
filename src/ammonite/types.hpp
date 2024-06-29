#ifndef TYPES
#define TYPES

#include <cstddef>
#include <vector>

typedef bool (*AmmoniteValidator)(unsigned char* data, std::size_t size, void* userPtr);
typedef void (*AmmoniteKeyCallback)(std::vector<int> keycodes, int action, void* userPtr);
typedef void (*AmmoniteWork)(void* userPtr);

#endif
