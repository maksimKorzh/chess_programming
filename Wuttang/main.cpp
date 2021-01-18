#include "init.h"
#include "uci.h"
//#include "xboard.h"
#include "position.h"
#include "search.h"
//#include "polybook.h"
#include <string>
#include <string.h>
#include <iostream>

int main(int argc, char *argv[])
{
    initAll();

    Position pos;
    searchInfo info;
    initHashTable(pos.hashTable, 128);

    std::cout << "Welcome to " << name << " made by Lokesh\nType 'wuttang' for console mode...\n";
    std::string input;

    for (int argNum = 0; argNum < argc; ++argNum)
    {
        if (!strncmp(argv[argNum], "NoBook", 6))
        {
            //engineOptions.useBook = false;
        }
    }

    while (true)
    {
        std::cin.sync();
        std::getline(std::cin, input);

        if (input == "uci")
        {
            UCI_loop(pos, info);
        }
        /*else if (input == "xboard")
        {
            //XBoard_loop(pos, info);
        }
        else if (input == "wuttang")
        {
            //Console_loop(pos, info);
        }*/
        else if (input == "quit")
        {
            info.quit = true;
        }
        else
        {
            std::cout << "Unknown command\n";
        }
        if (info.quit)
        {
            break;
        }
    }

    delete []pos.hashTable.pTable;
    //cleanPolyBook();

    return 0;
}
