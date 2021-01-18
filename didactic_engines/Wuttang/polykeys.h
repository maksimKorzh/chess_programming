#ifndef POLYKEYS_H
#define POLYKEYS_H

#include "data.h"

#ifdef _MSC_VER
#  define U64_POLY(u) (u##ui64)
#else
#  define U64_POLY(u) (u##ULL)
#endif

extern const Key Random64Poly[781];

#endif
