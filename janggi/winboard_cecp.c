/********************************************************/
/* Example of a WinBoard-protocol driver, by H.G.Muller */
/********************************************************/

#include <stdio.h>

// four different constants, with values for WHITE and BLACK that suit your engine
#define WHITE   1
#define BLACK   2
#define NONE    0
#define ANALYZE  3

// some value that cannot occur as a valid move
#define INVALID 666

// some parameter of your engine
#define MAXMOVES 500  /* maximum game length  */
#define MAXPLY   60   /* maximum search depth */

#define OFF 0
#define ON  1

#define DEFAULT_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

typedef int MOVE;        // in this example moves are encoded as an int

int moveNr;              // part of game state; incremented by MakeMove
MOVE gameMove[MAXMOVES]; // holds the game history

// Some routines your engine should have to do the various essential things
int  MakeMove(int stm, MOVE move);      // performs move, and returns new side to move
void UnMake(MOVE move);                 // unmakes the move;
int  Setup(char *fen);                  // sets up the position from the given FEN, and returns the new side to move
void SetMemorySize(int n);              // if n is different from last time, resize all tables to make memory usage below n MB
char *MoveToText(MOVE move);            // converts the move from your internal format to text like e2e2, e1g1, a7a8q.
MOVE ParseMove(char *moveText);         // converts a long-algebraic text move to your internal move format
int  SearchBestMove(int stm, int timeLeft, int mps, int timeControl, int inc, int timePerMove, MOVE *move, MOVE *ponderMove);
void PonderUntilInput(int stm);         // Search current position for stm, deepening forever until there is input.

// Some global variables that control your engine's behavior
int ponder;
int randomize;
int postThinking;
int resign;         // engine-defined option
int contemptFactor; // likewise

int TakeBack(int n)
{ // reset the game and then replay it to the desired point
  int last, stm;
  stm = Setup(NULL);
  last = moveNr - n; if(last < 0) last = 0;
  for(moveNr=0; moveNr<last; moveNr++) stm = MakeMove(stm, gameMove[moveNr]);
  return stm;
}

void PrintResult(int stm, int score)
{
  if(score == 0) printf("1/2-1/2\n");
  if(score > 0 && stm == WHITE || score < 0 && stm == BLACK) printf("1-0\n");
  else printf("0-1\n");
}

int main()
{
  int stm;                                 // side to move
  int engineSide=NONE;                     // side played by engine
  int timeLeft;                            // timeleft on engine's clock
  int mps, timeControl, inc, timePerMove;  // time-control parameters, to be used by Search
  int maxDepth;                            // used by search
  MOVE move, ponderMove;
  int i, score;
  char inBuf[80], command[80];

  while(1) { // infinite loop

    fflush(stdout);                 // make sure everything is printed before we do something that might take time

    if(stm == engineSide) {         // if it is the engine's turn to move, set it thinking, and let it move
 
      score = SearchBestMove(stm, timeLeft, mps, timeControl, inc, timePerMove, &move, &ponderMove);

      if(move == INVALID) {         // game apparently ended
        engineSide = NONE;          // so stop playing
        PrintResult(stm, score);
      } else {
        stm = MakeMove(stm, move);  // assumes MakeMove returns new side to move
        gameMove[moveNr++] = move;  // remember game
   printf("move %s\n", MoveToText(move));
      }
    }

    fflush(stdout); // make sure everything is printed before we do something that might take time

    // now it is not our turn (anymore)
    if(engineSide == ANALYZE) {       // in analysis, we always ponder the position
        PonderUntilInput(stm);
    } else
    if(engineSide != NONE && ponder == ON && moveNr != 0) { // ponder while waiting for input
      if(ponderMove == INVALID) {     // if we have no move to ponder on, ponder the position
        PonderUntilInput(stm);
      } else {
        int newStm = MakeMove(stm, ponderMove);
        PonderUntilInput(newStm);
        UnMake(ponderMove);
      }
    }

  noPonder:
    // wait for input, and read it until we have collected a complete line
    for(i = 0; (inBuf[i] = getchar()) != '\n'; i++);
    inBuf[i+1] = 0;

    // extract the first word
    sscanf(inBuf, "%s", command);

    // recognize the command,and execute it
    if(!strcmp(command, "quit"))    { break; } // breaks out of infinite loop
    if(!strcmp(command, "force"))   { engineSide = NONE;    continue; }
    if(!strcmp(command, "analyze")) { engineSide = ANALYZE; continue; }
    if(!strcmp(command, "exit"))    { engineSide = NONE;    continue; }
    if(!strcmp(command, "otim"))    { goto noPonder; } // do not start pondering after receiving time commands, as move will follow immediately
    if(!strcmp(command, "time"))    { sscanf(inBuf, "time %d", &timeLeft); goto noPonder; }
    if(!strcmp(command, "level"))   {
      int min, sec=0;
      sscanf(inBuf, "level %d %d %d", &mps, &min, &inc) == 3 ||  // if this does not work, it must be min:sec format
      sscanf(inBuf, "level %d %d:%d %d", &mps, &min, &sec, &inc);
      timeControl = 60*min + sec; timePerMove = -1;
      continue;
    }
    if(!strcmp(command, "protover")){
      printf("feature ping=1 setboard=1 colors=0 usermove=1 memory=1 debug=1");
      printf("feature option=\"Resign -check 0\"");           // example of an engine-defined option
      printf("feature option=\"Contempt -spin 0 -200 200\""); // and another one
      printf("feature done=1");
      continue;
    }
    if(!strcmp(command, "option")) { // setting of engine-define option; find out which
      if(sscanf(inBuf+7, "Resign=%d",   &resign)         == 1) continue;
      if(sscanf(inBuf+7, "Contempt=%d", &contemptFactor) == 1) continue;
      continue;
    }
    if(!strcmp(command, "sd"))      { sscanf(inBuf, "sd %d", &maxDepth);    continue; }
    if(!strcmp(command, "st"))      { sscanf(inBuf, "st %d", &timePerMove); continue; }
    if(!strcmp(command, "memory"))  { SetMemorySize(atoi(inBuf+7)); continue; }
    if(!strcmp(command, "ping"))    { printf("pong%s", inBuf+4); continue; }
//  if(!strcmp(command, ""))        { sscanf(inBuf, " %d", &); continue; }
    if(!strcmp(command, "new"))     { engineSide = BLACK; stm = Setup(DEFAULT_FEN); maxDepth = MAXPLY; randomize = OFF; continue; }
    if(!strcmp(command, "setboard")){ engineSide = NONE;  stm = Setup(inBuf+9); continue; }
    if(!strcmp(command, "easy"))    { ponder = OFF; continue; }
    if(!strcmp(command, "hard"))    { ponder = ON;  continue; }
    if(!strcmp(command, "undo"))    { stm = TakeBack(1); continue; }
    if(!strcmp(command, "remove"))  { stm = TakeBack(2); continue; }
    if(!strcmp(command, "go"))      { engineSide = stm;  continue; }
    if(!strcmp(command, "post"))    { postThinking = ON; continue; }
    if(!strcmp(command, "nopost"))  { postThinking = OFF;continue; }
    if(!strcmp(command, "random"))  { randomize = ON;    continue; }
    if(!strcmp(command, "hint"))    { if(ponderMove != INVALID) printf("Hint: %s\n", MoveToText(ponderMove)); continue; }
    if(!strcmp(command, "book"))    {  continue; }
    // ignored commands:
    if(!strcmp(command, "xboard"))  { continue; }
    if(!strcmp(command, "computer")){ continue; }
    if(!strcmp(command, "name"))    { continue; }
    if(!strcmp(command, "ics"))     { continue; }
    if(!strcmp(command, "accepted")){ continue; }
    if(!strcmp(command, "rejected")){ continue; }
    if(!strcmp(command, "variant")) { continue; }
    if(!strcmp(command, ""))  {  continue; }
    if(!strcmp(command, "usermove")){
      int move = ParseMove(inBuf+9);
      if(move == INVALID) printf("Illegal move\n");
      else {
        stm = MakeMove(stm, move);
   ponderMove = INVALID;
        gameMove[moveNr++] = move;  // remember game
      }
      continue;
    }
    printf("Error: unknown command\n");
  }
  return 0;
}
