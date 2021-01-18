#ifndef XBOARD_H
#define XBOARD_H

#include "position.h"
#include "search.h"

void XBoard_loop(Position &pos, searchInfo &info);
void Console_loop(Position &pos, searchInfo &info);

#endif // XBOARD_H
