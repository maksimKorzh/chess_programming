
//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

#include <stdio.h>
#include <string.h>

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

#define	NULL_MOVE

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

typedef	int	BOOL;

#define	fFALSE	0
#define	fTRUE	1

typedef	unsigned __int64	U64;	// This is an MSVC data type.  If you need
									//  to change it, your compiler may have a
									//  64-bit unsigned data type.  If not,
									//  you have some work to do, since you
									//  will have to make this an array or
									//  struct, and deal with the places where
									//  math is done on these.

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

#ifdef	DEBUG
void VAssertFailed(const char * szMod, int iLine);
#define	Assert(cond)		if (!(cond)) VAssertFailed(s_aszModule, __LINE__)
#else
#define	Assert(cond)
#endif

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

#define	filA		0
#define	filB		1
#define	filC		2
#define	filD		3
#define	filE		4
#define	filF		5
#define	filG		6
#define	filH		7

#define	filMAX		8		// Number of files on the board.

#define	filIBD		16		// The number of files on the internal board.  The
							//  internal board is 16 files by 8 ranks.

#define	rnk1		0
#define	rnk2		1
#define	rnk3		2
#define	rnk4		3
#define	rnk5		4
#define	rnk6		5
#define	rnk7		6
#define	rnk8		7

#define	rnkMAX		8		// Number of ranks on the board.

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

#define	coEMPTY		-1

#define	coWHITE		0		// Piece colors.
#define	coBLACK		1

#define	coMAX		2

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

#define	pcPAWN		0		// Piece types.
#define	pcKNIGHT	1
#define	pcBISHOP	2
#define	pcROOK		3
#define	pcQUEEN		4
#define	pcKING		5

#define	pcMAX		6

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Piece record.  I'm going to make a couple of arrays of these in order to
//	keep convenient track of what is on the board.

typedef struct	tagPI {
	int	pc;					// Type of piece.
	int	co;					// Its color.
	BOOL	fDead;			// Whether it's been captured (saves having to
							//  mess around with a linked list or other
							//  nonsense.
	int	isq;				// Index into the board of the square this piece
							//  is sitting on.
	int	val;				// Value in centipawns of this piece (does not
							//  include any positional factors, it's just
							//  the "block of wood" value).
}	PI, * PPI;

#define	cpiMAX	(filMAX * 2)
#define	ipiNIL	-1

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Square.  The internal board is an array of 128 of these.  Half of the
//	records are not used, because of the move generation architecture.

typedef	struct	tagSQ {
	PPI	ppi;				// A pointer to the piece on this square, or NULL.
	int	isq;				// Index into board array.
}	SQ, * PSQ;

#define	csqMAX	(rnkMAX * filIBD)
#define	isqNIL	-1

#define	isqA1	0
#define	isqB1	1
#define	isqC1	2
#define	isqD1	3
#define	isqE1	4
#define	isqF1	5
#define	isqG1	6
#define	isqH1	7
#define	isqA2	16
#define	isqB2	17
#define	isqC2	18
#define	isqD2	19
#define	isqE2	20
#define	isqF2	21
#define	isqG2	22
#define	isqH2	23
#define	isqA3	32
#define	isqB3	33
#define	isqC3	34
#define	isqD3	35
#define	isqE3	36
#define	isqF3	37
#define	isqG3	38
#define	isqH3	39
#define	isqA4	48
#define	isqB4	49
#define	isqC4	50
#define	isqD4	51
#define	isqE4	52
#define	isqF4	53
#define	isqG4	54
#define	isqH4	55
#define	isqA5	64
#define	isqB5	65
#define	isqC5	66
#define	isqD5	67
#define	isqE5	68
#define	isqF5	69
#define	isqG5	70
#define	isqH5	71
#define	isqA6	80
#define	isqB6	81
#define	isqC6	82
#define	isqD6	83
#define	isqE6	84
#define	isqF6	85
#define	isqG6	86
#define	isqH6	87
#define	isqA7	96
#define	isqB7	97
#define	isqC7	98
#define	isqD7	99
#define	isqE7	100
#define	isqF7	101
#define	isqG7	102
#define	isqH7	103
#define	isqA8	112
#define	isqB8	113
#define	isqC8	114
#define	isqD8	115
#define	isqE8	116
#define	isqF8	117
#define	isqG8	118
#define	isqH8	119

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

#define	cfNONE	0x0000		// These flags indicate castling legality.
#define	cfE1G1	0x0001
#define	cfE1C1	0x0002
#define	cfE8G8	0x0004
#define	cfE8C8	0x0008

#define	cfMAX	16			// The number of different combinations of flags.

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

#define	valPAWN		100		// Values in centi-pawns of the pieces.
#define	valMINOR	320
#define	valROOK		550
#define	valQUEEN	950
#define	valKING		10000	// This is just a big number.

#define	valMIN	-40000		// These are aways away from valMATE in order
#define	valMAX	40000		//  to avoid unforeseen problems.

#define	valMATE	32767		// -Mate is -32767.  Mate in 1 is 32766. -Mate
							//  in 1 is -32765.  Mate in 2 is 32764.  Etc.

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Candidate move flags.  The move generator marks each move it generates
//	with zero or more of these.

#define	cmfNONE		0x00000000	// A normal move.
#define	cmfPR_MASK	0x0000000F	// If a move promotes, the "pc" it promotes
								//  to will be stuck in these bits.
#define	cmfSET_ENP	0x00010000	// e2-e4 will set this flag.
#define	cmfMAKE_ENP	0x00020000	// An en-passant capture will set this flag.
#define	cmfCASTLE	0x00040000	// O-O will set this flag.
#define	cmfCAPTURE	0x00080000	// Any capture (including en-passant) will set
								//  this flag.

//	Candidate move key.  In the search, it is very useful to search the best
//	move first, so in the candidate move record, a sort key is kept.  The
//	sort key may consist of some of these flags.

#define	cmkNONE		0x00000000
#define	cmkQUEEN	0x00010000
#define	cmkCAPTURE	0x00020000
#define	cmkHASH		0x08000000
#define	cmkPV		0x10000000

//	Candidate move.  The move generate emits records of this type.

typedef	struct	tagCM {
	int	isqFrom;			// Where the moving piece originated.
	int	isqTo;				// Where it is going.
	unsigned	cmk;		// Sort key.
	unsigned	cmf;		// Flags.
}	CM, * PCM;

#define	ccmMAX		8192	// Number of moves in the candidate move array.

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	I have a function that converts from a CM to a record that allows for
//	display in SAN (standard algebraic notation) format.  The function emits
//	records of this type.

#define	sanfNONE		0x00
#define	sanfRNK_AMBIG	0x01
#define	sanfFIL_AMBIG	0x02
#define	sanfCHECK		0x04
#define	sanfMATE		0x08

typedef	struct tagSAN {
	int	isqFrom;			// Where the moving piece originated.
	int	isqTo;				// Where it is going.
	unsigned	cmf;		// Flags, same flags as in the CM.
	unsigned	sanf;		// San flags, which indicate ambiguity and other
							//  special conditions.
	int	pc;					// The type of piece being moved (in the case of
							//  promotion, this will be pcPAWN, not the
							//  promoted piece).
}	SAN, * PSAN;

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

typedef	U64	HASHK,				// Hash key.  This will be used for the hash
	* PHASHK;					//  table, repetition check, etc.

HASHK s_arghashkPc[pcMAX][		// Any array of keys for each piece of each
	coMAX][csqMAX];				//  color on each square.  This will be used
								//  to create a position hash key, and to
								//  update it when the engine moves pieces
								//  around.

//	NOTE!  I don't use either of the following yet.

HASHK s_arghashkEnP[filMAX];	// Hash keys to be XOR'd in if an en-passant
								//  capture is possible.
HASHK s_arghashkCf[cfMAX];		// Hash keys to beXOR'd in based upon castling
								//  legality.

//	This is to be called with the side to move changes, or if in the first
//	position, it is black to move.

#define HashkSwitch(hashkDst)	((hashkDst) ^ 0x21D420B884CD6731)

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Transposition hash.  This structure is packed, meaning that it contains
//	chars and shorts, which are normally a bad idea.  The reason this is done
//	is that a huge array of these is allocated, and the number of items in the
//	array for a given amount of memory should be maximized.

#define	hashfNONE	0x0
#define	hashfALPHA	0x1			// The score is "val" or worse.  If
								//  "hashfBETA" is also set, the score is
								//  exactly "val".
#define	hashfBETA	0x2			// The score is "val" or better.

typedef	struct	tagHASH {
	HASHK	hashk;				// Full hash key.  This is stored in order to
								//  try to avoid collisions.  The effects of
								//  collisions are otherwise ignored.
	short	val;				// This is either an exact value or a bound,
								//  as indicated by the "hashf" flags.
	char	isqFrom;			// Where the move started.
	char	isqTo;				// Where it ended.
	char	pcTo;				// Promoting piece, if there was a promotion,
								//  otherwise zero.
	char	hashf;				// Flags, as defined above.
	char	plyRem;				// Draft under this node.
	char	seq;				// Sequence number, which the use of which is
								//  described in "hash.c".
}	HASH, * PHASH;

//	The hash table is composed of two parallel tables, each with different
//	replacement schemes.  It ends up being an array of these:

typedef	struct tagHASHD {
	HASH	hashDepth;			// "Different search or deeper depth" table.
	HASH	hashAlways;			// "Always replace" table.
}	HASHD, * PHASHD, * RGHASHD;

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	There will be an array of these structures.  At the root of the search,
//	a pointer is set to the first element in the array, and the fields in the
//	element will be set based upon current context.  When "MakeMove" is
//	called, the fields in the next element will be appropriately set, and a
//	pointer to this new element will be passed into the recursed search
//	function.  This saves me from having to pass a lot of parameters around,
//	and saves me some other headache.

//	So this is a stack.  "STE" stands for "stack element", which is something
//	historical from my othe chess program.  Sorry.

#define	csteMAX	256			// This is just a sufficiently large number.

typedef	struct	tagSTE {
	int	coUs;				// Moving side.
	int	isqEnP;				// En-passant square, isqNIL if illegal.
	int	cf;					// Castling flags.
	int	plyFifty;			// Fifty-move counter (100 (2 * 50) is a draw).
	PCM	pcmFirst;			// Top of move generator stack.
	int	valAlpha;			// Alpha (lower bound).
	int	valBeta;			// Beta (upper bound).
	int	plyRem;				// Number of plies of full-width left to do.
	int	valPcUs;			// The material value of all of our non-pawns.
	int	valPcThem;			// The material value of all of their non-pawns.
	int	valPnUs;			// The material value of all of our pawns.
	int	valPnThem;			// The material value of all of their pawns.
	HASHK	hashk;			// Hash key for this position.  Castling and
							//  en-passant aren't accounted for.
	PHASH	phashDepth;		// If this is not null, I found a transposition
							//  hash element that matched this positition,
							//  *and* it contained a valid "best" move.  I
							//  use these to affect the CM keys so this move
							//  will be searched first.
	PHASH	phashAlways;	// Same as "phashDepth", but for different hash
							//  table.
	CM	cmPv;				// If "cmPv.isqFrom" is not isqNIL, if this move
							//  is found in the move list, it should be
							//  searched first.  These elements are primed at
							//  the start of each call to the search function.
							//  They are primed with the PV returned by the
							//  previous call to the search function.
	CM	argcmPv[csteMAX];	// This is the partial PV constructed by a search
							//  that returns a value between alpha and beta.
	int	ccmPv;				// The number of moves in "argcmPv".
#ifdef	NULL_MOVE
	BOOL	fNull;			// A null-move is allowed.
#endif
	BOOL	fInCheck;		// The side to move is in check.
	PPI	ppiTook;			// This is used during "VMakeMove" to indicate the
							//  piece that was captured, in order that
							//  "VUnmakeMove" can find it.
}	STE, * PSTE;

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

typedef	unsigned long	TM;	// Time value (in milliseconds).

#define	tmANALYZE	(TM)-1	// This is just a flag passed to "VThink" to make
							//  it go into analyze mode and think forever.

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Time control types.

#define	tctTOURNEY		0
#define	tctINCREMENT	1	// Also handles sudden-death controls (incr=0).
#define	tctFIXED_DEPTH	2
#define	tctFIXED_TIME	3

typedef	struct	tagTIME_CONTROL {
	int	tct;		// Type of time control.
	int	cMoves;		// Moves in tournament control.
	int	plyDepth;	// Depth in fixed depth time control.
	TM	tmBase;		// Base for fischer control, or total time for a
					//  tournament time control.
	TM	tmIncr;		// Increment for fischer control.
}	TIME_CONTROL, * PTIME_CONTROL;

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This maintains a record of previous moves in this game.  It's use to prime
//	the repetition hash, and in case I'm told to back up a move or more.

typedef	struct tagGAME_CONTEXT {
	char	aszFen[256];	// Starting fen.
	CM	argcm[2048];		// List of moves made so far.
	HASHK	arghashk[2049];	// Hash keys *before* each move made.  This causes
							//  some nasty problems, because after a move is
							//  made, this one more element in this array than
							//  is in the "argcm" array.  So loops through
							//  this array look kind of weird.
	int	ccm;				// Number of moves made so far.
	int	movStart;			// First game move (this is also in the FEN).
	int	coStart;			// First-moving color (this is also in the FEN).
}	GAME_CONTEXT, * PGAME_CONTEXT;

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Thinking modes.

#define	smodeTHINK		0	// The engine is thinking, and will eventually
							//  move.
#define	smodeANALYZE	1	// The engine is thinking forever.
#define	smodePONDER		2	// The engine has gotten one move ahead of the
							//  real game score, and is thinking about what
							//  to do if that move it's gotten ahead by really
							//  happens.
							// The engine can be put into "think" mode by
							//  simply changing the state, and telling it when
							//  to stop thinking.
#define	smodeIDLE		3	// Doing nothing.  A call to "Think" is not
							//  in the backtrace.

//	There will be approximately one of these active.  It represents the
//	board position, and also includes some current search state information.
//	When a move is made during the search, the board (argsq) in here is
//	messed with, the piece list (argpi) is messed with, and a new STE element
//	is initialized.  When a move is undone, some of the STE information is
//	used to unmake the move on the board and piece list, and the rest is just
//	popped off and discarded.

//	A parameter to this thing is passed around, a lot.  This is annoying, but
//	you'll thank me if you ever try to implement a parallel search.  If you
//	have two of these elements, you can have two searches running at the same
//	time, the only ill effect being hash table overwrite weirdness, since that
//	is the only real "global" non-constant data used by this engine.

//	This is a big structure, and you may not be able to put one of these on
//	the stack.

#define	chashkREP	2048

typedef	struct	tagCON {
	PI	argpi[coMAX][		// Piece list.
		cpiMAX];	
	int	argcpi[coMAX];		// Number of pieces per side (at start of search).
	SQ	argsq[csqMAX];		// Board.
	STE	argste[csteMAX];	// Search context stack.
	CM	argcm[ccmMAX];		// Move generator stack.
	HASHK	arghashkRep[	// Repetition hash.
		chashkREP];
	char	aszPonder[16];	// Move the engine is pondering if in ponder mode.
	BOOL	fAbort;			// This is set to TRUE if the engine should
							//  immediately end its search.  The engine should
							//  unmake all made moves as it is unwinding, but
							//  it shouldn't expect that a move is going to be
							//  made when it is done.
	BOOL	fTimeout;		// This is true if the engine is out of time and
							//  should end its search at its convenience.  In
							//  my implementation, it will finish the first
							//  ply before responding to this, otherwise it
							//  will respond immediately.
	U64	nodes;				// Number of nodes searched by the current search.
	U64	nodesNext;			// Nodes I must exceed before next callback to
							//  interface.
	BOOL	fPost;			// TRUE if variation information should be posted
							//  to the interface.
	BOOL	fPonder;		// TRUE if the engine should ponder (ever).
	GAME_CONTEXT	gc;		// A list of moves made in the game so far, etc.
	TIME_CONTROL	tc;		// Time control currently in use.
	unsigned tmRemaining;	// Time on clock at start of this move.
	unsigned tmStart;		// Time I started thinking.
	unsigned tmEnd;			// Time I am scheduled to end.
	int	plyDepth;			// Depth of current search.
	int	smode;				// Engine search mode (think, ponder, etc.).
	char	aszBook[256];	// Opening book currently loaded.
	BOOL fUseBook;			// TRUE if I should use the book.
	unsigned	cbMaxHash;	// Number of transposition hash element bytes
							//  at most (actual number will be <= this).
	unsigned	cbMaxBook;	// Number of transposition hash element bytes
							//  at most (actual number will be <= this).
	int	bdm;				// Book dump mode.
}	CON, * PCON;

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	A few utility functions.

#define IsqFromRnkFil(rnk, fil)		((rnk) * filIBD + (fil))
#define RnkFromIsq(isq)				((isq) / filIBD)
#define FilFromIsq(isq)				((isq) & (filIBD - 1))

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Book dump mode.  The engine can be made to dump its book to stdout at
//	startup, if the book initialization function is called with something
//	other than "bdmNONE".

#define	bdmNONE		0
#define	bdmFULL		1
#define	bdmCOMPACT	2

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

extern char const s_argbPc[			// Array of upper- and lower-case piece
	coMAX][pcMAX];
									//  ascii (p, N, k, etc.).
extern int	const s_argvalPc[];		// Array of "block of wood" values of the
									//  pieces.
extern int	const s_argvalPcOnly[];	// Same as "s_argvalPc", only king and
									//  pawn are zeroed.
extern int	const s_argvalPnOnly[];	// Same as "s_argvalPc", only all except
									//  pawn are zeroed.
extern char const * s_argszCo[];	// "White", "Black".

extern int const *					// These are shared by the moe generator
	s_argpvecPiece[];
extern int const * s_argpvecPawn[];	//  and the attack detection code.  They
									//  are documented in "move.c".

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

#ifdef	DEBUG
void VDumpBoard(PCON pcon);
#endif

//	These are documented in the source modules.

BOOL FInitHashe(PCON pcon);
void VInitHashk(void);
BOOL FInitCon(PCON pcon, char const * szFen);
void VGenMoves(PCON pcon, PSTE pste, BOOL fCapsOnly);
void VMakeNullMove(PCON pcon, PSTE pste);
void VMakeMove(PCON pcon, PSTE pste, PCM pcm);
void VUnmakeMove(PCON pcon, PSTE pste, PCM pcm);
int ValEval(PCON pcon, PSTE pste);
int ValSearch(PCON pcon, PSTE pste);
int ValSearchQ(PCON pcon, PSTE pste);
void VThink(PCON pcon, TM tmRemaining);
void VInitAttackData(void);
void VInitHash(void);
BOOL FAttacked(PCON pcon, int isqDef, int coAtk);
void VCmToSz(PCM pcm, char * sz);
void VFixSte(PCON pcon);
BOOL FAdvance(PCON pcon, char * sz);
void VUndoMove(PCON pcon);
unsigned TmNow(void);
void VDumpPv(PCON pcon, int ply, int val, int ch);
void VSetTime(PCON pcon);
void VSetTimeControl(PCON pcon, int cMoves, TM tmBase, TM tmIncr);
void VSetFixedDepthTimeControl(PCON pcon, int plyDepth);
void VCheckTime(PCON pcon);
void VSetPv(PCON pcon);
void VDrawCheck(PCON pcon);
BOOL FRepSet(PCON pcon, HASHK hashk);
void VSetRep(PCON pcon, HASHK hashk);
void VClearRep(PCON pcon, HASHK hashk);
void VSetRepHash(PCON pcon);
void VClearHashe(void);
BOOL FProbeHash(PCON pcon, PSTE pste, int * pval);
void VRecordHash(PCON pcon, PSTE pste, PCM pcm, int val, int hashf);
void VFindHash(PSTE pste, PHASH phash);
void VCmToSan(PCON pcon, PSTE pste, PCM pcmSrc, PSAN psan);
void VSanToSz(PSAN psan, char * sz);
BOOL FGenBook(PCON pcon, char * szFile, int bdm);
BOOL FBookMove(PCON pcon, PCM pcmMove);
void VMates(int coWin);
BOOL FStalemated(PCON pcon);
PCON PconInitEngine(void);
void VIncrementHashSeq(void);

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-
