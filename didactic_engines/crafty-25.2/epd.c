/*>>> epd.c: Extended Position Description routines */
#if defined(EPD)
/* Revised: 1996.06.23 */
/*
 Copyright (C) 1996 by Steven J. Edwards (sje@mv.mv.com)
 All rights reserved.  This code may be freely redistibuted and used by
 both research and commerical applications.  No warranty exists.
 */
/*
 Everything in this source file is independent of the host program, as
 are the prototypes in the epd.h include file.  Requests for changes
 and additions should be communicated to the author via the e-mail
 address given above.
 */
/*
 This file was originally prepared on an Apple Macintosh using the
 Metrowerks CodeWarrior 6 ANSI C compiler.  Tabs are set at every
 four columns.  Further testing and development was performed on a
 generic PC running Linux 1.3.20 and using the gcc 2.7.0 compiler.
 */
/* system includes */
#  include <ctype.h>
#  include <time.h>
#  include "chess.h"
#  include "data.h"
/* EPD definitions (host independent) */
#  include "epddefs.h"
/* EPD routine prototypes (host independent) */
#  include "epd.h"
/* ASCII character constants */
#  define ascii_nul ((char) 0x00)
#  define ascii_sp  ((char) 0x20)
/* tree limit; adjust according to memory availability */
#  define treeL 16384
/* tree overrun safety margin */
#  define treemarginL 256
/* played moves history limit; adjust according to memory availability */
#  define pmhL 512
/* data flows (input and output) */
#  define flowL 2
/* character case mapping */
#  define map_lower(ch) (isupper((ch)) ? tolower((ch)) : (ch))
#  define map_upper(ch) (islower((ch)) ? toupper((ch)) : (ch))
/* identifier character check */
#  define IdentChar(ch) (isalpha((ch)) || isdigit((ch)) || ((ch) == '_'))
/* vacancy check */
#  define Vacant(sq) (EPDboard.rbv[(sq)] == cp_v0)
/* token record type (for token chain) */
typedef struct tknS {
  charptrT tkn_str;             /* allocated token string value */
  struct tknS *tkn_prev;        /* previous record */
  struct tknS *tkn_next;        /* next record */
} tknT, *tknptrT;

/* tree stack entry record type */
typedef struct tseS {
  siT tse_count;                /* entry count for this level */
  mptrT tse_base;               /* first move in moveset */
  mptrT tse_curr;               /* current move of interest in moveset */
} tseT, *tseptrT;

/* color to move strings */
/* global game chain anchors */
static gamptrT head_gamptr;
static gamptrT tail_gamptr;

/* EPD standard opcode mnemonics */
static charptrT epdsostrv[epdsoL];

/* EPD refcom operand strings */
static charptrT refcomstrv[refcomL];

/* EPD refreq operand strings */
static charptrT refreqstrv[refreqL];

/* PGN Seven Tag Roster names */
static charptrT pgnstrstrv[pgnstrL];

/* game termination indication marker strings */
static charptrT gtimstrv[gtimL];

/* player name strings */
/* character conversion vectors (colors and pieces) */
static char asccv[rcL];
static char ascpv[rpL];

/* character conversion vectors (ranks and files) */
static char ascrv[rankL];
static char ascfv[fileL];

/* promotion piece from special case move code coversion vector */
static pT cv_p_scmvv[scmvL];

/* various color and piece conversion vectors */
static cpT cv_cp_c_pv[rcL][rpL];
static cT cv_c_cpv[cpL];
static pT cv_p_cpv[cpL];
static cT inv_cv[rcL];

/* direction vectors */
static dvT dvv[dxL];
static xdvT xdvv[dxL];

/* extension board (border detection) */
static xbT xb;

/* token chain anchors */
static tknptrT head_tknptr;
static tknptrT tail_tknptr;

/* local SAN vector and its index */
static sanT lsan;
static siT lsani;

/* census vectors */
static siT count_cv[rcL];
static siT count_cpv[rcL][rpL];

/* the current board */
static rbT EPDboard;

/* the current environment stack entry */
static eseT ese;

/* the current tree stack entry */
static tseT tse;

/* the master ply index */
static siT ply;

/* the base of the move tree and its current pointer */
static mptrT treebaseptr;
static mptrT treeptr;

/* the base of the tree stack entry stack and its current pointer */
static tseptrT tsebaseptr;
static tseptrT tseptr;

/* base of the environment stack and its current pointer */
static eseptrT esebaseptr;
static eseptrT eseptr;

/* return area for board data */
static rbT ret_rb;

/* return area for move data */
static mT ret_m;

/*--> EPDFatal: emit fatal diagnostic and quit */
nonstatic void EPDFatal(charptrT s) {
  fprintf(stderr, "EPD Fatal error: %s.\n", s);
  exit(1);
}

/*--> EPDSwitchFault: emit switch fault diagnostic and quit */
nonstatic void EPDSwitchFault(charptrT s) {
  fprintf(stderr, "Switch fault detected.\n");
  EPDFatal(s);
  return;
}

/*--> EPDMemoryGrab: allocate memory */
nonstatic voidptrT EPDMemoryGrab(liT n) {
  voidptrT ptr;

  ptr = (voidptrT) malloc((n == 0) ? 1 : n);
  if (ptr == NULL)
    EPDFatal("EPDMemoryGrab");
  return ptr;
}

/*--> EPDMemoryFree: deallocate memory */
nonstatic void EPDMemoryFree(voidptrT ptr) {
  if (ptr != NULL)
    free(ptr);
  return;
}

/*--> EPDStringGrab: allocate and copy a string */
nonstatic charptrT EPDStringGrab(charptrT s) {
  charptrT ptr;

  ptr = (charptrT) EPDMemoryGrab(strlen(s) + 1);
  strcpy(ptr, s);
  return ptr;
}

/*--> EPDStringAppendChar: append a character to a string */
nonstatic charptrT EPDStringAppendChar(charptrT s, char c) {
  charptrT ptr;
  liT length;

/* the first argument is deallocated */
  length = strlen(s);
  ptr = (charptrT) EPDMemoryGrab(length + 2);
  strcpy(ptr, s);
  EPDMemoryFree(s);
  *(ptr + length) = c;
  *(ptr + length + 1) = ascii_nul;
  return ptr;
}

/*--> EPDStringAppendStr: append a string to a string */
nonstatic charptrT EPDStringAppendStr(charptrT s0, charptrT s1) {
  charptrT ptr;
  liT length;

/* the first argument is deallocated */
  length = strlen(s0) + strlen(s1);
  ptr = (charptrT) EPDMemoryGrab(length + 1);
  strcpy(ptr, s0);
  strcat(ptr, s1);
  EPDMemoryFree(s0);
  return ptr;
}

/*--> EPDNewGPM: allocate and initialize a new GPM record */
static gpmptrT EPDNewGPM(mptrT mptr) {
  gpmptrT gpmptr;
  cT c;
  sqT sq;
  cpT cp0, cp1;

  gpmptr = (gpmptrT) EPDMemoryGrab(sizeof(gpmT));
  gpmptr->gpm_m = *mptr;
  gpmptr->gpm_ese.ese_actc = ese.ese_actc;
  gpmptr->gpm_ese.ese_cast = ese.ese_cast;
  gpmptr->gpm_ese.ese_epsq = ese.ese_epsq;
  gpmptr->gpm_ese.ese_hmvc = ese.ese_hmvc;
  gpmptr->gpm_ese.ese_fmvn = ese.ese_fmvn;
  for (c = c_w; c <= c_b; c++)
    gpmptr->gpm_ese.ese_ksqv[c] = ese.ese_ksqv[c];
  for (sq = sq_a1; sq <= sq_h8; sq += 2) {
    cp0 = EPDboard.rbv[sq + 0];
    cp1 = EPDboard.rbv[sq + 1];
    gpmptr->gpm_nbv[sq >> 1] = ((cp1 << nybbW) | cp0);
  };
  gpmptr->gpm_prev = gpmptr->gpm_next = NULL;
  return gpmptr;
}

/*--> EPDReleaseGPM: release a GPM record */
static
void EPDReleaseGPM(gpmptrT gpmptr) {
  if (gpmptr != NULL)
    EPDMemoryFree(gpmptr);
  return;
}

/*--> EPDAppendGPM: append a GPM record to a game  */
static
void EPDAppendGPM(gamptrT gamptr, gpmptrT gpmptr) {
  if (gamptr->gam_tailgpm == NULL)
    gamptr->gam_headgpm = gpmptr;
  else
    gamptr->gam_tailgpm->gpm_next = gpmptr;
  gpmptr->gpm_prev = gamptr->gam_tailgpm;
  gpmptr->gpm_next = NULL;
  gamptr->gam_tailgpm = gpmptr;
  return;
}

/*--> EPDUnthreadGPM: unthread a GPM record from a game */
static
void EPDUnthreadGPM(gamptrT gamptr, gpmptrT gpmptr) {
  if (gpmptr->gpm_prev == NULL)
    gamptr->gam_headgpm = gpmptr->gpm_next;
  else
    gpmptr->gpm_prev->gpm_next = gpmptr->gpm_next;
  if (gpmptr->gpm_next == NULL)
    gamptr->gam_tailgpm = gpmptr->gpm_prev;
  else
    gpmptr->gpm_next->gpm_prev = gpmptr->gpm_prev;
  return;
}

/*--> EPDReleaseGPMoveChain: release the game played moves chain */
static
void EPDReleaseGPMoveChain(gamptrT gamptr) {
  gpmptrT gpmptr;

  while (gamptr->gam_headgpm != NULL) {
    gpmptr = gamptr->gam_tailgpm;
    EPDUnthreadGPM(gamptr, gpmptr);
    EPDReleaseGPM(gpmptr);
  };
  return;
}

/*--> EPDNewGAM: allocate and initialize a new GAM record */
static gamptrT EPDNewGAM(void) {
  gamptrT gamptr;
  pgnstrT pgnstr;

  gamptr = (gamptrT) EPDMemoryGrab(sizeof(gamT));
  for (pgnstr = 0; pgnstr < pgnstrL; pgnstr++)
    gamptr->gam_strv[pgnstr] = EPDStringGrab("");
  gamptr->gam_gtim = gtim_u;
  gamptr->gam_headgpm = gamptr->gam_tailgpm = NULL;
  gamptr->gam_prev = gamptr->gam_next = NULL;
  return gamptr;
}

/*--> EPDReleaseGAM: release a GAM record */
static
void EPDReleaseGAM(gamptrT gamptr) {
  pgnstrT pgnstr;

  if (gamptr != NULL) {
    for (pgnstr = 0; pgnstr < pgnstrL; pgnstr++)
      if (gamptr->gam_strv[pgnstr] != NULL)
        EPDMemoryFree(gamptr->gam_strv[pgnstr]);
    EPDReleaseGPMoveChain(gamptr);
    EPDMemoryFree(gamptr);
  };
  return;
}

/*--> EPDAppendGAM: append a GAM record to the game chain */
static
void EPDAppendGAM(gamptrT gamptr) {
  if (tail_gamptr == NULL)
    head_gamptr = gamptr;
  else
    tail_gamptr->gam_next = gamptr;
  gamptr->gam_prev = tail_gamptr;
  gamptr->gam_next = NULL;
  tail_gamptr = gamptr;
  return;
}

/*--> EPDUnthreadGAM: unthread a GAM record from the game chain */
static
void EPDUnthreadGAM(gamptrT gamptr) {
  if (gamptr->gam_prev == NULL)
    head_gamptr = gamptr->gam_next;
  else
    gamptr->gam_prev->gam_next = gamptr->gam_next;
  if (gamptr->gam_next == NULL)
    tail_gamptr = gamptr->gam_prev;
  else
    gamptr->gam_next->gam_prev = gamptr->gam_prev;
  return;
}

/*--> EPDReleaseGameChain: release the game chain */
static
void EPDReleaseGameChain(void) {
  gamptrT gamptr;

  while (head_gamptr != NULL) {
    gamptr = tail_gamptr;
    EPDUnthreadGAM(gamptr);
    EPDReleaseGAM(gamptr);
  };
  return;
}

/*--> EPDGamePlyCount: return ply count of a game */
static siT EPDGamePlyCount(gamptrT gamptr) {
  siT count;
  gpmptrT gpmptr;

  count = 0;
  gpmptr = gamptr->gam_headgpm;
  while (gpmptr != NULL) {
    count++;
    gpmptr = gpmptr->gpm_next;
  };
  return count;
}

/*--> EPDGameOpen: create/open a new game structure */
nonstatic gamptrT EPDGameOpen(void) {
  gamptrT gamptr;

  gamptr = EPDNewGAM();
  EPDAppendGAM(gamptr);
  return gamptr;
}

/*--> EPDGameClose: close/destroy a game structure */
nonstatic void EPDGameClose(gamptrT gamptr) {
  if (gamptr != NULL) {
    EPDUnthreadGAM(gamptr);
    EPDReleaseGAM(gamptr);
  };
  return;
}

/*--> EPDGameAppendMove: append a move to a game structure */
nonstatic void EPDGameAppendMove(gamptrT gamptr, mptrT mptr) {
  gpmptrT gpmptr;

  gpmptr = EPDNewGPM(mptr);
  EPDAppendGPM(gamptr, gpmptr);
  return;
}

/*--> EPDNewTKN: allocate and initialize a new TKN record */
static tknptrT EPDNewTKN(charptrT s) {
  tknptrT tknptr;

  tknptr = (tknptrT) EPDMemoryGrab(sizeof(tknT));
  tknptr->tkn_str = EPDStringGrab(s);
  tknptr->tkn_prev = tknptr->tkn_next = NULL;
  return tknptr;
}

/*--> EPDReleaseTKN: release a TKN record */
static
void EPDReleaseTKN(tknptrT tknptr) {
  if (tknptr != NULL) {
    if (tknptr->tkn_str != NULL)
      EPDMemoryFree(tknptr->tkn_str);
    EPDMemoryFree(tknptr);
  };
  return;
}

/*--> EPDAppendTKN: append a TKN record to the token chain */
static
void EPDAppendTKN(tknptrT tknptr) {
  if (tail_tknptr == NULL)
    head_tknptr = tknptr;
  else
    tail_tknptr->tkn_next = tknptr;
  tknptr->tkn_prev = tail_tknptr;
  tknptr->tkn_next = NULL;
  tail_tknptr = tknptr;
  return;
}

/*--> EPDUnthreadTKN: unthread a TKN record from the token chain */
static
void EPDUnthreadTKN(tknptrT tknptr) {
  if (tknptr->tkn_prev == NULL)
    head_tknptr = tknptr->tkn_next;
  else
    tknptr->tkn_prev->tkn_next = tknptr->tkn_next;
  if (tknptr->tkn_next == NULL)
    tail_tknptr = tknptr->tkn_prev;
  else
    tknptr->tkn_next->tkn_prev = tknptr->tkn_prev;
  return;
}

/*--> EPDReleaseTokenChain: release the token chain */
static
void EPDReleaseTokenChain(void) {
  tknptrT tknptr;

  while (head_tknptr != NULL) {
    tknptr = tail_tknptr;
    EPDUnthreadTKN(tknptr);
    EPDReleaseTKN(tknptr);
  };
  return;
}

/*--> EPDTokenize: create the token chain */
nonstatic void EPDTokenize(charptrT s) {
  siT i;
  char c;
  tknptrT tknptr;
  charptrT str;

/* first, release any existing chain */
  EPDReleaseTokenChain();
/* scan the input until end of string */
  i = 0;
  c = *(s + i++);
  while (c != ascii_nul) {
/* skip leading whitespace */
    while ((c != ascii_nul) && isspace(c))
      c = *(s + i++);
/* if not at end of string, then a token has started */
    if (c != ascii_nul) {
      str = EPDStringGrab("");
      while ((c != ascii_nul) && !isspace(c)) {
        str = EPDStringAppendChar(str, c);
        c = *(s + i++);
      };
      tknptr = EPDNewTKN(str);
      EPDAppendTKN(tknptr);
      EPDMemoryFree(str);
    };
  };
  return;
}

/*--> EPDTokenCount: count the tokens in the token chain */
nonstatic siT EPDTokenCount(void) {
  siT n;
  tknptrT tknptr;

  n = 0;
  tknptr = head_tknptr;
  while (tknptr != NULL) {
    n++;
    tknptr = tknptr->tkn_next;
  };
  return n;
}

/*--> EPDTokenFetch: fetch n-th (zero origin) token from the token chain */
nonstatic charptrT EPDTokenFetch(siT n) {
  charptrT s;
  siT i;
  tknptrT tknptr;

  i = 0;
  tknptr = head_tknptr;
  while ((tknptr != NULL) && (i < n)) {
    i++;
    tknptr = tknptr->tkn_next;
  };
  if (tknptr == NULL)
    s = NULL;
  else
    s = tknptr->tkn_str;
  return s;
}

/*--> EPDCICharEqual: test for case independent character equality */
nonstatic siT EPDCICharEqual(char ch0, char ch1) {
  siT flag;

  if (map_lower(ch0) == map_lower(ch1))
    flag = 1;
  else
    flag = 0;
  return flag;
}

/*--> EPDPieceFromCP: fetch piece from color-piece */
nonstatic pT EPDPieceFromCP(cpT cp) {
  pT p;

  p = cv_p_cpv[cp];
  return p;
}

/*--> EPDCheckPiece: test if a character is a piece letter */
nonstatic siT EPDCheckPiece(char ch) {
  siT flag;
  pT p;

  flag = 0;
  p = p_p;
  while (!flag && (p <= p_k))
    if (EPDCICharEqual(ch, ascpv[p]))
      flag = 1;
    else
      p++;
  return flag;
}

/*--> EPDEvaluatePiece: evaluate a piece letter character */
nonstatic pT EPDEvaluatePiece(char ch) {
  pT p;
  siT flag;

  flag = 0;
  p = p_p;
  while (!flag && (p <= p_k))
    if (EPDCICharEqual(ch, ascpv[p]))
      flag = 1;
    else
      p++;
  if (!flag)
    p = p_nil;
  return p;
}

/*--> EPDCheckColor: test if a character is a color letter */
nonstatic siT EPDCheckColor(char ch) {
  siT flag;
  cT c;

  flag = 0;
  c = c_w;
  while (!flag && (c <= c_b))
    if (EPDCICharEqual(ch, asccv[c]))
      flag = 1;
    else
      c++;
  return flag;
}

/*--> EPDEvaluateColor: evaluate a color letter character */
nonstatic cT EPDEvaluateColor(char ch) {
  cT c;
  siT flag;

  flag = 0;
  c = c_w;
  while (!flag && (c <= c_b))
    if (EPDCICharEqual(ch, asccv[c]))
      flag = 1;
    else
      c++;
  if (!flag)
    c = c_nil;
  return c;
}

/*--> EPDCheckRank: test if a character is a rank character */
nonstatic siT EPDCheckRank(char ch) {
  siT flag;
  rankT rank;

  flag = 0;
  rank = rank_1;
  while (!flag && (rank <= rank_8))
    if (EPDCICharEqual(ch, ascrv[rank]))
      flag = 1;
    else
      rank++;
  return flag;
}

/*--> EPDEvaluateRank: evaluate a color rank character */
nonstatic rankT EPDEvaluateRank(char ch) {
  rankT rank;
  siT flag;

  flag = 0;
  rank = rank_1;
  while (!flag && (rank <= rank_8))
    if (EPDCICharEqual(ch, ascrv[rank]))
      flag = 1;
    else
      rank++;
  if (!flag)
    rank = rank_nil;
  return rank;
}

/*--> EPDCheckFile: test if a character is a file character */
nonstatic siT EPDCheckFile(char ch) {
  siT flag;
  fileT file;

  flag = 0;
  file = file_a;
  while (!flag && (file <= file_h))
    if (EPDCICharEqual(ch, ascfv[file]))
      flag = 1;
    else
      file++;
  return flag;
}

/*--> EPDEvaluateFile: evaluate a color file character */
nonstatic rankT EPDEvaluateFile(char ch) {
  fileT file;
  siT flag;

  flag = 0;
  file = file_a;
  while (!flag && (file <= file_h))
    if (EPDCICharEqual(ch, ascfv[file]))
      flag = 1;
    else
      file++;
  if (!flag)
    file = file_nil;
  return file;
}

/*--> EPDNewEOV: allocate a new EOV record */
nonstatic eovptrT EPDNewEOV(void) {
  eovptrT eovptr;

  eovptr = (eovptrT) EPDMemoryGrab(sizeof(eovT));
  eovptr->eov_eob = eob_nil;
  eovptr->eov_str = NULL;
  eovptr->eov_prev = eovptr->eov_next = NULL;
  return eovptr;
}

/*--> EPDReleaseEOV: release an EOV record */
nonstatic void EPDReleaseEOV(eovptrT eovptr) {
  if (eovptr != NULL) {
    if (eovptr->eov_str != NULL)
      EPDMemoryFree(eovptr->eov_str);
    EPDMemoryFree(eovptr);
  };
  return;
}

/*--> EPDAppendEOV: append an EOV record */
nonstatic void EPDAppendEOV(eopptrT eopptr, eovptrT eovptr) {
  if (eopptr->eop_taileov == NULL)
    eopptr->eop_headeov = eovptr;
  else
    eopptr->eop_taileov->eov_next = eovptr;
  eovptr->eov_prev = eopptr->eop_taileov;
  eovptr->eov_next = NULL;
  eopptr->eop_taileov = eovptr;
  return;
}

/*--> EPDUnthreadEOV: unthread an EOV record */
static
void EPDUnthreadEOV(eopptrT eopptr, eovptrT eovptr) {
  if (eovptr->eov_prev == NULL)
    eopptr->eop_headeov = eovptr->eov_next;
  else
    eovptr->eov_prev->eov_next = eovptr->eov_next;
  if (eovptr->eov_next == NULL)
    eopptr->eop_taileov = eovptr->eov_prev;
  else
    eovptr->eov_next->eov_prev = eovptr->eov_prev;
  return;
}

/*--> EPDCreateEOVSym: create a new EOV record with a symbol value */
nonstatic eovptrT EPDCreateEOVSym(charptrT sym) {
  eovptrT eovptr;

  eovptr = EPDNewEOV();
  eovptr->eov_eob = eob_symbol;
  eovptr->eov_str = EPDStringGrab(sym);
  return eovptr;
}

/*--> EPDCreateEOVInt: create a new EOV record with an integer value */
nonstatic eovptrT EPDCreateEOVInt(liT lval) {
  eovptrT eovptr;
  char tv[tL];

  sprintf(tv, "%ld", lval);
  eovptr = EPDNewEOV();
  eovptr->eov_eob = eob_symbol;
  eovptr->eov_str = EPDStringGrab(tv);
  return eovptr;
}

/*--> EPDLocateEOV: try to locate 1st EOV record with given string value */
nonstatic eovptrT EPDLocateEOV(eopptrT eopptr, charptrT strval) {
  eovptrT eovptr;
  siT flag;

  flag = 0;
  eovptr = eopptr->eop_headeov;
  while (!flag && (eovptr != NULL))
    if (strcmp(strval, eovptr->eov_str) == 0)
      flag = 1;
    else
      eovptr = eovptr->eov_next;
  if (!flag)
    eovptr = NULL;
  return eovptr;
}

/*--> EPDReplaceEOVStr: replace EOV string value with given string value */
nonstatic void EPDReplaceEOVStr(eovptrT eovptr, charptrT str) {
  if (eovptr->eov_str != NULL) {
    EPDMemoryFree(eovptr->eov_str);
    eovptr->eov_str = NULL;
  };
  if (str != NULL)
    eovptr->eov_str = EPDStringGrab(str);
  return;
}

/*--> EPDNewEOP: allocate a new EOP record */
nonstatic eopptrT EPDNewEOP(void) {
  eopptrT eopptr;

  eopptr = (eopptrT) EPDMemoryGrab(sizeof(eopT));
  eopptr->eop_opsym = NULL;
  eopptr->eop_headeov = eopptr->eop_taileov = NULL;
  eopptr->eop_prev = eopptr->eop_next = NULL;
  return eopptr;
}

/*--> EPDReleaseEOP: release an EOP record */
nonstatic void EPDReleaseEOP(eopptrT eopptr) {
  eovptrT eovptr0, eovptr1;

  if (eopptr != NULL) {
    if (eopptr->eop_opsym != NULL)
      EPDMemoryFree(eopptr->eop_opsym);
    eovptr0 = eopptr->eop_headeov;
    while (eovptr0 != NULL) {
      eovptr1 = eovptr0->eov_next;
      EPDUnthreadEOV(eopptr, eovptr0);
      EPDReleaseEOV(eovptr0);
      eovptr0 = eovptr1;
    };
    EPDMemoryFree(eopptr);
  };
  return;
}

/*--> EPDAppendEOP: append an EOP record */
nonstatic void EPDAppendEOP(epdptrT epdptr, eopptrT eopptr) {
  if (epdptr->epd_taileop == NULL)
    epdptr->epd_headeop = eopptr;
  else
    epdptr->epd_taileop->eop_next = eopptr;
  eopptr->eop_prev = epdptr->epd_taileop;
  eopptr->eop_next = NULL;
  epdptr->epd_taileop = eopptr;
  return;
}

/*--> EPDUnthreadEOP: unthread an EOP record */
static
void EPDUnthreadEOP(epdptrT epdptr, eopptrT eopptr) {
  if (eopptr->eop_prev == NULL)
    epdptr->epd_headeop = eopptr->eop_next;
  else
    eopptr->eop_prev->eop_next = eopptr->eop_next;
  if (eopptr->eop_next == NULL)
    epdptr->epd_taileop = eopptr->eop_prev;
  else
    eopptr->eop_next->eop_prev = eopptr->eop_prev;
  return;
}

/*--> EPDCreateEOP: create a new EOP record with opsym */
nonstatic eopptrT EPDCreateEOP(charptrT opsym) {
  eopptrT eopptr;

  eopptr = EPDNewEOP();
  eopptr->eop_opsym = EPDStringGrab(opsym);
  return eopptr;
}

/*--> EPDCreateEOPCode: create a new EOP record using opsym index */
nonstatic eopptrT EPDCreateEOPCode(epdsoT epdso) {
  eopptrT eopptr;

  eopptr = EPDCreateEOP(EPDFetchOpsym(epdso));
  return eopptr;
}

/*--> EPDLocateEOP: attempt to locate EOP record with given opsym */
nonstatic eopptrT EPDLocateEOP(epdptrT epdptr, charptrT opsym) {
  eopptrT eopptr;
  siT flag;

  flag = 0;
  eopptr = epdptr->epd_headeop;
  while (!flag && (eopptr != NULL))
    if (strcmp(opsym, eopptr->eop_opsym) == 0)
      flag = 1;
    else
      eopptr = eopptr->eop_next;
  if (!flag)
    eopptr = NULL;
  return eopptr;
}

/*--> EPDLocateEOPCode: attempt to locate EOP record with given code */
nonstatic eopptrT EPDLocateEOPCode(epdptrT epdptr, epdsoT epdso) {
  return EPDLocateEOP(epdptr, epdsostrv[epdso]);
}

/*--> EPDDropIfLocEOPCode: try to locate/drop EOP record with given code */
nonstatic void EPDDropIfLocEOPCode(epdptrT epdptr, epdsoT epdso) {
  eopptrT eopptr;

  eopptr = EPDLocateEOP(epdptr, epdsostrv[epdso]);
  if (eopptr != NULL) {
    EPDUnthreadEOP(epdptr, eopptr);
    EPDReleaseEOP(eopptr);
  };
  return;
}

/*--> EPDAddOpInt: add a single integer operand operation */
nonstatic void EPDAddOpInt(epdptrT epdptr, epdsoT epdso, liT val) {
  eopptrT eopptr;
  eovptrT eovptr;

  eovptr = EPDCreateEOVInt(val);
  eopptr = EPDCreateEOPCode(epdso);
  EPDAppendEOV(eopptr, eovptr);
  EPDDropIfLocEOPCode(epdptr, epdso);
  EPDAppendEOP(epdptr, eopptr);
  return;
}

/*--> EPDAddOpSym: add a single symbol operand operation */
nonstatic void EPDAddOpSym(epdptrT epdptr, epdsoT epdso, charptrT s) {
  eopptrT eopptr;
  eovptrT eovptr;

  eovptr = EPDCreateEOVSym(s);
  eopptr = EPDCreateEOPCode(epdso);
  EPDAppendEOV(eopptr, eovptr);
  EPDDropIfLocEOPCode(epdptr, epdso);
  EPDAppendEOP(epdptr, eopptr);
  return;
}

/*--> EPDNewEPD: allocate a new EPD record */
nonstatic epdptrT EPDNewEPD(void) {
  epdptrT epdptr;
  siT i;

  epdptr = (epdptrT) EPDMemoryGrab(sizeof(epdT));
  for (i = 0; i < nbL; i++)
    epdptr->epd_nbv[i] = ((cp_v0 << nybbW) | cp_v0);
  epdptr->epd_actc = c_v;
  epdptr->epd_cast = 0;
  epdptr->epd_epsq = sq_nil;
  epdptr->epd_headeop = epdptr->epd_taileop = NULL;
  return epdptr;
}

/*--> EPDReleaseOperations: release EPD operation list */
nonstatic void EPDReleaseOperations(epdptrT epdptr) {
  eopptrT eopptr0, eopptr1;

  if (epdptr != NULL) {
    eopptr0 = epdptr->epd_headeop;
    while (eopptr0 != NULL) {
      eopptr1 = eopptr0->eop_next;
      EPDUnthreadEOP(epdptr, eopptr0);
      EPDReleaseEOP(eopptr0);
      eopptr0 = eopptr1;
    };
    epdptr->epd_headeop = NULL;
    epdptr->epd_taileop = NULL;
  };
  return;
}

/*--> EPDReleaseEPD: release an EPD record */
nonstatic void EPDReleaseEPD(epdptrT epdptr) {
  if (epdptr != NULL) {
    EPDReleaseOperations(epdptr);
    EPDMemoryFree(epdptr);
  };
  return;
}

/*--> EPDFetchOpsym: return a pointer to the indicated mnemonic */
nonstatic charptrT EPDFetchOpsym(epdsoT epdso) {
  return epdsostrv[epdso];
}

/*--> EPDCountOperands: count operands */
static siT EPDCountOperands(eopptrT eopptr) {
  siT count;
  eovptrT eovptr;

  count = 0;
  eovptr = eopptr->eop_headeov;
  while (eovptr != NULL) {
    count++;
    eovptr = eovptr->eov_next;
  };
  return count;
}

/*--> EPDCountOperations: count operations */
static siT EPDCountOperations(epdptrT epdptr) {
  siT count;
  eopptrT eopptr;

  count = 0;
  eopptr = epdptr->epd_headeop;
  while (eopptr != NULL) {
    count++;
    eopptr = eopptr->eop_next;
  };
  return count;
}

/*--> EPDSortOperands: sort operands according to string value */
static
void EPDSortOperands(eopptrT eopptr) {
  siT count;
  siT pass, flag;
  eovptrT ptr0, ptr1, ptr2, ptr3;

  count = EPDCountOperands(eopptr);
  if (count > 1) {
    flag = 1;
    pass = 0;
    while (flag && (pass < (count - 1))) {
      flag = 0;
      ptr0 = eopptr->eop_headeov;
      ptr1 = ptr0->eov_next;
      while (ptr1 != NULL) {
        if (strcmp(ptr0->eov_str, ptr1->eov_str) > 0) {
          flag = 1;
          ptr2 = ptr0->eov_prev;
          ptr3 = ptr1->eov_next;
          ptr0->eov_prev = ptr1;
          ptr0->eov_next = ptr3;
          ptr1->eov_prev = ptr2;
          ptr1->eov_next = ptr0;
          if (ptr2 == NULL)
            eopptr->eop_headeov = ptr1;
          else
            ptr2->eov_next = ptr1;
          if (ptr3 == NULL)
            eopptr->eop_taileov = ptr0;
          else
            ptr3->eov_prev = ptr0;
        } else
          ptr0 = ptr1;
        ptr1 = ptr0->eov_next;
      };
      pass++;
    };
  };
  return;
}

/*--> EPDSortOperations: sort operations according to opcode */
static
void EPDSortOperations(epdptrT epdptr) {
  siT count;
  siT pass, flag;
  eopptrT ptr0, ptr1, ptr2, ptr3;

  count = EPDCountOperations(epdptr);
  if (count > 1) {
    flag = 1;
    pass = 0;
    while (flag && (pass < (count - 1))) {
      flag = 0;
      ptr0 = epdptr->epd_headeop;
      ptr1 = ptr0->eop_next;
      while (ptr1 != NULL) {
        if (strcmp(ptr0->eop_opsym, ptr1->eop_opsym) > 0) {
          flag = 1;
          ptr2 = ptr0->eop_prev;
          ptr3 = ptr1->eop_next;
          ptr0->eop_prev = ptr1;
          ptr0->eop_next = ptr3;
          ptr1->eop_prev = ptr2;
          ptr1->eop_next = ptr0;
          if (ptr2 == NULL)
            epdptr->epd_headeop = ptr1;
          else
            ptr2->eop_next = ptr1;
          if (ptr3 == NULL)
            epdptr->epd_taileop = ptr0;
          else
            ptr3->eop_prev = ptr0;
        } else
          ptr0 = ptr1;
        ptr1 = ptr0->eop_next;
      };
      pass++;
    };
  };
  return;
}

/*--> EPDNormalize: apply normalizing sorts */
static
void EPDNormalize(epdptrT epdptr) {
  eopptrT eopptr;
  charptrT opsym;
  siT flag;

/* sort all operations */
  EPDSortOperations(epdptr);
/* sort operands for selected standard operations */
  eopptr = epdptr->epd_headeop;
  while (eopptr != NULL) {
    flag = 0;
    opsym = eopptr->eop_opsym;
    if (!flag && (strcmp(opsym, epdsostrv[epdso_am]) == 0)) {
      EPDSortOperands(eopptr);
      flag = 1;
    };
    if (!flag && (strcmp(opsym, epdsostrv[epdso_bm]) == 0)) {
      EPDSortOperands(eopptr);
      flag = 1;
    };
    eopptr = eopptr->eop_next;
  };
  return;
}

/*--> EPDCloneEPDBase: clone an EPD structure, base items only */
nonstatic epdptrT EPDCloneEPDBase(epdptrT epdptr) {
  epdptrT nptr;
  siT index;

  nptr = EPDNewEPD();
  for (index = 0; index < nbL; index++)
    nptr->epd_nbv[index] = epdptr->epd_nbv[index];
  nptr->epd_actc = epdptr->epd_actc;
  nptr->epd_cast = epdptr->epd_cast;
  nptr->epd_epsq = epdptr->epd_epsq;
  return nptr;
}

/*--> EPDCloneEOV: clone an EOV structure */
nonstatic eovptrT EPDCloneEOV(eovptrT eovptr) {
  eovptrT nptr;

  nptr = EPDNewEOV();
  nptr->eov_eob = eovptr->eov_eob;
  if (eovptr->eov_str != NULL)
    nptr->eov_str = EPDStringGrab(eovptr->eov_str);
  return nptr;
}

/*--> EPDCloneEOP: clone an EOP structure */
nonstatic eopptrT EPDCloneEOP(eopptrT eopptr) {
  eopptrT nptr;
  eovptrT eovptr, rptr;

  nptr = EPDNewEOP();
  if (eopptr->eop_opsym != NULL)
    nptr->eop_opsym = EPDStringGrab(eopptr->eop_opsym);
  rptr = eopptr->eop_headeov;
  while (rptr != NULL) {
    eovptr = EPDCloneEOV(rptr);
    EPDAppendEOV(nptr, eovptr);
    rptr = rptr->eov_next;
  };
  return nptr;
}

/*--> EPDSetKings: set the king location vector */
static
void EPDSetKings(void) {
  sqT sq;

/* this operates only on the local environment */
  ese.ese_ksqv[c_w] = ese.ese_ksqv[c_b] = sq_nil;
  for (sq = sq_a1; sq <= sq_h8; sq++)
    switch (EPDboard.rbv[sq]) {
      case cp_wk:
        ese.ese_ksqv[c_w] = sq;
        break;
      case cp_bk:
        ese.ese_ksqv[c_b] = sq;
        break;
      default:
        break;
    };
  return;
}

/*--> EPDSet: set up an EPD structure for the given position */
nonstatic epdptrT EPDSet(rbptrT rbptr, cT actc, castT cast, sqT epsq) {
  epdptrT epdptr;
  sqT sq;
  cpT cp0, cp1;

/* this does not reference the current position */
  epdptr = EPDNewEPD();
  for (sq = sq_a1; sq <= sq_h8; sq += 2) {
    cp0 = rbptr->rbv[sq + 0];
    cp1 = rbptr->rbv[sq + 1];
    epdptr->epd_nbv[sq >> 1] = ((cp1 << nybbW) | cp0);
  };
  epdptr->epd_actc = actc;
  epdptr->epd_cast = cast;
  epdptr->epd_epsq = epsq;
  return epdptr;
}

/*--> EPDSetCurrentPosition: set current position */
nonstatic void EPDSetCurrentPosition(rbptrT rbptr, cT actc, castT cast,
    sqT epsq, siT hmvc, siT fmvn) {
  sqT sq;

/* this changes the current position */
  for (sq = sq_a1; sq <= sq_h8; sq++)
    EPDboard.rbv[sq] = rbptr->rbv[sq];
  ese.ese_actc = actc;
  ese.ese_cast = cast;
  ese.ese_epsq = epsq;
  ese.ese_hmvc = hmvc;
  ese.ese_fmvn = fmvn;
  EPDSetKings();
  return;
}

/*--> EPDGetCurrentPosition: return EPD structure for current position */
nonstatic epdptrT EPDGetCurrentPosition(void) {
  epdptrT epdptr;
  sqT sq;
  cpT cp0, cp1;

  epdptr = EPDNewEPD();
  for (sq = sq_a1; sq <= sq_h8; sq += 2) {
    cp0 = EPDboard.rbv[sq + 0];
    cp1 = EPDboard.rbv[sq + 1];
    epdptr->epd_nbv[sq >> 1] = ((cp1 << nybbW) | cp0);
  };
  epdptr->epd_actc = ese.ese_actc;
  epdptr->epd_cast = ese.ese_cast;
  epdptr->epd_epsq = ese.ese_epsq;
  return epdptr;
}

/*--> EPDFetchACTC: fetch current active color */
nonstatic cT EPDFetchACTC(void) {
/* return the value of the current active color */
  return ese.ese_actc;
}

/*--> EPDFetchCAST: fetch current castling availability */
nonstatic castT EPDFetchCAST(void) {
/* return the value of the current castling availability */
  return ese.ese_cast;
}

/*--> EPDFetchEPSQ: fetch current en passant target square */
nonstatic sqT EPDFetchEPSQ(void) {
/* return the value of the current en passant target square */
  return ese.ese_epsq;
}

/*--> EPDFetchHMVC: fetch current halfmove clock */
nonstatic siT EPDFetchHMVC(void) {
/* return the value of the current halfmove clock */
  return ese.ese_hmvc;
}

/*--> EPDFetchFMVN: fetch current fullmove number */
nonstatic siT EPDFetchFMVN(void) {
/* return the value of the current fullmove number */
  return ese.ese_fmvn;
}

/*--> EPDFetchBoard: fetch current board */
nonstatic rbptrT EPDFetchBoard(void) {
/* copy from the local board into the designated static return area */
  ret_rb = EPDboard;
  return &ret_rb;
}

/*--> EPDFetchCP: fetch color-piece */
nonstatic cpT EPDFetchCP(sqT sq) {
  cpT cp;

/* fetch from the local board */
  cp = EPDboard.rbv[sq];
  return cp;
}

/*--> EPDGetGTIM: get game termination marker indicator */
nonstatic gtimT EPDGetGTIM(gamptrT gamptr) {
  return gamptr->gam_gtim;
}

/*--> EPDPutGTIM: put game termination marker indicator */
nonstatic void EPDPutGTIM(gamptrT gamptr, gtimT gtim) {
  gamptr->gam_gtim = gtim;
  return;
}

/*--> EPDGenBasic: generate basic EPD notation for a given position */
nonstatic charptrT EPDGenBasic(rbptrT rbptr, cT actc, castT cast, sqT epsq) {
  charptrT ptr;
  epdptrT epdptr;

/* this does not reference the current position */
  epdptr = EPDSet(rbptr, actc, cast, epsq);
  ptr = EPDEncode(epdptr);
  EPDReleaseEPD(epdptr);
  return ptr;
}

/*--> EPDGenBasicCurrent: generate basic EPD for current position */
nonstatic charptrT EPDGenBasicCurrent(void) {
  charptrT ptr;

/* this references but does not change the current position */
  ptr = EPDGenBasic(&EPDboard, ese.ese_actc, ese.ese_cast, ese.ese_epsq);
  return ptr;
}

/*--> EPDDecode: read an EPD structure from a string */
nonstatic epdptrT EPDDecode(charptrT s) {
  epdptrT epdptr;
  eopptrT eopptr;
  eovptrT eovptr;
  siT flag, quoteflag;
  siT ch;
  liT i;
  siT j, d;
  byteptrT bptr;
  fileT file;
  rankT rank;
  sqT sq;
  cT c;
  pT p;
  cpT cp;

/* this does not reference the current position */
/* set up */
  flag = 1;
  i = 0;
  ch = *(s + i++);
/* initialize the return structure */
  epdptr = EPDNewEPD();
/* skip whitespace */
  if (flag) {
    while (flag && (ch != ascii_nul) && isspace(ch))
      ch = *(s + i++);
    if (ch == ascii_nul)
      flag = 0;
  };
/* process piece placement data */
  if (flag) {
    rank = rank_8;
    file = file_a;
    while (flag && (ch != ascii_nul) && !isspace(ch)) {
      switch (ch) {
        case '/':
          if ((file != fileL) || (rank == rank_1))
            flag = 0;
          else {
            rank--;
            file = file_a;
          };
          break;
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
          d = ch - '0';
          if ((file + d) > fileL)
            flag = 0;
          else
            for (j = 0; j < d; j++) {
              sq = map_sq(rank, file);
              bptr = &epdptr->epd_nbv[sq >> 1];
              if ((sq % 2) == 0) {
                *bptr &= ~nybbM;
                *bptr |= cp_v0;
              } else {
                *bptr &= ~(nybbM << nybbW);
                *bptr |= (cp_v0 << nybbW);
              };
              file++;
            };
          break;
        default:
          if (!EPDCheckPiece((char) ch) || (file >= fileL))
            flag = 0;
          else {
            p = EPDEvaluatePiece((char) ch);
            if (isupper(ch))
              c = c_w;
            else
              c = c_b;
            sq = map_sq(rank, file);
            bptr = &epdptr->epd_nbv[sq >> 1];
            cp = cv_cp_c_pv[c][p];
            if ((sq % 2) == 0) {
              *bptr &= ~nybbM;
              *bptr |= cp;
            } else {
              *bptr &= ~(nybbM << nybbW);
              *bptr |= (cp << nybbW);
            };
            file++;
          };
          break;
      };
      ch = *(s + i++);
    };
    if (flag)
      if ((file != fileL) || (rank != rank_1))
        flag = 0;
  };
/* need at least one whitespace character */
  if (flag)
    if ((ch == ascii_nul) || !isspace(ch))
      flag = 0;
/* skip whitespace */
  if (flag) {
    while (flag && (ch != ascii_nul) && isspace(ch))
      ch = *(s + i++);
    if (ch == ascii_nul)
      flag = 0;
  };
/* process active color */
  if (flag) {
    if (!EPDCheckColor((char) ch))
      flag = 0;
    else {
      epdptr->epd_actc = EPDEvaluateColor((char) ch);
      ch = *(s + i++);
    };
  };
/* need at least one whitespace character */
  if (flag)
    if ((ch == ascii_nul) || !isspace(ch))
      flag = 0;
/* skip whitespace */
  if (flag) {
    while (flag && (ch != ascii_nul) && isspace(ch))
      ch = *(s + i++);
    if (ch == ascii_nul)
      flag = 0;
  };
/* process castling availability */
  if (flag) {
    epdptr->epd_cast = 0;
    if (ch == '-')
      ch = *(s + i++);
    else {
/* white kingside castling availability */
      if (flag && (ch == map_upper(ascpv[p_k]))) {
        epdptr->epd_cast |= cf_wk;
        ch = *(s + i++);
        if (ch == ascii_nul)
          flag = 0;
      };
/* white queenside castling availability */
      if (flag && (ch == map_upper(ascpv[p_q]))) {
        epdptr->epd_cast |= cf_wq;
        ch = *(s + i++);
        if (ch == ascii_nul)
          flag = 0;
      };
/* black kingside castling availability */
      if (flag && (ch == map_lower(ascpv[p_k]))) {
        epdptr->epd_cast |= cf_bk;
        ch = *(s + i++);
        if (ch == ascii_nul)
          flag = 0;
      };
/* black queenside castling availability */
      if (flag && (ch == map_lower(ascpv[p_q]))) {
        epdptr->epd_cast |= cf_bq;
        ch = *(s + i++);
        if (ch == ascii_nul)
          flag = 0;
      };
    };
  };
/* need at least one whitespace character */
  if (flag)
    if ((ch == ascii_nul) || !isspace(ch))
      flag = 0;
/* skip whitespace */
  if (flag) {
    while (flag && (ch != ascii_nul) && isspace(ch))
      ch = *(s + i++);
    if (ch == ascii_nul)
      flag = 0;
  };
/* process en passant target */
  if (flag) {
    if (ch == '-') {
      epdptr->epd_epsq = sq_nil;
      ch = *(s + i++);
      if ((ch != ascii_nul) && !isspace(ch))
        flag = 0;
    } else {
      if (!EPDCheckFile((char) ch))
        flag = 0;
      else {
        file = EPDEvaluateFile((char) ch);
        ch = *(s + i++);
        if ((ch == ascii_nul) || !EPDCheckRank((char) ch))
          flag = 0;
        else {
          epdptr->epd_epsq = map_sq(EPDEvaluateRank((char) ch), file);
          ch = *(s + i++);
          if ((ch != ascii_nul) && !isspace(ch))
            flag = 0;
        };
      };
    }
  }
/* skip whitespace (end-of-line is not an error) */
  if (flag)
    while ((ch != ascii_nul) && isspace(ch))
      ch = *(s + i++);
/* process operation sequence (if any) */
  if (flag) {
    while (flag && (ch != ascii_nul)) {
/* allocate a new operation */
      eopptr = EPDNewEOP();
/* form opsym (first character) */
      if (IdentChar(ch)) {
        eopptr->eop_opsym = EPDStringGrab("");
        eopptr->eop_opsym = EPDStringAppendChar(eopptr->eop_opsym, (char) ch);
        ch = *(s + i++);
      } else
        flag = 0;
/* form remainder of opsym */
      while (IdentChar(ch)) {
        eopptr->eop_opsym = EPDStringAppendChar(eopptr->eop_opsym, (char) ch);
        ch = *(s + i++);
      };
/* skip whitespace */
      if (flag) {
        while (flag && (ch != ascii_nul) && isspace(ch))
          ch = *(s + i++);
        if (ch == ascii_nul)
          flag = 0;
      };
/* process operand list */
      while (flag && (ch != ';')) {
/* allocate operand value */
        eovptr = EPDNewEOV();
/* set quoted string as appropriate */
        if (ch == '"') {
          quoteflag = 1;
          eovptr->eov_eob = eob_string;
          ch = *(s + i++);
        } else {
          quoteflag = 0;
          eovptr->eov_eob = eob_symbol;
        };
        eovptr->eov_str = EPDStringGrab("");
        if (quoteflag) {
          while (flag && (ch != '"')) {
            if (ch == ascii_nul)
              flag = 0;
            else {
              eovptr->eov_str =
                  EPDStringAppendChar(eovptr->eov_str, (char) ch);
              ch = *(s + i++);
            };
          };
          if (ch == '"')
            ch = *(s + i++);
        } else {
          while (flag && !isspace(ch) && (ch != ';')) {
            if (ch == ascii_nul)
              flag = 0;
            else {
              eovptr->eov_str =
                  EPDStringAppendChar(eovptr->eov_str, (char) ch);
              ch = *(s + i++);
            };
          };
        };
/* append operand onto operation */
        if (flag)
          EPDAppendEOV(eopptr, eovptr);
        else
          EPDReleaseEOV(eovptr);
/* skip whitespace */
        while (flag && (ch != ascii_nul) && isspace(ch))
          ch = *(s + i++);
      };
/* process semicolon */
      if (flag) {
        if (ch == ';')
          ch = *(s + i++);
        else
          flag = 0;
      }
/* append operation */
      if (flag)
        EPDAppendEOP(epdptr, eopptr);
      else
        EPDReleaseEOP(eopptr);
/* skip whitespace (end-of-line is not an error) */
      if (flag)
        while (flag && (ch != ascii_nul) && isspace(ch))
          ch = *(s + i++);
    };
  };
/* check for fault */
  if (!flag) {
    EPDReleaseEPD(epdptr);
    epdptr = NULL;
  };
/* normalize */
  if (epdptr != NULL)
    EPDNormalize(epdptr);
  return epdptr;
}

/*--> EPDEncode: write an EPD structure to a string */
nonstatic charptrT EPDEncode(epdptrT epdptr) {
  charptrT ptr;
  sqT sq;
  cpT cp;
  rankT rank;
  fileT file;
  siT bi, ps, ch;
  char bv[tL];
  eopptrT eopptr;
  eovptrT eovptr;
  charptrT s0, s1;

/* this does not reference the current position */
  bi = 0;
/* normalize */
  EPDNormalize(epdptr);
/* output board */
  for (rank = rank_8; rank >= rank_1; rank--) {
    ps = 0;
    for (file = file_a; file <= file_h; file++) {
      sq = map_sq(rank, file);
      if ((sq % 2) == 0)
        cp = (epdptr->epd_nbv[sq >> 1] & nybbM);
      else
        cp = ((epdptr->epd_nbv[sq >> 1] >> nybbW) & nybbM);
      if (cp == cp_v0)
        ps++;
      else {
        if (ps != 0) {
          bv[bi++] = '0' + ps;
          ps = 0;
        };
        ch = ascpv[cv_p_cpv[cp]];
        if (cv_c_cpv[cp] == c_w)
          ch = map_upper(ch);
        else
          ch = map_lower(ch);
        bv[bi++] = ch;
      };
    };
    if (ps != 0) {
      bv[bi++] = '0' + ps;
      ps = 0;
    };
    if (rank != rank_1)
      bv[bi++] = '/';
  };
  bv[bi++] = ascii_sp;
/* output active color (lower case) */
  bv[bi++] = map_lower(asccv[epdptr->epd_actc]);
  bv[bi++] = ascii_sp;
/* output castling availablility */
  if (epdptr->epd_cast == 0)
    bv[bi++] = '-';
  else {
    if (epdptr->epd_cast & cf_wk)
      bv[bi++] = map_upper(ascpv[p_k]);
    if (epdptr->epd_cast & cf_wq)
      bv[bi++] = map_upper(ascpv[p_q]);
    if (epdptr->epd_cast & cf_bk)
      bv[bi++] = map_lower(ascpv[p_k]);
    if (epdptr->epd_cast & cf_bq)
      bv[bi++] = map_lower(ascpv[p_q]);
  };
  bv[bi++] = ascii_sp;
/* output ep capture square */
  if (epdptr->epd_epsq == sq_nil)
    bv[bi++] = '-';
  else {
    bv[bi++] = ascfv[map_file(epdptr->epd_epsq)];
    bv[bi++] = ascrv[map_rank(epdptr->epd_epsq)];
  };
/* NUL termination */
  bv[bi++] = ascii_nul;
/* allocate and copy basic result */
  ptr = EPDStringGrab(bv);
/* construct and append operations */
  eopptr = epdptr->epd_headeop;
  while (eopptr != NULL) {
/* leading space */
    s0 = EPDStringGrab(" ");
/* opcode */
    s0 = EPDStringAppendStr(s0, eopptr->eop_opsym);
/* construct and append operands */
    eovptr = eopptr->eop_headeov;
    while (eovptr != NULL) {
/* leading space */
      s1 = EPDStringGrab(" ");
/* conjure operand value */
      switch (eovptr->eov_eob) {
        case eob_string:
          s1 = EPDStringAppendChar(s1, '"');
          s1 = EPDStringAppendStr(s1, eovptr->eov_str);
          s1 = EPDStringAppendChar(s1, '"');
          break;
        case eob_symbol:
          s1 = EPDStringAppendStr(s1, eovptr->eov_str);
          break;
        default:
          EPDSwitchFault("EPDEncode");
          break;
      };
/* append */
      s0 = EPDStringAppendStr(s0, s1);
      EPDMemoryFree(s1);
/* next operand */
      eovptr = eovptr->eov_next;
    };
/* trailing semicolon */
    s0 = EPDStringAppendChar(s0, ';');
/* append operation */
    ptr = EPDStringAppendStr(ptr, s0);
    EPDMemoryFree(s0);
/* advance */
    eopptr = eopptr->eop_next;
  };
  return ptr;
}

/*--> EPDRealize: set the current position according to EPD */
nonstatic void EPDRealize(epdptrT epdptr) {
  sqT sq;
  cpT cp;
  eopptrT eopptr;
  eovptrT eovptr;

/* this changes the current position */
  for (sq = sq_a1; sq <= sq_h8; sq++) {
    if ((sq % 2) == 0)
      cp = (epdptr->epd_nbv[sq >> 1] & nybbM);
    else
      cp = ((epdptr->epd_nbv[sq >> 1] >> nybbW) & nybbM);
    EPDboard.rbv[sq] = cp;
  };
  ese.ese_actc = epdptr->epd_actc;
  ese.ese_cast = epdptr->epd_cast;
  ese.ese_epsq = epdptr->epd_epsq;
  eopptr = EPDLocateEOPCode(epdptr, epdso_hmvc);
  if ((eopptr != NULL) && ((eovptr = eopptr->eop_headeov) != NULL))
    ese.ese_hmvc = atoi(eovptr->eov_str);
  else
    ese.ese_hmvc = 0;
  eopptr = EPDLocateEOPCode(epdptr, epdso_fmvn);
  if ((eopptr != NULL) && ((eovptr = eopptr->eop_headeov) != NULL))
    ese.ese_fmvn = atoi(eovptr->eov_str);
  else
    ese.ese_fmvn = 1;
  EPDSetKings();
  return;
}

/*--> EPDInitArray: set the current position to the initial array */
nonstatic void EPDInitArray(void) {
  sqT sq;

/* this changes the current position */
  for (sq = sq_a1; sq <= sq_h8; sq++)
    EPDboard.rbv[sq] = cp_v0;
  EPDboard.rbv[sq_a1] = EPDboard.rbv[sq_h1] = cp_wr;
  EPDboard.rbv[sq_b1] = EPDboard.rbv[sq_g1] = cp_wn;
  EPDboard.rbv[sq_c1] = EPDboard.rbv[sq_f1] = cp_wb;
  EPDboard.rbv[sq_d1] = cp_wq;
  EPDboard.rbv[sq_e1] = cp_wk;
  for (sq = sq_a2; sq <= sq_h2; sq++)
    EPDboard.rbv[sq] = cp_wp;
  EPDboard.rbv[sq_a8] = EPDboard.rbv[sq_h8] = cp_br;
  EPDboard.rbv[sq_b8] = EPDboard.rbv[sq_g8] = cp_bn;
  EPDboard.rbv[sq_c8] = EPDboard.rbv[sq_f8] = cp_bb;
  EPDboard.rbv[sq_d8] = cp_bq;
  EPDboard.rbv[sq_e8] = cp_bk;
  for (sq = sq_a7; sq <= sq_h7; sq++)
    EPDboard.rbv[sq] = cp_bp;
  ese.ese_actc = c_w;
  ese.ese_cast = cf_wk | cf_wq | cf_bk | cf_bq;
  ese.ese_epsq = sq_nil;
  ese.ese_hmvc = 0;
  ese.ese_fmvn = 1;
  EPDSetKings();
  return;
}

/*--> EPDSANEncodeChar: encode SAN character */
static
void EPDSANEncodeChar(char ch) {
  if ((lsani < (sanL - 1)) || ((ch == '\0') && (lsani < sanL)))
    lsan[lsani++] = ch;
  else
    EPDFatal("EPDSANEncodeChar: overflow");
  return;
}

/*--> EPDSANEncodeStr: encode a SAN string */
static
void EPDSANEncodeStr(charptrT s) {
  charptrT p;

  p = s;
  while (*p)
    EPDSANEncodeChar(*p++);
  return;
}

/*--> EPDSANEncodeFile: encode SAN file from square */
static
void EPDSANEncodeFile(sqT sq) {
  EPDSANEncodeChar(ascfv[map_file(sq)]);
  return;
}

/*--> EPDSANEncodeRank: encode SAN rank from square */
static
void EPDSANEncodeRank(sqT sq) {
  EPDSANEncodeChar(ascrv[map_rank(sq)]);
  return;
}

/*--> EPDSANEncodeSq: encode SAN square */
static
void EPDSANEncodeSq(sqT sq) {
  EPDSANEncodeFile(sq);
  EPDSANEncodeRank(sq);
  return;
}

/*--> EPDSANEncodeCI: encode an appropriate capture indicator */
static
void EPDSANEncodeCI(siT index) {
  switch (index) {
    case 0:
      EPDSANEncodeChar('x');
      break;
    case 1:
      break;
    case 2:
      EPDSANEncodeChar(':');
      break;
    case 3:
      EPDSANEncodeChar('*');
      break;
    case 4:
      EPDSANEncodeChar('-');
      break;
  };
  return;
}

/*--> EPDSANEncodeAux: encode SAN format move with variants */
static
void EPDSANEncodeAux(mptrT mptr, sanT san, ssavT ssav) {
  siT i;

/* reset local index */
  lsani = 0;
/* busted? */
  if (mptr->m_flag & mf_bust)
    EPDSANEncodeChar('*');
/* process according to moving piece */
  switch (cv_p_cpv[mptr->m_frcp]) {
    case p_p:
      switch (mptr->m_scmv) {
        case scmv_reg:
          if (mptr->m_tocp != cp_v0) {
            EPDSANEncodeFile(mptr->m_frsq);
            if (ssav[ssa_edcr] == 1)
              EPDSANEncodeRank(mptr->m_frsq);
            EPDSANEncodeCI(ssav[ssa_capt]);
            if (ssav[ssa_ptar] == 0)
              EPDSANEncodeSq(mptr->m_tosq);
            else
              EPDSANEncodeFile(mptr->m_tosq);
          } else {
            EPDSANEncodeFile(mptr->m_frsq);
            if (ssav[ssa_edcr] == 1)
              EPDSANEncodeRank(mptr->m_frsq);
            if (ssav[ssa_move] == 1)
              EPDSANEncodeChar('-');
            if (ssav[ssa_edcf] == 1)
              EPDSANEncodeFile(mptr->m_tosq);
            EPDSANEncodeRank(mptr->m_tosq);
          };
          break;
        case scmv_epc:
          EPDSANEncodeFile(mptr->m_frsq);
          if (ssav[ssa_edcr] == 1)
            EPDSANEncodeRank(mptr->m_frsq);
          EPDSANEncodeCI(ssav[ssa_capt]);
          if (ssav[ssa_ptar] == 0)
            EPDSANEncodeSq(mptr->m_tosq);
          else
            EPDSANEncodeFile(mptr->m_tosq);
          if (ssav[ssa_epct] == 1)
            EPDSANEncodeStr("ep");
          break;
        case scmv_ppn:
        case scmv_ppb:
        case scmv_ppr:
        case scmv_ppq:
          if (mptr->m_tocp != cp_v0) {
            EPDSANEncodeFile(mptr->m_frsq);
            if (ssav[ssa_edcr] == 1)
              EPDSANEncodeRank(mptr->m_frsq);
            EPDSANEncodeCI(ssav[ssa_capt]);
            if (ssav[ssa_ptar] == 0)
              EPDSANEncodeSq(mptr->m_tosq);
            else
              EPDSANEncodeFile(mptr->m_tosq);
          } else {
            EPDSANEncodeFile(mptr->m_frsq);
            if (ssav[ssa_edcr] == 1)
              EPDSANEncodeRank(mptr->m_frsq);
            if (ssav[ssa_move] == 1)
              EPDSANEncodeChar('-');
            if (ssav[ssa_edcf] == 1)
              EPDSANEncodeFile(mptr->m_tosq);
            EPDSANEncodeRank(mptr->m_tosq);
          };
          switch (ssav[ssa_prom]) {
            case 0:
              EPDSANEncodeChar('=');
              EPDSANEncodeChar(ascpv[cv_p_scmvv[mptr->m_scmv]]);
              break;
            case 1:
              EPDSANEncodeChar(ascpv[cv_p_scmvv[mptr->m_scmv]]);
              break;
            case 2:
              EPDSANEncodeChar('/');
              EPDSANEncodeChar(ascpv[cv_p_scmvv[mptr->m_scmv]]);
              break;
            case 3:
              EPDSANEncodeChar('(');
              EPDSANEncodeChar(ascpv[cv_p_scmvv[mptr->m_scmv]]);
              EPDSANEncodeChar(')');
              break;
          };
          break;
      };
      break;
    case p_n:
    case p_b:
    case p_r:
    case p_q:
      EPDSANEncodeChar(ascpv[cv_p_cpv[mptr->m_frcp]]);
      if (((mptr->m_flag & mf_sanf) || (ssav[ssa_edcf] == 1))
          || ((mptr->m_flag & mf_sanr) && (ssav[ssa_edcf] == 2)))
        EPDSANEncodeFile(mptr->m_frsq);
      if (((mptr->m_flag & mf_sanr) || (ssav[ssa_edcr] == 1))
          || ((mptr->m_flag & mf_sanf) && (ssav[ssa_edcr] == 2)))
        EPDSANEncodeRank(mptr->m_frsq);
      if (mptr->m_tocp != cp_v0)
        EPDSANEncodeCI(ssav[ssa_capt]);
      else if (ssav[ssa_move] == 1)
        EPDSANEncodeChar('-');
      EPDSANEncodeSq(mptr->m_tosq);
      break;
    case p_k:
      switch (mptr->m_scmv) {
        case scmv_reg:
          EPDSANEncodeChar(ascpv[p_k]);
          if (ssav[ssa_edcf] == 1)
            EPDSANEncodeFile(mptr->m_frsq);
          if (ssav[ssa_edcr] == 1)
            EPDSANEncodeRank(mptr->m_frsq);
          if (mptr->m_tocp != cp_v0)
            EPDSANEncodeCI(ssav[ssa_capt]);
          else if (ssav[ssa_move] == 1)
            EPDSANEncodeChar('-');
          EPDSANEncodeSq(mptr->m_tosq);
          break;
        case scmv_cks:
          switch (ssav[ssa_cast]) {
            case 0:
              EPDSANEncodeStr("O-O");
              break;
            case 1:
              EPDSANEncodeStr("0-0");
              break;
            case 2:
              EPDSANEncodeStr("OO");
              break;
            case 3:
              EPDSANEncodeStr("00");
              break;
            case 4:
              EPDSANEncodeChar(ascpv[p_k]);
              if (ssav[ssa_edcf] == 1)
                EPDSANEncodeFile(mptr->m_frsq);
              if (ssav[ssa_edcr] == 1)
                EPDSANEncodeRank(mptr->m_frsq);
              if (ssav[ssa_move] == 1)
                EPDSANEncodeChar('-');
              EPDSANEncodeSq(mptr->m_tosq);
              break;
          };
          break;
        case scmv_cqs:
          switch (ssav[ssa_cast]) {
            case 0:
              EPDSANEncodeStr("O-O-O");
              break;
            case 1:
              EPDSANEncodeStr("0-0-0");
              break;
            case 2:
              EPDSANEncodeStr("OOO");
              break;
            case 3:
              EPDSANEncodeStr("000");
              break;
            case 4:
              EPDSANEncodeChar(ascpv[p_k]);
              if (ssav[ssa_edcf] == 1)
                EPDSANEncodeFile(mptr->m_frsq);
              if (ssav[ssa_edcr] == 1)
                EPDSANEncodeRank(mptr->m_frsq);
              if (ssav[ssa_move] == 1)
                EPDSANEncodeChar('-');
              EPDSANEncodeSq(mptr->m_tosq);
              break;
          };
          break;
      };
      break;
  };
/* insert markers */
  if ((mptr->m_flag & mf_chec) && !(mptr->m_flag & mf_chmt))
    switch (ssav[ssa_chec]) {
      case 0:
        EPDSANEncodeChar('+');
        break;
      case 1:
        break;
      case 2:
        EPDSANEncodeStr("ch");
        break;
    };
  if (mptr->m_flag & mf_chmt)
    switch (ssav[ssa_chmt]) {
      case 0:
        EPDSANEncodeChar('#');
        break;
      case 1:
        break;
      case 2:
        EPDSANEncodeChar('+');
        break;
      case 3:
        EPDSANEncodeStr("++");
        break;
    };
  if (mptr->m_flag & mf_draw)
    if (ssav[ssa_draw] == 1)
      EPDSANEncodeChar('=');
/* map to lower case if indicated */
  if (ssav[ssa_case] == 1)
    for (i = 0; i < lsani; i++)
      lsan[i] = map_lower(lsan[i]);
/* pad and copy */
  while (lsani < sanL)
    EPDSANEncodeChar('\0');
  for (i = 0; i < sanL; i++)
    san[i] = lsan[i];
  return;
}

/*--> EPDSANEncode: encode a move into a SAN string */
nonstatic void EPDSANEncode(mptrT mptr, sanT san) {
  ssaT ssa;
  ssavT ssav;

/* select canonical encoding (zero point in variant space) */
  for (ssa = 0; ssa < ssaL; ssa++)
    ssav[ssa] = 0;
  EPDSANEncodeAux(mptr, san, ssav);
  return;
}

/*--> EPDSANDecodeBump: increment a style vector and return overflow */
static siT EPDSANDecodeBump(ssavT ssav, ssavT bssav) {
  siT flag;
  ssaT ssa;

  flag = 1;
  ssa = 0;
  while (flag && (ssa < ssaL)) {
    flag = 0;
    ssav[ssa]++;
    if (ssav[ssa] == bssav[ssa]) {
      flag = 1;
      ssav[ssa] = 0;
    };
    ssa++;
  };
  return flag;
}

/*--> EPDSANDecodeFlex: locate a move from SAN (flexible interpretation) */
static mptrT EPDSANDecodeFlex(sanT san) {
  mptrT mptr;
  ssavT ssav, bssav;
  siT i, flag;
  mptrT rmptr;
  sanT lcsan, rsan;

/* set default return value */
  mptr = NULL;
/* set minimal upper bounds */
  for (i = 0; i < ssaL; i++)
    bssav[i] = 1;
/* scan for upper bound conditions */
  rmptr = tse.tse_base;
  for (i = 0; i < tse.tse_count; i++) {
/* letter case */
    bssav[ssa_case] = 2;
/* capturing */
    if ((rmptr->m_tocp != cp_v0) || (rmptr->m_scmv == scmv_epc))
      bssav[ssa_capt] = 5;
/* checking */
    if (rmptr->m_flag & mf_chec)
      bssav[ssa_chec] = 3;
/* castling */
    if ((rmptr->m_scmv == scmv_cks) || (rmptr->m_scmv == scmv_cqs))
      bssav[ssa_cast] = 5;
/* promoting */
    if ((rmptr->m_scmv == scmv_ppn) || (rmptr->m_scmv == scmv_ppb)
        || (rmptr->m_scmv == scmv_ppr) || (rmptr->m_scmv == scmv_ppq))
      bssav[ssa_prom] = 4;
/* pawn destination target */
    if (cv_p_cpv[rmptr->m_frcp] == p_p)
      bssav[ssa_ptar] = 2;
/* checkmating */
    if (rmptr->m_flag & mf_chmt)
      bssav[ssa_chmt] = 4;
/* en passant capturing */
    if (rmptr->m_scmv == scmv_epc)
      bssav[ssa_epct] = 2;
/* drawing */
    if (rmptr->m_flag & mf_draw)
      bssav[ssa_draw] = 2;
/* moving (non-capturing) */
    if ((rmptr->m_tocp == cp_v0) && (rmptr->m_scmv != scmv_epc))
      bssav[ssa_move] = 2;
/* extra disambiguation: file */
    if (!(rmptr->m_flag & mf_sanf))
      bssav[ssa_edcf] = 3;
/* extra disambiguation: rank */
    if (!(rmptr->m_flag & mf_sanr))
      bssav[ssa_edcr] = 3;
    rmptr++;
  };
/* make a lower case copy of the input */
  for (i = 0; i < sanL; i++)
    lcsan[i] = map_lower(san[i]);
/* initialize the index style vector */
  for (i = 0; i < ssaL; i++)
    ssav[i] = 0;
/* search */
  flag = 0;
  while (!flag && (mptr == NULL)) {
    rmptr = tse.tse_base;
    i = 0;
/* scan candidate moves */
    while ((mptr == NULL) && (i < tse.tse_count)) {
/* encode the current style version of a candidate */
      EPDSANEncodeAux(rmptr, rsan, ssav);
/* select either original or lower case comparison */
      if (ssav[ssa_case] == 0) {
        if (strcmp(san, rsan) == 0)
          mptr = rmptr;
      } else {
        if (strcmp(lcsan, rsan) == 0)
          mptr = rmptr;
      };
/* next candidate */
      rmptr++;
      i++;
    };
/* update the overflow termination flag */
    flag = EPDSANDecodeBump(ssav, bssav);
  };
  return mptr;
}

/*--> EPDSANDecode: locate a move from SAN (strict interpretation) */
static mptrT EPDSANDecode(sanT san) {
  mptrT mptr;
  mptrT rmptr;
  sanT rsan;
  siT i;

/* set default return value */
  mptr = NULL;
/* assume current moveset properly generated */
  rmptr = tse.tse_base;
  i = 0;
/* search */
  while ((mptr == NULL) && (i < tse.tse_count)) {
    EPDSANEncode(rmptr, rsan);
    if (strcmp(san, rsan) == 0)
      mptr = rmptr;
    else {
      rmptr++;
      i++;
    };
  };
  return mptr;
}

/*--> EPDSANDecodeAux: locate a move from SAN */
nonstatic mptrT EPDSANDecodeAux(sanT san, siT strict) {
  mptrT mptr;

  if (strict)
    mptr = EPDSANDecode(san);
  else
    mptr = EPDSANDecodeFlex(san);
  return mptr;
}

/*--> EPDAttack: determine if a color attacks a square */
static siT EPDAttack(cT c, sqT sq) {
  siT flag;
  dxT dx;
  dvT dv;
  xdvT xdv;
  sqptrT sqptr0, sqptr1;
  xsqptrT xsqptr0, xsqptr1;

/* clear result */
  flag = 0;
/* set origin square pointers  */
  sqptr0 = &EPDboard.rbv[sq];
  xsqptr0 = &xb.xbv[map_xsq_sq(sq)];
/* process according to specified color */
  if (c == c_w) {
/* pawn attacks */
    if ((*(xsqptr0 + xdv_7) == cp_v0) && (*(sqptr0 + dv_7) == cp_wp))
      flag = 1;
    else if ((*(xsqptr0 + xdv_6) == cp_v0) && (*(sqptr0 + dv_6) == cp_wp))
      flag = 1;
/* knight attacks */
    if (!flag) {
      dx = dx_8;
      while (!flag && (dx <= dx_f))
        if ((*(xsqptr0 + xdvv[dx]) == cp_v0)
            && (*(sqptr0 + dvv[dx]) == cp_wn))
          flag = 1;
        else
          dx++;
    };
/* orthogonal sweeps */
    if (!flag) {
      dx = dx_0;
      while (!flag && (dx <= dx_3)) {
        dv = dvv[dx];
        xdv = xdvv[dx];
        sqptr1 = sqptr0;
        xsqptr1 = xsqptr0;
        while ((*(xsqptr1 += xdv) == cp_v0) && (*(sqptr1 += dv) == cp_v0));
        if ((*xsqptr1 == cp_v0) && ((*sqptr1 == cp_wq) || (*sqptr1 == cp_wr)))
          flag = 1;
        else
          dx++;
      };
    };
/* diagonal sweeps */
    if (!flag) {
      dx = dx_4;
      while (!flag && (dx <= dx_7)) {
        dv = dvv[dx];
        xdv = xdvv[dx];
        sqptr1 = sqptr0;
        xsqptr1 = xsqptr0;
        while ((*(xsqptr1 += xdv) == cp_v0) && (*(sqptr1 += dv) == cp_v0));
        if ((*xsqptr1 == cp_v0) && ((*sqptr1 == cp_wq) || (*sqptr1 == cp_wb)))
          flag = 1;
        else
          dx++;
      };
    };
/* king attacks */
    if (!flag) {
      dx = dx_0;
      while (!flag && (dx <= dx_7))
        if ((*(xsqptr0 + xdvv[dx]) == cp_v0)
            && (*(sqptr0 + dvv[dx]) == cp_wk))
          flag = 1;
        else
          dx++;
    };
  } else {
/* pawn attacks */
    if ((*(xsqptr0 + xdv_4) == cp_v0) && (*(sqptr0 + dv_4) == cp_bp))
      flag = 1;
    else if ((*(xsqptr0 + xdv_5) == cp_v0) && (*(sqptr0 + dv_5) == cp_bp))
      flag = 1;
/* knight attacks */
    if (!flag) {
      dx = dx_8;
      while (!flag && (dx <= dx_f))
        if ((*(xsqptr0 + xdvv[dx]) == cp_v0)
            && (*(sqptr0 + dvv[dx]) == cp_bn))
          flag = 1;
        else
          dx++;
    };
/* orthogonal sweeps */
    if (!flag) {
      dx = dx_0;
      while (!flag && (dx <= dx_3)) {
        dv = dvv[dx];
        xdv = xdvv[dx];
        sqptr1 = sqptr0;
        xsqptr1 = xsqptr0;
        while ((*(xsqptr1 += xdv) == cp_v0) && (*(sqptr1 += dv) == cp_v0));
        if ((*xsqptr1 == cp_v0) && ((*sqptr1 == cp_bq) || (*sqptr1 == cp_br)))
          flag = 1;
        else
          dx++;
      };
    };
/* diagonal sweeps */
    if (!flag) {
      dx = dx_4;
      while (!flag && (dx <= dx_7)) {
        dv = dvv[dx];
        xdv = xdvv[dx];
        sqptr1 = sqptr0;
        xsqptr1 = xsqptr0;
        while ((*(xsqptr1 += xdv) == cp_v0) && (*(sqptr1 += dv) == cp_v0));
        if ((*xsqptr1 == cp_v0) && ((*sqptr1 == cp_bq) || (*sqptr1 == cp_bb)))
          flag = 1;
        else
          dx++;
      };
    };
/* king attacks */
    if (!flag) {
      dx = dx_0;
      while (!flag && (dx <= dx_7))
        if ((*(xsqptr0 + xdvv[dx]) == cp_v0)
            && (*(sqptr0 + dvv[dx]) == cp_bk))
          flag = 1;
        else
          dx++;
    };
  };
  return flag;
}

/*--> EPDWhiteAttacks: check if White attacks a square */
static siT EPDWhiteAttacks(sqT sq) {
  return EPDAttack(c_w, sq);
}

/*--> EPDBlackAttacks: check if White attacks a square */
static siT EPDBlackAttacks(sqT sq) {
  return EPDAttack(c_b, sq);
}

/*--> EPDTestAKIC: test for active king in check */
static siT EPDTestAKIC(void) {
  siT flag;

  if (ese.ese_actc == c_w)
    flag = EPDBlackAttacks(ese.ese_ksqv[c_w]);
  else
    flag = EPDWhiteAttacks(ese.ese_ksqv[c_b]);
  return flag;
}

/*--> EPDTestPKIC: test for passive king in check */
static siT EPDTestPKIC(void) {
  siT flag;

  if (ese.ese_actc == c_b)
    flag = EPDBlackAttacks(ese.ese_ksqv[c_w]);
  else
    flag = EPDWhiteAttacks(ese.ese_ksqv[c_b]);
  return flag;
}

/*--> EPDCensus: calculate local census vectors */
static
void EPDCensus(void) {
  cT c;
  pT p;
  sqT sq;
  cpT cp;

/* clear census vectors */
  for (c = c_w; c <= c_b; c++) {
    count_cv[c] = 0;
    for (p = p_p; p <= p_k; p++)
      count_cpv[c][p] = 0;
  };
/* calculate census vectors */
  for (sq = sq_a1; sq <= sq_h8; sq++) {
    cp = EPDboard.rbv[sq];
    if (cp != cp_v0) {
      c = cv_c_cpv[cp];
      count_cv[c]++;
      count_cpv[c][cv_p_cpv[cp]]++;
    };
  };
  return;
}

/*--> EPDIsLegal: determine if current position is legal */
nonstatic siT EPDIsLegal(void) {
  siT flag;
  cT c;
  fileT file;
  siT apv[rcL];

/* set default return value: legal position */
  flag = 1;
/* calculate the local census vectors */
  EPDCensus();
/* calculate available promoted pawns */
  for (c = c_w; c <= c_b; c++)
    apv[c] = fileL - count_cpv[c][p_p];
/* check white pawn count */
  if (flag && (count_cpv[c_w][p_p] > fileL))
    flag = 0;
/* check black pawn count */
  if (flag && (count_cpv[c_b][p_p] > fileL))
    flag = 0;
/* check white knight count */
  if (flag && (count_cpv[c_w][p_n] > 2)) {
    apv[c_w] -= (count_cpv[c_w][p_n] - 2);
    if (apv[c_w] < 0)
      flag = 0;
  };
/* check black knight count */
  if (flag && (count_cpv[c_b][p_n] > 2)) {
    apv[c_b] -= (count_cpv[c_b][p_n] - 2);
    if (apv[c_b] < 0)
      flag = 0;
  };
/* check white bishop count */
  if (flag && (count_cpv[c_w][p_b] > 2)) {
    apv[c_w] -= (count_cpv[c_w][p_b] - 2);
    if (apv[c_w] < 0)
      flag = 0;
  };
/* check black bishop count */
  if (flag && (count_cpv[c_b][p_b] > 2)) {
    apv[c_b] -= (count_cpv[c_b][p_b] - 2);
    if (apv[c_b] < 0)
      flag = 0;
  };
/* check white rook count */
  if (flag && (count_cpv[c_w][p_r] > 2)) {
    apv[c_w] -= (count_cpv[c_w][p_r] - 2);
    if (apv[c_w] < 0)
      flag = 0;
  };
/* check black rook count */
  if (flag && (count_cpv[c_b][p_r] > 2)) {
    apv[c_b] -= (count_cpv[c_b][p_r] - 2);
    if (apv[c_b] < 0)
      flag = 0;
  };
/* check white queen count */
  if (flag && (count_cpv[c_w][p_q] > 1)) {
    apv[c_w] -= (count_cpv[c_w][p_q] - 1);
    if (apv[c_w] < 0)
      flag = 0;
  };
/* check black queen count */
  if (flag && (count_cpv[c_b][p_q] > 1)) {
    apv[c_b] -= (count_cpv[c_b][p_q] - 1);
    if (apv[c_b] < 0)
      flag = 0;
  };
/* check white king count */
  if (flag && (count_cpv[c_w][p_k] != 1))
    flag = 0;
/* check black king count */
  if (flag && (count_cpv[c_b][p_k] != 1))
    flag = 0;
/* check pawn placement */
  if (flag) {
    file = file_a;
    while (flag && (file <= file_h))
      if ((EPDboard.rbm[rank_1][file] == cp_wp)
          || (EPDboard.rbm[rank_1][file] == cp_bp)
          || (EPDboard.rbm[rank_8][file] == cp_wp)
          || (EPDboard.rbm[rank_8][file] == cp_bp))
        flag = 0;
      else
        file++;
  };
/* check white kingside castling availability */
  if (flag && (ese.ese_cast & cf_wk))
    if ((EPDboard.rbv[sq_e1] != cp_wk) || (EPDboard.rbv[sq_h1] != cp_wr))
      flag = 0;
/* check white queenside castling availability */
  if (flag && (ese.ese_cast & cf_wq))
    if ((EPDboard.rbv[sq_e1] != cp_wk) || (EPDboard.rbv[sq_a1] != cp_wr))
      flag = 0;
/* check black kingside castling availability */
  if (flag && (ese.ese_cast & cf_bk))
    if ((EPDboard.rbv[sq_e8] != cp_bk) || (EPDboard.rbv[sq_h8] != cp_br))
      flag = 0;
/* check black queenside castling availability */
  if (flag && (ese.ese_cast & cf_bq))
    if ((EPDboard.rbv[sq_e8] != cp_bk) || (EPDboard.rbv[sq_a8] != cp_br))
      flag = 0;
/* check en passant target square */
  if (flag && (ese.ese_epsq != sq_nil)) {
    if (ese.ese_actc == c_w) {
      if (map_rank(ese.ese_epsq) != rank_6)
        flag = 0;
      else if (EPDboard.rbv[ese.ese_epsq + dv_3] != cp_bp)
        flag = 0;
      else if (EPDboard.rbv[ese.ese_epsq] != cp_v0)
        flag = 0;
      else if (EPDboard.rbv[ese.ese_epsq + dv_1] != cp_v0)
        flag = 0;
    } else {
      if (map_rank(ese.ese_epsq) != rank_3)
        flag = 0;
      else if (EPDboard.rbv[ese.ese_epsq + dv_1] != cp_wp)
        flag = 0;
      else if (EPDboard.rbv[ese.ese_epsq] != cp_v0)
        flag = 0;
      else if (EPDboard.rbv[ese.ese_epsq + dv_3] != cp_v0)
        flag = 0;
    }
  }
/* check for passive king in check */
  if (flag && EPDTestPKIC())
    flag = 0;
  return flag;
}

/*--> EPDGeneratePL: generate psuedolegal moves */
static
void EPDGeneratePL(void) {
  dxT dx;
  dvT dv;
  xdvT xdv;
  xsqptrT xsqptr0, xsqptr1;
  fileT frfile;
  rankT frrank;
  mT gen_m;

/* set up the generation base */
  if (ply == 0)
    treeptr = tse.tse_base = treebaseptr;
  else
    treeptr = tse.tse_base = (tseptr - 1)->tse_base + (tseptr - 1)->tse_count;
/* test against safety margin */
  if ((treeptr - treebaseptr) >= (treeL - treemarginL))
    EPDFatal("EPDGeneratePL: move tree size safety limit exceeded");
/* set up current generation items */
  tse.tse_curr = treeptr;
  tse.tse_count = 0;
/* set the psuedoinvariant generated move template components */
  gen_m.m_scmv = scmv_reg;
  gen_m.m_flag = 0;
/* look at each origin square of the active color */
  for (gen_m.m_frsq = sq_a1; gen_m.m_frsq <= sq_h8; gen_m.m_frsq++) {
/* get origin square and moving piece */
    gen_m.m_frcp = EPDboard.rbv[gen_m.m_frsq];
/* continue if it is an active piece */
    if (cv_c_cpv[gen_m.m_frcp] == ese.ese_actc) {
/* generate moves for active color piece */
      xsqptr0 = &xb.xbv[map_xsq_sq(gen_m.m_frsq)];
      switch (cv_p_cpv[gen_m.m_frcp]) {
        case p_p:
/* pawn moves: a bit tricky; colors done separately */
          frfile = map_file(gen_m.m_frsq);
          frrank = map_rank(gen_m.m_frsq);
          if (ese.ese_actc == c_w) {
/* one square non-capture */
            gen_m.m_tocp = EPDboard.rbv[gen_m.m_tosq = gen_m.m_frsq + dv_1];
            if (gen_m.m_tocp == cp_v0) {
              if (frrank != rank_7) {
/* non-promotion */
                *treeptr++ = gen_m;
                tse.tse_count++;
              } else {
/* promotion */
                for (gen_m.m_scmv = scmv_ppn; gen_m.m_scmv <= scmv_ppq;
                    gen_m.m_scmv++) {
                  *treeptr++ = gen_m;
                  tse.tse_count++;
                }
                gen_m.m_scmv = scmv_reg;
              };
            }
/* two squares forward */
            if ((frrank == rank_2) && Vacant(gen_m.m_frsq + dv_1)
                && Vacant(gen_m.m_frsq + (2 * dv_1))) {
              gen_m.m_tosq = gen_m.m_frsq + (2 * dv_1);
              gen_m.m_tocp = cp_v0;
              *treeptr++ = gen_m;
              tse.tse_count++;
            };
/* capture to left */
            if (frfile != file_a) {
              gen_m.m_tosq = gen_m.m_frsq + dv_5;
              gen_m.m_tocp = EPDboard.rbv[gen_m.m_tosq];
              if (cv_c_cpv[gen_m.m_tocp] == inv_cv[ese.ese_actc]) {
                if (frrank != rank_7) {
/* non-promote */
                  *treeptr++ = gen_m;
                  tse.tse_count++;
                } else {
/* promote */
                  for (gen_m.m_scmv = scmv_ppn; gen_m.m_scmv <= scmv_ppq;
                      gen_m.m_scmv++) {
                    *treeptr++ = gen_m;
                    tse.tse_count++;
                  };
                  gen_m.m_scmv = scmv_reg;
                };
              }
            };
/* capture to right */
            if (frfile != file_h) {
              gen_m.m_tosq = gen_m.m_frsq + dv_4;
              gen_m.m_tocp = EPDboard.rbv[gen_m.m_tosq];
              if (cv_c_cpv[gen_m.m_tocp] == inv_cv[ese.ese_actc]) {
                if (frrank != rank_7) {
/* non-promote */
                  *treeptr++ = gen_m;
                  tse.tse_count++;
                } else {
/* promote */
                  for (gen_m.m_scmv = scmv_ppn; gen_m.m_scmv <= scmv_ppq;
                      gen_m.m_scmv++) {
                    *treeptr++ = gen_m;
                    tse.tse_count++;
                  };
                  gen_m.m_scmv = scmv_reg;
                };
              }
            };
/* en passant */
            if ((frrank == rank_5) && (ese.ese_epsq != sq_nil)) {
/* capture to left */
              if ((frfile != file_a)
                  && ((gen_m.m_tosq = gen_m.m_frsq + dv_5) == ese.ese_epsq)) {
                gen_m.m_tocp = cp_v0;
                gen_m.m_scmv = scmv_epc;
                *treeptr++ = gen_m;
                tse.tse_count++;
                gen_m.m_scmv = scmv_reg;
              };
/* capture to right */
              if ((frfile != file_h)
                  && ((gen_m.m_tosq = gen_m.m_frsq + dv_4) == ese.ese_epsq)) {
                gen_m.m_tocp = cp_v0;
                gen_m.m_scmv = scmv_epc;
                *treeptr++ = gen_m;
                tse.tse_count++;
                gen_m.m_scmv = scmv_reg;
              };
            };
          } else {
/* one square non-capture */
            gen_m.m_tocp = EPDboard.rbv[gen_m.m_tosq = gen_m.m_frsq + dv_3];
            if (gen_m.m_tocp == cp_v0) {
              if (frrank != rank_2) {
/* non-promotion */
                *treeptr++ = gen_m;
                tse.tse_count++;
              } else {
/* promotion */
                for (gen_m.m_scmv = scmv_ppn; gen_m.m_scmv <= scmv_ppq;
                    gen_m.m_scmv++) {
                  *treeptr++ = gen_m;
                  tse.tse_count++;
                }
                gen_m.m_scmv = scmv_reg;
              };
            }
/* two squares forward */
            if ((frrank == rank_7) && Vacant(gen_m.m_frsq + dv_3)
                && Vacant(gen_m.m_frsq + (2 * dv_3))) {
              gen_m.m_tosq = gen_m.m_frsq + (2 * dv_3);
              gen_m.m_tocp = cp_v0;
              *treeptr++ = gen_m;
              tse.tse_count++;
            };
/* capture to left */
            if (frfile != file_a) {
              gen_m.m_tosq = gen_m.m_frsq + dv_6;
              gen_m.m_tocp = EPDboard.rbv[gen_m.m_tosq];
              if (cv_c_cpv[gen_m.m_tocp] == inv_cv[ese.ese_actc]) {
                if (frrank != rank_2) {
/* non-promote */
                  *treeptr++ = gen_m;
                  tse.tse_count++;
                } else {
/* promote */
                  for (gen_m.m_scmv = scmv_ppn; gen_m.m_scmv <= scmv_ppq;
                      gen_m.m_scmv++) {
                    *treeptr++ = gen_m;
                    tse.tse_count++;
                  };
                  gen_m.m_scmv = scmv_reg;
                };
              }
            };
/* capture to right */
            if (frfile != file_h) {
              gen_m.m_tosq = gen_m.m_frsq + dv_7;
              gen_m.m_tocp = EPDboard.rbv[gen_m.m_tosq];
              if (cv_c_cpv[gen_m.m_tocp] == inv_cv[ese.ese_actc]) {
                if (frrank != rank_2) {
/* non-promote */
                  *treeptr++ = gen_m;
                  tse.tse_count++;
                } else {
/* promote */
                  for (gen_m.m_scmv = scmv_ppn; gen_m.m_scmv <= scmv_ppq;
                      gen_m.m_scmv++) {
                    *treeptr++ = gen_m;
                    tse.tse_count++;
                  };
                  gen_m.m_scmv = scmv_reg;
                };
              }
            };
/* en passant */
            if ((frrank == rank_4) && (ese.ese_epsq != sq_nil)) {
/* capture to left */
              if ((frfile != file_a)
                  && ((gen_m.m_tosq = gen_m.m_frsq + dv_6) == ese.ese_epsq)) {
                gen_m.m_tocp = cp_v0;
                gen_m.m_scmv = scmv_epc;
                *treeptr++ = gen_m;
                tse.tse_count++;
                gen_m.m_scmv = scmv_reg;
              };
/* capture to right */
              if ((frfile != file_h)
                  && ((gen_m.m_tosq = gen_m.m_frsq + dv_7) == ese.ese_epsq)) {
                gen_m.m_tocp = cp_v0;
                gen_m.m_scmv = scmv_epc;
                *treeptr++ = gen_m;
                tse.tse_count++;
                gen_m.m_scmv = scmv_reg;
              };
            };
          };
          break;
        case p_n:
/* knight moves: very simple */
          for (dx = dx_8; dx <= dx_f; dx++)
            if (*(xsqptr0 + xdvv[dx]) == cp_v0) {
              gen_m.m_tocp = EPDboard.rbv[gen_m.m_tosq =
                  gen_m.m_frsq + dvv[dx]];
              if (cv_c_cpv[gen_m.m_tocp] != ese.ese_actc) {
                *treeptr++ = gen_m;
                tse.tse_count++;
              };
            };
          break;
        case p_b:
/* bishop moves: diagonal sweeper */
          for (dx = dx_4; dx <= dx_7; dx++) {
            dv = dvv[dx];
            xdv = xdvv[dx];
            gen_m.m_tosq = gen_m.m_frsq;
            xsqptr1 = xsqptr0;
            while ((*(xsqptr1 += xdv) == cp_v0)
                && ((gen_m.m_tocp = EPDboard.rbv[gen_m.m_tosq +=
                            dv]) == cp_v0)) {
              *treeptr++ = gen_m;
              tse.tse_count++;
            };
            if ((*xsqptr1 == cp_v0)
                && (cv_c_cpv[gen_m.m_tocp] == inv_cv[ese.ese_actc])) {
              *treeptr++ = gen_m;
              tse.tse_count++;
            };
          };
          break;
        case p_r:
/* rook moves: orthogonal sweeper */
          for (dx = dx_0; dx <= dx_3; dx++) {
            dv = dvv[dx];
            xdv = xdvv[dx];
            gen_m.m_tosq = gen_m.m_frsq;
            xsqptr1 = xsqptr0;
            while ((*(xsqptr1 += xdv) == cp_v0)
                && ((gen_m.m_tocp = EPDboard.rbv[gen_m.m_tosq +=
                            dv]) == cp_v0)) {
              *treeptr++ = gen_m;
              tse.tse_count++;
            };
            if ((*xsqptr1 == cp_v0)
                && (cv_c_cpv[gen_m.m_tocp] == inv_cv[ese.ese_actc])) {
              *treeptr++ = gen_m;
              tse.tse_count++;
            };
          };
          break;
        case p_q:
/* queen moves: orthogonal and diagonal sweeper */
          for (dx = dx_0; dx <= dx_7; dx++) {
            dv = dvv[dx];
            xdv = xdvv[dx];
            gen_m.m_tosq = gen_m.m_frsq;
            xsqptr1 = xsqptr0;
            while ((*(xsqptr1 += xdv) == cp_v0)
                && ((gen_m.m_tocp = EPDboard.rbv[gen_m.m_tosq +=
                            dv]) == cp_v0)) {
              *treeptr++ = gen_m;
              tse.tse_count++;
            };
            if ((*xsqptr1 == cp_v0)
                && (cv_c_cpv[gen_m.m_tocp] == inv_cv[ese.ese_actc])) {
              *treeptr++ = gen_m;
              tse.tse_count++;
            };
          };
          break;
        case p_k:
/* king moves: one square adjacent regular */
          for (dx = dx_0; dx <= dx_7; dx++)
            if (*(xsqptr0 + xdvv[dx]) == cp_v0) {
              gen_m.m_tocp = EPDboard.rbv[gen_m.m_tosq =
                  gen_m.m_frsq + dvv[dx]];
              if (cv_c_cpv[gen_m.m_tocp] != ese.ese_actc) {
                *treeptr++ = gen_m;
                tse.tse_count++;
              };
            };
/* castling; process according to active color */
          if (ese.ese_actc == c_w) {
            if ((ese.ese_cast & cf_wk) && !EPDBlackAttacks(sq_e1)
                && Vacant(sq_f1)
                && !EPDBlackAttacks(sq_f1) && Vacant(sq_g1)
                && !EPDBlackAttacks(sq_g1)) {
              gen_m.m_tosq = sq_g1;
              gen_m.m_tocp = cp_v0;
              gen_m.m_scmv = scmv_cks;
              *treeptr++ = gen_m;
              tse.tse_count++;
              gen_m.m_scmv = scmv_reg;
            };
            if ((ese.ese_cast & cf_wq) && !EPDBlackAttacks(sq_e1)
                && Vacant(sq_d1)
                && !EPDBlackAttacks(sq_d1) && Vacant(sq_c1)
                && !EPDBlackAttacks(sq_c1) && Vacant(sq_b1)) {
              gen_m.m_tosq = sq_c1;
              gen_m.m_tocp = cp_v0;
              gen_m.m_scmv = scmv_cqs;
              *treeptr++ = gen_m;
              tse.tse_count++;
              gen_m.m_scmv = scmv_reg;
            };
          } else {
            if ((ese.ese_cast & cf_bk) && !EPDWhiteAttacks(sq_e8)
                && Vacant(sq_f8)
                && !EPDWhiteAttacks(sq_f8) && Vacant(sq_g8)
                && !EPDWhiteAttacks(sq_g8)) {
              gen_m.m_tosq = sq_g8;
              gen_m.m_tocp = cp_v0;
              gen_m.m_scmv = scmv_cks;
              *treeptr++ = gen_m;
              tse.tse_count++;
              gen_m.m_scmv = scmv_reg;
            };
            if ((ese.ese_cast & cf_bq) && !EPDWhiteAttacks(sq_e8)
                && Vacant(sq_d8)
                && !EPDWhiteAttacks(sq_d8) && Vacant(sq_c8)
                && !EPDWhiteAttacks(sq_c8) && Vacant(sq_b8)) {
              gen_m.m_tosq = sq_c8;
              gen_m.m_tocp = cp_v0;
              gen_m.m_scmv = scmv_cqs;
              *treeptr++ = gen_m;
              tse.tse_count++;
              gen_m.m_scmv = scmv_reg;
            };
          };
          break;
      };
    };
  };
  return;
}

/*--> EPDSameMoveRef: check if two move references are the same move */
static siT EPDSameMoveRef(mptrT mptr0, mptrT mptr1) {
  siT flag;

  if ((mptr0->m_tosq == mptr1->m_tosq) && (mptr0->m_frsq == mptr1->m_frsq)
      && (mptr0->m_frcp == mptr1->m_frcp) && (mptr0->m_tocp == mptr1->m_tocp)
      && (mptr0->m_scmv == mptr1->m_scmv))
    flag = 1;
  else
    flag = 0;
  return flag;
}

/*--> EPDFindMove: locate the move in the current generation set */
static mptrT EPDFindMove(mptrT mptr) {
  mptrT rmptr;
  siT flag;
  siT index;

  rmptr = tse.tse_base;
  flag = 0;
  index = 0;
  while (!flag && (index < tse.tse_count))
    if (EPDSameMoveRef(mptr, rmptr))
      flag = 1;
    else {
      rmptr++;
      index++;
    };
  if (!flag)
    rmptr = NULL;
  return rmptr;
}

/*--> EPDExecute: execute the supplied move */
static
void EPDExecute(mptrT mptr) {
  sqT pcsq;
  cpT ppcp;

/* test for overflow */
  if (ply == (pmhL - 1))
    EPDFatal("EPDExecute: played move history overflow");
/* save old environment and generation records */
  *eseptr++ = ese;
  *tseptr++ = tse;
/* set the legality tested flag */
  mptr->m_flag |= mf_exec;
/* process according to move case */
  switch (mptr->m_scmv) {
    case scmv_reg:
      EPDboard.rbv[mptr->m_frsq] = cp_v0;
      EPDboard.rbv[mptr->m_tosq] = mptr->m_frcp;
      break;
    case scmv_epc:
      if (ese.ese_actc == c_w)
        pcsq = mptr->m_tosq + dv_3;
      else
        pcsq = mptr->m_tosq + dv_1;
      EPDboard.rbv[mptr->m_frsq] = cp_v0;
      EPDboard.rbv[mptr->m_tosq] = mptr->m_frcp;
      EPDboard.rbv[pcsq] = cp_v0;
      break;
    case scmv_cks:
      if (ese.ese_actc == c_w) {
        EPDboard.rbv[sq_e1] = cp_v0;
        EPDboard.rbv[sq_g1] = cp_wk;
        EPDboard.rbv[sq_h1] = cp_v0;
        EPDboard.rbv[sq_f1] = cp_wr;
      } else {
        EPDboard.rbv[sq_e8] = cp_v0;
        EPDboard.rbv[sq_g8] = cp_bk;
        EPDboard.rbv[sq_h8] = cp_v0;
        EPDboard.rbv[sq_f8] = cp_br;
      };
      break;
    case scmv_cqs:
      if (ese.ese_actc == c_w) {
        EPDboard.rbv[sq_e1] = cp_v0;
        EPDboard.rbv[sq_c1] = cp_wk;
        EPDboard.rbv[sq_a1] = cp_v0;
        EPDboard.rbv[sq_d1] = cp_wr;
      } else {
        EPDboard.rbv[sq_e8] = cp_v0;
        EPDboard.rbv[sq_c8] = cp_bk;
        EPDboard.rbv[sq_a8] = cp_v0;
        EPDboard.rbv[sq_d8] = cp_br;
      };
      break;
    case scmv_ppn:
      if (ese.ese_actc == c_w)
        ppcp = cp_wn;
      else
        ppcp = cp_bn;
      EPDboard.rbv[mptr->m_frsq] = cp_v0;
      EPDboard.rbv[mptr->m_tosq] = ppcp;
      break;
    case scmv_ppb:
      if (ese.ese_actc == c_w)
        ppcp = cp_wb;
      else
        ppcp = cp_bb;
      EPDboard.rbv[mptr->m_frsq] = cp_v0;
      EPDboard.rbv[mptr->m_tosq] = ppcp;
      break;
    case scmv_ppr:
      if (ese.ese_actc == c_w)
        ppcp = cp_wr;
      else
        ppcp = cp_br;
      EPDboard.rbv[mptr->m_frsq] = cp_v0;
      EPDboard.rbv[mptr->m_tosq] = ppcp;
      break;
    case scmv_ppq:
      if (ese.ese_actc == c_w)
        ppcp = cp_wq;
      else
        ppcp = cp_bq;
      EPDboard.rbv[mptr->m_frsq] = cp_v0;
      EPDboard.rbv[mptr->m_tosq] = ppcp;
      break;
  };
/* set values for updated environment record: active color */
  ese.ese_actc = inv_cv[ese.ese_actc];
/* set values for updated environment record: castling availablity */
  if (ese.ese_cast != 0) {
    if (ese.ese_cast & cf_wk)
      if ((mptr->m_frsq == sq_e1) || (mptr->m_frsq == sq_h1)
          || (mptr->m_tosq == sq_h1))
        ese.ese_cast &= ~cf_wk;
    if (ese.ese_cast & cf_wq)
      if ((mptr->m_frsq == sq_e1) || (mptr->m_frsq == sq_a1)
          || (mptr->m_tosq == sq_a1))
        ese.ese_cast &= ~cf_wq;
    if (ese.ese_cast & cf_bk)
      if ((mptr->m_frsq == sq_e8) || (mptr->m_frsq == sq_h8)
          || (mptr->m_tosq == sq_h8))
        ese.ese_cast &= ~cf_bk;
    if (ese.ese_cast & cf_bq)
      if ((mptr->m_frsq == sq_e8) || (mptr->m_frsq == sq_a8)
          || (mptr->m_tosq == sq_a8))
        ese.ese_cast &= ~cf_bq;
  };
/* set values for updated environment record: en passant */
  if (ese.ese_actc == c_b) {
    if ((mptr->m_frcp == cp_wp) && (map_rank(mptr->m_frsq) == rank_2)
        && (map_rank(mptr->m_tosq) == rank_4))
      ese.ese_epsq = mptr->m_frsq + dv_1;
    else
      ese.ese_epsq = sq_nil;
  } else {
    if ((mptr->m_frcp == cp_bp) && (map_rank(mptr->m_frsq) == rank_7)
        && (map_rank(mptr->m_tosq) == rank_5))
      ese.ese_epsq = mptr->m_frsq + dv_3;
    else
      ese.ese_epsq = sq_nil;
  }
/* set values for updated environment record: halfmove clock */
  if ((mptr->m_tocp != cp_v0) || (cv_p_cpv[mptr->m_frcp] == p_p))
    ese.ese_hmvc = 0;
  else
    ese.ese_hmvc++;
/* set values for updated environment record: fullmove number */
  if (ese.ese_actc == c_w)
    ese.ese_fmvn++;
/* set values for updated environment record: king locations */
  switch (mptr->m_frcp) {
    case cp_wk:
      ese.ese_ksqv[c_w] = mptr->m_tosq;
      break;
    case cp_bk:
      ese.ese_ksqv[c_b] = mptr->m_tosq;
      break;
    default:
      break;
  };
/* check/bust flags */
  if (EPDTestAKIC())
    mptr->m_flag |= mf_chec;
  if (EPDTestPKIC())
    mptr->m_flag |= mf_bust;
/* increment ply index */
  ply++;
  return;
}

/*--> EPDExecuteUpdate: update the current move pointer, then execute */
nonstatic void EPDExecuteUpdate(mptrT mptr) {
  tse.tse_curr = EPDFindMove(mptr);
  if (tse.tse_curr == NULL)
    EPDFatal("EPDExecuteUpdate: can't find move");
  EPDExecute(tse.tse_curr);
  return;
}

/*--> EPDRetract: retract the supplied move */
static
void EPDRetract(mptrT mptr) {
/* decrement ply */
  ply--;
/* restore the current environment and generation */
  ese = *--eseptr;
  tse = *--tseptr;
/* process by move case */
  switch (mptr->m_scmv) {
    case scmv_reg:
      EPDboard.rbv[mptr->m_tosq] = mptr->m_tocp;
      EPDboard.rbv[mptr->m_frsq] = mptr->m_frcp;
      break;
    case scmv_epc:
      EPDboard.rbv[mptr->m_tosq] = cp_v0;
      EPDboard.rbv[mptr->m_frsq] = mptr->m_frcp;
      if (ese.ese_actc == c_w)
        EPDboard.rbv[mptr->m_tosq + dv_3] = cp_bp;
      else
        EPDboard.rbv[mptr->m_tosq + dv_1] = cp_wp;
      break;
    case scmv_cks:
      if (ese.ese_actc == c_w) {
        EPDboard.rbv[sq_g1] = cp_v0;
        EPDboard.rbv[sq_e1] = cp_wk;
        EPDboard.rbv[sq_f1] = cp_v0;
        EPDboard.rbv[sq_h1] = cp_wr;
      } else {
        EPDboard.rbv[sq_g8] = cp_v0;
        EPDboard.rbv[sq_e8] = cp_bk;
        EPDboard.rbv[sq_f8] = cp_v0;
        EPDboard.rbv[sq_h8] = cp_br;
      }
      break;
    case scmv_cqs:
      if (ese.ese_actc == c_w) {
        EPDboard.rbv[sq_c1] = cp_v0;
        EPDboard.rbv[sq_e1] = cp_wk;
        EPDboard.rbv[sq_d1] = cp_v0;
        EPDboard.rbv[sq_a1] = cp_wr;
      } else {
        EPDboard.rbv[sq_c8] = cp_v0;
        EPDboard.rbv[sq_e8] = cp_bk;
        EPDboard.rbv[sq_d8] = cp_v0;
        EPDboard.rbv[sq_a8] = cp_br;
      };
      break;
    case scmv_ppn:
    case scmv_ppb:
    case scmv_ppr:
    case scmv_ppq:
      EPDboard.rbv[mptr->m_tosq] = mptr->m_tocp;
      EPDboard.rbv[mptr->m_frsq] = mptr->m_frcp;
      break;
  };
  return;
}

/*--> EPDRetractUpdate: retract last executed move */
nonstatic void EPDRetractUpdate(void) {
  mptrT mptr;

  mptr = (tseptr - 1)->tse_curr;
  EPDRetract(mptr);
  return;
}

/*--> EPDRetractAll: retract all moves in current variation */
nonstatic void EPDRetractAll(void) {
  while (ply > 0)
    EPDRetractUpdate();
  return;
}

/*--> EPDCollapse: collapse the played move stack */
nonstatic void EPDCollapse(void) {
/* process for nonzero ply */
  if (ply > 0) {
/* reset the stack pointers */
    treeptr = treebaseptr;
    eseptr = esebaseptr;
    tseptr = tsebaseptr;
/* reset the ply */
    ply = 0;
  };
  return;
}

/*--> EPDReset: collapse, then reset starting position */
nonstatic void EPDReset(void) {
  EPDCollapse();
  EPDInitArray();
  return;
}

/*--> EPDMLExec: execute the current move list */
static
void EPDMLExec(void) {
  siT i;
  mptrT mptr;

/* test and mark each move for legality and checking status */
  mptr = tse.tse_base;
  for (i = 0; i < tse.tse_count; i++) {
    tse.tse_curr = mptr;
    EPDExecute(mptr);
    EPDRetract(mptr);
    mptr++;
  };
  return;
}

/*--> EPDMLPolice: remove illegal moves the current move list */
static
void EPDMLPolice(void) {
  mptrT tptr, mptr;
  siT i, bust;
  mT t_m;

/* move illegal moves to end of list */
  mptr = tse.tse_base;
  bust = 0;
  i = 0;
  while (i < (tse.tse_count - bust))
    if (mptr->m_flag & mf_bust) {
      tptr = (tse.tse_base + (tse.tse_count - 1)) - bust;
      t_m = *mptr;
      *mptr = *tptr;
      *tptr = t_m;
      bust++;
    } else {
      mptr++;
      i++;
    };
/* shrink */
  tse.tse_count -= bust;
  return;
}

/*--> EPDMLDisambiguate: insert disambiguation flags in move list */
static
void EPDMLDisambiguate(void) {
  siT i, j, tmc, rmc, fmc;
  mptrT mptr0, mptr1;
  pT p;
  rankT rank;
  fileT file;

/* it's magic */
  mptr0 = tse.tse_base;
  for (i = 0; i < tse.tse_count; i++) {
/* the outer loop disambiguates a single move per cycle */
    p = cv_p_cpv[mptr0->m_frcp];
    if ((p != p_p) && (p != p_k)) {
      rank = map_rank(mptr0->m_frsq);
      file = map_file(mptr0->m_frsq);
      tmc = rmc = fmc = 0;
      mptr1 = tse.tse_base;
      for (j = 0; j < tse.tse_count; j++) {
/* the inner loop examines all possible sibling puns */
        if ((i != j) && (mptr0->m_frcp == mptr1->m_frcp)
            && (mptr0->m_tosq == mptr1->m_tosq)) {
          tmc++;
          if (map_rank(mptr1->m_frsq) == rank)
            rmc++;
          if (map_file(mptr1->m_frsq) == file)
            fmc++;
        };
        mptr1++;
      };
/* check pun count for outer loop move */
      if (tmc > 0) {
/* file disambiguation has priority */
        if ((rmc > 0) || ((rmc == 0) && (fmc == 0)))
          mptr0->m_flag |= mf_sanf;
/* rank disambiguation may be needed */
        if (fmc > 0)
          mptr0->m_flag |= mf_sanr;
      };
    };
    mptr0++;
  };
  return;
}

/*--> EPDMLScanMate: scan current move list for mating moves */
static
void EPDMLScanMate(void) {
  siT i, j, mateflag, drawflag, moveflag;
  mptrT mptr0, mptr1;

/* scan */
  mptr0 = tse.tse_base;
  for (i = 0; i < tse.tse_count; i++) {
    tse.tse_curr = mptr0;
    EPDExecute(mptr0);
/* now at next higher ply, generate psuedolegal set */
    EPDGeneratePL();
/* try to find at least one legal move */
    mptr1 = tse.tse_base;
    moveflag = 0;
    j = 0;
    while (!moveflag && (j < tse.tse_count)) {
      tse.tse_curr = mptr1;
      EPDExecute(mptr1);
      EPDRetract(mptr1);
      if (!(mptr1->m_flag & mf_bust))
        moveflag = 1;
      else {
        mptr1++;
        j++;
      };
    };
/* any second level moves detected? */
    if (moveflag != 0) {
/* not a mate */
      mateflag = drawflag = 0;
    } else {
/* a mating move is detected */
      if (EPDTestAKIC()) {
        mateflag = 1;
        drawflag = 0;
      } else {
        drawflag = 1;
        mateflag = 0;
      };
    };
/* undo execution */
    EPDRetract(mptr0);
/* now back at lower ply */
    if (mateflag)
      mptr0->m_flag |= mf_chmt;
    else if (drawflag)
      mptr0->m_flag |= (mf_draw | mf_stmt);
/* next move */
    mptr0++;
  };
  return;
}

/*--> EPDGenClean: generate move list with first level processing */
static
void EPDGenClean(void) {
/* basic psuedolegal generation */
  EPDGeneratePL();
/* set legality flags, remove illegal moves, and disambiguate */
  EPDMLExec();
  EPDMLPolice();
  EPDMLDisambiguate();
  return;
}

/*--> EPDGenMoves: generate legal moves and set mate flags */
nonstatic void EPDGenMoves(void) {
/* perform basic first level generation */
  EPDGenClean();
/* handle two ply draw and checkmate detection */
  EPDMLScanMate();
  return;
}

/*--> EPDFetchMoveCount: fetch the move count */
nonstatic siT EPDFetchMoveCount(void) {
  return tse.tse_count;
}

/*--> EPDFetchMove: fetch the nth move */
nonstatic mptrT EPDFetchMove(siT index) {
  ret_m = *(tse.tse_base + index);
  return &ret_m;
}

/*--> EPDSetMoveFlags: set move flags from current generation set */
nonstatic void EPDSetMoveFlags(mptrT mptr) {
  mptrT rmptr;

  rmptr = EPDFindMove(mptr);
  if (rmptr != NULL)
    mptr->m_flag = rmptr->m_flag;
  return;
}

/*--> EPDSortSAN: ASCII SAN sort move list */
nonstatic void EPDSortSAN(void) {
  mptrT mptr, mptr0, mptr1;
  siT i, j, pair, pass, flag;
  sanptrT sanptr, sptr0, sptr1;
  char t_ch;
  mT t_m;

/* allocate the SAN string vector */
  sanptr = (sanptrT) EPDMemoryGrab(sizeof(sanT) * tse.tse_count);
/* construct the SAN string entries */
  mptr = tse.tse_base;
  for (i = 0; i < tse.tse_count; i++)
    EPDSANEncode(mptr++, *(sanptr + i));
/* a low tech bubble sort */
  flag = 1;
  pass = 0;
  while (flag && (pass < (tse.tse_count - 1))) {
    sptr0 = sanptr;
    sptr1 = sanptr + 1;
    mptr0 = tse.tse_base;
    mptr1 = tse.tse_base + 1;
    flag = 0;
    pair = 0;
    while (pair < (tse.tse_count - pass - 1)) {
/* case sensitive ascending order */
      if (strcmp((charptrT) sptr0, (charptrT) sptr1) > 0) {
        flag = 1;
        for (j = 0; j < sanL; j++) {
          t_ch = (*sptr0)[j];
          (*sptr0)[j] = (*sptr1)[j];
          (*sptr1)[j] = t_ch;
        };
        t_m = *mptr0;
        *mptr0 = *mptr1;
        *mptr1 = t_m;
      };
      sptr0++;
      sptr1++;
      mptr0++;
      mptr1++;
      pair++;
    };
    pass++;
  };
  EPDMemoryFree(sanptr);
  return;
}

/*--> EPDRepairMove: repair a move operation */
static
void EPDRepairMove(eopptrT eopptr) {
  eovptrT eovptr;
  mptrT mptr;
  mT m;
  sanT san;

/* repair a single move from the current position */
  eovptr = eopptr->eop_headeov;
  if (eovptr != NULL) {
    mptr = EPDSANDecodeAux(eovptr->eov_str, 0);
    if (mptr != NULL) {
      m = *mptr;
      EPDSANEncode(&m, san);
      if (strcmp(eovptr->eov_str, san) != 0)
        EPDReplaceEOVStr(eovptr, san);
    };
  };
  return;
}

/*--> EPDRepairMoveset: repair a moveset operation */
static
void EPDRepairMoveset(eopptrT eopptr) {
  eovptrT eovptr;
  mptrT mptr;
  mT m;
  sanT san;

/* check each move from the current position */
  eovptr = eopptr->eop_headeov;
  while (eovptr != NULL) {
    mptr = EPDSANDecodeAux(eovptr->eov_str, 0);
    if (mptr != NULL) {
      m = *mptr;
      EPDSANEncode(&m, san);
      if (strcmp(eovptr->eov_str, san) != 0)
        EPDReplaceEOVStr(eovptr, san);
    };
    eovptr = eovptr->eov_next;
  };
  return;
}

/*--> EPDRepairVariation: repair a variation operation */
static
void EPDRepairVariation(eopptrT eopptr) {
  eovptrT eovptr;
  mptrT mptr;
  mT m;
  sanT san;

/* play move sequence from the current position */
  eovptr = eopptr->eop_headeov;
  while (eovptr != NULL) {
    mptr = EPDSANDecodeAux(eovptr->eov_str, 0);
    if (mptr == NULL)
      eovptr = NULL;
    else {
      m = *mptr;
      EPDSANEncode(&m, san);
      if (strcmp(eovptr->eov_str, san) != 0)
        EPDReplaceEOVStr(eovptr, san);
      tse.tse_curr = EPDFindMove(mptr);
      if (tse.tse_curr == NULL)
        EPDFatal("EPDRepairVariation: can't find move");
      EPDExecute(mptr);
      EPDGenMoves();
      eovptr = eovptr->eov_next;
    }
  };
/* retract any executed moves */
  EPDRetractAll();
  return;
}

/*--> EPDPurgeOpFile: purge operation from input file to output file */
nonstatic siT EPDPurgeOpFile(charptrT opsym, charptrT fn0, charptrT fn1) {
  siT flag;
  fptrT fptr0, fptr1;
  epdptrT epdptr;
  eopptrT eopptr;
  charptrT eptr;
  char ev[epdL];

/* set default return value (success) */
  flag = 1;
/* clear the input and output file pointers */
  fptr0 = fptr1 = NULL;
/* open the input file for reading */
  if (flag) {
    fptr0 = fopen(fn0, "r");
    if (fptr0 == NULL)
      flag = 0;
  };
/* open the output file for writing */
  if (flag) {
    fptr1 = fopen(fn1, "w");
    if (fptr1 == NULL)
      flag = 0;
  };
/* scan entire file */
  while (flag && (fgets(ev, (epdL - 1), fptr0) != NULL)) {
/* decode a record */
    epdptr = EPDDecode(ev);
/* check record decode validity */
    if (epdptr == NULL)
      flag = 0;
    else {
/* locate the operation to be purged */
      eopptr = EPDLocateEOP(epdptr, opsym);
      if (eopptr != NULL) {
        EPDUnthreadEOP(epdptr, eopptr);
        EPDReleaseEOP(eopptr);
      };
/* encode the record (includes normalization) */
      eptr = EPDEncode(epdptr);
/* release EPD structure */
      EPDReleaseEPD(epdptr);
/* check result */
      if (eptr == NULL)
        flag = 0;
      else {
        fprintf(fptr1, "%s\n", eptr);
        EPDMemoryFree(eptr);
      };
    };
  };
/* close input and output files */
  if (fptr0 != NULL)
    fclose(fptr0);
  if (fptr1 != NULL)
    fclose(fptr1);
  return flag;
}

/*--> EPDRepairEPD: repair an EPD structure */
nonstatic siT EPDRepairEPD(epdptrT epdptr) {
  siT flag;
  eopptrT eopptr;

/* set default return value: repair successful */
  flag = 1;
/* set up the position as the current position */
  EPDRealize(epdptr);
/* check legality */
  if (!EPDIsLegal())
    flag = 0;
  else {
/* generate moves and notation */
    EPDGenMoves();
/* repair moveset "am" */
    eopptr = EPDLocateEOP(epdptr, epdsostrv[epdso_am]);
    if (eopptr != NULL)
      EPDRepairMoveset(eopptr);
/* repair moveset "bm" */
    eopptr = EPDLocateEOP(epdptr, epdsostrv[epdso_bm]);
    if (eopptr != NULL)
      EPDRepairMoveset(eopptr);
/* repair move "pm" */
    eopptr = EPDLocateEOP(epdptr, epdsostrv[epdso_pm]);
    if (eopptr != NULL)
      EPDRepairMove(eopptr);
/* repair variation "pv" */
    eopptr = EPDLocateEOP(epdptr, epdsostrv[epdso_pv]);
    if (eopptr != NULL)
      EPDRepairVariation(eopptr);
/* repair move "sm" */
    eopptr = EPDLocateEOP(epdptr, epdsostrv[epdso_sm]);
    if (eopptr != NULL)
      EPDRepairMove(eopptr);
/* repair variation "sv" */
    eopptr = EPDLocateEOP(epdptr, epdsostrv[epdso_sv]);
    if (eopptr != NULL)
      EPDRepairVariation(eopptr);
  };
  return flag;
}

/*--> EPDRepairFile: repair input file to output file */
nonstatic siT EPDRepairFile(charptrT fn0, charptrT fn1) {
  siT flag;
  fptrT fptr0, fptr1;
  epdptrT epdptr;
  charptrT eptr;
  char ev[epdL];

/* set default return value (success) */
  flag = 1;
/* clear the input and output file pointers */
  fptr0 = fptr1 = NULL;
/* open the input file for reading */
  if (flag) {
    fptr0 = fopen(fn0, "r");
    if (fptr0 == NULL)
      flag = 0;
  };
/* open the output file for writing */
  if (flag) {
    fptr1 = fopen(fn1, "w");
    if (fptr1 == NULL)
      flag = 0;
  };
/* scan entire file */
  while (flag && (fgets(ev, (epdL - 1), fptr0) != NULL)) {
/* decode a record */
    epdptr = EPDDecode(ev);
/* check record decode validity */
    if (epdptr == NULL)
      flag = 0;
    else {
/* make repairs */
      flag = EPDRepairEPD(epdptr);
/* continue if repair okay */
      if (flag) {
/* encode the normalized record */
        eptr = EPDEncode(epdptr);
/* check result */
        if (eptr == NULL)
          flag = 0;
        else {
          fprintf(fptr1, "%s\n", eptr);
          EPDMemoryFree(eptr);
        };
      };
/* release EPD structure */
      EPDReleaseEPD(epdptr);
    };
  };
/* close input and output files */
  if (fptr0 != NULL)
    fclose(fptr0);
  if (fptr1 != NULL)
    fclose(fptr1);
  return flag;
}

/*--> EPDNormalizeFile: normalize input file to output file */
nonstatic siT EPDNormalizeFile(charptrT fn0, charptrT fn1) {
  siT flag;
  fptrT fptr0, fptr1;
  epdptrT epdptr;
  charptrT eptr;
  char ev[epdL];

/* set default return value (success) */
  flag = 1;
/* clear the input and output file pointers */
  fptr0 = fptr1 = NULL;
/* open the input file for reading */
  if (flag) {
    fptr0 = fopen(fn0, "r");
    if (fptr0 == NULL)
      flag = 0;
  };
/* open the output file for writing */
  if (flag) {
    fptr1 = fopen(fn1, "w");
    if (fptr1 == NULL)
      flag = 0;
  };
/* scan entire file */
  while (flag && (fgets(ev, (epdL - 1), fptr0) != NULL)) {
/* decode a record */
    epdptr = EPDDecode(ev);
/* check record decode validity */
    if (epdptr == NULL)
      flag = 0;
    else {
/* encode the record (this includes normalization) */
      eptr = EPDEncode(epdptr);
/* release EPD structure */
      EPDReleaseEPD(epdptr);
/* check result */
      if (eptr == NULL)
        flag = 0;
      else {
        fprintf(fptr1, "%s\n", eptr);
        EPDMemoryFree(eptr);
      };
    };
  };
/* close input and output files */
  if (fptr0 != NULL)
    fclose(fptr0);
  if (fptr1 != NULL)
    fclose(fptr1);
  return flag;
}

/*--> EPDScoreFile: score a benchmark file */
nonstatic siT EPDScoreFile(charptrT fn, bmsptrT bmsptr) {
  siT flag;
  siT skipflag;
  siT am_flag, bm_flag;
  siT solved = 0;
  fptrT fptr;
  epdptrT epdptr;
  eopptrT am_eopptr, bm_eopptr, acd_eopptr, acn_eopptr, acs_eopptr;
  eopptrT sm_eopptr, sv_eopptr, pm_eopptr, pv_eopptr;
  charptrT result;
  char ev[epdL];

/* set default return value (success) */
  flag = 1;
/* clear the input file pointer */
  fptr = NULL;
/* preset the summary structure */
  bmsptr->bms_acdflag = bmsptr->bms_acnflag = bmsptr->bms_acsflag = 1;
  bmsptr->bms_total = bmsptr->bms_solve = bmsptr->bms_unsol = 0;
  bmsptr->bms_total_acd = bmsptr->bms_solve_acd = bmsptr->bms_unsol_acd = 0;
  bmsptr->bms_total_acn = bmsptr->bms_solve_acn = bmsptr->bms_unsol_acn = 0;
  bmsptr->bms_total_acs = bmsptr->bms_solve_acs = bmsptr->bms_unsol_acs = 0;
/* open the input file for reading */
  if (flag) {
    fptr = fopen(fn, "r");
    if (fptr == NULL)
      flag = 0;
  };
/* scan entire file */
  while (flag && (fgets(ev, (epdL - 1), fptr) != NULL)) {
/* decode a record */
    epdptr = EPDDecode(ev);
/* check record decode validity */
    if (epdptr == NULL)
      flag = 0;
    else {
/* clear the move result pointer */
      result = NULL;
/* initialize various operation pointers */
      am_eopptr = EPDLocateEOPCode(epdptr, epdso_am);
      bm_eopptr = EPDLocateEOPCode(epdptr, epdso_bm);
      acd_eopptr = EPDLocateEOPCode(epdptr, epdso_acd);
      acn_eopptr = EPDLocateEOPCode(epdptr, epdso_acn);
      acs_eopptr = EPDLocateEOPCode(epdptr, epdso_acs);
      sm_eopptr = EPDLocateEOPCode(epdptr, epdso_sm);
      sv_eopptr = EPDLocateEOPCode(epdptr, epdso_sv);
      pm_eopptr = EPDLocateEOPCode(epdptr, epdso_pm);
      pv_eopptr = EPDLocateEOPCode(epdptr, epdso_pv);
/* test for am/bm existence */
      if ((am_eopptr == NULL) && (bm_eopptr == NULL))
        skipflag = 1;
      else
        skipflag = 0;
/* try to locate a result move (note priority) */
      if (!skipflag) {
        if (result == NULL)
          if ((pv_eopptr != NULL) && (pv_eopptr->eop_headeov != NULL))
            result = pv_eopptr->eop_headeov->eov_str;
        if (result == NULL)
          if ((pm_eopptr != NULL) && (pm_eopptr->eop_headeov != NULL))
            result = pm_eopptr->eop_headeov->eov_str;
        if (result == NULL)
          if ((sv_eopptr != NULL) && (sv_eopptr->eop_headeov != NULL))
            result = sv_eopptr->eop_headeov->eov_str;
        if (result == NULL)
          if ((sm_eopptr != NULL) && (sm_eopptr->eop_headeov != NULL))
            result = sm_eopptr->eop_headeov->eov_str;
        if (result == NULL)
          skipflag = 1;
      };
/* determine solve status */
      if (!skipflag) {
/* check for clearance with the am set */
        if ((am_eopptr == NULL) || (EPDLocateEOV(am_eopptr, result) == NULL))
          am_flag = 1;
        else
          am_flag = 0;
/* check for clearance with the bm set */
        if ((bm_eopptr == NULL) || (EPDLocateEOV(bm_eopptr, result) != NULL))
          bm_flag = 1;
        else
          bm_flag = 0;
/* set solution flag */
        solved = am_flag && bm_flag;
      };
/* update statistics block */
      if (!skipflag) {
/* clear acd flag if acd is missing */
        if ((acd_eopptr == NULL) || (acd_eopptr->eop_headeov == NULL))
          bmsptr->bms_acdflag = 0;
/* clear acn flag if acn is missing */
        if ((acn_eopptr == NULL) || (acn_eopptr->eop_headeov == NULL))
          bmsptr->bms_acnflag = 0;
/* clear acs flag if acs is missing */
        if ((acs_eopptr == NULL) || (acs_eopptr->eop_headeov == NULL))
          bmsptr->bms_acsflag = 0;
/* increment record count */
        bmsptr->bms_total++;
/* fold in acd value */
        if (bmsptr->bms_acdflag) {
          bmsptr->bms_total_acd += atoi(acd_eopptr->eop_headeov->eov_str);
          if (solved)
            bmsptr->bms_solve_acd += atoi(acd_eopptr->eop_headeov->eov_str);
          else
            bmsptr->bms_unsol_acd += atoi(acd_eopptr->eop_headeov->eov_str);
        };
/* fold in acn value */
        if (bmsptr->bms_acnflag) {
          bmsptr->bms_total_acn += atoi(acn_eopptr->eop_headeov->eov_str);
          if (solved)
            bmsptr->bms_solve_acn += atoi(acn_eopptr->eop_headeov->eov_str);
          else
            bmsptr->bms_unsol_acn += atoi(acn_eopptr->eop_headeov->eov_str);
        };
/* fold in acs value */
        if (bmsptr->bms_acsflag) {
          bmsptr->bms_total_acs += atoi(acs_eopptr->eop_headeov->eov_str);
          if (solved)
            bmsptr->bms_solve_acs += atoi(acs_eopptr->eop_headeov->eov_str);
          else
            bmsptr->bms_unsol_acs += atoi(acs_eopptr->eop_headeov->eov_str);
        };
/* update remaining items according to solved status */
        if (solved)
          bmsptr->bms_solve++;
        else
          bmsptr->bms_unsol++;
      };
/* release EPD structure */
      EPDReleaseEPD(epdptr);
    };
  };
/* close input file */
  if (fptr != NULL)
    fclose(fptr);
  return flag;
}

/*--> EPDEnumerate: enumeration of current position */
static liT EPDEnumerate(siT depth) {
  liT total;
  mptrT mptr;
  siT i;

/* enumerate current position to the indicated depth */
  if (depth == 0)
    total = 1;
  else {
    total = 0;
    EPDGeneratePL();
    mptr = tse.tse_base;
    for (i = 0; i < tse.tse_count; i++) {
      tse.tse_curr = mptr;
      EPDExecute(mptr);
      if (!(mptr->m_flag & mf_bust))
        total += EPDEnumerate((siT) (depth - 1));
      EPDRetract(mptr);
      mptr++;
    };
  };
  return total;
}

/*--> EPDEnumerateFile: enumerate input file to output file */
nonstatic siT EPDEnumerateFile(siT depth, charptrT fn0, charptrT fn1,
    liptrT totalptr) {
  siT flag;
  fptrT fptr0, fptr1;
  time_t start_time;
  liT acn, acs;
  epdptrT epdptr;
  charptrT eptr;
  char ev[epdL];

/* set default return value (success) */
  flag = 1;
/* clear the grand total */
  *totalptr = 0;
/* clear the input and output file pointers */
  fptr0 = fptr1 = NULL;
/* open the input file for reading */
  if (flag) {
    fptr0 = fopen(fn0, "r");
    if (fptr0 == NULL)
      flag = 0;
  };
/* open the output file for writing */
  if (flag) {
    fptr1 = fopen(fn1, "w");
    if (fptr1 == NULL)
      flag = 0;
  };
/* scan entire file */
  while (flag && (fgets(ev, (epdL - 1), fptr0) != NULL)) {
/* decode a record */
    epdptr = EPDDecode(ev);
/* check record decode validity */
    if (epdptr == NULL)
      flag = 0;
    else {
/* set up the current position */
      EPDRealize(epdptr);
/* check legality */
      if (!EPDIsLegal())
        flag = 0;
      else {
/* perform enumeration */
        start_time = time(NULL);
        acn = EPDEnumerate(depth);
        acs = time(NULL) - start_time;
/* update the grand total */
        *totalptr += acn;
/* record the updated field: acd */
        EPDAddOpInt(epdptr, epdso_acd, depth);
/* record the updated field: acn */
        EPDAddOpInt(epdptr, epdso_acn, acn);
/* record the updated field: acs */
        EPDAddOpInt(epdptr, epdso_acs, acs);
/* encode the record */
        EPDNormalize(epdptr);
        eptr = EPDEncode(epdptr);
/* check result */
        if (eptr == NULL)
          flag = 0;
        else {
          fprintf(fptr1, "%s\n", eptr);
          EPDMemoryFree(eptr);
        };
      };
/* release EPD structure */
      EPDReleaseEPD(epdptr);
    };
  };
/* close input and output files */
  if (fptr0 != NULL)
    fclose(fptr0);
  if (fptr1 != NULL)
    fclose(fptr1);
  return flag;
}

/*--> EPDMoveList: generate a string representation of a move list */
nonstatic charptrT EPDMoveList(gamptrT gamptr) {
  charptrT s;
  charptrT b;
  siT count;
  gpmptrT gpmptr;
  mT m;
  siT sn;
  cT sc, c;
  siT pi, index, limit, length, n, column;
  sanT san;
  char tv[tL];

/* calculate upper bound on storage requirement */
  count = EPDGamePlyCount(gamptr);
  limit = (((count + 1) / 2) * 5) + 4 + (count * 8) + 8 + 1;
  b = (charptrT) EPDMemoryGrab(limit);
/* set the inital played move pointer */
  gpmptr = gamptr->gam_headgpm;
/* set up starting color and starting move number */
  if (count == 0)
    sc = c_w;
  else
    sc = gpmptr->gpm_ese.ese_actc;
  if (count == 0)
    sn = 1;
  else
    sn = gpmptr->gpm_ese.ese_fmvn;
/* more set up */
  pi = 0;
  index = 0;
  c = sc;
  n = sn;
  column = 0;
/* loop through moves */
  for (pi = 0; pi < count; pi++) {
/* handle move number indication */
    if ((c == c_w) || ((pi == 0) && (sc == c_b))) {
      sprintf(tv, "%hd.", n);
      length = strlen(tv);
      if ((column + 1 + length) >= columnL) {
        strcpy((b + index), "\n");
        index++;
        column = 0;
      };
      if (column != 0) {
        strcpy((b + index), " ");
        index++;
        column++;
      };
      strcpy((b + index), tv);
      index += length;
      column += length;
      n++;
    };
/* handle ellipsis */
    if ((pi == 0) && (sc == c_b)) {
      sprintf(tv, "...");
      length = strlen(tv);
      if ((column + 1 + length) >= columnL) {
        strcpy((b + index), "\n");
        index++;
        column = 0;
      };
      if (column != 0) {
        strcpy((b + index), " ");
        index++;
        column++;
      };
      strcpy((b + index), tv);
      index += length;
      column += length;
    };
/* handle move */
    m = gpmptr->gpm_m;
    EPDSANEncode(&m, san);
    length = strlen(san);
    if ((column + 1 + length) >= columnL) {
      strcpy((b + index), "\n");
      index++;
      column = 0;
    };
    if (column != 0) {
      strcpy((b + index), " ");
      index++;
      column++;
    };
    strcpy((b + index), san);
    index += length;
    column += length;
    gpmptr = gpmptr->gpm_next;
    c = inv_cv[c];
  };
/* append game termination marker */
  sprintf(tv, "%s", gtimstrv[gamptr->gam_gtim]);
  length = strlen(tv);
  if ((column + 1 + length) >= columnL) {
    strcpy((b + index), "\n");
    index++;
    column = 0;
  };
  if (column != 0) {
    strcpy((b + index), " ");
    index++;
    column++;
  };
  strcpy((b + index), tv);
  index += length;
  column += length;
/* closing newline */
  if (column != 0)
    strcpy((b + index), "\n");
/* allocate and copy to result */
  s = EPDStringGrab(b);
  EPDMemoryFree(b);
  return s;
}

/*--> EPDPGNFetchTagIndex: return a PGN Seven Tag Roster tag index */
nonstatic pgnstrT EPDPGNFetchTagIndex(charptrT s) {
  pgnstrT pgnstr;
  pgnstrT rstr;

  pgnstr = pgnstr_nil;
  rstr = 0;
  while ((pgnstr == pgnstr_nil) && (rstr < pgnstrL))
    if (strcmp(s, EPDPGNFetchTagName(rstr)) == 0)
      pgnstr = rstr;
    else
      rstr++;
  return pgnstr;
}

/*--> EPDPGNFetchTagName: return a PGN Seven Tag Roster tag name */
nonstatic charptrT EPDPGNFetchTagName(pgnstrT pgnstr) {
  return pgnstrstrv[pgnstr];
}

/*--> EPDPGNGetSTR: return a string from the Seven Tag Roster */
nonstatic charptrT EPDPGNGetSTR(gamptrT gamptr, pgnstrT pgnstr) {
  return gamptr->gam_strv[pgnstr];
}

/*--> EPDPGNPutSTR: enter a string into the Seven Tag Roster */
nonstatic void EPDPGNPutSTR(gamptrT gamptr, pgnstrT pgnstr, charptrT s) {
  if (gamptr->gam_strv[pgnstr] != NULL)
    EPDMemoryFree(gamptr->gam_strv[pgnstr]);
  gamptr->gam_strv[pgnstr] = EPDStringGrab(s);
  return;
}

/*--> EPDPGNGenSTR: return a string with the entire STR */
nonstatic charptrT EPDPGNGenSTR(gamptrT gamptr) {
  charptrT s;
  pgnstrT pgnstr;
  char tv[tL];

  s = EPDStringGrab("");
  for (pgnstr = 0; pgnstr < pgnstrL; pgnstr++) {
    sprintf(tv, "[%s \"%s\"]\n", pgnstrstrv[pgnstr],
        gamptr->gam_strv[pgnstr]);
    s = EPDStringAppendStr(s, tv);
  };
  return s;
}

/*--> EPDPGNHistory: generate a string for PGN version of history */
nonstatic charptrT EPDPGNHistory(gamptrT gamptr) {
  charptrT s;
  charptrT ms;

  s = EPDPGNGenSTR(gamptr);
  s = EPDStringAppendChar(s, '\n');
  ms = EPDMoveList(gamptr);
  s = EPDStringAppendStr(s, ms);
  EPDMemoryFree(ms);
  s = EPDStringAppendChar(s, '\n');
  return s;
}

/*--> EPDCopyOutPTP: copy STR from an EDP structure (ptp operation) */
nonstatic void EPDCopyOutPTP(gamptrT gamptr, epdptrT epdptr) {
  eopptrT eopptr;
  eovptrT eovptr;
  pgnstrT pgnstr;

  if (epdptr != NULL) {
    eopptr = EPDLocateEOPCode(epdptr, epdso_ptp);
    eovptr = eopptr->eop_headeov;
    while ((eovptr != NULL) && (eovptr->eov_next != NULL)) {
      pgnstr = EPDPGNFetchTagIndex(eovptr->eov_str);
      if (pgnstr != pgnstr_nil)
        EPDPGNPutSTR(gamptr, pgnstr, eovptr->eov_next->eov_str);
      eovptr = eovptr->eov_next->eov_next;
    };
  };
  return;
}

/*--> EPDFetchRefcomStr: return  pointer of indicated refcom string */
nonstatic charptrT EPDFetchRefcomStr(refcomT refcom) {
  return refcomstrv[refcom];
}

/*--> EPDFetchRefreqStr: return  pointer of indicated refreq string */
nonstatic charptrT EPDFetchRefreqStr(refreqT refreq) {
  return refreqstrv[refreq];
}

/*--> EPDFetchRefcomIndex: return a referee command index */
nonstatic refcomT EPDFetchRefcomIndex(charptrT s) {
  refcomT refcom;
  refcomT rcom;

  refcom = refcom_nil;
  rcom = 0;
  while ((refcom == refcom_nil) && (rcom < refcomL))
    if (strcmp(s, EPDFetchRefcomStr(rcom)) == 0)
      refcom = rcom;
    else
      rcom++;
  return refcom;
}

/*--> EPDExtractRefcomIndex: extract a referee command index */
nonstatic refreqT EPDExtractRefcomIndex(epdptrT epdptr) {
  refcomT refcom;
  eopptrT eopptr;
  eovptrT eovptr;

/* set default return value */
  refcom = refreq_nil;
  if (epdptr != NULL)
    if ((eopptr = EPDLocateEOPCode(epdptr, epdso_refcom)) != NULL)
      if ((eovptr = eopptr->eop_headeov) != NULL)
        refcom = EPDFetchRefcomIndex(eovptr->eov_str);
  return refcom;
}

/*--> EPDComm: slave to Duplex autoplay program */
nonstatic siT EPDComm(refintptrT refintptr, charptrT pipebase) {
  siT flag;
  siT done;
  siT flow;
  refcomT refcom;
  charptrT pfnv[flowL];
  fptrT pfptrv[flowL];
  epdptrT epdptr0, epdptr1;
  charptrT eptr;
  char ev[epdL];

/* set default result: success */
  flag = 1;
/* set up the EPD Kit for a new game */
  EPDInitArray();
/* generate pipe file names and clear their pointers */
  for (flow = 0; flow < flowL; flow++) {
    pfnv[flow] = EPDStringGrab(pipebase);
    pfnv[flow] = EPDStringAppendStr(pfnv[flow], ".pc");
    pfnv[flow] = EPDStringAppendChar(pfnv[flow], (char) (flow + '0'));
    pfptrv[flow] = NULL;
  };
/* pipe files already created by Duplex, attempt open */
  flow = 0;
  while (flag && (flow < flowL)) {
    pfptrv[flow] = fopen(pfnv[flow], "a+");
    if (pfptrv[flow] == NULL)
      flag = 0;
    else
      flow++;
  };
/* sign on to Duplex */
  if (flag) {
    epdptr0 = EPDGetCurrentPosition();
    EPDAddOpSym(epdptr0, epdso_refreq, refreqstrv[refreq_sign_on]);
    eptr = EPDEncode(epdptr0);
    EPDReleaseEPD(epdptr0);
    fprintf(pfptrv[0], "%s\n", eptr);
    fflush(pfptrv[0]);
    EPDMemoryFree(eptr);
  };
/* run event cycle loop */
  done = 0;
  while (flag && !done) {
/* read an incoming EPD message */
    if (fgets(ev, (epdL - 1), pfptrv[1]) == NULL)
      flag = 0;
    else {
/* decode the message */
      epdptr1 = EPDDecode(ev);
      if ((epdptr1 == NULL)
          || ((refcom = EPDExtractRefcomIndex(epdptr1)) == refcom_nil))
        flag = 0;
      else {
/* send the message to the callback routine */
        epdptr0 = (*refintptr) (epdptr1, &flag);
/* release input storage */
        EPDReleaseEPD(epdptr1);
/* construct and transmit output string */
        if (flag && (epdptr0 != NULL)) {
          eptr = EPDEncode(epdptr0);
          fprintf(pfptrv[0], "%s\n", eptr);
          fflush(pfptrv[0]);
          EPDMemoryFree(eptr);
        };
/* release output storage */
        if (epdptr0 != NULL)
          EPDReleaseEPD(epdptr0);
/* set the completion flag on disconnect */
        if (flag && !done && (refcom == refcom_disconnect))
          done = 1;
      };
    };
  };
/* close pipes and release pipe file names */
  for (flow = 0; flow < flowL; flow++) {
    if (pfptrv[flow] != NULL)
      fclose(pfptrv[flow]);
    EPDMemoryFree(pfnv[flow]);
  };
  return flag;
}

/*--> EPDInit: one time initialization for EPD */
nonstatic void EPDInit(void) {
  cpT cp;
  sqT sq;
  xsqT xsq;

/* this sets up the current position */
/* initialize ascii color conversion vector */
  asccv[c_w] = 'w';
  asccv[c_b] = 'b';
/* initialize ascii piece conversion vector */
  ascpv[p_p] = 'P';
  ascpv[p_n] = 'N';
  ascpv[p_b] = 'B';
  ascpv[p_r] = 'R';
  ascpv[p_q] = 'Q';
  ascpv[p_k] = 'K';
/* initialize ascii rank conversion vector */
  ascrv[rank_1] = '1';
  ascrv[rank_2] = '2';
  ascrv[rank_3] = '3';
  ascrv[rank_4] = '4';
  ascrv[rank_5] = '5';
  ascrv[rank_6] = '6';
  ascrv[rank_7] = '7';
  ascrv[rank_8] = '8';
/* initialize ascii file conversion vector */
  ascfv[file_a] = 'a';
  ascfv[file_b] = 'b';
  ascfv[file_c] = 'c';
  ascfv[file_d] = 'd';
  ascfv[file_e] = 'e';
  ascfv[file_f] = 'f';
  ascfv[file_g] = 'g';
  ascfv[file_h] = 'h';
/* initialize piece letter from special case move indicator */
  cv_p_scmvv[scmv_reg] = p_nil;
  cv_p_scmvv[scmv_epc] = p_nil;
  cv_p_scmvv[scmv_cks] = p_nil;
  cv_p_scmvv[scmv_cqs] = p_nil;
  cv_p_scmvv[scmv_ppn] = p_n;
  cv_p_scmvv[scmv_ppb] = p_b;
  cv_p_scmvv[scmv_ppr] = p_r;
  cv_p_scmvv[scmv_ppq] = p_q;
/* initialize various color piece conversion arrays */
  for (cp = cp_wp; cp <= cp_wk; cp++)
    cv_c_cpv[cp] = c_w;
  for (cp = cp_bp; cp <= cp_bk; cp++)
    cv_c_cpv[cp] = c_b;
  cv_c_cpv[cp_v0] = c_v;
  cv_c_cpv[cp_x0] = cv_c_cpv[cp_x1] = cv_c_cpv[cp_x2] = c_x;
  cv_p_cpv[cp_wp] = cv_p_cpv[cp_bp] = p_p;
  cv_p_cpv[cp_wn] = cv_p_cpv[cp_bn] = p_n;
  cv_p_cpv[cp_wb] = cv_p_cpv[cp_bb] = p_b;
  cv_p_cpv[cp_wr] = cv_p_cpv[cp_br] = p_r;
  cv_p_cpv[cp_wq] = cv_p_cpv[cp_bq] = p_q;
  cv_p_cpv[cp_wk] = cv_p_cpv[cp_bk] = p_k;
  cv_p_cpv[cp_v0] = p_v;
  cv_p_cpv[cp_x0] = cv_p_cpv[cp_x1] = cv_p_cpv[cp_x2] = p_x;
  cv_cp_c_pv[c_w][p_p] = cp_wp;
  cv_cp_c_pv[c_w][p_n] = cp_wn;
  cv_cp_c_pv[c_w][p_b] = cp_wb;
  cv_cp_c_pv[c_w][p_r] = cp_wr;
  cv_cp_c_pv[c_w][p_q] = cp_wq;
  cv_cp_c_pv[c_w][p_k] = cp_wk;
  cv_cp_c_pv[c_b][p_p] = cp_bp;
  cv_cp_c_pv[c_b][p_n] = cp_bn;
  cv_cp_c_pv[c_b][p_b] = cp_bb;
  cv_cp_c_pv[c_b][p_r] = cp_br;
  cv_cp_c_pv[c_b][p_q] = cp_bq;
  cv_cp_c_pv[c_b][p_k] = cp_bk;
  inv_cv[c_w] = c_b;
  inv_cv[c_b] = c_w;
/* initialize directional vectors */
  dvv[dx_0] = dv_0;
  dvv[dx_1] = dv_1;
  dvv[dx_2] = dv_2;
  dvv[dx_3] = dv_3;
  dvv[dx_4] = dv_4;
  dvv[dx_5] = dv_5;
  dvv[dx_6] = dv_6;
  dvv[dx_7] = dv_7;
  dvv[dx_8] = dv_8;
  dvv[dx_9] = dv_9;
  dvv[dx_a] = dv_a;
  dvv[dx_b] = dv_b;
  dvv[dx_c] = dv_c;
  dvv[dx_d] = dv_d;
  dvv[dx_e] = dv_e;
  dvv[dx_f] = dv_f;
  xdvv[dx_0] = xdv_0;
  xdvv[dx_1] = xdv_1;
  xdvv[dx_2] = xdv_2;
  xdvv[dx_3] = xdv_3;
  xdvv[dx_4] = xdv_4;
  xdvv[dx_5] = xdv_5;
  xdvv[dx_6] = xdv_6;
  xdvv[dx_7] = xdv_7;
  xdvv[dx_8] = xdv_8;
  xdvv[dx_9] = xdv_9;
  xdvv[dx_a] = xdv_a;
  xdvv[dx_b] = xdv_b;
  xdvv[dx_c] = xdv_c;
  xdvv[dx_d] = xdv_d;
  xdvv[dx_e] = xdv_e;
  xdvv[dx_f] = xdv_f;
/* initialize the extended board */
  for (xsq = 0; xsq < xsqL; xsq++)
    xb.xbv[xsq] = cp_x0;
  for (sq = sq_a1; sq <= sq_h8; sq++)
    xb.xbv[map_xsq_sq(sq)] = cp_v0;
/* initialize the standard opcode string vector */
  epdsostrv[epdso_acd] = "acd";
  epdsostrv[epdso_acn] = "acn";
  epdsostrv[epdso_acs] = "acs";
  epdsostrv[epdso_am] = "am";
  epdsostrv[epdso_bm] = "bm";
  epdsostrv[epdso_c0] = "c0";
  epdsostrv[epdso_c1] = "c1";
  epdsostrv[epdso_c2] = "c2";
  epdsostrv[epdso_c3] = "c3";
  epdsostrv[epdso_c4] = "c4";
  epdsostrv[epdso_c5] = "c5";
  epdsostrv[epdso_c6] = "c6";
  epdsostrv[epdso_c7] = "c7";
  epdsostrv[epdso_c8] = "c8";
  epdsostrv[epdso_c9] = "c9";
  epdsostrv[epdso_cc] = "cc";
  epdsostrv[epdso_ce] = "ce";
  epdsostrv[epdso_dm] = "dm";
  epdsostrv[epdso_draw_accept] = "draw_accept";
  epdsostrv[epdso_draw_claim] = "draw_claim";
  epdsostrv[epdso_draw_offer] = "draw_offer";
  epdsostrv[epdso_draw_reject] = "draw_reject";
  epdsostrv[epdso_eco] = "eco";
  epdsostrv[epdso_fmvn] = "fmvn";
  epdsostrv[epdso_hmvc] = "hmvc";
  epdsostrv[epdso_id] = "id";
  epdsostrv[epdso_nic] = "nic";
  epdsostrv[epdso_noop] = "noop";
  epdsostrv[epdso_pm] = "pm";
  epdsostrv[epdso_ptp] = "ptp";
  epdsostrv[epdso_pv] = "pv";
  epdsostrv[epdso_rc] = "rc";
  epdsostrv[epdso_refcom] = "refcom";
  epdsostrv[epdso_refreq] = "refreq";
  epdsostrv[epdso_resign] = "resign";
  epdsostrv[epdso_sm] = "sm";
  epdsostrv[epdso_sv] = "sv";
  epdsostrv[epdso_tcgs] = "tcgs";
  epdsostrv[epdso_tcri] = "tcri";
  epdsostrv[epdso_tcsi] = "tcsi";
  epdsostrv[epdso_ts] = "ts";
  epdsostrv[epdso_v0] = "v0";
  epdsostrv[epdso_v1] = "v1";
  epdsostrv[epdso_v2] = "v2";
  epdsostrv[epdso_v3] = "v3";
  epdsostrv[epdso_v4] = "v4";
  epdsostrv[epdso_v5] = "v5";
  epdsostrv[epdso_v6] = "v6";
  epdsostrv[epdso_v7] = "v7";
  epdsostrv[epdso_v8] = "v8";
  epdsostrv[epdso_v9] = "v9";
/* set the EPD refcom operand strings */
  refcomstrv[refcom_conclude] = "conclude";
  refcomstrv[refcom_disconnect] = "disconnect";
  refcomstrv[refcom_execute] = "execute";
  refcomstrv[refcom_fault] = "fault";
  refcomstrv[refcom_inform] = "inform";
  refcomstrv[refcom_respond] = "respond";
  refcomstrv[refcom_reset] = "reset";
/* set the EPD refreq operand strings */
  refreqstrv[refreq_fault] = "fault";
  refreqstrv[refreq_reply] = "reply";
  refreqstrv[refreq_sign_on] = "sign_on";
  refreqstrv[refreq_sign_off] = "sign_off";
/* set the PGN Seven Tag Roster names */
  pgnstrstrv[pgnstr_event] = "Event";
  pgnstrstrv[pgnstr_site] = "Site";
  pgnstrstrv[pgnstr_date] = "Date";
  pgnstrstrv[pgnstr_round] = "Round";
  pgnstrstrv[pgnstr_white] = "White";
  pgnstrstrv[pgnstr_black] = "Black";
  pgnstrstrv[pgnstr_result] = "Result";
/* set the game termination indication marker vector */
  gtimstrv[gtim_w] = "1-0";
  gtimstrv[gtim_b] = "0-1";
  gtimstrv[gtim_d] = "1/2-1/2";
  gtimstrv[gtim_u] = "*";
/* clear the global game chain anchor pointers */
  head_gamptr = tail_gamptr = NULL;
/* clear the token chain anchor pointers */
  head_tknptr = tail_tknptr = NULL;
/* clear the current ply */
  ply = 0;
/* allocate the move tree */
  treeptr = treebaseptr = (mptrT) EPDMemoryGrab(sizeof(mT) * treeL);
/* allocate the tree stack entry stack */
  tseptr = tsebaseptr = (tseptrT) EPDMemoryGrab(sizeof(tseT) * pmhL);
/* allocate the environment stack entry stack */
  eseptr = esebaseptr = (eseptrT) EPDMemoryGrab(sizeof(eseT) * pmhL);
/* set the current position to be the initial array */
  EPDInitArray();
/* generation */
  EPDGenMoves();
  return;
}

/*--> EPDTerm: one time termination for EPD */
nonstatic void EPDTerm(void) {
/* release any existing game chain */
  EPDReleaseGameChain();
/* release any existing token chain */
  EPDReleaseTokenChain();
/* deallocate various stacks */
  EPDMemoryFree(esebaseptr);
  EPDMemoryFree(tsebaseptr);
  EPDMemoryFree(treebaseptr);
/* "Wanna see my sprocket collection?" */
  return;
}
#endif
/*<<< epd.c: EOF */
