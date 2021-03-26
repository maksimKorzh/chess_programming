/***************************************************************************/
/*                                 MaxQi,                                  */
/* Version of the sub-2KB (source) micro-Max Chess program, fused to a     */
/* generic WinBoard interface, loading its move-generator tables from file */
/* Adapted to play Xiang Qi, which required rather specialized changes     */
/***************************************************************************/
/* micro-Max version 4.8 (~1950 characters) features:                      */
/* - recursive negamax search                                              */
/* - all-capture quiescence search with MVV/LVA priority                   */
/* - (internal) iterative deepening                                        */
/* - best-move-first 'sorting'                                             */
/* - a hash table storing score and best move                              */
/* - futility pruning                                                      */
/* - king safety through magnetic frozen king                              */
/* - null-move pruning                                                     */
/* - Late-move reductions                                                  */
/* - full FIDE rules (expt minor promotion) and move-legality checking     */
/* - keep hash + rep-draw detect                                           */
/* - end-game Pawn-push bonus, new piece values, gradual promotion         */
/***************************************************************************/
/* This version reads the piece description from a file fmax.ini           */
/* The format supports many fairy pieces, including hoppers.               */
/* f) now supports 15 piece types, by requisitioning WHITE bit             */
/* g) supports larger board width.                                         */
/* h) castling bug ('in-check by non-captures') corrected                  */
/* i) rep-draw bug ('side-to-move') corrected                              */
/* k) allow user underpromotions, recognize & ignore 'variant' command     */
/* l) edit bug corrected (i & j file clear)                                */
/* m) piece values no longer quantized, game-stage counting bug corrected  */
/* n) edit-menu K-side castling bug corrected.                             */
/* o) retrieve the requested variant from the .ini file                    */
/* p) clear hash table on variant switch                                   */
/* q) reduced piece-material count for better Pawn push                    */
/* r) hash-table bug corrected (X still ORed with flags)                   */
/* s) Bug that prevented initialization center points corrected            */
/* t) castling bug after edit fixed                                        */
/* u) converted to protocol 2; ping implemented                            */
/* v) white e.p. rights hash bug fixed;                                    */
/* w) piece indicators programable, multi-path support                     */
/* x) e.p. changed to support Berolina Pawns                               */
/* y) capture vlue of 6-7th-rank Pawn reduced in Shatranj                  */
/* z) bug in promotion input corrected                                     */
/* A) stalemate-detection bug in printResult fixed                         */
/* B) Invalidate hash on game-level promotion (might be under-promotion!)  */
/* C) King move evaluation based on negative piece value in stead of nr    */
/* D) WB memory command added, undo fixed                                  */
/* E) 15th piece read in                                                   */
/* F) accepts ini fileargument                                             */
/* Xiang Qi adaptations:                                                   */
/*    orient board sideways for easy implementation of King-facing rule    */
/*    allow board to have 9 rows (= files)                                 */
/*    add array for specifying board zones                                 */
/*    add zone limiter for each piece                                      */
/*    change promotion code to act when crossing river                     */
/*    remove stalemate code                                                */
/***************************************************************************/

     /*****************************************************************/
     /*                      LICENCE NOTIFICATION                     */
     /* Fairy-Max 4.8 is free software, and you have my permission do */
     /* with it whatever you want, whether it is commercial or not.   */
     /* Note, however, that Fairy-Max can easily be configured through*/
     /* its fmax.ini file to play Chess variants that are legally pro-*/
     /* tected by patents, and that to do so would also require per-  */
     /* mission of the holders of such patents. No guarantees are     */
     /* given that Fairy-Max does anything in particular, or that it  */
     /* would not wreck the hardware it runs on, and running it is    */
     /* entirely for your own risk.  H.G,Muller, author of Fairy-Max  */
     /*****************************************************************/


#define MULTIPATH

/* fused to generic Winboard driver */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>

#ifndef WIN32

#include <sys/time.h>
int GetTickCount() // with thanks to Tord
{	struct timeval t;
	gettimeofday(&t, NULL);
	return t.tv_sec*1000 + t.tv_usec/1000;
}
#define INI_FILE "/usr/share/games/fmax/qmax.ini"

#else

#include <windows.h>
#define INI_FILE "qmax.ini"

#endif

int StartKey;

#define EMPTY -1
#define WHITE 0
#define BLACK 16

#define STATE 128

#define FAC 128

/* make unique integer from engine move representation */
#define PACK_MOVE 256*K + L;

/* convert intger argument back to engine move representation */
#define UNPACK_MOVE(A) K = (A)>>8 & 255; L = (A) & 255;

/* Global variables visible to engine. Normally they */
/* would be replaced by the names under which these  */
/* are known to your engine, so that they can be     */
/* manipulated directly by the interface.            */

int Side;
int Move;
int PromPiece;
int Result;
int TimeLeft;
int MovesLeft;
int MaxDepth;
int Post;
int Fifty;
int UnderProm;
int pos[8]={3,0,1,2,4,5,6,7},pc[]={4,6,5,5,1,1,7,3},
    mask[]={0x18,0x81,0xAA,0x55,0xAA,0x55,0xFF,0xFF};
int GameNr;
char piecename[32], piecetype[32], defaultchar[]=".PPKNBRQEWFMACHG";
char *inifile = INI_FILE;

int Ticks, tlim, Setup, SetupQ;

int GameHistory[1024];
char HistoryBoards[1024][STATE], setupPosition[129];
int GamePtr, HistPtr;

#define W while
#define K(A,B) *(int*)(T+A+((B&31)<<8))
#define J(A) K(y+A,b[y])-K(x+A,u)-K(y+A,t)

int U=(1<<23)-1;
struct _ {int K,V;char X,Y,D,F;} *A;           /* hash table, 16M+8 entries*/

int M=136,S=128,I=8e3,Q,O,K,N,j,R,J,Z,LL,L,    /* M=0x88                   */
BW,BH,sh,
w[16]={0,2,2,-1,7,8,12,23,7,5},                /* relative piece values    */
of[256],
od[16];                                        /* 1st dir. in o[] per piece*/

char
o[256],
oo[32],                                        /* initial piece setup      */
b[513],                                        /* board: 16x8+dummy, + PST */
T[8200],                                       /* hash translation table   */
centr[32],
n[]=".P*KEEQQAHCR????x+pkeeqqahcr????";        /* piece symbols on printout*/

char zn[] = {                                  /* zones of xiangqi board   */
1,1,1,1,1,2,2,2,2,2,    0,0,0,0,0,0,
1,1,1,1,1,2,2,2,2,2,    0,0,0,0,0,0,
1,1,1,1,1,2,2,2,2,2,    0,0,0,0,0,0,
0,0,0,1,1,2,2,0,0,0,    0,0,0,0,0,0,
0,0,0,1,1,2,2,0,0,0,    0,0,0,0,0,0,
0,0,0,1,1,2,2,0,0,0,    0,0,0,0,0,0,
1,1,1,1,1,2,2,2,2,2,    0,0,0,0,0,0,
1,1,1,1,1,2,2,2,2,2,    0,0,0,0,0,0,
1,1,1,1,1,2,2,2,2,2,    0,0,0,0,0,0
};

pboard()
{int i;
 i=-1;W(++i<144)printf(" %c",(i&15)==BW&&(i+=15-BW)?10:n[b[i]&31]);
}

D(k,q,l,e,z,n)          /* recursive minimax search, k=moving side, n=depth*/
int k,q,l,e,z,n;        /* (q,l)=window, e=current eval. score, E=e.p. sqr.*/
{                       /* e=score, z=prev.dest; J,Z=hashkeys; return score*/
 int j,r,m,v,d,h,i,P,V,f=J,g=Z,C,s,flag,F;
 unsigned char t,p,u,x,y,X,Y,B,lu;
 struct _*a=A+(J+k&U-1);                       /* lookup pos. in hash table*/
 q-=q<e;l-=l<=e;                               /* adj. window: delay bonus */
 d=a->D;m=a->V;F=a->F;                         /* resume at stored depth   */
 X=a->X;Y=a->Y;                                /* start at best-move hint  */
if(z&S&&a->K==Z)printf("# root hit %d %d %x\n",a->D,a->V,a->F);
 if(a->K-Z|z&S  |                              /* miss: other pos. or empty*/
  !(m<=q|F&8&&m>=l|F&S))                       /*   or window incompatible */
  d=X=0,Y=-1;                                  /* start iter. from scratch */
 W(d++<n||d<3||              /*** min depth = 2   iterative deepening loop */
   z&S&&K==I&&(GetTickCount()-Ticks<tlim&d<=MaxDepth|| /* root: deepen upto time   */
   (K=X,L=Y,d=3)))                             /* time's up: go do best    */
 {x=B=X;lu=1;                                  /* start scan at prev. best */
  h=Y-255;                                       /* if move, request 1st try */
  P=d>2&&l+I?D(16-k,-l,1-l,-e,2*S,d-3):I;      /* search null move         */
  m=-P<l|R<5?d-2?-I:e:-P;   /*** prune if > beta  unconsidered:static eval */
  N++;                                         /* node count (for timing)  */
  do{u=b[x];                                   /* scan board looking for   */
   if(u)m=lu|u&15^3?m:(d=98,I),lu=u&15^3;        /* Kings facing each other  */
   if(u&&(u&16)==k)                            /*  own piece (inefficient!)*/
   {r=p=u&15;                                  /* p = piece type (set r>0) */
    j=od[p];                                   /* first step vector f.piece*/
    W(r=o[++j])                                /* loop over directions o[] */
    {A:                                        /* resume normal after best */
     flag=h?3:of[j];                           /* move modes (for fairies) */
     y=x;                                      /* (x,y)=move               */
     do{                                       /* y traverses ray, or:     */
      y=h?Y:y+r;                               /* sneak in prev. best move */
      if(y>=16*BH|(y&15)>=BW)break;            /* board edge hit           */
      t=b[y];                                  /* captured piece           */
      if(flag&1+!t)                            /* mode (capt/nonc) allowed?*/
      {if(t&&(t&16)==k||flag>>10&zn[y])break;  /* capture own or bad zone  */
       i=w[t&15];                              /* value of capt. piece t   */
       if(i<0)m=I,d=98;                        /* K capture                */
       if(m>=l&d>1)goto C;                     /* abort on fail high       */
       v=d-1?e:i-p;                            /*** MVV/LVA scoring if d=1**/
       if(d-!t>1)                              /*** all captures if d=2  ***/
       {v=centr[p]?b[x+257]-b[y+257]:0;        /* center positional pts.   */
        b[x]=0;b[y]=u;                         /* do move                  */
        v-=w[p]>0|R<10?0:20;                   /*** freeze K in mid-game ***/
        if(p<3)                                /* pawns:                   */
        {v+=2;                                 /* end-game Pawn-push bonus */
         if(zn[x]-zn[y])b[y]+=5,               /* upgrade Pawn and         */
          i+=w[p+5]-w[p];                      /*          promotion bonus */
        }
        if(z&S && GamePtr<6) v+=(rand()>>10&31)-16; // randomize in root
        J+=J(0);Z+=J(4);
        v+=e+i;V=m>q?m:q;                      /*** new eval & alpha    ****/
        C=d-1-(d>5&p>2&!t&!h);                 /* nw depth, reduce non-cpt.*/
        C=R<10|P-I|d<3||t&&p-3?C:d;            /* extend 1 ply if in-check */
        do
         s=C>2|v>V?-D(16-k,-l,-V,-v,/*** futility, recursive eval. of reply */
                                     0,C):v;
        W(s>q&++C<d); v=s;                     /* no fail:re-srch unreduced*/
        if(z&S&&K-I)                           /* move pending: check legal*/
        {if(v+I&&x==K&y==L)                    /*   if move found          */
         {Q=-e-i;
          a->D=99;a->V=500;                    /* lock game in hash as loss*/
          R-=i/FAC;                            /*** total captd material ***/
          Fifty = t|p<3?0:Fifty+1;
                     return l;}                /*   & not in check, signal */
         v=m;                                  /* (prevent fail-lows on    */
        }                                      /*   K-capt. replies)       */
        J=f;Z=g;
        b[y]=t;b[x]=u;                         /* undo move                */
       }                                       /*          if non-castling */
       if(v>m)                                 /* new best, update max,best*/
        m=v,X=x,Y=y;                           /* no marking!              */
       if(h){h=0;goto A;}                      /* redo after doing old best*/
      }
      s=t;
      t+=flag&4;                               /* fake capt. for nonsliding*/
      if(s&&flag&8)t=0,flag^=flag>>4&15;       /* hoppers go to next phase */
      if(!(flag&S))                            /* zig-zag piece?           */
       r^=flag>>12,flag^=flag>>4&15;           /* alternate vector & mode  */
     }W(!t);                                   /* if not capt. continue ray*/
   }}
   if((++x&15)>=BW)x=x+16&240,lu=1;            /* next sqr. of board, wrap */
   if(x>=16*BH)x=0;
  }W(x-B);           
C:if(a->D<99)                                  /* protect game history     */
   a->K=Z,a->V=m,a->D=d,a->X=X,                /* always store in hash tab */
   a->F=8*(m>q)|S*(m<l),a->Y=Y;                /* move, type (bound/exact),*/
if(z&S&&Post){
  printf("%2d ",d-2);
  printf("%6d ",m);
  printf("%8d %10d %c%c%c%c\n",(GetTickCount()-Ticks)/10,N,
     'i'-(X>>4&15),'9'-(X&15),'i'-(Y>>4&15),'9'-(Y&15)),fflush(stdout);}
 }                                             /*    encoded in X S,8 bits */
if(z&4*S)K=X,L=Y&~S;
 return m+=m<e;                                /* delayed-loss bonus       */
}


/* Generic main() for Winboard-compatible engine     */
/* (Inspired by TSCP)                                */
/* Author: H.G. Muller                               */

/* The engine is invoked through the following       */
/* subroutines, that can draw on the global vaiables */
/* that are maintained by the interface:             */
/* Side         side to move                         */
/* Move         move input to or output from engine  */
/* PromPiece    requested piece on promotion move    */
/* TimeLeft     ms left to next time control         */
/* MovesLeft    nr of moves to play within TimeLeft  */
/* MaxDepth     search-depth limit in ply            */
/* Post         boolean to invite engine babble      */

/* InitEngine() progran start-up initialization      */
/* InitGame()   initialization to start new game     */
/*              (sets Side, but not time control)    */
/* Think()      think up move from current position  */
/*              (leaves move in Move, can be invalid */
/*               if position is check- or stalemate) */
/* DoMove()     perform the move in Move             */
/*              (togglese Side)                      */
/* ReadMove()   convert input move to engine format  */
/* PrintMove()  print Move on standard output        */
/* Legal()      check Move for legality              */
/* ClearBoard() make board empty                     */
/* PutPiece()   put a piece on the board             */

/* define this to the codes used in your engine,     */
/* if the engine hasn't defined it already.          */

int PrintResult(int s)
{
        int i, j, k, cnt=0;

        /* search last 50 states with this stm for third repeat */
        for(j=2; j<=100 && j <= HistPtr; j+=2)
        {
            for(k=0; k<STATE; k++)
                if(HistoryBoards[HistPtr][k] !=
                   HistoryBoards[HistPtr-j&1023][k] )
                   {
                     goto differs;}
            /* is the same, count it */
            if(++cnt > 1) /* third repeat */
            {
                printf("1/2-1/2 {Draw by repetition}\n");
                return 1;
            }
          differs: ;
        }
        K=I;
        cnt = D(s,-I,I,Q,4*S,3);
        if(cnt==0 && K==0 && L==0) {
                printf("1/2-1/2 {Stalemate}\n");
                return 2;
        }
        if(cnt==-I+1) {
                if (s == WHITE)
                        printf("0-1 {Black mates}\n");
                else
                        printf("1-0 {White mates}\n");
                return 3;
        }
        if(Fifty >=100) {
                printf("1/2-1/2 {Draw by fifty move rule}\n");
                return 4;
        }
        return 0;
}


InitEngine()
{
 int i, j;

 N=8100;W(N-->256)T[N]=rand()>>9;
 srand(GetTickCount());
}

InitGame()
{
 int i,j;

 for(i=0;i<16*BH;i++)b[i]=0;           /* clear board   */
 b[23]=b[119]=10;b[BW+8]=b[BW+104]=26; /* place Cannons */
 K=BH;W(K--)
 {b[16*K]=oo[K+16]+16;b[16*K+BW-1]=oo[K];if(!(K&1))b[16*K+BW-7]=18,b[16*K+6]=1; /* initial board setup*/
  L=BW;W(L--)b[L+16*K+257]=(K-(BW-1)/2.)*(K-(BW-1)/2.)+(L-(BH-1)/2.)*(L-(BH-1)/2.); /* center-pts table   */
 }                                                   /*(in unused half b[])*/
 Side = WHITE; Q=0; O=S;
 Fifty = 0; R = 0;
 for(i=0; i<BW; i++) if(i!=3) R += (w[oo[i]]/FAC) + (w[oo[i+16]]/FAC);
 UnderProm = -1;
}

void CopyBoard(int s)
{
        int i, j, k, cnt=0;

        /* copy game representation of engine to HistoryBoard */
        /* don't forget castling rights and e.p. state!       */
        for(i=0; i<BH; i++)
        for(j=0; j<BW; j++)                 /* board squares  */
            HistoryBoards[s][BW*i+j] = b[16*i+j]|64*(16*i+j==O);
}
                                         
void PrintVariants()
{
        int i, j, count=0; char c, buf[80];
        FILE *f;

        f = fopen(inifile, "r");
        if(f==NULL) return;

        /* search for game names in definition file */
        do {
           while(fscanf(f, "Game: %s", buf) != 1 && c != EOF) 
               while((c = fgetc(f)) != EOF && c != '\n');
           if(c == EOF) break;
           if(count++) printf(",");
           printf("%s", buf);
        } while(c != EOF);

        fclose(f);
}
                                         
int LoadGame(char *name)
{
        int i, j, count=0; char c, buf[80];
        static int currentVariant;
        FILE *f;

        f = fopen(inifile, "r");
        if(f==NULL)
        {   printf("telluser piece-desription file '%s'  not found\n", inifile);
            exit(0);
        }
        if(fscanf(f, "version 4.8(%c)", &c)!=1 || c != 'w')
        { printf("telluser incompatible qmax.ini file\n"); exit(0); }

        if(name != NULL)
        {  /* search for game name in definition file */
           while(fscanf(f, "Game: %s", buf)!=1 || strcmp(name, buf) ) {
               while((c = fgetc(f)) != EOF && c != '\n');
               count++;
               if(c == EOF) {
                   printf("telluser variant %s not supported\n", name);
                   fclose(f);
                   return; /* keep old settings */
               }
           }
           currentVariant = count;
        }

        /* We have found variant, or if none specified, are at beginning of file */
        if(fscanf(f, "%dx%d", &BW, &BH)!=2 || BW>12 || BH>9)
        { printf("telluser unsupported board size %dx%d\n",BW,BH); exit(0); }

        for(i=0; i<BH; i++) fscanf(f, "%d", oo+i);
        for(i=0; i<BH; i++) fscanf(f, "%d", oo+i+16);

        for(i= 0; i<=U; i++)
            A[i].K = A[i].D = A[i].X = A[i].Y = A[i].F = 0; /* clear hash */
        for(i=0; i<32; i++) piecetype[i] = 0;

        i=0; j=-1; c=0;
        while(fscanf(f, "%d,%x", o+j, of+j)==2 ||
                                      fscanf(f,"%c:%d",&c, w+i+1)==2)
        {   if(c)
            { od[++i]=j; centr[i] = c>='a';
              piecetype[c&31]=i; piecename[i]=c&31;
            }
            j++; o[j]=0;
            /* printf("tell c='%c' i=%d od[i]=%d j=%d (%3d,%8x)\n",c,i,od[i],j,o[j-1],of[j-1]); /**/
            c=0; if(i>15 || j>255) break;
        }

        fclose(f);
	sh = w[7] < 250 ? 3 : 0;
}

int main(int argc, char **argv)
{
        int Computer, MaxTime, MaxMoves, TimeInc, sec, i, j;
        char line[256], command[256], c, cc;
        int m, nr;
        FILE *f;

        if(argc>1 && sscanf(argv[1], "%d", &m)==1)
        { U = (1<<m)-1; argc--; argv++; }
        A = (struct _ *) calloc(U+1, sizeof(struct _));
        if(argc>1) inifile = argv[1];

	signal(SIGINT, SIG_IGN);
        printf("tellics say     MaxQi 4.8 (F)\n");
        printf("tellics say     by H.G. Muller\n");
        InitEngine();
        LoadGame(NULL);
        InitGame();
        Computer = EMPTY;
        MaxTime  = 10000;  /* 10 sec */
        MaxDepth = 30;     /* maximum depth of your search */

        for (;;) {
		fflush(stdout);
                if (Side == Computer) {
                        /* think up & do move, measure time used  */
                        /* it is the responsibility of the engine */
                        /* to control its search time based on    */
                        /* MovesLeft, TimeLeft, MaxMoves, TimeInc */
                        /* Next 'MovesLeft' moves have to be done */
                        /* within TimeLeft+(MovesLeft-1)*TimeInc  */
                        /* If MovesLeft<0 all remaining moves of  */
                        /* the game have to be done in this time. */
                        /* If MaxMoves=1 any leftover time is lost*/
                        Ticks = GetTickCount();
                        m = MovesLeft<=0 ? 40 : MovesLeft;
                        tlim = (0.6-0.06*(BW-8))*(TimeLeft+(m-1)*TimeInc)/(m+7);
                        if(tlim>TimeLeft/15) tlim = TimeLeft/15;
                        PromPiece = 0; /* Always promote to Queen ourselves */
                        N=0;K=I;
                        if (D(Side,-I,I,Q,S,3)==I) {
                            Side ^= BLACK^WHITE;
                            if(UnderProm>=0 && UnderProm != L)
                            {    printf("tellics I hate under-promotions!\n");
                                 printf("resign { underpromotion } \n");
                                 Computer = EMPTY;
                                 continue;
                            } else UnderProm = -1;
                            printf("move ");
                            printf("%c%c%c%c",'i'-(K>>4),'9'-(K&15),
                                          'i'-(L>>4&15),'9'-(L&15));
                            printf("\n");
                            m = GetTickCount() - Ticks;

                            /* time-control accounting */
                            TimeLeft -= m;
                            TimeLeft += TimeInc;
                            if(--MovesLeft == 0) {
                                MovesLeft = MaxMoves;
                                if(MaxMoves == 1)
                                     TimeLeft  = MaxTime;
                                else TimeLeft += MaxTime;
                            }

                            GameHistory[GamePtr++] = PACK_MOVE;
                            CopyBoard(HistPtr=HistPtr+1&1023);
                            if(PrintResult(Side))
                                Computer = EMPTY;
                        } else {
                            if(!PrintResult(Side))
                                printf("resign { refuses own move }\n");
                            Computer = EMPTY;
                        }
                        continue;
		}
		if (!fgets(line, 256, stdin))
			return;
		if (line[0] == '\n')
			continue;
		sscanf(line, "%s", command);
		if (!strcmp(command, "xboard"))
			continue;
                if (!strcmp(command, "protover")) {
                        printf("feature myname=\"MaxQi 4.8F\"\n");
                        printf("feature memory=1\n");
                        printf("feature smp=1\n");
                        printf("feature setboard=0 ping=1 done=0\n");
                        printf("feature variants=\"");
                        PrintVariants();
                        printf("\" done=1\n");
                        continue;
                }
                if (!strcmp(command, "ping")) { int nr=0;
                        sscanf(line, "ping %d", &nr);
                        printf("pong %d\n", nr);
			continue;
                }
                if (!strcmp(command, "p")) {
                        pboard();
			continue;
                }
                if (!strcmp(command, "memory")) {
                        int mem, mask;
			sscanf(line+6, "%d", &mem); mem = (mem*1024*1024)/12; // max nr of hash entries
			mask = 0x7FFFFFFF; while(mask > mem) mask >>= 1;
			if(mask != U) {
			    free(A); U = mask;
			    A = (struct _ *) calloc(U+1, sizeof(struct _));
			}
			continue;
                }
		if (!strcmp(command, "new")) {
                        /* start new game */
                        LoadGame("xiangqi");
                        InitGame();
                        GamePtr   = Setup = 0;
                        GameNr++;
                        HistPtr   = 0;
                        Computer  = BLACK;
                        TimeLeft  = MaxTime;
                        MovesLeft = MaxMoves;
                        for(nr=0; nr<1024; nr++)
                            for(m=0; m<STATE; m++)
                                HistoryBoards[nr][m] = 0;
			continue;
		}
		if (!strcmp(command, "quit"))
                        /* exit engine */
			return;
		if (!strcmp(command, "force")) {
                        /* computer plays neither */
                        Computer = EMPTY;
			continue;
		}
		if (!strcmp(command, "white")) {
                        /* set white to move in current position */
                        Side     = WHITE;
                        Computer = BLACK;
			continue;
		}
		if (!strcmp(command, "black")) {
                        /* set blck to move in current position */
                        Side     = BLACK;
                        Computer = WHITE;
			continue;
		}
		if (!strcmp(command, "st")) {
                        /* move-on-the-bell mode     */
                        /* indicated by MaxMoves = 1 */
                        sscanf(line, "st %d", &MaxTime);
                        MovesLeft = MaxMoves = 1;
                        TimeLeft  = MaxTime *= 1000;
                        TimeInc   = 0;
			continue;
		}
		if (!strcmp(command, "sd")) {
                        /* set depth limit (remains in force */
                        /* until next 'sd n' command)        */
                        sscanf(line, "sd %d", &MaxDepth);
                        MaxDepth += 2; /* QS depth */
			continue;
		}
                if (!strcmp(command, "level")) {
                        /* normal or blitz time control */
                        sec = 0;
                        if(sscanf(line, "level %d %d %d",
                                 &MaxMoves, &MaxTime, &TimeInc)!=3 &&
                           sscanf(line, "level %d %d:%d %d",
                                 &MaxMoves, &MaxTime, &sec, &TimeInc)!=4)
                             continue;
                        MovesLeft = MaxMoves;
                        TimeLeft  = MaxTime = 60000*MaxTime + 1000*sec;
                        TimeInc  *= 1000;
                        continue;
                }
		if (!strcmp(command, "time")) {
                        /* set time left on clock */
                        sscanf(line, "time %d", &TimeLeft);
                        TimeLeft  *= 10; /* centi-sec to ms */
			continue;
		}
		if (!strcmp(command, "otim")) {
                        /* opponent's time (not kept, so ignore) */
			continue;
		}
		if (!strcmp(command, "easy")) {
			continue;
		}
		if (!strcmp(command, "hard")) {
			continue;
		}
		if (!strcmp(command, "accepted")) {
			continue;
		}
		if (!strcmp(command, "rejected")) {
			continue;
		}
		if (!strcmp(command, "random")) {
			continue;
		}
		if (!strcmp(command, "go")) {
                        /* set computer to play current side to move */
                        Computer = Side;
                        MovesLeft = -(GamePtr+(Side==WHITE)>>1);
                        while(MaxMoves>0 && MovesLeft<=0)
                            MovesLeft += MaxMoves;
			continue;
		}
		if (!strcmp(command, "hint")) {
                        Ticks = GetTickCount(); tlim = 1000;
                        D(Side,-I,I,Q,4*S,6);
                        if (K==0 && L==0)
				continue;
                        printf("Hint: ");
                        printf("%c%c%c%c",'a'+(K&7),'8'-(K>>4),
                                          'a'+(L&7),'8'-(L>>4));
                        printf("\n");
			continue;
		}
                if (!strcmp(command, "undo")   && (nr=1) ||
                    !strcmp(command, "remove") && (nr=2)   ) {
                        /* 'take back' moves by replaying game */
                        /* from history until desired ply      */
                        if (GamePtr - nr < 0)
				continue;
                        GamePtr -= nr;
                        HistPtr -= nr;   /* erase history boards */
                        while(nr-- > 0)  
                            for(m=0; m<STATE; m++)
                                HistoryBoards[HistPtr+nr+1&1023][m] = 0;
                        InitGame();
			if(Setup) {
			    for(i=0; i<128; i++) b[i] = setupPosition[i];
			    Side = setupPosition[128]; Q = SetupQ;
			}
			for(i=0; i<=U; i++) A[i].D = A[i].K = 0; // clear hash table
                        for(nr=0; nr<GamePtr; nr++) {
                            UNPACK_MOVE(GameHistory[nr]);
                            D(Side,-I,I,Q,S,3);
                            Side ^= BLACK^WHITE;
                        }
			continue;
		}
		if (!strcmp(command, "post")) {
                        Post = 1;
			continue;
		}
		if (!strcmp(command, "nopost")) {
                        Post = 0;
			continue;
		}
		if (!strcmp(command, "variant")) {
                        sscanf(line, "variant %s", command);
                        LoadGame(command);
                        InitGame(); Setup = 0;
			continue;
		}
                if (!strcmp(command, "edit")) {
                        int color = WHITE, p;

                        while(fgets(line, 256, stdin)) {
                                m = line[0];
                                if(m=='.') break;
                                if(m=='#') {
                                        for(i=0; i<128; i++) b[i]=0;
                                        Q=0; R=0; O=S;
                                        continue;
                                }
                                if(m=='c') {
                                        color = WHITE+BLACK - color;
                                        Q = -Q;
                                        continue;
                                }
                                if( m >= 'A' && m <= 'Z'
                                    && line[1] >= 'a' && line[1] <= 'a'+BH-1
                                    && line[2] >= '0' && line[2] <= '0'+BW-1) {
                                        m = 16*('i'-line[1])+'9'-line[2];
                                        p = 4; /* Elephant code */
                                        switch(line[0])
                                        {
                                        case 'K':
                                            b[m]=3+color;
                                            break;
                                        case 'R': p++;
                                        case 'C': p++;
                                        case 'H': p++;
                                        case 'A': p+=4;
                                            b[m]=p+color;
                                            Q+=w[p]; R+=w[p]/FAC;
                                            break;
                                        case 'P':
                                            if((color==WHITE) == ((m&15)<5))
                                                p = 6; else p = 1;
                                        case 'E': p+=(color==BLACK);
                                            b[m]=p+color;
                                            Q+=w[p]; R+=w[p]/FAC;
                                            break;
                                        }
                                        continue;
                                }
                        }
                        if(Side != color) Q = -Q;
			GamePtr = HistPtr = 0; Setup = 1; SetupQ = Q; // start anew
			for(i=0; i<128; i++) setupPosition[i] = b[i]; // remember position
			setupPosition[128] = Side;
			continue;
		}
                /* command not recognized, assume input move */
                m = line[0]<'a' | line[0]>='a'+BH | line[1]<'0' | line[1]>='0'+BW |
                    line[2]<'a' | line[2]>='a'+BH | line[3]<'0' | line[3]>='0'+BW |
                    line[4] != '\n';
                {char *c=line; K=16*('i'-c[0])+'9'-c[1];
                               L=16*('i'-c[2])+'9'-c[3]; }
                if (m)
                        /* doesn't have move syntax */
			printf("Error (unknown command): %s\n", command);
                else if(D(Side,-I,I,Q,S,3)!=I) {
                        /* did have move syntax, but illegal move */
                        printf("Illegal move:%s\n", line);
                } else {  /* legal move, perform it */
                        GameHistory[GamePtr++] = PACK_MOVE;
                        Side ^= BLACK^WHITE;
                        CopyBoard(HistPtr=HistPtr+1&1023);
                        if(PrintResult(Side)) Computer = EMPTY;
		}
	}
}



