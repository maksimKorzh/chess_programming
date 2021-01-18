#include "init.h"
#include "hashKey.h"
#include "movegen.h"
#include "bitboards.h"
#include "polybook.h"

void initAll()
{
    initHashKey();
    initMvvLva();
    initEvalMask();
    //initPolyBook();
}
