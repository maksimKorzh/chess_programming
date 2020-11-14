
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

void VAssertFailedW(const char * szMod, int iLine);

#define	Assert(cond)		if (!(cond)) VAssertFailedW(s_aszModule, __LINE__)

#define	fTRUE	1
#define	fFALSE	0

void cdecl VSendToWinboard(const char * szFmt, ...);
void VProcessEngineCommand(void * pv, BOOL fThinking);
BOOL FInitEngineThread(void);
void cdecl VSendToEngine(const char * szFmt, ...);
void VStripWb(char * sz);
int	CszVectorizeWb(char * sz, char * rgsz[], int * pibFirst);
