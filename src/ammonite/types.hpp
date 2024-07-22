#ifndef TYPES
#define TYPES

#include <atomic>
#include <vector>

typedef void (*AmmoniteKeyCallback)(std::vector<int> keycodes, int action, void* userPtr);
typedef void (*AmmoniteWork)(void* userPtr);

typedef std::atomic_flag AmmoniteCompletion;

#endif
