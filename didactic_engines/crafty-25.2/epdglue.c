/*>>> epdglue.c: glue to connect Crafty to the EPD Kit routines */
#if defined(EPD)
/* Revised: 1996.04.21 */
/*
 Copyright (C) 1996 by Steven J. Edwards (sje@mv.mv.com)
 All rights reserved.  This code may be freely redistibuted and used by
 both research and commerical applications.  No warranty exists.
 */
/*
 The contents of this source file form the programmatic glue between
 the host program Crafty and the EPD Kit.  Therefore, this file will
 have to be changed if used with a different host program.  Also, the
 contents of the prototype include file (epdglue.h) may also require
 modification for a different host.
 The contents of the other source files in the EPD Kit (epddefs.h,
 epd.h, and epd.c) should not have to be changed for different hosts.
 */
/*
 This file was originally prepared on an Apple Macintosh using the
 Metrowerks CodeWarrior 6 ANSI C compiler.  Tabs are set at every
 four columns.  Further testing and development was performed on a
 generic PC running Linux 1.2.9 and using the gcc 2.6.3 compiler.
 */
/* system includes */
#  if defined(UNIX)
#    include <unistd.h>
#  endif
#  include <ctype.h>
#  include <time.h>
#  if !defined(UNIX)
#    include <process.h>
#  endif
/* Crafty includes */
#  include "chess.h"
#  include "data.h"
/* EPD Kit definitions (host program independent) */
#  include "epddefs.h"
/* EPD Kit routine prototypes (host program independent) */
#  include "epd.h"
/* prototypes for this file (host program dependent) */
#  include "epdglue.h"
/* EPD glue command type */
typedef siT egcommT, *egcommptrT;

#  define egcommL 26
#  define egcomm_nil (-1)
#  define egcomm_epdapgn  0 /* append a PGN game to a file */
#  define egcomm_epdbfix  1 /* fix file for Bookup import */
#  define egcomm_epdcert  2 /* display certain evaluation (if possible) */
#  define egcomm_epdcics  3 /* slave to an Internet Chess Server */
#  define egcomm_epdcomm  4 /* slave to the Duplex referee program */
#  define egcomm_epddpgn  5 /* display the current game in PGN */
#  define egcomm_epddsml  6 /* display SAN move list */
#  define egcomm_epddstr  7 /* display PGN Seven Tag Roster */
#  define egcomm_epddtpv  8 /* display PGN tag pair value */
#  define egcomm_epdenum  9 /* enumerate EPD file */
#  define egcomm_epdhelp 10 /* display EPD help */
#  define egcomm_epdlink 11 /* slave to the Argus referee program */
#  define egcomm_epdlpgn 12 /* load a PGN game from a file */
#  define egcomm_epdlrec 13 /* load an EPD record form a file */
#  define egcomm_epdmore 14 /* more help */
#  define egcomm_epdnoop 15 /* no operation */
#  define egcomm_epdpfdn 16 /* process file: data normalization */
#  define egcomm_epdpfdr 17 /* process file: data repair */
#  define egcomm_epdpfga 18 /* process file: general analysis */
#  define egcomm_epdpflc 19 /* process file: locate cooks */
#  define egcomm_epdpfop 20 /* process file: operation purge */
#  define egcomm_epdscor 21 /* score EPD benchmark result file */
#  define egcomm_epdshow 22 /* show EPD four fields for current position */
#  define egcomm_epdspgn 23 /* save a PGN game to a file */
#  define egcomm_epdstpv 24 /* set PGN tag pair value */
#  define egcomm_epdtest 25 /* developer testing */
/* output text buffer */
#  define tbufL 256
static char tbufv[tbufL];

/* EPD glue command strings */
static charptrT egcommstrv[egcommL];

/* EPD glue command string descriptions */
static charptrT eghelpstrv[egcommL];

/* EPD glue command parameter counts (includes command token) */
/* the current (default) EPD game structure */
static gamptrT default_gamptr;

/*--> EGPrint: print a string to the output */
static
void EGPrint(charptrT s) {
/* this is an internal EPD glue routine */
/*
 This routine is provided as an alternative to direct writing to the
 standard output.  All EPD glue printing output goes through here.  The
 idea is that the host program may have some special requirements for
 printing output (like a window display), so a convenient single point
 is provided to handle this.
 Note that there is no corresponding routine for reading from the
 standard input because the EPD glue does no interactive reading, except
 for a single getchar() call in the epdhelp display pager.
 */
/* for Crafty, the standard output is used */
  printf("%s", s);
  return;
}

/*--> EGPrintTB: print the contents of the text buffer */
static
void EGPrintTB(void) {
/* this is an internal EPD glue routine */
  EGPrint(tbufv);
  return;
}

/*--> EGPL: print a string followed by a newline */
static
void EGPL(charptrT s) {
/* this is an internal EPD glue routine */
  EGPrint(s);
  EGPrint("\n");
  return;
}

/*--> EGLocateCommand: locate an EPD glue command from a token */
static egcommT EGLocateCommand(charptrT s) {
  egcommT egcomm, index;

/* this is an internal EPD glue routine */
/* set the default return value: no match */
  egcomm = egcomm_nil;
/* scan the EPD glue command string vector */
  index = 0;
  while ((index < egcommL) && (egcomm == egcomm_nil))
    if (strcmp(s, egcommstrv[index]) == 0)
      egcomm = index;
    else
      index++;
  return egcomm;
}

/*--> EGMapFromHostColor: map a color from the host to the EPD style */
static cT EGMapFromHostColor(siT color) {
  cT c;

/* this is an internal glue routine */
/* map from Crafty's color representation */
  if (color == 1)
    c = c_w;
  else
    c = c_b;
  return c;
}

/*--> EGMapToHostColor: map a color to the host from the EPD style */
static siT EGMapToHostColor(cT c) {
  siT color;

/* this is an internal glue routine */
/* map to Crafty's color representation */
  if (c == c_w)
    color = 1;
  else
    color = 0;
  return color;
}

/*--> EGMapFromHostCP: map a color piece from the host to the EPD style */
static cpT EGMapFromHostCP(siT hostcp) {
  cpT cp = 0;

/* this is an internal glue routine */
/* map from Crafty's color-piece representation */
  switch (hostcp) {
    case -queen:
      cp = cp_bq;
      break;
    case -rook:
      cp = cp_br;
      break;
    case -bishop:
      cp = cp_bb;
      break;
    case -king:
      cp = cp_bk;
      break;
    case -knight:
      cp = cp_bn;
      break;
    case -pawn:
      cp = cp_bp;
      break;
    case 0:
      cp = cp_v0;
      break;
    case pawn:
      cp = cp_wp;
      break;
    case knight:
      cp = cp_wn;
      break;
    case king:
      cp = cp_wk;
      break;
    case bishop:
      cp = cp_wb;
      break;
    case rook:
      cp = cp_wr;
      break;
    case queen:
      cp = cp_wq;
      break;
  };
  return cp;
}

/*--> EGMapToHostCP: map a color piece to the host from the EPD style */
static siT EGMapToHostCP(cpT cp) {
  siT hostcp = 0;

/* this is an internal glue routine */
/* map to Crafty's color-piece representation */
  switch (cp) {
    case cp_wp:
      hostcp = pawn;
      break;
    case cp_wn:
      hostcp = knight;
      break;
    case cp_wb:
      hostcp = bishop;
      break;
    case cp_wr:
      hostcp = rook;
      break;
    case cp_wq:
      hostcp = queen;
      break;
    case cp_wk:
      hostcp = king;
      break;
    case cp_bp:
      hostcp = -pawn;
      break;
    case cp_bn:
      hostcp = -knight;
      break;
    case cp_bb:
      hostcp = -bishop;
      break;
    case cp_br:
      hostcp = -rook;
      break;
    case cp_bq:
      hostcp = -queen;
      break;
    case cp_bk:
      hostcp = -king;
      break;
    case cp_v0:
      hostcp = 0;
      break;
  };
  return hostcp;
}

/*--> EGMapFromHostSq: map square index from host style */
static sqT EGMapFromHostSq(siT index) {
  sqT sq;

/* this is an internal glue routine */
/* Crafty's square index is the same as the EPD Kit square index */
  sq = index;
  return sq;
}

/*--> EGMapToHostSq: map square index to host style */
static siT EGMapToHostSq(sqT sq) {
  siT index;

/* this is an internal glue routine */
/* Crafty's square index is the same as the EPD Kit square index */
  index = sq;
  return index;
}

/*--> EGMapFromHostScore: map score from host style */
static cpevT EGMapFromHostScore(liT score) {
  cpevT cpev;
  liT distance;

/* this is an internal EPD glue routine */
/* check for a forced mate */
  if (score >= (MATE - MAXPLY)) {
/* convert forced mate score */
    distance = (MATE - score) / 2;
    cpev = synth_mate(distance);
  } else if (score <= (MAXPLY - MATE)) {
/* convert forced loss score */
    distance = (MATE + score) / 2;
    cpev = synth_loss(distance);
  } else {
/* convert regular score */
    cpev = score;
  };
  return cpev;
}

/*--> EGMapFromHostMove: map move from host style to EPD style */
static mT EGMapFromHostMove(liT move) {
  mT m;
  siT flag;

/* this is an internal EPD glue routine */
/* the EPD current position must be properly set */
  m.m_flag = 0;
  m.m_frsq = EGMapFromHostSq((siT) From(move));
  m.m_tosq = EGMapFromHostSq((siT) To(move));
  m.m_frcp = EPDFetchCP(m.m_frsq);
  m.m_tocp = EPDFetchCP(m.m_tosq);
/* determine special case move indication */
  flag = 0;
  if (!flag)
    if ((m.m_frcp == cp_wk) && (m.m_frsq == sq_e1) && (m.m_tosq == sq_g1)) {
      m.m_scmv = scmv_cks;
      flag = 1;
    };
  if (!flag)
    if ((m.m_frcp == cp_bk) && (m.m_frsq == sq_e8) && (m.m_tosq == sq_g8)) {
      m.m_scmv = scmv_cks;
      flag = 1;
    };
  if (!flag)
    if ((m.m_frcp == cp_wk) && (m.m_frsq == sq_e1) && (m.m_tosq == sq_c1)) {
      m.m_scmv = scmv_cqs;
      flag = 1;
    };
  if (!flag)
    if ((m.m_frcp == cp_bk) && (m.m_frsq == sq_e8) && (m.m_tosq == sq_c8)) {
      m.m_scmv = scmv_cqs;
      flag = 1;
    };
  if (!flag)
    if ((m.m_frcp == cp_wp) && (m.m_tosq == EPDFetchEPSQ())) {
      m.m_scmv = scmv_epc;
      flag = 1;
    };
  if (!flag)
    if ((m.m_frcp == cp_bp) && (m.m_tosq == EPDFetchEPSQ())) {
      m.m_scmv = scmv_epc;
      flag = 1;
    };
  if (!flag)
    if (Promote(move) != 0) {
      switch (Promote(move)) {
        case knight:
          m.m_scmv = scmv_ppn;
          break;
        case bishop:
          m.m_scmv = scmv_ppb;
          break;
        case rook:
          m.m_scmv = scmv_ppr;
          break;
        case queen:
          m.m_scmv = scmv_ppq;
          break;
      };
      flag = 1;
    };
  if (!flag)
    m.m_scmv = scmv_reg;
  return m;
}

/*--> EGGetIndexedHostPosition: copy from indexed host position */
static
void EGGetIndexedHostPosition(TREE * RESTRICT tree, siT posdex, int active) {
  sqT sq;
  rbT rb;
  cT actc;
  castT cast;
  sqT epsq;
  siT hmvc;
  siT fmvn;

/* this is an internal EPD glue routine */
/*
 This routine is called from within the EPD glue to copy the host program's
 current position at the given dpeth into the EPD Kit.  Information about
 the previous EPD Kit current position is lost.
 */
/* read from the host piece placement */
  for (sq = sq_a1; sq <= sq_h8; sq++)
    rb.rbv[sq] = EGMapFromHostCP(tree->position.board[EGMapToHostSq(sq)]);
/* read from the host piece active color */
  actc = EGMapFromHostColor((siT) active);
/* read from the host piece castling availability */
  cast = 0;
  switch (tree->status[posdex].castle[1]) {
    case 0:
      break;
    case 1:
      cast |= cf_wk;
      break;
    case 2:
      cast |= cf_wq;
      break;
    case 3:
      cast |= cf_wk | cf_wq;
      break;
  };
  switch (tree->status[posdex].castle[0]) {
    case 0:
      break;
    case 1:
      cast |= cf_bk;
      break;
    case 2:
      cast |= cf_bq;
      break;
    case 3:
      cast |= cf_bk | cf_bq;
      break;
  };
/* read from the host piece en passant target square */
  epsq = sq_nil;
  if (tree->status[posdex].enpassant_target != 0) {
    sq = sq_a1;
    while ((epsq == sq_nil) && (sq <= sq_h8))
      if (tree->status[posdex].enpassant_target == EGMapToHostSq(sq))
        epsq = sq;
      else
        sq++;
  };
/* read from the host halfmove clock */
  hmvc = tree->status[posdex].reversible;
/* read from the host fullmove number */
  fmvn = move_number;
/* set the EPD current position */
  EPDSetCurrentPosition(&rb, actc, cast, epsq, hmvc, fmvn);
  return;
}

/*--> EGGetHostPosition: copy from host position to EPD Kit position */
static
void EGGetHostPosition(void) {
/* this is an internal EPD glue routine */
/*
 This routine is called from within the EPD glue to copy the host program's
 current position into the EPD Kit.  Information about the previous EPD Kit
 current position is lost.
 */
/* transfer from ply zero host position */
  EGGetIndexedHostPosition(block[0], 0, game_wtm);
  return;
}

/*--> EGPutHostPosition: copy from EPD Kit position to host position */
static
void EGPutHostPosition(void) {
  sqT sq;
  rbT rb;
  cT actc;
  castT cast;
  sqT epsq;
  siT hmvc;
  siT fmvn;
  siT index;
  TREE *tree = block[0];

/* this is an internal EPD glue routine */
/*
 This routine is called from within the EPD glue to copy the EPD Kit's current
 position into the host program.  If the previous host program current position
 is different from the new position, then information about the previous host
 program current position is lost.  This means that the host program preserves
 history information if and only if such preservation is appropriate.
 Actually, the host position data is completely overwritten, so the above
 comment is temporarily false, but will be true as developement proceeds.
 */
/* fetch the EPD current position data items */
  rb = *EPDFetchBoard();
  actc = EPDFetchACTC();
  cast = EPDFetchCAST();
  epsq = EPDFetchEPSQ();
  hmvc = EPDFetchHMVC();
  fmvn = EPDFetchFMVN();
/* copy the board into the host board */
  for (sq = sq_a1; sq <= sq_h8; sq++)
    tree->position.board[EGMapToHostSq(sq)] = EGMapToHostCP(rb.rbv[sq]);
/* copy the active color */
  game_wtm = EGMapToHostColor(actc);
/* copy the castling availibility */
  tree->status[0].castle[1] = 0;
  if (cast & cf_wk)
    tree->status[0].castle[1] += 1;
  if (cast & cf_wq)
    tree->status[0].castle[1] += 2;
  tree->status[0].castle[0] = 0;
  if (cast & cf_bk)
    tree->status[0].castle[0] += 1;
  if (cast & cf_bq)
    tree->status[0].castle[0] += 2;
/* copy the en passant target square */
  if (epsq == sq_nil)
    tree->status[0].enpassant_target = 0;
  else
    tree->status[0].enpassant_target = EGMapToHostSq(epsq);
/* copy the halfmove clock */
  tree->status[0].reversible = hmvc;
/* copy the fullmove number */
  move_number = fmvn;
/* set secondary host data items */
  SetChessBitBoards(tree);
  rep_index = 0;
  tree->rep_list[0] = HashKey;
  moves_out_of_book = 0;
  last_mate_score = 0;
/* clear the host killer information */
  for (index = 0; index < MAXPLY; index++) {
    tree->killers[index].move1 = 0;
    tree->killers[index].move2 = 0;
  }
/* clear miscellaneous host items */
  ponder_move = 0;
  last_pv.pathd = 0;
  last_pv.pathl = 0;
  over = 0;
  return;
}

/*--> EGEncodeHostHistory: generate a string from host move history */
static charptrT EGEncodeHostHistory(void) {
  charptrT sptr;
  gamptrT gamptr;
  pgnstrT pgnstr;
  mptrT mptr;
  siT flag;
  siT ch = 0;
  charptrT s;
  mT m;

/* this works only for games starting form the initial position */
/* set default return value: failure */
  sptr = NULL;
/* set okay  */
  flag = 1;
/* reset the EPD current position */
  EPDReset();
/* make sure the host history file exists */
  if (history_file == NULL)
    flag = 0;
  else {
    rewind(history_file);
    ch = fgetc(history_file);
  };
/* open a game structure */
  gamptr = EPDGameOpen();
/* copy tag data from default game structure */
  if (default_gamptr != NULL)
    for (pgnstr = 0; pgnstr < pgnstrL; pgnstr++)
      EPDPGNPutSTR(gamptr, pgnstr, EPDPGNGetSTR(default_gamptr, pgnstr));
/* copy GTIM from default game structure */
  if (default_gamptr != NULL)
    EPDPutGTIM(gamptr, EPDGetGTIM(default_gamptr));
/* read the host history file */
  while (flag && (ch != EOF)) {
/* skip whitespace */
    while ((ch != EOF) && isspace(ch))
      ch = fgetc(history_file);
/* if not EOF, then construct a move */
    if (ch != EOF) {
/* attach the first character */
      s = EPDStringGrab("");
      s = EPDStringAppendChar(s, (char) ch);
      ch = fgetc(history_file);
/* attach the remaining characters */
      while ((ch != EOF) && !isspace(ch)) {
        s = EPDStringAppendChar(s, (char) ch);
        ch = fgetc(history_file);
      };
/* match the move */
      EPDGenMoves();
      mptr = EPDSANDecodeAux(s, 0);
      if (mptr == NULL)
        flag = 0;
      else {
/* execute the move */
        m = *mptr;
        EPDGameAppendMove(gamptr, &m);
        EPDExecuteUpdate(&m);
        EPDCollapse();
      };
/* release move buffer storage */
      EPDMemoryFree(s);
    };
  };
/* construct the string representation of the game */
  if (flag)
    sptr = EPDPGNHistory(gamptr);
/* clean up if there was a problem */
  if (!flag)
    if (sptr != NULL) {
      EPDMemoryFree(sptr);
      sptr = NULL;
    };
/* close the game structure */
  EPDGameClose(gamptr);
  return sptr;
}

/*--> EGIterate: wrapper function for host Iterate() call */
static
int EGIterate(siT wtm_flag, siT think_flag) {
  int value;
  epdptrT epdptr;
  TREE *tree = block[0];

  EPDGenMoves();
/* save current kit position */
  epdptr = EPDGetCurrentPosition();
/* more than one move, execute regular search */
  value = Iterate(wtm_flag, think_flag, 0);
/* restore kit position */
  EPDRealize(epdptr);
  EPDReleaseEPD(epdptr);
  EPDGenMoves();
  last_pv = tree->pv[0];
  last_value = value;
  return value;
}

/*--> EGCommHandler: callback routine for handling Duplex events */
static epdptrT EGCommHandler(epdptrT epdptr0, siptrT flagptr) {
  epdptrT epdptr1;
  eopptrT eopptr;
  charptrT s;
  fptrT fptr;
  mptrT mptr;
  mT m;
  sanT san;
  int move;
  char tv[tL];
  TREE *tree = block[0];

/* set flag: no errors (so far) */
  *flagptr = 1;
/* clear result */
  epdptr1 = NULL;
/* process according to input referee command */
  switch (EPDExtractRefcomIndex(epdptr0)) {
    case refcom_conclude:
/* end of game */
      s = EPDPGNHistory(default_gamptr);
      if (s == NULL)
        *flagptr = 0;
      else {
/* append game to PGN output file */
        sprintf(tv, "c%05hd.pgn", ((siT) getpid()));
        fptr = fopen(tv, "a");
        if (fptr == NULL)
          *flagptr = 0;
        else {
          fprintf(fptr, "%s", s);
          fclose(fptr);
        };
        EPDMemoryFree(s);
      };
/* clean up and remove the temporary history file */
      if (history_file != NULL) {
        fclose(history_file);
        history_file = NULL;
      };
      sprintf(tv, "h%05hd.pml", ((siT) getpid()));
      remove(tv);
/* close the game structure */
      if (default_gamptr != NULL) {
        EPDGameClose(default_gamptr);
        default_gamptr = NULL;
      };
      break;
    case refcom_disconnect:
/* channel shutdown */
/* clean up and remove the temporary history file */
      if (history_file != NULL) {
        fclose(history_file);
        history_file = NULL;
      };
      sprintf(tv, "h%05hd.pml", ((siT) getpid()));
      remove(tv);
/* ensure game structure is closed */
      if (default_gamptr != NULL) {
        EPDGameClose(default_gamptr);
        default_gamptr = NULL;
      };
      break;
    case refcom_execute:
/* execute the supplied move */
      eopptr = EPDLocateEOPCode(epdptr0, epdso_sm);
      move = InputMove(tree, 0, game_wtm, 0, 0, eopptr->eop_headeov->eov_str);
      if (history_file) {
        fseek(history_file, ((((move_number - 1) * 2) + 1 - game_wtm) * 10),
            SEEK_SET);
        fprintf(history_file, "%9s\n", eopptr->eop_headeov->eov_str);
      }
      MakeMoveRoot(tree, game_wtm, move);
      game_wtm = Flip(game_wtm);
      if (game_wtm)
        move_number++;
/* execute the move in the EPD Kit */
      EPDGenMoves();
      mptr = EPDSANDecodeAux(eopptr->eop_headeov->eov_str, 0);
      m = *mptr;
      EPDGameAppendMove(default_gamptr, &m);
      EPDExecuteUpdate(&m);
      EPDCollapse();
      break;
    case refcom_fault:
/* a problem occurred */
      EGPL("EPDCommHandler: refcom fault");
      *flagptr = 0;
      break;
    case refcom_inform:
/* process incidental information */
      EPDCopyOutPTP(default_gamptr, epdptr0);
/* update the current game termination marker */
      s = EPDPGNGetSTR(default_gamptr, pgnstr_result);
      if ((s == NULL) || (strcmp(s, "*") == 0))
        EPDPutGTIM(default_gamptr, gtim_u);
      else if (strcmp(s, "1-0") == 0)
        EPDPutGTIM(default_gamptr, gtim_w);
      else if (strcmp(s, "0-1") == 0)
        EPDPutGTIM(default_gamptr, gtim_b);
      else if (strcmp(s, "1/2-1/2") == 0)
        EPDPutGTIM(default_gamptr, gtim_d);
      break;
    case refcom_respond:
/* execute the supplied move (if any) */
      eopptr = EPDLocateEOPCode(epdptr0, epdso_sm);
      if (eopptr != NULL) {
        move =
            InputMove(tree, 0, game_wtm, 0, 0, eopptr->eop_headeov->eov_str);
        if (history_file) {
          fseek(history_file, ((((move_number - 1) * 2) + 1 - game_wtm) * 10),
              SEEK_SET);
          fprintf(history_file, "%9s\n", eopptr->eop_headeov->eov_str);
        }
        MakeMoveRoot(tree, game_wtm, move);
        game_wtm = Flip(game_wtm);
        if (game_wtm)
          move_number++;
/* execute the move in the EPD Kit */
        EPDGenMoves();
        mptr = EPDSANDecodeAux(eopptr->eop_headeov->eov_str, 0);
        m = *mptr;
        EPDGameAppendMove(default_gamptr, &m);
        EPDExecuteUpdate(&m);
        EPDCollapse();
      };
/* calculate move + respond */
      EPDGenMoves();
      if (EPDFetchMoveCount() > 0) {
/* at least one move exists, set up for search */
        ponder_move = 0;
        thinking = 1;
        last_pv.pathd = 0;
        last_pv.pathl = 0;
        tree->status[1] = tree->status[0];
/* search */
        EGIterate((siT) game_wtm, (siT) think);
/* process search result */
        strcpy(tv, OutputMove(tree, 0, game_wtm, last_pv.path[1]));
        move = last_pv.path[1];
/* locate SAN move */
        mptr = EPDSANDecodeAux(tv, 0);
        m = *mptr;
        EPDSANEncode(&m, san);
/* output to temporary history file */
        if (history_file) {
          fseek(history_file, ((((move_number - 1) * 2) + 1 - game_wtm) * 10),
              SEEK_SET);
          fprintf(history_file, "%9s\n", san);
        }
/* update host position */
        MakeMoveRoot(tree, game_wtm, move);
        game_wtm = Flip(game_wtm);
        if (game_wtm)
          move_number++;
/* create reply EPD structure */
        epdptr1 = EPDGetCurrentPosition();
/* add in referee request */
        EPDAddOpSym(epdptr1, epdso_refreq, EPDFetchRefreqStr(refreq_reply));
/* add in supplied move */
        EPDAddOpSym(epdptr1, epdso_sm, san);
/* execute the move in the EPD Kit */
        mptr = EPDSANDecodeAux(san, 0);
        m = *mptr;
        EPDGameAppendMove(default_gamptr, &m);
        EPDExecuteUpdate(&m);
        EPDCollapse();
      } else {
/* no moves exist, so no move response possible */
        epdptr1 = EPDGetCurrentPosition();
        eopptr = EPDCreateEOPCode(epdso_refreq);
        EPDAppendEOV(eopptr,
            EPDCreateEOVSym(EPDFetchRefreqStr(refreq_reply)));
        EPDAppendEOP(epdptr1, eopptr);
      };
      break;
    case refcom_reset:
/* reset EPD Kit */
      EPDReset();
/* reset host for a new game */
      ponder = 0;
      ponder_move = 0;
      last_pv.pathd = 0;
      last_pv.pathl = 0;
      InitializeChessBoard(tree);
      InitializeHashTables(0);
      game_wtm = 1;
      move_number = 1;
/* open the temporary history file */
      if (history_file != NULL) {
        fclose(history_file);
        history_file = NULL;
      };
      sprintf(tv, "h%05hd.pml", ((siT) getpid()));
      remove(tv);
      history_file = fopen(tv, "w+");
/* open the current game structure */
      if (default_gamptr != NULL)
        EPDGameClose(default_gamptr);
      default_gamptr = EPDGameOpen();
      break;
    case refcom_nil:
      *flagptr = 0;
      break;
  };
/* clean up if there was a problem */
  if (!(*flagptr))
    if (epdptr1 != NULL) {
      EPDReleaseEPD(epdptr1);
      epdptr1 = NULL;
    };
  return epdptr1;
}

/*--> EGProcessAPGN: process the EG command epdapgn */
static siT EGProcessAPGN(void) {
  siT flag;
  charptrT s;
  fptrT fptr;

/* this is an internal EPD glue routine */
/* set the default return value: success */
  flag = 1;
/* parameter count check */
  if (EPDTokenCount() != 2) {
    EGPL("This command takes one parameter.");
    flag = 0;
  };
/* process the epdapgn command */
  if (flag) {
    s = EGEncodeHostHistory();
    if (s == NULL)
      flag = 0;
    else {
      fptr = fopen(EPDTokenFetch(1), "a");
      if (fptr == NULL)
        flag = 0;
      else {
        fprintf(fptr, "%s", s);
        fclose(fptr);
      };
      EPDMemoryFree(s);
    };
  };
  return flag;
}

/*--> EGProcessBFIX: process the EG command epdbfix */
static siT EGProcessBFIX(void) {
  siT flag;
  fptrT fptr0, fptr1;
  eopptrT eopptr;
  epdptrT epdptr, nptr;
  charptrT s;
  char ev[epdL];

/* this is an internal EPD glue routine */
/* set the default return value: success */
  flag = 1;
/* clear the file pointers */
  fptr0 = fptr1 = NULL;
/* parameter count check */
  if (EPDTokenCount() != 3) {
    EGPL("This command takes two parameters");
    flag = 0;
  };
/* set up the input file */
  if (flag) {
    fptr0 = fopen(EPDTokenFetch(1), "r");
    if (fptr0 == NULL) {
      sprintf(tbufv, "Can't open %s for reading\n", EPDTokenFetch(1));
      EGPrintTB();
      flag = 0;
    };
  };
/* set up the output file */
  if (flag) {
    fptr1 = fopen(EPDTokenFetch(2), "w");
    if (fptr1 == NULL) {
      sprintf(tbufv, "Can't open %s for writing\n", EPDTokenFetch(2));
      EGPrintTB();
      flag = 0;
    };
  };
/* scan the file */
  if (flag)
    while (flag && (fgets(ev, (epdL - 1), fptr0) != NULL)) {
/* decode the record into an EPD structure */
      epdptr = EPDDecode(ev);
/* check record decode validity */
      if (epdptr == NULL)
        flag = 0;
      else {
/* clone the input EPD structure base */
        nptr = EPDCloneEPDBase(epdptr);
/* copy the ce operation */
        eopptr = EPDLocateEOPCode(epdptr, epdso_ce);
        if (eopptr != NULL)
          EPDAppendEOP(nptr, EPDCloneEOP(eopptr));
/* copy the pv operation */
        eopptr = EPDLocateEOPCode(epdptr, epdso_pv);
        if (eopptr != NULL)
          EPDAppendEOP(nptr, EPDCloneEOP(eopptr));
/* output the new EPD strucutre */
        s = EPDEncode(nptr);
        fprintf(fptr1, "%s\n", s);
        EPDMemoryFree(s);
/* deallocate both EPD structures */
        EPDReleaseEPD(epdptr);
        EPDReleaseEPD(nptr);
      };
    };
/* ensure file close */
  if (fptr0 != NULL)
    fclose(fptr0);
  if (fptr1 != NULL)
    fclose(fptr1);
  return flag;
}

/*--> EGProcessCICS: process the EG command epdcics */
static siT EGProcessCICS(void) {
  siT flag;

/* this is an internal EPD glue routine */
/* set the default return value: success */
  flag = 1;
/* parameter count check */
  if (EPDTokenCount() != 3) {
    EGPL("This command takes two parameters");
    flag = 0;
  };
/* process the epdcics command */
  if (flag) {
    EGPL("This command is not yet implemented.");
    flag = 0;
  };
  return flag;
}

/*--> EGProcessCOMM: process the EG command epdcomm */
static siT EGProcessCOMM(void) {
  siT flag;
  fptrT save_history_file;
  gamptrT save_default_gamptr;

/* this is an internal EPD glue routine */
/* set the default return value: success */
  flag = 1;
/* parameter count check */
  if (EPDTokenCount() != 2) {
    EGPL("This command takes one parameter.");
    flag = 0;
  };
/* process the epdcomm command */
  if (flag) {
/* save the history file pointer */
    save_history_file = history_file;
    history_file = NULL;
/* save the game structure */
    save_default_gamptr = default_gamptr;
    default_gamptr = NULL;
/* process via callback */
    EGPL("Duplex slave mode begin");
    flag = EPDComm(EGCommHandler, EPDTokenFetch(1));
    EGPL("Duplex slave mode end");
/* restore the game structure */
    if (default_gamptr != NULL) {
      EPDGameClose(default_gamptr);
      default_gamptr = NULL;
    };
    default_gamptr = save_default_gamptr;
/* restore the history file pointer */
    history_file = save_history_file;
  };
  return flag;
}

/*--> EGProcessDPGN: process the EG command epddpgn */
static siT EGProcessDPGN(void) {
  siT flag;
  charptrT s;

/* this is an internal EPD glue routine */
/* set the default return value: success */
  flag = 1;
/* parameter count check */
  if (EPDTokenCount() != 1) {
    EGPL("This command takes no parameters.");
    flag = 0;
  };
/* process the epddpgn command */
  if (flag) {
    s = EGEncodeHostHistory();
    if (s == NULL)
      flag = 0;
    else {
      EGPrint(s);
      EPDMemoryFree(s);
    };
  };
  return flag;
}

/*--> EGProcessDSML: process the EG command epddsml */
static siT EGProcessDSML(void) {
  siT flag;
  siT count, index;
  siT length, column;
  mT m;
  sanT san;

/* this is an internal EPD glue routine */
/* set the default return value: success */
  flag = 1;
/* parameter count check */
  if (EPDTokenCount() != 1) {
    EGPL("This command takes no parameters.");
    flag = 0;
  };
/* process the epddsml command */
  if (flag) {
/* copy the current host position into the EPD Kit */
    EGGetHostPosition();
/* generate  and count */
    EPDGenMoves();
    count = EPDFetchMoveCount();
    switch (count) {
      case 0:
        EGPL("No moves are available.");
        break;
      case 1:
        EGPL("There is 1 move available.");
        break;
      default:
        sprintf(tbufv, "There are %hd moves available.\n", count);
        EGPrintTB();
        break;
    };
/* list moves */
    if (count > 0) {
      column = 0;
      EPDSortSAN();
      for (index = 0; index < count; index++) {
        m = *EPDFetchMove(index);
        EPDSANEncode(&m, san);
        length = strlen(san);
        if ((column + 1 + length) < columnL) {
          sprintf(tbufv, " %s", san);
          EGPrintTB();
          column += 1 + length;
        } else {
          EGPrint("\n");
          EGPrint(san);
          column = length;
        };
      };
      if (column > 0)
        EGPrint("\n");
    };
  };
  return flag;
}

/*--> EGProcessDSTR: process the EG command epddstr */
static siT EGProcessDSTR(void) {
  siT flag;
  charptrT s;

/* this is an internal EPD glue routine */
/* set the default return value: success */
  flag = 1;
/* parameter count check */
  if (EPDTokenCount() != 1) {
    EGPL("This command takes no parameters.");
    flag = 0;
  };
/* process the epddstr command */
  if (flag) {
    s = EPDPGNGenSTR(default_gamptr);
    EGPrint(s);
    EPDMemoryFree(s);
  };
  return flag;
}

/*--> EGProcessDTPV: process the EG command epddtpv */
static siT EGProcessDTPV(void) {
  siT flag;
  pgnstrT pgnstr;

/* this is an internal EPD glue routine */
/* set the default return value: success */
  flag = 1;
/* parameter count check */
  if (EPDTokenCount() != 2) {
    EGPL("This command takes one parameter.");
    flag = 0;
  };
/* process the epddtpv command */
  if (flag) {
    pgnstr = EPDPGNFetchTagIndex(EPDTokenFetch(1));
    if (pgnstr == pgnstr_nil) {
      EGPL("Unknown tag name; available tag names are:");
      for (pgnstr = 0; pgnstr < pgnstrL; pgnstr++) {
        EGPrint("   ");
        EGPrint(EPDPGNFetchTagName(pgnstr));
      };
      EGPrint("\n");
      flag = 0;
    } else {
      sprintf(tbufv, "[%s \"%s\"]\n", EPDPGNFetchTagName(pgnstr),
          EPDPGNGetSTR(default_gamptr, pgnstr));
      EGPrintTB();
    };
  };
  return flag;
}

/*--> EGProcessENUM: process the EG command epdenum */
static siT EGProcessENUM(void) {
  siT flag;
  liT total;

/* this is an internal EPD glue routine */
/* set the default return value: success */
  flag = 1;
/* parameter count check */
  if (EPDTokenCount() != 4) {
    EGPL("This command takes three parameters");
    flag = 0;
  };
/* process the epdenum command */
  if (flag) {
    flag =
        EPDEnumerateFile((siT) atol(EPDTokenFetch(1)), EPDTokenFetch(2),
        EPDTokenFetch(3), &total);
    if (flag) {
      sprintf(tbufv, "Enumeration to depth %ld totals %ld\n",
          atol(EPDTokenFetch(1)), total);
      EGPrintTB();
    };
  };
  return flag;
}

/*--> EGProcessHELP: process the EG command epdhelp */
static siT EGProcessHELP(void) {
  siT flag;
  siT i;
  egcommT egcomm;

/* this is an internal EPD glue routine */
/* set the default return value: success */
  flag = 1;
/* parameter count check */
  if (EPDTokenCount() != 1) {
    EGPL("This command takes no parameters.");
    flag = 0;
  };
/* process the epdhelp command */
  if (flag) {
/* it is not clear exactly why the next statment is needed */
    getchar();
/* list all the commands */
    egcomm = 0;
    while (egcomm < egcommL) {
      EGPL("Available EPD glue command list");
      EGPL("-------------------------------");
      i = 0;
      while ((egcomm < egcommL) && (i < 10)) {
        sprintf(tbufv, "%s: %s\n", egcommstrv[egcomm], eghelpstrv[egcomm]);
        EGPrintTB();
        i++;
        egcomm++;
      };
      if (egcomm < egcommL) {
        EGPL("");
        EGPL("Press <return> for more command help");
        getchar();
        EGPL("");
      };
    };
  };
  return flag;
}

/*--> EGProcessLINK: process the EG command epdlink */
static siT EGProcessLINK(void) {
  siT flag;

/* this is an internal EPD glue routine */
/* set the default return value: success */
  flag = 1;
/* parameter count check */
  if (EPDTokenCount() != 3) {
    EGPL("This command takes two parameters");
    flag = 0;
  };
/* process the epdlink command */
  if (flag) {
    EGPL("This command is not yet implemented.");
    flag = 0;
  };
  return flag;
}

/*--> EGProcessLPGN: process the EG command epdlpgn */
static siT EGProcessLPGN(void) {
  siT flag;

/* this is an internal EPD glue routine */
/* set the default return value: success */
  flag = 1;
/* parameter count check */
  if (EPDTokenCount() != 2) {
    EGPL("This command takes one parameter.");
    flag = 0;
  };
/* process the epdlpgn command */
  if (flag) {
    EGPL("This command is not yet implemented.");
    flag = 0;
  };
  return flag;
}

/*--> EGProcessLREC: process the EG command epdlrec */
static siT EGProcessLREC(void) {
  siT flag;
  fptrT fptr;
  liT i, n;
  epdptrT epdptr;
  char ev[epdL];

/* this is an internal EPD glue routine */
/* set the default return value: success */
  flag = 1;
/* parameter count check */
  if (EPDTokenCount() != 3) {
    EGPL("This command takes two parameters");
    flag = 0;
  };
/* process the epdlrec command */
  if (flag) {
    fptr = fopen(EPDTokenFetch(1), "r");
    if (fptr == NULL)
      flag = 0;
    else {
/* read the indicated record */
      i = 0;
      n = atol(EPDTokenFetch(2));
      while (flag && (i < n))
        if (fgets(ev, (epdL - 1), fptr) == NULL)
          flag = 0;
        else
          i++;
      fclose(fptr);
/* set the current position */
      if (flag) {
        epdptr = EPDDecode(ev);
        if (epdptr == NULL)
          flag = 0;
        else {
          EPDRealize(epdptr);
          EPDReleaseEPD(epdptr);
          EGPutHostPosition();
        };
      };
    };
  };
  return flag;
}

/*--> EGProcessMORE: process the EG command epdmore */
static siT EGProcessMORE(void) {
  siT flag;

/* this is an internal EPD glue routine */
/* set the default return value: success */
  flag = 1;
/* parameter count check */
  if (EPDTokenCount() != 2) {
    EGPL("This command takes one parameter.");
    flag = 0;
  };
/* process the epdmore command */
  if (flag)
    switch (EGLocateCommand(EPDTokenFetch(1))) {
      case egcomm_epdapgn:
        EGPL("epdapgn: Append the game to a PGN file");
        EGPL("");
        EGPL("This command is used to append the current game to a file");
        EGPL("using Portable Game Notation (PGN) format.  It takes one");
        EGPL("parameter which is the file name of interest.");
        break;
      case egcomm_epdbfix:
        EGPL("epdbfix: Fix an EPD file for import into Bookup");
        EGPL("");
        EGPL("This command takes two parameters.  Both parameters are");
        EGPL("names of EPD files.  The first is an existing EPD file with");
        EGPL("analysis data.  The second is the EPD file to be created");
        EGPL("from the first by removing all operations except the ce and");
        EGPL("pv operations.  This second file can then be imported into");
        EGPL("the Bookup program.");
        break;
      case egcomm_epdcert:
        EGPL("epdcert: Try to display a certain evaluation");
        EGPL("");
        EGPL("This command takes no parameters.  The current position is");
        EGPL("examined for legality, and if legal, an attempt is made to");
        EGPL("return an absolute certain evaluation.  An absolute certain");
        EGPL("evaluation is available for checkmates, stalemates, draws by");
        EGPL("insufficient material, and mate in one positions.  Also,");
        EGPL("if there are only a few chessmen on the board, then the");
        EGPL("current position is used as an index into an external file");
        EGPL("corresponding to the active color and the distribution of");
        EGPL("pieces.  If the appropriate external file is available, the");
        EGPL("position is evaluated by a single file look-up operation.");
        EGPL("These files are called tablebases and they should appear in");
        EGPL("the TB subdirectory of the current working directory.  These");
        EGPL("files may be obtained via ftp from the chess.onenet.net site");
        EGPL("in the pub/chess/TB directory.");
        break;
      case egcomm_epdcics:
        EGPL("epdcics: Slave to an Internet Chess Server");
        EGPL("");
        EGPL("This command causes the host program Crafty to becomed slaved");
        EGPL("to an Internet Chess Server (ICS).  This means that the host");
        EGPL("program will follow directions from the indicated Internet");
        EGPL("Chess Server until either the ICS tells the program to exit");
        EGPL("slave mode or the program is interrupted by the user.  This");
        EGPL("command takes two parameters.  The first is the name of the");
        EGPL("machine running the ICS and the second is the port number in");
        EGPL("use by the ICS.");
        break;
      case egcomm_epdcomm:
        EGPL("epdcomm: Slave to the Duplex referee program");
        EGPL("");
        EGPL("This command causes the host program Crafty to becomed slaved");
        EGPL("to the simple referee program Duplex.  Duplex must be started");
        EGPL("prior to entering this command.  Once the host program is");
        EGPL("slaved to Duplex, it will remain so until it is instructed");
        EGPL("by Duplex to terminate or is interrupted by the user.  This");
        EGPL("command is used to support the Duplex function of controlling");
        EGPL("automatic play between two host programs on the same Unix");
        EGPL("system.  The communication used is based on named pipes which");
        EGPL("are special files that support FIFO data operation.  This");
        EGPL("command takes one parameter which is a base name for a pair");
        EGPL("of named pipes.  The host output pipe name is generated by");
        EGPL("adding a \".pc0\" suffix to the base name and the host input");
        EGPL("pipe name is generated by addint a \".pc1\" suffix to the");
        EGPL("base name.  (Both pipe files are created and later removed");
        EGPL("automatically by the Duplex program.)  The Duplex program is");
        EGPL("given three parameters to run a series of games between two");
        EGPL("host programs: the first is the base name given to the first");
        EGPL("host program, the second is the base name given to the second");
        EGPL("host program, and the third is the count of the number of");
        EGPL("games to be played.");
        break;
      case egcomm_epddpgn:
        EGPL("epddpgn: Display the game using PGN");
        EGPL("");
        EGPL("This command displays the current game using Portable Game");
        EGPL("Notation (PGN).  It takes no parameters.");
        break;
      case egcomm_epddsml:
        EGPL("epddsml: Display SAN move list");
        EGPL("");
        EGPL("This command displays the ASCII sorted list of available");
        EGPL("moves using SAN (Standard Algebraic Notation).  The count");
        EGPL("of moves is also displayed.  This command takes no");
        EGPL("parameters.");
        break;
      case egcomm_epddstr:
        EGPL("epddstr: Display the PGN Seven Tag Roster");
        EGPL("");
        EGPL("This command displays the current values of each of the tag");
        EGPL("pairs of the PGN Seven Tag Roster.  These tags are: Event, ");
        EGPL("Site, Date, Round, White, Black, and Result.  This command");
        EGPL("takes no parameters.");
        break;
      case egcomm_epddtpv:
        EGPL("epddtpv: Display a PGN tag pair value");
        EGPL("");
        EGPL("This command displays the current value for the indicated");
        EGPL("PGN tag pair.  These available tag pair names are: Event, ");
        EGPL("Site, Date, Round, White, Black, and Result.  This command");
        EGPL("takes a single parameter which is the name of the tag pair");
        EGPL("to be displayed.");
        break;
      case egcomm_epdenum:
        EGPL("epdenum: Enumerate positions in an EPD file");
        EGPL("");
        EGPL("This command takes three parameters.  The first is a non-");
        EGPL("negative integer that gives a ply depth limit.  The second");
        EGPL("is an input EPD file with positions to be enumerated to the");
        EGPL("given depth.  The third parameter is the result EPD file");
        EGPL("with enumeration data given with the acd, acn, and acs");
        EGPL("operations.  The grand total of all enumerations is printed");
        EGPL("to the standard output.");
        break;
      case egcomm_epdhelp:
        EGPL("epdhelp: Display available epdglue commands");
        EGPL("");
        EGPL("This command takes no parameters.  It displays a list");
        EGPL("of all the epdglue commands with a single line description");
        EGPL("of each command.  The epdmore command gives a detailed");
        EGPL("description of a specified command.");
        break;
      case egcomm_epdlink:
        EGPL("epdlink: Slave to the Argus moderator program");
        EGPL("");
        EGPL("This command used to slave the host program to the Argus");
        EGPL("tournament referee program.  It takes two parameters.  The");
        EGPL("first is the name of the machine running the Argus referee");
        EGPL("and the second is the port number in use by Argus on the");
        EGPL("indicated machine.  The Argus program must be started prior");
        EGPL("to using this command.  Argus acts as a general tournament");
        EGPL("supervisor and mediator for any number of chessplaying");
        EGPL("programs competing in a compuer chess event.  It supplies");
        EGPL("the participating program entrants with everything needed");
        EGPL("for an entire tournament.");
        break;
      case egcomm_epdlpgn:
        EGPL("epdlpgn: Load the game from a PGN file");
        EGPL("");
        EGPL("This command loads a single game from a Portable Game");
        EGPL("Notation (PGN) file.  If there is more than one game in the");
        EGPL("file, then only the first one is loaded.  This command tales");
        EGPL("a single parameter which is the name of the file containing");
        EGPL("the PGN data of interest.");
        break;
      case egcomm_epdlrec:
        EGPL("epdlpgn: Load a selected EPD record from a file");
        EGPL("");
        EGPL("This command loads an EPD record from a file and causes it");
        EGPL("to become the current position in the host program.  This");
        EGPL("command takes two parameters.  The first is the name of the");
        EGPL("file with EPD records and the second is the record number");
        EGPL("of interest.  The epdlrec command is intended to assist with");
        EGPL("benchmark analysis by making it easy to select and load the");
        EGPL("Nth record of a benchmark file.");
        break;
      case egcomm_epdmore:
        EGPL("epdmore: Display detailed help for a given command");
        EGPL("");
        EGPL("This command takes a single parameter which is the name");
        EGPL("of one of the available epdglue commands.  A brief paragraph");
        EGPL("of helpful assistance is displayed.");
        break;
      case egcomm_epdnoop:
        EGPL("epdnoop: No operation");
        EGPL("");
        EGPL("This command performs no operation.  It is provided for");
        EGPL("development purposes.");
        break;
      case egcomm_epdpfdn:
        EGPL("epdpfdn: Process file: data normalization");
        EGPL("");
        EGPL("This command takes two parameters.  The first is the name of");
        EGPL("an input EPD data file.  The second is the name of the EPD");
        EGPL("output file to be produced from normalizing the input.  The");
        EGPL("normalization process produces a canonical external");
        EGPL("representation for each EPD input record.");
        break;
      case egcomm_epdpfdr:
        EGPL("epdpfdr: Process file: data repair");
        EGPL("");
        EGPL("This command takes two parameters.  The first is the name of");
        EGPL("an input EPD data file.  The second is the name of the EPD");
        EGPL("output file to be produced from repairing the input.  The");
        EGPL("repair process attempts to map all chess move data present");
        EGPL("in the input into Standard Algebraic Notation.  This repair");
        EGPL("effort affects the am, bm, pm, pv, sm, and sv operations.");
        break;
      case egcomm_epdpfga:
        EGPL("epdpfga: Process file: general analysis");
        EGPL("");
        EGPL("This command takes two parameters.  The first is the name of");
        EGPL("an input EPD data file.  The second is the name of the EPD");
        EGPL("output file to be produced from applying general analysis");
        EGPL("to each position in the input file.  The output analysis is");
        EGPL("contained in the acd, acn, acs, ce, and pv operations.");
        break;
      case egcomm_epdpflc:
        EGPL("epdpflc: Process file: locate cooks");
        EGPL("");
        EGPL("This command is used to scan an EPD file and report on any");
        EGPL("checkmating cooks.  The signle parameter is the name of the");
        EGPL("EPD file to be scanned.  Each record is scanned and if the");
        EGPL("centipawn evaluation indicates a forced mate, then the move");
        EGPL("appearing as the result of a search is checked against the");
        EGPL("moves appearing in the bm (best moves) operation on the same");
        EGPL("record.  If the result move does not appear in the bm list,");
        EGPL("then the record is reported as a cook.");
        break;
      case egcomm_epdpfop:
        EGPL("epdpfop: Process file: operation purge");
        EGPL("");
        EGPL("This command takes threee parameters.  The first is the name");
        EGPL("of an EPD operation mnemonic to be purged.  The second is the");
        EGPL("name fo the input EPD file, and the third is the name of the");
        EGPL("EPD output to be produced by purging the specified operation");
        EGPL("from the input file.");
        break;
      case egcomm_epdscor:
        EGPL("epdscor: Score EPD analysis file");
        EGPL("");
        EGPL("This command takes one parameter.  It is the name of an input");
        EGPL("EPD data file containing analysis result data.  The input");
        EGPL("data analysis is scanned and a brief statistical report is");
        EGPL("displayed.");
        break;
      case egcomm_epdshow:
        EGPL("epdshow: Show EPD four fields for the current position");
        EGPL("");
        EGPL("This command takes no parameters.  It causes the EPD four");
        EGPL("data fields for the current position to be displayed.");
        break;
      case egcomm_epdspgn:
        EGPL("epdspgn: Save the game to a PGN file");
        EGPL("");
        EGPL("This command is used to save the current game to a file");
        EGPL("using Portable Game Notation (PGN) format.  It takes one");
        EGPL("parameter which is the file name of interest.  The previous");
        EGPL("copy of the file, if any, is overwritten.");
        break;
      case egcomm_epdstpv:
        EGPL("epdstpv: Set a PGN tag pair value");
        EGPL("");
        EGPL("This command sets the current value for the indicated");
        EGPL("PGN tag pair.  These available tag pair names are: Event, ");
        EGPL("Site, Date, Round, White, Black, and Result.  This command");
        EGPL("takes two parameters.  The first which is the name of the");
        EGPL("tag pair of interest and the second is the value to be");
        EGPL("assigned.  Underscore characters in the value are mapped");
        EGPL("to blanks; this allows embedded spaces to appear in the");
        EGPL("received value.");
        break;
      case egcomm_epdtest:
        EGPL("epdtest: Developer testing");
        EGPL("");
        EGPL("This command takes no parameters.  It is used for developer");
        EGPL("testing purposes.");
        break;
      case egcomm_nil:
        sprintf(tbufv, "Unknown command: %s\n", EPDTokenFetch(1));
        EGPrintTB();
        flag = 0;
        break;
    };
  return flag;
}

/*--> EGProcessNOOP: process the EG command epdnoop */
static siT EGProcessNOOP(void) {
  siT flag;

/* this is an internal EPD glue routine */
/* set the default return value: success */
  flag = 1;
/* process the epdnoop command */
  return flag;
}

/*--> EGProcessPFDN: process the EG command epdpfdn */
static siT EGProcessPFDN(void) {
  siT flag;

/* this is an internal EPD glue routine */
/* set the default return value: success */
  flag = 1;
/* parameter count check */
  if (EPDTokenCount() != 3) {
    EGPL("This command takes two parameters");
    flag = 0;
  };
/* process the epdpfdn command */
  if (flag)
    flag = EPDNormalizeFile(EPDTokenFetch(1), EPDTokenFetch(2));
  return flag;
}

/*--> EGProcessPFDR: process the EG command epdpfdr */
static siT EGProcessPFDR(void) {
  siT flag;

/* this is an internal EPD glue routine */
/* set the default return value: success */
  flag = 1;
/* parameter count check */
  if (EPDTokenCount() != 3) {
    EGPL("This command takes two parameters");
    flag = 0;
  };
/* process the epdpfdr command */
  if (flag)
    flag = EPDRepairFile(EPDTokenFetch(1), EPDTokenFetch(2));
  return flag;
}

/*--> EGProcessPFGA: process the EG command epdpfga */
static siT EGProcessPFGA(void) {
  siT flag;
  fptrT fptr0, fptr1;
  liT record;
  time_t start_time;
  liT result;
  siT index;
  liT host_acd, host_acn, host_acs;
  cpevT host_ce;
  charptrT s;
  liT move;
  mT m;
  sanT san;
  eovptrT eovptr;
  eopptrT eopptr;
  epdptrT epdptr;
  char ev[epdL];
  TREE *tree = block[0];

/* this is an internal EPD glue routine */
/* set the default return value: success */
  flag = 1;
/* clear the file pointers */
  fptr0 = fptr1 = NULL;
/* parameter count check */
  if (EPDTokenCount() != 3) {
    EGPL("This command takes two parameters");
    flag = 0;
  };
/* set up the input file */
  if (flag) {
    fptr0 = fopen(EPDTokenFetch(1), "r");
    if (fptr0 == NULL) {
      sprintf(tbufv, "Can't open %s for reading\n", EPDTokenFetch(1));
      EGPrintTB();
      flag = 0;
    };
  };
/* set up the output file */
  if (flag) {
    fptr1 = fopen(EPDTokenFetch(2), "w");
    if (fptr1 == NULL) {
      sprintf(tbufv, "Can't open %s for writing\n", EPDTokenFetch(2));
      EGPrintTB();
      flag = 0;
    };
  };
/* scan the file */
  if (flag) {
/* initialize the record count */
    record = 0;
/* read one record per loop */
    while (flag && (fgets(ev, (epdL - 1), fptr0) != NULL)) {
/* decode the record into an EPD structure */
      epdptr = EPDDecode(ev);
/* check record decode validity */
      if (epdptr == NULL)
        flag = 0;
      else {
/* set up the position in the EPD Kit */
        EPDRealize(epdptr);
/* legality test */
        if (!EPDIsLegal())
          flag = 0;
        else {
/* output record information to the display */
          sprintf(tbufv, "PFGA: EPD record: %ld", (record + 1));
          EGPrintTB();
          if (((eopptr = EPDLocateEOPCode(epdptr, epdso_id)) != NULL)
              && ((eovptr = eopptr->eop_headeov) != NULL)
              && ((s = eovptr->eov_str) != NULL)) {
            EGPrint(" ID: ");
            EGPrint(s);
          };
          EGPrint("\n");
/* output record information to the log file */
          if (log_file != NULL) {
            fprintf(log_file, "PFGA: EPD record: %ld", (record + 1));
            if (((eopptr = EPDLocateEOPCode(epdptr, epdso_id)) != NULL)
                && ((eovptr = eopptr->eop_headeov) != NULL)
                && ((s = eovptr->eov_str) != NULL))
              fprintf(log_file, "   ID: %s", s);
            fprintf(log_file, "\n");
          };
/* set up the host current position */
          EGPutHostPosition();
/* set host search parameters */
          tree->status[1] = tree->status[0];
          iteration = 0;
          ponder = 0;
/* get the starting time */
          start_time = time(NULL);
/* run host search; EPD Kit position may be changed */
          result = EGIterate(EGMapToHostColor(EPDFetchACTC()), think);
/* refresh the EPD Kit current position */
          EGGetHostPosition();
/* extract analysis count: depth */
          host_acd = iteration;
          if (host_acd == 0)
            host_acd = 1;
/* insert analysis count: depth */
          EPDAddOpInt(epdptr, epdso_acd, host_acd);
/* extract analysis count: nodes */
          host_acn = tree->nodes_searched;
          if (host_acn == 0)
            host_acn = 1;
/* insert analysis count: nodes */
          EPDAddOpInt(epdptr, epdso_acn, host_acn);
/* extract analysis count: seconds */
          host_acs = time(NULL) - start_time;
          if (host_acs == 0)
            host_acs = 1;
/* insert analysis count: seconds */
          EPDAddOpInt(epdptr, epdso_acs, host_acs);
/* extract centipawn evaluation */
          host_ce = EGMapFromHostScore(result);
/*
          host_ce = (EGMapToHostColor(EPDFetchACTC())) ? result : -result;
*/
/* insert centipawn evaluation */
          EPDAddOpInt(epdptr, epdso_ce, host_ce);
/* delete predicted move */
          EPDDropIfLocEOPCode(epdptr, epdso_pm);
/* extract/insert predicted variation */
          EPDDropIfLocEOPCode(epdptr, epdso_pv);
          eopptr = EPDCreateEOPCode(epdso_pv);
          for (index = 1; index < (int) tree->pv[0].pathl; index++) {
/* generate moves for the current position */
            EPDGenMoves();
/* fetch the predicted move at this ply */
            move = tree->pv[0].path[index];
/* map the host move to EPD style */
            m = EGMapFromHostMove(move);
/* set the flag bits in the EPD move */
            EPDSetMoveFlags(&m);
/* construct the SAN for the move */
            EPDSANEncode(&m, san);
/* create and append the SAN move */
            eovptr = EPDCreateEOVSym(san);
            EPDAppendEOV(eopptr, eovptr);
/* execute the move to update the EPD position */
            EPDExecuteUpdate(&m);
          };
/* retract predicted variation moves */
          EPDRetractAll();
/* append the pv operation to the EPD structure */
          EPDAppendEOP(epdptr, eopptr);
/* encode the EPD into a string, write, and release */
          s = EPDEncode(epdptr);
          fprintf(fptr1, "%s\n", s);
          fflush(fptr1);
          EPDMemoryFree(s);
        };
/* deallocate the EPD structure */
        EPDReleaseEPD(epdptr);
      };
/* increment EPD record count */
      record++;
    };
  };
/* ensure file close */
  if (fptr0 != NULL)
    fclose(fptr0);
  if (fptr1 != NULL)
    fclose(fptr1);
  return flag;
}

/*--> EGProcessPFLC: process the EG command epdpflc */
static siT EGProcessPFLC(void) {
  siT flag;
  fptrT fptr;
  epdptrT epdptr;
  eopptrT eopptr;
  eovptrT eovptr;
  liT record;
  charptrT s;
  char ev[epdL];

/* this is an internal EPD glue routine */
/* set the default return value: success */
  flag = 1;
/* parameter count check */
  if (EPDTokenCount() != 2) {
    EGPL("This command takes one parameter.");
    flag = 0;
  };
/* clear the file pointer */
  fptr = NULL;
/* set up the input file */
  if (flag) {
    fptr = fopen(EPDTokenFetch(1), "r");
    if (fptr == NULL) {
      sprintf(tbufv, "Can't open %s for reading\n", EPDTokenFetch(1));
      EGPrintTB();
      flag = 0;
    };
  };
/* scan the file */
  if (flag) {
/* clear record count */
    record = 0;
/* scan each record */
    while (flag && (fgets(ev, (epdL - 1), fptr) != NULL)) {
/* decode the record into an EPD structure */
      epdptr = EPDDecode(ev);
/* check record decode validity */
      if (epdptr == NULL)
        flag = 0;
      else {
/* did the expectation indicate a forced mate? */
        if (((eopptr = EPDLocateEOPCode(epdptr, epdso_ce)) != NULL)
            && ((eovptr = eopptr->eop_headeov) != NULL)
            && forced_mate(atol(eovptr->eov_str))) {
/* get the analysis result move from the pv */
          eopptr = EPDLocateEOPCode(epdptr, epdso_pv);
          if ((eopptr != NULL) && ((eovptr = eopptr->eop_headeov) != NULL)) {
/* get a pointer to the result move */
            s = eovptr->eov_str;
/* get the best move operation */
            eopptr = EPDLocateEOPCode(epdptr, epdso_bm);
            if (eopptr != NULL) {
/* check for a matching best move */
              eovptr = EPDLocateEOV(eopptr, s);
/* if not found, then it's a cook */
              if (eovptr == NULL) {
/* issue report */
                sprintf(tbufv, "PFLC: record %ld:   cook: %s", (record + 1),
                    s);
                EGPrintTB();
                eopptr = EPDLocateEOPCode(epdptr, epdso_id);
                if ((eopptr != NULL)
                    && ((eovptr = eopptr->eop_headeov) != NULL)) {
                  EGPrint("   ID: ");
                  EGPrint(eovptr->eov_str);
                };
                EGPrint("\n");
              };
            };
          };
        };
/* deallocate the EPD structure */
        EPDReleaseEPD(epdptr);
      };
/* next record */
      record++;
    };
  };
/* ensure input file closed */
  if (fptr != NULL)
    fclose(fptr);
  return flag;
}

/*--> EGProcessPFOP: process the EG command epdpfop */
static siT EGProcessPFOP(void) {
  siT flag;

/* this is an internal EPD glue routine */
/* set the default return value: success */
  flag = 1;
/* parameter count check */
  if (EPDTokenCount() != 4) {
    EGPL("This command takes three parameters");
    flag = 0;
  };
/* process the epdpfop command */
  if (flag)
    flag =
        EPDPurgeOpFile(EPDTokenFetch(1), EPDTokenFetch(2), EPDTokenFetch(3));
  return flag;
}

/*--> EGProcessSCOR: process the EG command epdscor */
static siT EGProcessSCOR(void) {
  siT flag;
  bmsT bms;

/* this is an internal EPD glue routine */
/* set the default return value: success */
  flag = 1;
/* parameter count check */
  if (EPDTokenCount() != 2) {
    EGPL("This command takes one parameter.");
    flag = 0;
  };
/* process the epdscor command */
  if (flag) {
    flag = EPDScoreFile(EPDTokenFetch(1), &bms);
    if (flag) {
      sprintf(tbufv, "total: %ld\n", bms.bms_total);
      EGPrintTB();
      if (bms.bms_total != 0) {
        if (bms.bms_acdflag) {
          sprintf(tbufv, "total acd: %ld   mean total acd: %.2f\n",
              bms.bms_total_acd, ((lrT) bms.bms_total_acd / bms.bms_total));
          EGPrintTB();
        };
        if (bms.bms_acnflag) {
          sprintf(tbufv, "total acn: %ld   mean total acn: %.2f\n",
              bms.bms_total_acn, ((lrT) bms.bms_total_acn / bms.bms_total));
          EGPrintTB();
        };
        if (bms.bms_acsflag) {
          sprintf(tbufv, "total acs: %ld   mean total acs: %.2f\n",
              bms.bms_total_acs, ((lrT) bms.bms_total_acs / bms.bms_total));
          EGPrintTB();
        };
        if (bms.bms_acnflag && bms.bms_acsflag)
          if (bms.bms_total_acs != 0) {
            sprintf(tbufv, "total mean node frequency: %.2f Hz\n",
                ((lrT) bms.bms_total_acn / bms.bms_total_acs));
            EGPrintTB();
          };
        sprintf(tbufv, "solve: %ld   solve percent: %.2f\n", bms.bms_solve,
            (((lrT) bms.bms_solve * 100.0) / bms.bms_total));
        EGPrintTB();
        if (bms.bms_solve != 0) {
          if (bms.bms_acdflag) {
            sprintf(tbufv, "solve acd: %ld   mean solve acd: %.2f\n",
                bms.bms_solve_acd, ((lrT) bms.bms_solve_acd / bms.bms_solve));
            EGPrintTB();
          };
          if (bms.bms_acnflag) {
            sprintf(tbufv, "solve acn: %ld   mean solve acn: %.2f\n",
                bms.bms_solve_acn, ((lrT) bms.bms_solve_acn / bms.bms_solve));
            EGPrintTB();
          };
          if (bms.bms_acsflag) {
            sprintf(tbufv, "solve acs: %ld   mean solve acs: %.2f\n",
                bms.bms_solve_acs, ((lrT) bms.bms_solve_acs / bms.bms_solve));
            EGPrintTB();
          };
          if (bms.bms_acnflag && bms.bms_acsflag)
            if (bms.bms_solve_acs != 0) {
              sprintf(tbufv, "solve mean node frequency: %.2f Hz\n",
                  ((lrT) bms.bms_solve_acn / bms.bms_solve_acs));
              EGPrintTB();
            };
        };
        sprintf(tbufv, "unsol: %ld   unsol percent: %.2f\n", bms.bms_unsol,
            (((lrT) bms.bms_unsol * 100.0) / bms.bms_total));
        EGPrintTB();
        if (bms.bms_unsol != 0) {
          if (bms.bms_acdflag) {
            sprintf(tbufv, "unsol acd: %ld   mean unsol acd: %.2f\n",
                bms.bms_unsol_acd, ((lrT) bms.bms_unsol_acd / bms.bms_unsol));
            EGPrintTB();
          };
          if (bms.bms_acnflag) {
            sprintf(tbufv, "unsol acn: %ld   mean unsol acn: %.2f\n",
                bms.bms_unsol_acn, ((lrT) bms.bms_unsol_acn / bms.bms_unsol));
            EGPrintTB();
          };
          if (bms.bms_acsflag) {
            sprintf(tbufv, "unsol acs: %ld   mean unsol acs: %.2f\n",
                bms.bms_unsol_acs, ((lrT) bms.bms_unsol_acs / bms.bms_unsol));
            EGPrintTB();
          };
          if (bms.bms_acnflag && bms.bms_acsflag)
            if (bms.bms_unsol_acs != 0) {
              sprintf(tbufv, "unsol mean node frequency: %.2f Hz\n",
                  ((lrT) bms.bms_unsol_acn / bms.bms_unsol_acs));
              EGPrintTB();
            };
        };
      };
    };
  };
  return flag;
}

/*--> EGProcessSHOW: process the EG command epdshow */
static siT EGProcessSHOW(void) {
  siT flag;
  charptrT s;

/* this is an internal EPD glue routine */
/* set the default return value: success */
  flag = 1;
/* parameter count check */
  if (EPDTokenCount() != 1) {
    EGPL("This command takes no parameters.");
    flag = 0;
  };
/* process the epdshow command */
  if (flag) {
/* load the host pointion into the EPD Kit */
    EGGetHostPosition();
/* get the EPD string for the current position */
    s = EPDGenBasicCurrent();
/* print and release */
    sprintf(tbufv, "%s\n", s);
    EGPrintTB();
    EPDMemoryFree(s);
  };
  return flag;
}

/*--> EGProcessSPGN: process the EG command epdspgn */
static siT EGProcessSPGN(void) {
  siT flag;
  charptrT s;
  fptrT fptr;

/* this is an internal EPD glue routine */
/* set the default return value: success */
  flag = 1;
/* parameter count check */
  if (EPDTokenCount() != 2) {
    EGPL("This command takes one parameter.");
    flag = 0;
  };
/* process the epdspgn command */
  if (flag) {
    s = EGEncodeHostHistory();
    if (s == NULL)
      flag = 0;
    else {
      fptr = fopen(EPDTokenFetch(1), "w");
      if (fptr == NULL)
        flag = 0;
      else {
        fprintf(fptr, "%s", s);
        fclose(fptr);
      };
      EPDMemoryFree(s);
    };
  };
  return flag;
}

/*--> EGProcessSTPV: process the EG command epdstpv */
static siT EGProcessSTPV(void) {
  siT flag;
  pgnstrT pgnstr;
  charptrT s;
  liT i, length;

/* this is an internal EPD glue routine */
/* set the default return value: success */
  flag = 1;
/* parameter count check */
  if (EPDTokenCount() != 3) {
    EGPL("This command takes two parameters");
    flag = 0;
  };
/* process the epdstpv command */
  if (flag) {
    pgnstr = EPDPGNFetchTagIndex(EPDTokenFetch(1));
    if (pgnstr == pgnstr_nil) {
      EGPL("Unknown tag name; available tag names are:");
      for (pgnstr = 0; pgnstr < pgnstrL; pgnstr++) {
        EGPrint("   ");
        EGPrint(EPDPGNFetchTagName(pgnstr));
      };
      EGPrint("\n");
      flag = 0;
    } else {
/* change underscore characters to blanks */
      s = EPDStringGrab(EPDTokenFetch(2));
      length = strlen(s);
      for (i = 0; i < length; i++)
        if (*(s + i) == '_')
          *(s + i) = ' ';
      EPDPGNPutSTR(default_gamptr, pgnstr, s);
      EPDMemoryFree(s);
    };
  };
  return flag;
}

/*--> EGProcessTEST: process the EG command epdtest */
static siT EGProcessTEST(void) {
  siT flag;

/* this is an internal EPD glue routine */
/* set the default return value: success */
  flag = 1;
/* parameter count check */
  if (EPDTokenCount() != 1) {
    EGPL("This command takes no parameters.");
    flag = 0;
  };
/* process the epdtest command */
  if (flag) {
    EGPL("This command is not yet implemented.");
    flag = 0;
  };
  return flag;
}

/*--> EGCommandCheck: check if a string starts with an EPD command token */
nonstatic int EGCommandCheck(char *s) {
  siT flag;
  charptrT tokenptr;
  egcommT egcomm;

/* this routine is called from Option() in option.c */
/*
 This routine is used to quickly check if an input command line starts with
 one of the available EPD glue command tokens.  The idea is that EPD action
 can be quickly selected (or deselected) for any command input text string.
 Because Crafty distributes command input one token at a time, the calls to
 this routine in this implementation will only have a single token for input.
 Other implementations that use an entire input command line will use the
 full power of this routine which includes token scanning and recording
 which is handled by the EPD Kit routines in the epd.c file.
 */
/* set default return value: no match */
  flag = 0;
/* tokenize the input string */
  EPDTokenize(s);
/* was there at least one token? */
  if (EPDTokenCount() > 0) {
/* get a pointer to the first token (origin zero) */
    tokenptr = EPDTokenFetch(0);
/* get the glue command indicator */
    egcomm = EGLocateCommand(tokenptr);
/* was there a command match? */
    if (egcomm != egcomm_nil)
      flag = 1;
  };
  return flag;
}

/*--> EGCommand: process an EPD command string */
nonstatic int EGCommand(char *s) {
  siT flag;
  egcommT egcomm;

/* this routine is called from Option() in option.c */
/*
 This routine actviates EPD glue command processing.  it is called with a
 string that represents an entire EPD glue command input, including any
 parameters following the command.
 Because Crafty distributes command input one token at a time, it is
 necessary for the host calling code to assemble a command line from the
 input token stream.  See the code in Option() for details.
 */
/* set the default return value: success */
  flag = 1;
/* check the command string (this also tokenizes it) */
  if (EGCommandCheck(s))
    egcomm = EGLocateCommand(EPDTokenFetch(0));
  else
    egcomm = egcomm_nil;
/* was a valid EPD glue command located? */
  if (egcomm == egcomm_nil) {
    EGPL("EG fault: can't locate valid EG command");
    flag = 0;
  } else {
/* a valid command token was found; perform command dispatch */
    switch (egcomm) {
      case egcomm_epdapgn:
        flag = EGProcessAPGN();
        break;
      case egcomm_epdbfix:
        flag = EGProcessBFIX();
        break;
      case egcomm_epdcics:
        flag = EGProcessCICS();
        break;
      case egcomm_epdcomm:
        flag = EGProcessCOMM();
        break;
      case egcomm_epddpgn:
        flag = EGProcessDPGN();
        break;
      case egcomm_epddsml:
        flag = EGProcessDSML();
        break;
      case egcomm_epddstr:
        flag = EGProcessDSTR();
        break;
      case egcomm_epddtpv:
        flag = EGProcessDTPV();
        break;
      case egcomm_epdenum:
        flag = EGProcessENUM();
        break;
      case egcomm_epdhelp:
        flag = EGProcessHELP();
        break;
      case egcomm_epdlink:
        flag = EGProcessLINK();
        break;
      case egcomm_epdlpgn:
        flag = EGProcessLPGN();
        break;
      case egcomm_epdlrec:
        flag = EGProcessLREC();
        break;
      case egcomm_epdmore:
        flag = EGProcessMORE();
        break;
      case egcomm_epdnoop:
        flag = EGProcessNOOP();
        break;
      case egcomm_epdpfdn:
        flag = EGProcessPFDN();
        break;
      case egcomm_epdpfdr:
        flag = EGProcessPFDR();
        break;
      case egcomm_epdpfga:
        flag = EGProcessPFGA();
        break;
      case egcomm_epdpflc:
        flag = EGProcessPFLC();
        break;
      case egcomm_epdpfop:
        flag = EGProcessPFOP();
        break;
      case egcomm_epdscor:
        flag = EGProcessSCOR();
        break;
      case egcomm_epdshow:
        flag = EGProcessSHOW();
        break;
      case egcomm_epdspgn:
        flag = EGProcessSPGN();
        break;
      case egcomm_epdstpv:
        flag = EGProcessSTPV();
        break;
      case egcomm_epdtest:
        flag = EGProcessTEST();
        break;
    };
/* check result */
    if (!flag) {
      sprintf(tbufv, "EG fault: a problem occurred during %s processing\n",
          EPDTokenFetch(0));
      EGPrintTB();
    };
  };
  return flag;
}

/*--> EGInit: one time EPD glue initialization */
nonstatic void EGInit(void) {
/* this is called by Initialize() in init.c */
  EGPL("EPD Kit revision date: 1996.04.21");
/* call the EPD one time set up code */
  EPDInit();
/* initialize the EPD glue command strings vector */
  egcommstrv[egcomm_epdapgn] = "epdapgn";
  egcommstrv[egcomm_epdbfix] = "epdbfix";
  egcommstrv[egcomm_epdcert] = "epdcert";
  egcommstrv[egcomm_epdcics] = "epdcics";
  egcommstrv[egcomm_epdcomm] = "epdcomm";
  egcommstrv[egcomm_epddpgn] = "epddpgn";
  egcommstrv[egcomm_epddsml] = "epddsml";
  egcommstrv[egcomm_epddstr] = "epddstr";
  egcommstrv[egcomm_epddtpv] = "epddtpv";
  egcommstrv[egcomm_epdenum] = "epdenum";
  egcommstrv[egcomm_epdhelp] = "epdhelp";
  egcommstrv[egcomm_epdlink] = "epdlink";
  egcommstrv[egcomm_epdlpgn] = "epdlpgn";
  egcommstrv[egcomm_epdlrec] = "epdlrec";
  egcommstrv[egcomm_epdmore] = "epdmore";
  egcommstrv[egcomm_epdnoop] = "epdnoop";
  egcommstrv[egcomm_epdpfdn] = "epdpfdn";
  egcommstrv[egcomm_epdpfdr] = "epdpfdr";
  egcommstrv[egcomm_epdpfga] = "epdpfga";
  egcommstrv[egcomm_epdpflc] = "epdpflc";
  egcommstrv[egcomm_epdpfop] = "epdpfop";
  egcommstrv[egcomm_epdscor] = "epdscor";
  egcommstrv[egcomm_epdshow] = "epdshow";
  egcommstrv[egcomm_epdspgn] = "epdspgn";
  egcommstrv[egcomm_epdstpv] = "epdstpv";
  egcommstrv[egcomm_epdtest] = "epdtest";
/* initialize the EPD glue command string descriptions vector */
  eghelpstrv[egcomm_epdapgn] = "Append PGN game to <file>";
  eghelpstrv[egcomm_epdbfix] = "Fix <file1> data for Bookup input <file2>";
  eghelpstrv[egcomm_epdcert] =
      "Display certain score for the current position";
  eghelpstrv[egcomm_epdcics] = "Slave to ICS at <hostname> and <portnumber>";
  eghelpstrv[egcomm_epdcomm] = "Slave to Duplex using <pipefile-basename>";
  eghelpstrv[egcomm_epddpgn] = "Display game using PGN";
  eghelpstrv[egcomm_epddsml] = "Display SAN move list";
  eghelpstrv[egcomm_epddstr] = "Display PGN Seven Tag Roster";
  eghelpstrv[egcomm_epddtpv] = "Display PGN tag pair <tag-name> value";
  eghelpstrv[egcomm_epdenum] = "Enumerate to <depth> from <file1> to <file2>";
  eghelpstrv[egcomm_epdhelp] = "Display EPD glue command descriptions";
  eghelpstrv[egcomm_epdlink] =
      "Slave to Argus at <hostname> and <portnumber>";
  eghelpstrv[egcomm_epdlpgn] = "Load PGN game from <file>";
  eghelpstrv[egcomm_epdlrec] = "Load EPD record from <file> <line-number>";
  eghelpstrv[egcomm_epdmore] = "Display more help for <command>";
  eghelpstrv[egcomm_epdnoop] = "No operation";
  eghelpstrv[egcomm_epdpfdn] = "Normalize EPD data from <file1> to <file2>";
  eghelpstrv[egcomm_epdpfdr] = "Repair EPD data from <file1> to <file2>";
  eghelpstrv[egcomm_epdpfga] = "Analyze EPD data from <file1> to <file2>";
  eghelpstrv[egcomm_epdpflc] = "Locate mating cooks in result <file>";
  eghelpstrv[egcomm_epdpfop] = "Purge EPD <opcode> from <file1> to <file2>";
  eghelpstrv[egcomm_epdscor] = "Score benchmark EPD results from <file>";
  eghelpstrv[egcomm_epdshow] =
      "Show EPD four fields for the current position";
  eghelpstrv[egcomm_epdspgn] = "Save PGN game to <file>";
  eghelpstrv[egcomm_epdstpv] = "Set PGN tag pair <tag-name> to <value>";
  eghelpstrv[egcomm_epdtest] = "EPD glue developer testing";
/* initialize the EPD glue command parameter counts vector */
/* set up the default game structure */
  default_gamptr = EPDGameOpen();
  return;
}

/*--> EGTerm: one time EPD glue termination */
nonstatic void EGTerm(void) {
/* this is called by Option() in option.c */
/* release the default game structure */
  if (default_gamptr != NULL)
    EPDGameClose(default_gamptr);
/* call the EPD one time close down code */
  EPDTerm();
  return;
}
#endif
/*<<< epdglue.c: EOF */
