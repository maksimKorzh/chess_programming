#ifndef POLYBOOK_H
#define POLYBOOK_H

#include "polykeys.h"
#include "data.h"
#include "position.h"

struct polyBookEntry
{
    Key key;
    unsigned short move;
    unsigned short weight;
    unsigned int learn;
};

void initPolyBook();
void cleanPolyBook();
int getBookMove(Position &pos);

#endif // POLYBOOK_H
