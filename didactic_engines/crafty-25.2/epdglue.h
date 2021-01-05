#pragma once
/* *INDENT-OFF* */
/*>>> epdglue.h: subprogram prototypes for epdglue.c */
/* Revised: 1995.12.11 */
/*
   Copyright (C) 1995 by Steven J. Edwards (sje@mv.mv.com)
   All rights reserved.  This code may be freely redistibuted and used by
   both research and commerical applications.  No warranty exists.
 */
/*
   The contents of this include file are the prototypes for the glue
   routines (epdglue.c) that form the programmatic interface between the
   host program Crafty and the EPD Kit.  Therefore, this file may have
   to be changed (as will epdglue.c) if used with a different host program.
   The contents of the other source files in the EPD Kit (epddefs.h,
   epd.h, and epd.c) should not have to be changed for different hosts.
 */
/*
   This file was originally prepared on an Apple Macintosh using the
   Metrowerks CodeWarrior 6 ANSI C compiler.  Tabs are set at every
   four columns.  Further testing and development was performed on a
   generic PC running Linux 1.2.9 and using the gcc 2.6.3 compiler.
 */
int EGCommandParmCount(char *s);
int EGCommandCheck(char *s);
int EGCommand(char *s);
void EGInit(void);
void EGTerm(void);
/* *INDENT-ON* */
