#ifndef INTERNALTYPES
#define INTERNALTYPES

#include <climits>
#include <semaphore>
#include <vector>

typedef void (*AmmoniteKeyCallback)(std::vector<int> keycodes, int action, void* userPtr);
typedef void (*AmmoniteWork)(void* userPtr);

typedef std::counting_semaphore<INT_MAX> AmmoniteGroup;
typedef unsigned int AmmoniteId;

#endif
