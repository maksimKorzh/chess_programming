
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

char const s_argbPc[coMAX][pcMAX] = {
	'P',	'N',	'B',	'R',	'Q',	'K',
	'p',	'n',	'b',	'r',	'q',	'k',
};

int	const s_argvalPc[] = {
	valPAWN,
	valMINOR,
	valMINOR,
	valROOK,
	valQUEEN,
	valKING,
};

int	const s_argvalPcOnly[] = {
	0,
	valMINOR,
	valMINOR,
	valROOK,
	valQUEEN,
	0,
};

int	const s_argvalPnOnly[] = {
	valPAWN,
	0,
	0,
	0,
	0,
	0,
};

char const * s_argszCo[] = { "White", "Black" };
