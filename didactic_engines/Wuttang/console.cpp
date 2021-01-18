#include "console.h"
#include "data.h"
#include "move.h"
#include "misc.h"
#include <iostream>
#include <string>

using namespace std;

void displayHelp()
{
    cout << "\nwuttang > new : Start a new game"
         << "\nwuttang > quit : To exit"
         << "\nwuttang > setdepth : Set depth of search"
         << "\nwuttang > settime : Set time of search"
         << "\nwuttang > print : Show board"
         << "\nwuttang > setboard : Enter FEN for position"
         << "\nwuttang > reset : To reset time and depth"
         << "\nwuttang > Enter moves in e7e8q notation";
}

/*void Console_loop(Position &pos, searchInfo &info)
{
    string input;
    int time = 10000;
    int move;

    info.timeset = true;
    info.depthset = false;
    info.depth = maxDepth;

    pos.parseFEN(startFEN);

    cout << "Type help for commands : \n";

    while(true)
    {
        cout << "\nwuttang > ";
        cin.sync();
        getline(cin, input);
        if (input == "help")
        {
            displayHelp();
        }
        else if (input == "new")
        {
            pos.parseFEN(startFEN);
        }
        else if (input == "quit")
        {
            info.quit = true;
        }
        else if (input == "print")
        {
            pos.display();
        }
        else if (input == "setdepth")
        {
            do {
                cout << "\nwuttang > Enter depth of search : ";
                cin >> info.depth;
                if (info.depth <= 0)
                {
                    cout << "wuttang > Invalid depth!";
                }
            } while (info.depth <= 0);
            info.depthset = true;
            info.timeset = false;
        }
        else if (input == "settime")
        {
            do {
                cout << "\nwuttang > Enter time of search : ";
                cin >> time;
                if (time <= 0)
                {
                    cout << "wuttang > Invalid time!";
                }
                time *= 1000;
            } while (time <= 0);
            info.timeset = true;
        }
        else if (input == "setboard")
        {
            cout << "\nwuttang > Enter FEN : ";
            cin.sync();
            getline(cin, input);
            pos.parseFEN(input);
        }
        else if (input == "reset")
        {
            info.depthset = false;
            info.depth = maxDepth;
            time = 10000;
            info.timeset = true;
        }
        else
        {
            move = parseMove(input, pos);
            if (move == NOMOVE)
            {
                cout << "\nwuttang > Invalid Move!";
            }
            else
            {
                pos.makeMove(move);
                if (info.timeset)
                {
                    info.starttime = getTimeMS();
                    info.stoptime = info.starttime + time;
                }
                cout << '\n';
                cin.sync();
                searchPosition(pos, info);
            }
        }

        if (info.quit)
        {
            break;
        }
    }
}*/
