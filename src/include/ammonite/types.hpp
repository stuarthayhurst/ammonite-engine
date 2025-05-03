#ifndef TYPES
#define TYPES

#include <climits>
#include <cstdint>
#include <semaphore>

using AmmoniteWork = void (*)(void* userPtr);

using AmmoniteGroup = std::counting_semaphore<INT_MAX>;
using AmmoniteId = uintmax_t;

#endif
