#ifndef SEARCH_H
#define SEARCH_H

#include "position.h"

struct searchInfo
{
    int starttime = 0;
    int stoptime = 0;
    int depth = 0;
    bool depthset = false;
    bool timeset = false;
    int movestogo = 0;
    bool infinite = false;

    long nodes = 0;

    bool quit = false;
    bool stopped = false;

    //float fh = 0;
    //float fhf = 0;
    //int nullCut = 0;

    int gameMode = UCIMODE;
    bool postThinking = true;
};

void searchPosition(Position &pos, searchInfo &info);

#endif // SEARCH_H
