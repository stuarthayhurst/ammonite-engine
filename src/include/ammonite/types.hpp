#ifndef TYPES
#define TYPES

#include <climits>
#include <cstdint>
#include <semaphore>
#include <vector>

using AmmoniteKeyCallback = void (*)(const std::vector<int>& keycodes, int action, void* userPtr);
using AmmoniteWork = void (*)(void* userPtr);

using AmmoniteGroup = std::counting_semaphore<INT_MAX>;
using AmmoniteId = uintmax_t;

#endif
