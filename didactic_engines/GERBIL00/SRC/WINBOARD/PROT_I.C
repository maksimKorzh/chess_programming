
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

#include <stdio.h>
#include <stdarg.h>
#include <windows.h>
#include "winboard.h"
#include "protocol.h"

static char const s_aszModule[] = __FILE__;

#define	fTRUE	1
#define	fFALSE	0

void cdecl VPrSendToIface(const char * szFmt, ...)
{
	char	aszBuf[256];
	va_list	lpArgPtr;

	va_start(lpArgPtr, szFmt);
	vsprintf(aszBuf, szFmt, lpArgPtr);
	VSendToWinboard(aszBuf);
}

void VPrCallback(void * pv)
{
	VProcessEngineCommand(pv, TRUE);
}

void VPrSendMove(char * szMove)
{
	VSendToWinboard("move %s", szMove);
}

void VPrSendHint(char * szMove)
{
	VSendToWinboard("Hint: %s", szMove);
}

void VPrSendAnalysis(int ply, int val, unsigned long tm,
	unsigned __int64 nodes, char * szLine)
{
	VSendToWinboard("%d %d %lu %I64u %s", ply, val, tm / 10, nodes, szLine);
}

void VPrSendResult(char * szResult, char * szWhy)
{
	if (szWhy != NULL)
		VSendToWinboard("%s {%s}", szResult, szWhy);
	else
		VSendToWinboard("%s", szResult);
}
