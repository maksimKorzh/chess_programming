#include "uci.h"
#include "data.h"
#include "misc.h"
#include <iostream>

void UCI_loop(Position &pos, searchInfo &info)
{
    info.gameMode = UCIMODE;

    setbuf(stdin, NULL);
    setbuf(stdout, NULL);

    char line[INPUTBUFFER];
    std::cout << "id name " << name << '\n';
    std::cout << "id author Lokesh\n";
    std::cout << "option name Hash type spin default 128 min 4 max " << MAX_HASH << '\n';
    std::cout << "option name Book type check default true\n";
    std::cout << "uciok\n";

    int MB = 128;

    while (true)
    {
        memset(&line[0], 0, INPUTBUFFER);
        fflush(stdout);
        if (!fgets(line, INPUTBUFFER, stdin))
        {
            continue;
        }
        if (line[0] == '\n')
        {
            continue;
        }

        if (!strncmp(line, "isready", 7))
        {
            std::cout << "readyok\n";
            continue;
        }
        else if (!strncmp(line, "position", 8))
        {
            parsePosition(line, pos);
        }
        else if (!strncmp(line, "ucinewgame", 10))
        {
            parsePosition("position startpos\n", pos);
        }
        else if (!strncmp(line, "go", 2))
        {
            parseGo(line, info, pos);
        }
        else if (!strncmp(line, "quit", 4))
        {
            info.quit = true;
            break;
        }
        else if (!strncmp(line, "uci", 3))
        {
            std::cout << "id name " << name << '\n';
            std::cout << "id author Lokesh\n";
            std::cout << "uciok\n";
        }
        else if (!strncmp(line, "setoption name Hash value ", 26))
        {
			sscanf(line,"%*s %*s %*s %*s %d",&MB);
			if(MB < 4) MB = 4;
			if(MB > MAX_HASH) MB = MAX_HASH;
			std::cout << "Set Hash to " << MB << " MB\n";
			initHashTable(pos.hashTable, MB);
		}
		else if (!strncmp(line, "setoption name Book value ", 26))
        {
			char *ptrTrue = nullptr;
			ptrTrue = strstr(line, "true");
			if (ptrTrue != nullptr)
            {
                //engineOptions.useBook = true;
            }
            else
            {
                //engineOptions.useBook = false;
            }
		}
        if (info.quit)
        {
            break;
        }
    }
}

void parseGo(const char* line, searchInfo &info, Position &pos)
{
    int depth = maxDepth, movestogo = 30, movetime = -1;
    int time = -1, inc = 0;
    const char *ptr = nullptr;
    info.timeset = false;

    if ((ptr = strstr(line, "infinite")))
    {
        ;
    }
    if ((ptr = strstr(line, "binc")) && pos.side == black)
    {
        inc = atoi(ptr+5);
    }
    if ((ptr = strstr(line, "winc")) && pos.side == white)
    {
        inc = atoi(ptr+5);
    }
    if ((ptr = strstr(line, "wtime")) && pos.side == white)
    {
        time = atoi(ptr+6);
    }
    if ((ptr = strstr(line, "btime")) && pos.side == black)
    {
        time = atoi(ptr+6);
    }
    if ((ptr = strstr(line, "movestogo")))
    {
        movestogo = atoi(ptr+10);
    }
    if ((ptr = strstr(line, "movetime")))
    {
        movetime = atoi(ptr+9);
    }
    if ((ptr = strstr(line, "depth")))
    {
        depth = atoi(ptr+6);
    }

    if (movetime != -1)
    {
        time = movetime;
        movestogo = 1;
    }
    info.starttime = getTimeMS();
    info.depth = depth;

    if (time != -1)
    {
        info.timeset = true;
        time /= movestogo;
        time -= 50;
        if (time + inc < 10)
        {
            info.stoptime = info.starttime + 10;
        }
        else
        {
            info.stoptime = info.starttime + time + inc;
        }
    }

    //std::cout << "time : " << time << " start : " << info.starttime << " stop : " << info.stoptime << " depth : " << info.depth << " timeset : " << info.timeset << '\n';
    searchPosition(pos, info);
}

void parsePosition(const char* lineIn, Position &pos)
{
    lineIn += 9;
    const char *ptrChar = lineIn;

    if (strncmp(lineIn, "startpos", 8) == 0)
    {
        pos.parseFEN(startFEN);
    }
    else
    {
        ptrChar = strstr(lineIn, "fen");
        if (ptrChar == nullptr)
        {
            pos.parseFEN(startFEN);
        }
        else
        {
            ptrChar += 4;
            pos.parseFEN(ptrChar);
        }
    }

    ptrChar = strstr(lineIn, "moves");
    int move;

    if (ptrChar != nullptr)
    {
        ptrChar += 6;
        while (*ptrChar)
        {
            move = parseMove(ptrChar, pos);
            if (move == NOMOVE)
            {
                break;
            }
            pos.makeMove(move);
            pos.ply = 0;

            while (*ptrChar && *ptrChar != ' ') ++ptrChar;
            ++ptrChar;
        }
    }
    //pos.display();
}
