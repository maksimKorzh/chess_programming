// vice.c

#include "stdio.h"
#include "defs.h"
#include "stdlib.h"
#include "string.h"


#define WAC1 "r1b1k2r/ppppnppp/2n2q2/2b5/3NP3/2P1B3/PP3PPP/RN1QKB1R w KQkq - 0 1"
#define PERFT "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"

int main() {

	AllInit();

	S_BOARD pos[1];
    S_SEARCHINFO info[1];
    info->quit = FALSE;
	pos->HashTable->pTable = NULL;
    InitHashTable(pos->HashTable, 64);
	setbuf(stdin, NULL);
    setbuf(stdout, NULL);
	
	printf("Welcome to Vice! Type 'vice' for console mode...\n");

	char line[256];
	while (TRUE) {
		memset(&line[0], 0, sizeof(line));

		fflush(stdout);
		if (!fgets(line, 256, stdin))
			continue;
		if (line[0] == '\n')
			continue;
		if (!strncmp(line, "uci",3)) {
			Uci_Loop(pos, info);
			if(info->quit == TRUE) break;
			continue;
		} else if (!strncmp(line, "xboard",6))	{
			XBoard_Loop(pos, info);
			if(info->quit == TRUE) break;
			continue;
		} else if (!strncmp(line, "vice",4))	{
			Console_Loop(pos, info);
			if(info->quit == TRUE) break;
			continue;
		} else if(!strncmp(line, "quit",4))	{
			break;
		}
	}

	free(pos->HashTable->pTable);
	
	return 0;
}








