#ifndef TYPES
#define TYPES

#include <vector>

typedef void (*AmmoniteKeyCallback)(std::vector<int> keycode, int action, void* userPtr);
typedef void (*AmmoniteWork)(void* userPtr);

#endif
