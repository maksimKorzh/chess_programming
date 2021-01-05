#pragma once
/* *INDENT-OFF* */
/*>>> epddefs.h: Extended Position Description definitions */
/* Revised: 1996.06.23 */
/*
   Copyright (C) 1996 by Steven J. Edwards (sje@mv.mv.com)
   All rights reserved.  This code may be freely redistibuted and used by
   both research and commerical applications.  No warranty exists.
 */
/*
   Everything in this source file is independent of the host program.
   Requests for changes and additions should be communicated to the author
   via the e-mail address given above.
 */
/*
   This file was originally prepared on an Apple Macintosh using the
   Metrowerks CodeWarrior 6 ANSI C compiler.  Tabs are set at every
   four columns.  Further testing and development was performed on a
   generic PC running Linux 1.3.20 and using the gcc 2.7.0 compiler.
 */
/* inclusion telltale */
#  if (!defined(_epddefs))
#    define _epddefs 1
/* subprogram storage class for non-statics (usually empty definition) */
#    define nonstatic
/* a bit */
#    define bit 0x01
/* bit positions */
#    define bit_00 (bit <<  0)
#    define bit_01 (bit <<  1)
#    define bit_02 (bit <<  2)
#    define bit_03 (bit <<  3)
#    define bit_04 (bit <<  4)
#    define bit_05 (bit <<  5)
#    define bit_06 (bit <<  6)
#    define bit_07 (bit <<  7)
#    define bit_08 (bit <<  8)
#    define bit_09 (bit <<  9)
#    define bit_10 (bit << 10)
#    define bit_11 (bit << 11)
#    define bit_12 (bit << 12)
#    define bit_13 (bit << 13)
#    define bit_14 (bit << 14)
#    define bit_15 (bit << 15)
/* bit width constants */
#    define nybbW  4
#    define byteW  8
/* simple masks */
#    define nybbM 0x000f
/* useful types */
typedef void *voidptrT;
typedef unsigned char byteT, *byteptrT;
typedef char *charptrT;
typedef short int siT, *siptrT;
typedef long int liT, *liptrT;
typedef float srT, *srptrT;
typedef double lrT, *lrptrT;
typedef FILE *fptrT;

/* text I/O buffer length */
#    define tL 256
/* EPD I/O buffer length */
#    define epdL 4096
/* the standard algebraic notation character vector type */
#    define sanL 16
		/* must be at least 8; extra room for alternatives */
typedef char sanT[sanL];
typedef sanT *sanptrT;

/* SAN style attributes, priority ordered (used for encoding) */
typedef siT ssaT;

#    define ssaL 12
#    define ssa_nil (-1)
#    define ssa_capt  0 /* 5 way: capture indicator */
#    define ssa_case  1 /* 2 way: letter case */
#    define ssa_chec  2 /* 3 way: checking */
#    define ssa_cast  3 /* 5 way: castling */
#    define ssa_prom  4 /* 4 way: promoting */
#    define ssa_ptar  5 /* 2 way: pawn target rank skip */
#    define ssa_chmt  6 /* 4 way: checkmating */
#    define ssa_epct  7 /* 2 way: en passant capture */
#    define ssa_draw  8 /* 2 way: drawing */
#    define ssa_move  9 /* 2 way: movement indicator */
#    define ssa_edcf 10 /* 2 way: extra disambiguating character (file) */
#    define ssa_edcr 11 /* 2 way: extra disambiguating character (rank) */
/* SAN style vector */
typedef siT ssavT[ssaL];

/* colors (ordering is critical) */
typedef siT cT, *cptrT;

#    define cQ 2
#    define cL (bit << cQ)
#    define rcQ 1
#    define rcL (bit << rcQ)
#    define c_nil (-1)
#    define c_w 0
		/* white */
#    define c_b 1
		/* black */
#    define c_v 2
		/* vacant */
#    define c_x 3
		/* extra */
/* pieces (ordering is critical) */
typedef siT pT, *pptrT;

#    define pL 8
#    define rpL 6
#    define p_nil (-1)
#    define p_p 0
		/* pawn */
#    define p_n 1
		/* knight */
#    define p_b 2
		/* bishop */
#    define p_r 3
		/* rook */
#    define p_q 4
		/* queen */
#    define p_k 5
		/* king */
#    define p_v 6
		/* vacant */
#    define p_x 7
		/* extra */
/* color piece combinations (ordering is critical) */
typedef siT cpT;

#    define cpL 16
#    define rcpL 12
#    define cp_nil (-1)
#    define cp_wp  0	/* white pawn */
#    define cp_wn  1	/* white knight */
#    define cp_wb  2	/* white bishop */
#    define cp_wr  3	/* white rook */
#    define cp_wq  4	/* white queen */
#    define cp_wk  5	/* white king */
#    define cp_bp  6	/* black pawn */
#    define cp_bn  7	/* black knight */
#    define cp_bb  8	/* black bishop */
#    define cp_br  9	/* black rook */
#    define cp_bq 10	/* black queen */
#    define cp_bk 11	/* black king */
#    define cp_v0 12	/* vacant */
#    define cp_x0 13	/* extra 0 */
#    define cp_x1 14	/* extra 1 */
#    define cp_x2 15	/* extra 2 */
/* ranks */
typedef siT rankT;

#    define rankM (0x0007)
#    define rankQ 3
#    define rankL (bit << rankQ)
#    define rank_nil (-1)
#    define rank_1 0
#    define rank_2 1
#    define rank_3 2
#    define rank_4 3
#    define rank_5 4
#    define rank_6 5
#    define rank_7 6
#    define rank_8 7
/* files */
typedef siT fileT;

#    define fileM (0x0007)
#    define fileQ 3
#    define fileL (bit << fileQ)
#    define file_nil (-1)
#    define file_a 0	/* QR */
#    define file_b 1	/* QN */
#    define file_c 2	/* QB */
#    define file_d 3	/* Q */
#    define file_e 4	/* K */
#    define file_f 5	/* KB */
#    define file_g 6	/* KN */
#    define file_h 7	/* KR */
/* location mappings */
#    define map_sq(r, f) (((r) << fileQ | (f)))
#    define map_file(sq) ((sq) & 0x07)
#    define map_rank(sq) ((sq) >> fileQ)
/* squares */
typedef siT sqT, *sqptrT;

#    define sqM (0x003f)
#    define sqQ (rankQ + fileQ)
#    define sqL (bit << sqQ)
#    define sq_nil (-1)
#    define sq_a1 map_sq(rank_1, file_a)
#    define sq_b1 map_sq(rank_1, file_b)
#    define sq_c1 map_sq(rank_1, file_c)
#    define sq_d1 map_sq(rank_1, file_d)
#    define sq_e1 map_sq(rank_1, file_e)
#    define sq_f1 map_sq(rank_1, file_f)
#    define sq_g1 map_sq(rank_1, file_g)
#    define sq_h1 map_sq(rank_1, file_h)
#    define sq_a2 map_sq(rank_2, file_a)
#    define sq_b2 map_sq(rank_2, file_b)
#    define sq_c2 map_sq(rank_2, file_c)
#    define sq_d2 map_sq(rank_2, file_d)
#    define sq_e2 map_sq(rank_2, file_e)
#    define sq_f2 map_sq(rank_2, file_f)
#    define sq_g2 map_sq(rank_2, file_g)
#    define sq_h2 map_sq(rank_2, file_h)
#    define sq_a3 map_sq(rank_3, file_a)
#    define sq_b3 map_sq(rank_3, file_b)
#    define sq_c3 map_sq(rank_3, file_c)
#    define sq_d3 map_sq(rank_3, file_d)
#    define sq_e3 map_sq(rank_3, file_e)
#    define sq_f3 map_sq(rank_3, file_f)
#    define sq_g3 map_sq(rank_3, file_g)
#    define sq_h3 map_sq(rank_3, file_h)
#    define sq_a4 map_sq(rank_4, file_a)
#    define sq_b4 map_sq(rank_4, file_b)
#    define sq_c4 map_sq(rank_4, file_c)
#    define sq_d4 map_sq(rank_4, file_d)
#    define sq_e4 map_sq(rank_4, file_e)
#    define sq_f4 map_sq(rank_4, file_f)
#    define sq_g4 map_sq(rank_4, file_g)
#    define sq_h4 map_sq(rank_4, file_h)
#    define sq_a5 map_sq(rank_5, file_a)
#    define sq_b5 map_sq(rank_5, file_b)
#    define sq_c5 map_sq(rank_5, file_c)
#    define sq_d5 map_sq(rank_5, file_d)
#    define sq_e5 map_sq(rank_5, file_e)
#    define sq_f5 map_sq(rank_5, file_f)
#    define sq_g5 map_sq(rank_5, file_g)
#    define sq_h5 map_sq(rank_5, file_h)
#    define sq_a6 map_sq(rank_6, file_a)
#    define sq_b6 map_sq(rank_6, file_b)
#    define sq_c6 map_sq(rank_6, file_c)
#    define sq_d6 map_sq(rank_6, file_d)
#    define sq_e6 map_sq(rank_6, file_e)
#    define sq_f6 map_sq(rank_6, file_f)
#    define sq_g6 map_sq(rank_6, file_g)
#    define sq_h6 map_sq(rank_6, file_h)
#    define sq_a7 map_sq(rank_7, file_a)
#    define sq_b7 map_sq(rank_7, file_b)
#    define sq_c7 map_sq(rank_7, file_c)
#    define sq_d7 map_sq(rank_7, file_d)
#    define sq_e7 map_sq(rank_7, file_e)
#    define sq_f7 map_sq(rank_7, file_f)
#    define sq_g7 map_sq(rank_7, file_g)
#    define sq_h7 map_sq(rank_7, file_h)
#    define sq_a8 map_sq(rank_8, file_a)
#    define sq_b8 map_sq(rank_8, file_b)
#    define sq_c8 map_sq(rank_8, file_c)
#    define sq_d8 map_sq(rank_8, file_d)
#    define sq_e8 map_sq(rank_8, file_e)
#    define sq_f8 map_sq(rank_8, file_f)
#    define sq_g8 map_sq(rank_8, file_g)
#    define sq_h8 map_sq(rank_8, file_h)
/* regular board */
typedef union rbU {
  cpT rbm[rankL][fileL];	/* rank/file indexing */
  cpT rbv[sqL]; 		/* square indexing */
} rbT, *rbptrT;

/* nybble board vector */
#    define nbL (sqL / (byteW / nybbW))
typedef byteT nbvT[nbL];

/* flanks */
typedef siT flankT;

#    define flankL 2
#    define flank_nil (-1)
#    define flank_k 0	/* kingside */
#    define flank_q 1	/* queenside */
/* direction indices */
typedef siT dxT;

#    define dxQ 4
#    define dxL (bit << dxQ)
#    define dx_nil (-1)
#    define dx_0  0
#    define dx_1  1
#    define dx_2  2
#    define dx_3  3
#    define dx_4  4
#    define dx_5  5
#    define dx_6  6
#    define dx_7  7
#    define dx_8  8
#    define dx_9  9
#    define dx_a 10
#    define dx_b 11
#    define dx_c 12
#    define dx_d 13
#    define dx_e 14
#    define dx_f 15
/* direction vector displacements */
typedef siT dvT;

#    define dv_0 (( 0 * fileL) + 1)
#    define dv_1 (( 1 * fileL) + 0)
#    define dv_2 (( 0 * fileL) - 1)
#    define dv_3 ((-1 * fileL) - 0)
#    define dv_4 (( 1 * fileL) + 1)
#    define dv_5 (( 1 * fileL) - 1)
#    define dv_6 ((-1 * fileL) - 1)
#    define dv_7 ((-1 * fileL) + 1)
#    define dv_8 (( 1 * fileL) + 2)
#    define dv_9 (( 2 * fileL) + 1)
#    define dv_a (( 2 * fileL) - 1)
#    define dv_b (( 1 * fileL) - 2)
#    define dv_c ((-1 * fileL) - 2)
#    define dv_d ((-2 * fileL) - 1)
#    define dv_e ((-2 * fileL) + 1)
#    define dv_f ((-1 * fileL) + 2)
/* extended direction vector offsets */
typedef siT xdvT;

#    define xdv_0 (( 0 * xfileL) + 1)
#    define xdv_1 (( 1 * xfileL) + 0)
#    define xdv_2 (( 0 * xfileL) - 1)
#    define xdv_3 ((-1 * xfileL) - 0)
#    define xdv_4 (( 1 * xfileL) + 1)
#    define xdv_5 (( 1 * xfileL) - 1)
#    define xdv_6 ((-1 * xfileL) - 1)
#    define xdv_7 ((-1 * xfileL) + 1)
#    define xdv_8 (( 1 * xfileL) + 2)
#    define xdv_9 (( 2 * xfileL) + 1)
#    define xdv_a (( 2 * xfileL) - 1)
#    define xdv_b (( 1 * xfileL) - 2)
#    define xdv_c ((-1 * xfileL) - 2)
#    define xdv_d ((-2 * xfileL) - 1)
#    define xdv_e ((-2 * xfileL) + 1)
#    define xdv_f ((-1 * xfileL) + 2)
/* extended rank, file, and square types */
typedef siT xrankT;

#    define xrankQ (rankQ + 1)
#    define xrankL (bit << xrankQ)
typedef siT xfileT;

#    define xfileQ (fileQ + 1)
#    define xfileM 0x0f
#    define xfileL (bit << xfileQ)
typedef siT xsqT, *xsqptrT;

#    define xsqQ (xrankQ + xfileQ)
#    define xsqL (bit << xsqQ)
/* the extended board type */
typedef union xbU {
  cpT xbm[xrankL][xfileL];
  cpT xbv[xsqL];
} xbT, *xbptrT;

/* extended board mapping macros */
#    define map_xrank_xsq(xsq) ((xsq) >> xfileQ)
#    define map_xfile_xsq(xsq) ((xsq) & xfileM)
#    define map_xsq_xrank_xfile(xrank, xfile) (((xrank) << xfileQ) | (xfile))
/* extended conversion macros */
#    define xbdrL 4
#    define map_xfile_file(file) ((file) + xbdrL)
#    define map_xrank_rank(rank) ((rank) + xbdrL)
#    define map_file_xfile(xfile) ((xfile) - xbdrL)
#    define map_rank_xrank(xrank) ((xrank) - xbdrL)
#    define map_sq_xsq(xsq) \
	(((((xsq) >> xfileQ) - xbdrL) << fileQ) | (((xsq) & xfileM) - xbdrL))
#    define map_xsq_sq(sq) \
	((((((sq) >> fileQ) & fileM) + xbdrL) << xfileQ) | \
	(((sq) & fileM) + xbdrL))
/* castling availability indicators */
typedef siT caiT;

#    define caiL (rcL * flankL)
#    define cai_nil (-1)
#    define cai_wk ((c_w * flankL) + flank_k)
#    define cai_wq ((c_w * flankL) + flank_q)
#    define cai_bk ((c_b * flankL) + flank_k)
#    define cai_bq ((c_b * flankL) + flank_q)
/* castling index mapper */
#    define castim(cai) (bit << (cai))
/* castling flags */
typedef siT castT;
typedef castT *castptrT;

#    define cf_wk castim(cai_wk)
#    define cf_wq castim(cai_wq)
#    define cf_bk castim(cai_bk)
#    define cf_bq castim(cai_bq)
/* centipawn evaluation */
typedef siT cpevT, *cpevptrT;

/* some interesting centipawn evaluations */
#    define cpev_best ((cpevT) ((((liT) bit) << 15) - 1))
#    define cpev_bust (-cpev_best)
#    define cpev_draw 0
#    define cpev_mate cpev_best
#    define cpev_wrck (cpev_bust - 1)
/* mate and loss synthesis macros (fullmove distance argument) */
#    define synth_mate(n) (cpev_mate - ((n) * 2) + 1)
#    define synth_loss(n) (cpev_bust + ((n) * 2))
/* distance to mate/loss macros (mate/loss centipawn argument) */
#    define synth_distance_mate(cpev)  ((siT) (((cpev_mate - (cpev)) + 1) / 2))
#    define synth_distance_loss(cpev)  ((siT) (((cpev) - cpev_bust) / 2))
/* maximum distance to mate/loss (fullmove distance count) */
#    define max_dist_mateL 1024
#    define max_dist_lossL 1024
/* forced mate/loss detection macros (mate/loss centipawn argument) */
#    define forced_mate(cpev) ((cpev) >= ((cpev_mate - (max_dist_mateL * 2)) + 1))
#    define forced_loss(cpev) ((cpev) <= (cpev_bust + (max_dist_lossL * 2)))
/* move flag bits */
typedef siT mfT;

#    define mf_bust (bit << 0)	/* illegal move */
#    define mf_chec (bit << 1)	/* checking */
#    define mf_chmt (bit << 2)	/* checkmating */
#    define mf_draw (bit << 3)	/* drawing (includes stalemating) */
#    define mf_exec (bit << 4)	/* executed at least once */
#    define mf_null (bit << 5)	/* special null move */
#    define mf_sanf (bit << 6)	/* needs file disambiguation */
#    define mf_sanr (bit << 7)	/* needs rank disambiguation */
#    define mf_stmt (bit << 8)	/* stalemating */
/* special case move type */
typedef siT scmvT;

#    define scmvQ 3
#    define scmvL (bit << scmvQ)
#    define scmv_nil (-1)
#    define scmv_reg 0	/* regular */
#    define scmv_epc 1	/* en passant capture */
#    define scmv_cks 2	/* castles kingside */
#    define scmv_cqs 3	/* castles queenside */
#    define scmv_ppn 4	/* pawn promotes to knight */
#    define scmv_ppb 5	/* pawn promotes to bishop */
#    define scmv_ppr 6	/* pawn promotes to rook */
#    define scmv_ppq 7	/* pawn promotes to queen */
/* move type */
typedef struct mS {
  mfT m_flag;			/* move flags */
  sqT m_frsq;			/* from square */
  sqT m_tosq;			/* to square */
  cpT m_frcp;			/* from color-piece */
  cpT m_tocp;			/* to color-piece */
  scmvT m_scmv; 		/* special case move indication */
} mT, *mptrT;

/* game termination indicator markers */
typedef siT gtimT, *gtimptrT;

#    define gtimL 4
#    define gtim_nil (-1)
#    define gtim_w 0	/* White won */
#    define gtim_b 1	/* Black won */
#    define gtim_d 2	/* draw */
#    define gtim_u 3	/* unknown */
/* clockstamp length hh:mm:ss */
#    define clockstampL (2 + 1 + 2 + 1 + 2 + 1)
/* datestamp length YYYY.MM.DD */
#    define datestampL (4 + 1 + 2 + 1 + 2 + 1)
/* duration length dddd:hh:mm:ss */
#    define durationL (4 + 1 + 2 + 1 + 2 + 1 + 2 + 1)
/* EPD operand basetype */
typedef siT eobT;

#    define eobL 2
#    define eob_nil (-1)
#    define eob_string 0	/* quoted string */
#    define eob_symbol 1	/* unquoted symbol */
/* EPD operand value type */
typedef struct eovS {
  eobT eov_eob; 		/* basetype */
  charptrT eov_str;		/* string value */
  struct eovS *eov_prev;	/* previous operand */
  struct eovS *eov_next;	/* next operand */
} eovT, *eovptrT;

/* EPD operation type */
typedef struct eopS {
  charptrT eop_opsym;		/* operation code symbol */
  eovptrT eop_headeov;		/* head of operand value list */
  eovptrT eop_taileov;		/* tail of operand value list */
  struct eopS *eop_prev;	/* previous operation */
  struct eopS *eop_next;	/* next operation */
} eopT, *eopptrT;

/* EPD record type */
typedef struct epdS {
  nbvT epd_nbv; 		/* piece placement nybble board vector */
  cT epd_actc;			/* active color */
  castT epd_cast;		/* castling availability */
  sqT epd_epsq; 		/* en passant target square */
  eopptrT epd_headeop;		/* head of operation list */
  eopptrT epd_taileop;		/* tail of operation list */
} epdT, *epdptrT;

/* EPD standard operators */
typedef siT epdsoT, *epdsoptrT;

#    define epdsoL 51
#    define epdso_nil (-1)
#    define epdso_acd	       0	/* analysis count: depth */
#    define epdso_acn	       1	/* analysis count: nodes */
#    define epdso_acs	       2	/* analysis count: seconds */
#    define epdso_am	       3	/* avoid move(s) */
#    define epdso_bm	       4	/* best move(s) */
#    define epdso_c0	       5	/* comment slot 0 */
#    define epdso_c1	       6	/* comment slot 1 */
#    define epdso_c2	       7	/* comment slot 2 */
#    define epdso_c3	       8	/* comment slot 3 */
#    define epdso_c4	       9	/* comment slot 4 */
#    define epdso_c5	      10	/* comment slot 5 */
#    define epdso_c6	      11	/* comment slot 6 */
#    define epdso_c7	      12	/* comment slot 7 */
#    define epdso_c8	      13	/* comment slot 8 */
#    define epdso_c9	      14	/* comment slot 9 */
#    define epdso_cc	      15	/* chess clock */
#    define epdso_ce	      16	/* centipawn evaluation */
#    define epdso_dm	      17	/* direct move count */
#    define epdso_draw_accept 18	/* draw accept */
#    define epdso_draw_claim  19	/* draw claim */
#    define epdso_draw_offer  20	/* draw offer */
#    define epdso_draw_reject 21	/* draw reject */
#    define epdso_eco	      22	/* ECO code */
#    define epdso_fmvn	      23	/* fullmove number */
#    define epdso_hmvc	      24	/* halfmove clock */
#    define epdso_id	      25	/* position identification */
#    define epdso_nic	      26	/* NIC code */
#    define epdso_noop	      27	/* no operation */
#    define epdso_pm	      28	/* predicted move */
#    define epdso_ptp	      29	/* PGN tag pair(s) */
#    define epdso_pv	      30	/* predicted variation */
#    define epdso_rc	      31	/* repetition count */
#    define epdso_refcom      32	/* referee command */
#    define epdso_refreq      33	/* referee request */
#    define epdso_resign      34	/* resign */
#    define epdso_sm	      35	/* supplied move */
#    define epdso_sv	      36	/* supplied variation */
#    define epdso_tcgs	      37	/* telecommunications: game selector */
#    define epdso_tcri	      38	/* telecommunications: receiver identification */
#    define epdso_tcsi	      39	/* telecommunications: sender identification */
#    define epdso_ts	      40	/* timestamp */
#    define epdso_v0	      41	/* variation slot 0 */
#    define epdso_v1	      42	/* variation slot 1 */
#    define epdso_v2	      43	/* variation slot 2 */
#    define epdso_v3	      44	/* variation slot 3 */
#    define epdso_v4	      45	/* variation slot 4 */
#    define epdso_v5	      46	/* variation slot 5 */
#    define epdso_v6	      47	/* variation slot 6 */
#    define epdso_v7	      48	/* variation slot 7 */
#    define epdso_v8	      49	/* variation slot 8 */
#    define epdso_v9	      50	/* variation slot 9 */
/* referee commands */
typedef siT refcomT, *refcomptrT;

#    define refcomL 7
#    define refcom_nil (-1)
#    define refcom_conclude   0
#    define refcom_disconnect 1
#    define refcom_execute    2
#    define refcom_fault      3
#    define refcom_inform     4
#    define refcom_respond    5
#    define refcom_reset      6
/* referee requests */
typedef siT refreqT, *refreqptrT;

#    define refreqL 4
#    define refreq_nil (-1)
#    define refreq_fault    0
#    define refreq_reply    1
#    define refreq_sign_on  2
#    define refreq_sign_off 3
/* referee interface procedure type */
typedef epdptrT(*refintptrT) (epdptrT epdptr, siptrT flagptr);

/* PGN Seven Tag Roster names */
typedef siT pgnstrT, *pgnstrptrT;

#    define pgnstrL 7
#    define pgnstr_nil (-1)
#    define pgnstr_event  0
#    define pgnstr_site   1
#    define pgnstr_date   2
#    define pgnstr_round  3
#    define pgnstr_white  4
#    define pgnstr_black  5
#    define pgnstr_result 6
/* benchmark score structure */
typedef struct bmsS {
  siT bms_acdflag;		/* ACD (depth) data valid flag */
  siT bms_acnflag;		/* ACN (nodes) data valid flag */
  siT bms_acsflag;		/* ACS (seconds) data valid flag */
  liT bms_total;		/* total record count */
  liT bms_solve;		/* solved record count */
  liT bms_unsol;		/* unsolved record count */
  liT bms_total_acd;		/* ACD used, all records */
  liT bms_solve_acd;		/* ACD used, solved records */
  liT bms_unsol_acd;		/* ACD used, unsolved records */
  liT bms_total_acn;		/* ACN used, all records */
  liT bms_solve_acn;		/* ACN used, solved records */
  liT bms_unsol_acn;		/* ACN used, unsolved records */
  liT bms_total_acs;		/* ACS used, all records */
  liT bms_solve_acs;		/* ACS used, solved records */
  liT bms_unsol_acs;		/* ACS used, unsolved records */
} bmsT, *bmsptrT;

/* environment stack entry record type */
typedef struct eseS {
  cT ese_actc;			/* active color */
  castT ese_cast;		/* castling availability */
  sqT ese_epsq; 		/* en passant target square */
  siT ese_hmvc; 		/* halfmove clock */
  siT ese_fmvn; 		/* fullmove number */
  sqT ese_ksqv[rcL];		/* king square locations */
} eseT, *eseptrT;

/* game played move record type (entries are prior to move) */
typedef struct gpmS {
  mT gpm_m;			/* the move to be played */
  eseT gpm_ese; 		/* environment statck entry storage */
  nbvT gpm_nbv; 		/* nybble board vector */
  struct gpmS *gpm_prev;	/* previous played move record */
  struct gpmS *gpm_next;	/* next played move record */
} gpmT, *gpmptrT;

/* game record type */
typedef struct gamS {
  charptrT gam_strv[pgnstrL];	/* PGN STR tag values */
  gtimT gam_gtim;		/* game termination indicator */
  gpmptrT gam_headgpm;		/* head of game played move list */
  gpmptrT gam_tailgpm;		/* tail of game played move list */
  struct gamS *gam_prev;	/* previous game */
  struct gamS *gam_next;	/* next game */
} gamT, *gamptrT;

/* statndard disply output column limit */
#    define columnL 80
/* inclusion telltale closure */
#  endif
/* *INDENT-ON* */
