#ifndef TYPES
#define TYPES

#include <atomic>
#include <climits>
#include <semaphore>
#include <vector>

typedef void (*AmmoniteKeyCallback)(std::vector<int> keycodes, int action, void* userPtr);
typedef void (*AmmoniteWork)(void* userPtr);

typedef std::counting_semaphore<INT_MAX> AmmoniteGroup;

#endif
