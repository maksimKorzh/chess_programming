#ifndef HASHKEYINCLUDE_H
#define HASHKEYINCLUDE_H

#include "position.h"

#define RAND64 (((Key)rand()) | ((Key)rand() << 15) | ((Key)rand() << 30) | ((Key)rand() << 45) | ((Key)rand() << 60))

extern Key pieceKeys[13][120];
extern Key castleKeys[16];
extern Key sideKey;

void initHashKey();
Key generatePosKey(const Position &pos);

#endif // HASHKEYINCLUDE_H
