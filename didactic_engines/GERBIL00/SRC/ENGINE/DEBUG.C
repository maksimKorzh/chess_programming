
//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-
//
//	Gerbil
//
//	Copyright (c) 2001, Bruce Moreland.  All rights reserved.
//
//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-
//
//	This file is part of the Gerbil chess program project.
//
//	Gerbil is free software; you can redistribute it and/or modify it under
//	the terms of the GNU General Public License as published by the Free
//	Software Foundation; either version 2 of the License, or (at your option)
//	any later version.
//
//	Gerbil is distributed in the hope that it will be useful, but WITHOUT ANY
//	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//	FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//	details.
//
//	You should have received a copy of the GNU General Public License along
//	with Gerbil; if not, write to the Free Software Foundation, Inc.,
//	59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

#include "engine.h"
#include "protocol.h"
#include <stdlib.h>

#ifdef	DEBUG
static char const s_aszModule[] = __FILE__;
#endif

#ifdef	DEBUG

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

static char s_argbBoardCo[coMAX] = "/ ";

//	This routine is probably never used, but if you want an ascii board for
//	debugging purposes, here it is.

void VDumpBoard(PCON pcon)
{
	int	rnk;
	int	fil;
	
	putchar(' ');
	for (fil = filA; fil <= filH; fil++)
		printf("   %c", fil + 'a');
	putchar('\n');
	for (rnk = rnk8; rnk >= rnk1; rnk--) {
		printf("  +");
		for (fil = filA; fil <= filH; fil++)
			printf("---+");
		putchar('\n');
		printf("  |");
		for (fil = filA; fil <= filH; fil++) {
			int	coSq = ((rnk ^ fil) & 1) ? coWHITE : coBLACK;

			putchar(s_argbBoardCo[coSq]);
			putchar(s_argbBoardCo[coSq]);
			putchar(s_argbBoardCo[coSq]);
			putchar('|');
		}
		putchar('\n');
		printf("%c |", rnk + '1');
		for (fil = filA; fil <= filH; fil++) {
			int	isq = IsqFromRnkFil(rnk, fil);
			int	coSq = ((rnk ^ fil) & 1) ? coWHITE : coBLACK;

			putchar(s_argbBoardCo[coSq]);
			if (pcon->argsq[isq].ppi == NULL)
				putchar(s_argbBoardCo[coSq]);
			else
				putchar(s_argbPc[pcon->argsq[isq].ppi->co][
					pcon->argsq[isq].ppi->pc]);
			putchar(s_argbBoardCo[coSq]);
			putchar('|');
		}
		printf(" %c\n", rnk + '1');
		printf("  |");
		for (fil = filA; fil <= filH; fil++) {
			int	coSq = ((rnk ^ fil) & 1) ? coWHITE : coBLACK;

			putchar(s_argbBoardCo[coSq]);
			putchar(s_argbBoardCo[coSq]);
			putchar(s_argbBoardCo[coSq]);
			putchar('|');
		}
		putchar('\n');
	}
	printf("  +");
	for (fil = filA; fil <= filH; fil++)
		printf("---+");
	putchar('\n');
	putchar(' ');
	for (fil = filA; fil <= filH; fil++)
		printf("   %c", fil + 'a');
	putchar('\n');
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This is the guts of the "Assert" macro.

void VAssertFailed(const char * szMod, int iLine)
{
	VPrSendToIface("Assert Failed: %s+%d\n", szMod, iLine);
	exit(1);
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

#endif	// DEBUG
