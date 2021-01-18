#include "xboard.h"
#include "data.h"
#include "misc.h"
#include "movegen.h"
#include "polybook.h"
#include <iostream>
#include <cstdio>
#include <string.h>

using namespace std;

int ThreeFoldRep(const Position &pos)
{
    int r = 0;
    for (int i = pos.hisPly - pos.fifty_move; i < pos.hisPly; ++i)
    {
        if (pos.history[i].posKey == pos.posKey)
        {
            ++r;
        }
    }
    return r;
}

bool DrawMaterial(const Position &pos)
{
    if (pos.pceNum[wP] || pos.pceNum[bP])
    {
        return false;
    }
    if (pos.pceNum[wQ] || pos.pceNum[wR] || pos.pceNum[bQ] || pos.pceNum[bR])
    {
        return false;
    }
    if (pos.pceNum[wB] > 1 || pos.pceNum[bB] > 1)
    {
        return false;
    }
    if (pos.pceNum[wN] > 1 || pos.pceNum[bN] > 1)
    {
        return false;
    }
    if (pos.pceNum[wN] && pos.pceNum[wB])
    {
        return false;
    }
    if (pos.pceNum[bN] && pos.pceNum[bB])
    {
        return false;
    }
    return true;
}

bool CheckResult(Position &pos)
{
    if (pos.fifty_move > 100)
    {
        cout << "1/2-1/2 {fifty move rule (claimed by wuttang)}\n";
        return true;
    }
    if (ThreeFoldRep(pos) >= 2)
    {
        cout << "1/2-1/2 {3-fold repetition (claimed by wuttang)}\n";
        return true;
    }
    if (DrawMaterial(pos))
    {
        cout << "1/2-1/2 {insufficient material (claimed by wuttang)}\n";
        return true;
    }

    Movelist list;
    generateAllMoves(pos, list);

    for (int moveNum = 0; moveNum < list.count; ++moveNum)
    {
        if (!pos.makeMove(list.moves[moveNum].move))
        {
            continue;
        }
        pos.takeMove();
        return false;
    }

    bool inCheck = pos.isSqAttacked(pos.kingSq[pos.side], pos.side^1);

    if (inCheck)
    {
        if (pos.side == white)
        {
            cout << "0-1 {black mates (claimed by wuttang)}\n";
            return true;
        }
        else
        {
            cout << "0-1 {white mates (claimed by wuttang)}\n";
            return true;
        }
    }
    else
    {
        cout << "1/2-1/2 {stalemate (claimed by wuttang)}\n";
        return true;
    }
}

void printOptions()
{
    cout << "feature ping=1 setboard=1 colors=0 usermove=1 memory=1\n"
         << "feature done=1\n";
}

void XBoard_loop(Position &pos, searchInfo &info)
{
    info.gameMode = XBOARDMODE;
    info.postThinking = true;
    setbuf(stdin, NULL);
    setbuf(stdout, NULL);

    int depth = -1, movestogo[2] = {30, 30}, movetime = -1;
    int time = -1, inc = 0;
    int engineSide = both;
    int timeLeft;
    int sec;
    int mps;
    int move = NOMOVE;
    char inBuf[80], command[80];
    int MB;

    while (true)
    {
        fflush(stdout);

        if (pos.side == engineSide && CheckResult(pos) == false)
        {
            info.starttime = getTimeMS();
            info.depth = depth;

            if (time != -1)
            {
                info.timeset = true;
                time /= movestogo[pos.side];
                time -= 50;
                if (time+inc < 10)
                {
                    info.stoptime = info.starttime + 10;
                }
                else
                {
                    info.stoptime = info.starttime + time + inc;
                }
            }

            if (depth == -1 || depth > maxDepth)
            {
                info.depth = maxDepth;
            }

            //cout << "time:" << time << " start:" << info.starttime << " stop:" << info.stoptime << " depth:" << info.depth << " timeset:" << info.timeset
            //     << " movestogo:" << movestogo[pos.side] << " mps:" << mps << '\n';
            searchPosition(pos, info);

            if (mps != 0)
            {
                --movestogo[pos.side^1];
                if (movestogo[pos.side^1] < 1)
                {
                    movestogo[pos.side^1] = mps;
                }
            }
        }
        fflush(stdout);

        memset(&inBuf[0], 0, sizeof(inBuf));
        fflush(stdout);
        if (!fgets(inBuf, 80, stdin))
        {
            continue;
        }
        sscanf(inBuf, "%s", command);

        cout << "command seen:" << inBuf << '\n';

        if (!strcmp(command, "quit"))
        {
            info.quit = true;
            break;
        }
        if (!strcmp(command, "force"))
        {
            engineSide = both;
            continue;
        }
        if (!strcmp(command, "protover"))
        {
            printOptions();
            continue;
        }
        if (!strcmp(command, "sd"))
        {
            sscanf(inBuf, "sd %d", &depth);
            //cout << "DEBUG depth:" << depth << '\n';
            continue;
        }
        if (!strcmp(command, "st"))
        {
            sscanf(inBuf, "st %d", &movetime);
            //cout << "DEBUG movetime:" << movetime << '\n';
            continue;
        }
        if (!strcmp(command, "time"))
        {
            sscanf(inBuf, "time %d", &time);
            time *= 10;
            //cout << "DEBUG time:" << time << '\n';
            continue;
        }
        if (!strcmp(command, "memory"))
        {
            sscanf(inBuf, "memory %d", &MB);
            if(MB < 4) MB = 4;
			if(MB > MAX_HASH) MB = MAX_HASH;
			cout << "Set Hash to " << MB << " MB\n";
			initHashTable(pos.hashTable, MB);
			continue;
        }
        if (!strcmp(command, "level"))
        {
            sec = 0;
            movetime = -1;
            if (sscanf(inBuf, "level %d %d %d", &mps, &timeLeft, &inc) != 3)
            {
                sscanf(inBuf, "level %d %d:%d %d", &mps, &timeLeft, &sec, &inc);
                //cout << "DEBUG level with :\n";
            }
            else
            {
                //cout << "DEBUG level without :\n";
            }
            timeLeft *= 60000;
			timeLeft += sec * 1000;
			movestogo[0] = movestogo[1] = 30;
			if(mps != 0)
            {
				movestogo[0] = movestogo[1] = mps;
			}
			time = -1;
			//cout << "DEBUG level timeLeft:" << timeLeft << " movesToGo:" << movestogo[0] << " inc:" << inc << " mps:" << mps << '\n';
			continue;
        }
        if (!strcmp(command, "ping"))
        {
            cout << "pong" << inBuf+4 << '\n';
            continue;
        }
        if (!strcmp(command, "new"))
        {
            clearHashTable(pos.hashTable);
            engineSide = black;
            pos.parseFEN(startFEN);
            depth = -1;
            continue;
        }
        if (!strcmp(command, "setboard"))
        {
            engineSide = both;
            pos.parseFEN(inBuf+9);
            continue;
        }
        if (!strcmp(command, "go"))
        {
            engineSide = pos.side;
            continue;
        }
        if (!strcmp(command, "usermove"))
        {
            --movestogo[pos.side];
            move = parseMove(inBuf+9, pos);
            if (move == NOMOVE)
            {
                continue;
            }
            pos.makeMove(move);
            pos.ply = 0;
        }
    }
}

void Console_loop(Position &pos, searchInfo &info)
{
    cout << "Welcome to Wuttang in console mode!\n"
         << "Type help for commands\n\n";

    info.gameMode = CONSOLEMODE;
    info.postThinking = true;
    setbuf(stdin, NULL);
    setbuf(stdout, NULL);

    int depth = maxDepth, movetime = 3000;
	int engineSide = both;
	int move = NOMOVE;
	char inBuf[80], command[80];

	engineSide = black;
	pos.parseFEN(startFEN);

	while (true)
    {
        fflush(stdout);

        if (pos.side == engineSide && CheckResult(pos) == false)
        {
            info.starttime = getTimeMS();
            info.depth = depth;

            if (movetime != 0)
            {
                info.timeset = true;
                info.stoptime = info.starttime + movetime;
            }

            searchPosition(pos, info);
        }

        cout << "\nWuttang > ";

        fflush(stdout);

        memset(&inBuf[0], 0, sizeof(inBuf));
		fflush(stdout);
		if (!fgets(inBuf, 80, stdin))
        {
            continue;
        }

        sscanf(inBuf, "%s", command);

        if (!strcmp(command, "help"))
        {
            cout << "Commands:\n"
                 << "quit - quit game\n"
			     << "force - computer will not think\n"
			     << "print - show board\n"
			     << "post - show thinking\n"
			     << "nopost - do not show thinking\n"
			     << "new - start new game\n"
			     << "go - set computer thinking\n"
			     << "depth x - set depth to x\n"
			     << "time x - set thinking time to x seconds (depth still applies if set)\n"
			     << "view - show current depth and movetime settings\n"
			     << "setboard x - set position to fen x\n"
			     << "** note ** - to reset time and depth, set to 0\n"
			     << "enter moves using b7b8q notation\n\n\n";
			continue;
        }
        if (!strcmp(command, "setboard"))
        {
            engineSide = both;
            pos.parseFEN(inBuf+9);
            continue;
        }
        if (!strcmp(command, "print"))
        {
            pos.display();
            continue;
        }
        if (!strcmp(command, "quit"))
        {
            info.quit = true;
            break;
        }
        if (!strcmp(command, "post"))
        {
            info.postThinking = true;
            continue;
        }
        if (!strcmp(command, "nopost"))
        {
            info.postThinking = false;
            continue;
        }
        if(!strcmp(command, "force"))
        {
			engineSide = both;
			continue;
		}
		if (!strcmp(command, "view"))
        {
            if (depth == maxDepth)
            {
                cout << "depth not set ";
            }
            else
            {
                cout << "depth " << depth;
            }
            if (movetime != 0)
            {
                cout << " movetime " << (float)movetime/1000 << '\n';
            }
            else
            {
                cout << " movetime not set\n";
            }
            continue;
        }
        if(!strcmp(command, "depth"))
        {
			sscanf(inBuf, "depth %d", &depth);
		    if(depth == 0) depth = maxDepth;
			continue;
		}

		if(!strcmp(command, "time"))
        {
			sscanf(inBuf, "time %d", &movetime);
			movetime *= 1000;
			continue;
		}

		if(!strcmp(command, "new"))
        {
			//clearHashTable(pos.hashTable);
			engineSide = black;
			pos.parseFEN(startFEN);
			continue;
		}

		if(!strcmp(command, "go"))
        {
			engineSide = pos.side;
			continue;
		}

		move = parseMove(inBuf, pos);
		if(move == NOMOVE)
        {
			cout << "Command unknown:" << inBuf << '\n';
			continue;
		}
		if (!pos.makeMove(move))
        {
            cout << "Invalid move! King in check!\n";
        }
		pos.ply = 0;
    }
}
