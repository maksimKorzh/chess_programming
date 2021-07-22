DECLARE SUB IO (A%, B%, X%, Y%, RESULT%)
DECLARE SUB SHOWBD ()
DECLARE FUNCTION EVALUATE% (ID%, PRUNE%)
DECLARE SUB SHOWMAN (A%, B%, FLAG%)
DECLARE SUB MOVELIST (A%, B%, XX%(), YY%(), NDX%)
DECLARE SUB MAKEMOVE (A%, B%, X%, Y%)
DECLARE SUB PAWN (A%, B%, XX%(), YY%(), NDX%)
DECLARE SUB KNIGHT (A%, B%, XX%(), YY%(), NDX%)
DECLARE SUB BISHOP (A%, B%, XX%(), YY%(), NDX%)
DECLARE SUB ROOK (A%, B%, XX%(), YY%(), NDX%)
DECLARE SUB QUEEN (A%, B%, XX%(), YY%(), NDX%)
DECLARE SUB KING (A%, B%, XX%(), YY%(), NDX%)
DECLARE FUNCTION INCHECK% (X%)
DECLARE SUB SQUARE (A%, B%, C%)
'How To Write A Chess Programm in QBASIC
'By Dean Menezes

'Since the 1950's, people have been trying to make progrms and machines that play chess. This tutorial
'tries to teach chess programming by using a commented QuickBASIC program:

'This is basically a chess program I wrote
'It includes comments to show some of the theory on making a chess program
'You can modify it to include things like opening book, mouse/graphics, etc.
DEFINT A-Z 'to speed things up
' First, you need an 8 by 8 array for the board:
DIM SHARED BOARD(0 TO 7, 0 TO 7)
' Then, you need to store a list of best moves:
DIM SHARED BESTA(0 TO 7), BESTB(0 TO 7), BESTX(0 TO 7), BESTY(0 TO 7)
' Since the evaluation subroutine is recursive, you also need to store current level and maximum level
DIM SHARED LEVEL, MAXLEVEL, SCORE, CFLAG
CFLAG = 0
LEVEL = 0
MAXLEVEL = 3
' Now we need to fill the starting position into the board array
DATA -500,-270,-300,-900,-7500,-300,-270,-500
DATA -100,-100,-100,-100, -100,-100,-100,-100
DATA 0, 0, 0, 0, 0, 0, 0, 0
DATA 0, 0, 0, 0, 0, 0, 0, 0
DATA 0, 0, 0, 0, 0, 0, 0, 0
DATA 0, 0, 0, 0, 0, 0, 0, 0
DATA 100, 100, 100, 100, 100, 100, 100, 100
DATA 500, 270, 300, 900, 5000, 300, 270, 500
FOR X = 0 TO 7
 FOR Y = 0 TO 7
   READ Z
   BOARD(X, Y) = Z
 NEXT Y
NEXT X
A = -1
RESULT = 0
' Now we make the main loop:
DO
 SCORE = 0
 CALL IO(A, B, X, Y, RESULT) ' Get white's move
 CLS
 CALL SHOWBD ' Update board to show white's move
 RESULT = EVALUATE(-1, 10000) 'Get black's move
 A = BESTA(1) 'start column for black's move
 B = BESTB(1) 'start row for black's move
 X = BESTX(1) 'end column for black's move
 Y = BESTY(1) 'end rom for black's move
LOOP

'Gen. for bishop
SUB BISHOP (A, B, XX(), YY(), NDX)
 ID = SGN(BOARD(B, A))
 FOR DXY = 1 TO 7 'work out diagonally one direction
   X = A - DXY
   Y = B + DXY
   IF X < 0 OR X > 7 OR Y < 0 OR Y > 7 THEN EXIT FOR 'stop when go off board
   GOSUB 3
   IF BOARD(Y, X) THEN EXIT FOR 'stop when hit piece
 NEXT
 FOR DXY = 1 TO 7
   X = A + DXY
   Y = B + DXY
   IF X < 0 OR X > 7 OR Y < 0 OR Y > 7 THEN EXIT FOR
   GOSUB 3
   IF BOARD(Y, X) THEN EXIT FOR
 NEXT
 FOR DXY = 1 TO 7
   X = A - DXY
   Y = B - DXY
   IF X < 0 OR X > 7 OR Y < 0 OR Y > 7 THEN EXIT FOR
   GOSUB 3
   IF BOARD(Y, X) THEN EXIT FOR
 NEXT
 FOR DXY = 1 TO 7
   X = A + DXY
   Y = B - DXY
   IF X < 0 OR X > 7 OR Y < 0 OR Y > 7 THEN EXIT FOR
   GOSUB 3
   IF BOARD(Y, X) THEN EXIT FOR
 NEXT
 EXIT SUB
3 REM
 IF ID <> SGN(BOARD(Y, X)) THEN 'make sure no piece of same color
   NDX = NDX + 1
   XX(NDX) = X
   YY(NDX) = Y
 END IF
 RETURN
END SUB

' Now we're going to look at the evaluation function
FUNCTION EVALUATE (ID, PRUNE)
 DIM XX(0 TO 26), YY(0 TO 26)
 LEVEL = LEVEL + 1 ' Update recursion level
 BESTSCORE = 10000 * ID
 FOR B = 7 TO 0 STEP -1 ' Loop through each square
   FOR A = 7 TO 0 STEP -1
     IF SGN(BOARD(B, A)) <> ID THEN GOTO 1 ' If the square does not have right color piece go to next square
     IF (LEVEL = 1) THEN CALL SHOWMAN(A, B, 8) ' This is just to show the move currently trying
     CALL MOVELIST(A, B, XX(), YY(), NDX) ' Get list of moves for current piece
     FOR I = 0 TO NDX ' Loop through each possible move
       X = XX(I)
       Y = YY(I)
       IF LEVEL = 1 THEN
         LOCATE 1, 1
         PRINT "TRYING: "; CHR$(65 + A); 8 - B; "-"; CHR$(65 + X); 8 - Y ' show the move currently trying
         CALL SHOWMAN(X, Y, 8)
       END IF
       OLDSCORE = SCORE
       MOVER = BOARD(B, A) ' Store these locations
       TARGET = BOARD(Y, X) ' so we can set the move back
       CALL MAKEMOVE(A, B, X, Y) ' Make the move so we can evaluate
       IF (LEVEL < MAXLEVEL) THEN SCORE = SCORE + EVALUATE(-ID, BESTSCORE - TARGET + ID * (8 - ABS(4 - X) - ABS(4 - Y)))
       SCORE = SCORE + TARGET - ID * (8 - ABS(4 - X) - ABS(4 - Y)) 'work out score for move
       IF (ID < 0 AND SCORE > BESTSCORE) OR (ID > 0 AND SCORE < BESTSCORE) THEN 'update current best score
         BESTA(LEVEL) = A
         BESTB(LEVEL) = B
         BESTX(LEVEL) = X
         BESTY(LEVEL) = Y
         BESTSCORE = SCORE
         IF (ID < 0 AND BESTSCORE >= PRUNE) OR (ID > 0 AND BESTSCORE <= PRUNE) THEN 'prune to avoid wasting time
           BOARD(B, A) = MOVER 'reset position back to before modified
           BOARD(Y, X) = TARGET
           SCORE = OLDSCORE
           IF (LEVEL = 1) THEN CALL SHOWMAN(X, Y, 0) 'Show the move currently trying
           IF (LEVEL = 1) THEN CALL SHOWMAN(A, B, 0) 'Show the move currently trying
           LEVEL = LEVEL - 1 'update recursion level
           EVALUATE = BESTSCORE 'return
           EXIT FUNCTION
         END IF
       END IF
       BOARD(B, A) = MOVER 'reset position back to before modified
       BOARD(Y, X) = TARGET
       SCORE = OLDSCORE
       IF (LEVEL = 1) THEN CALL SHOWMAN(X, Y, 0) 'show move currently trying
1   NEXT
     IF (LEVEL = 1) THEN CALL SHOWMAN(A, B, 0) 'show move currently trying
  NEXT
   NEXT
   LEVEL = LEVEL - 1 'update recursion level
   EVALUATE = BESTSCORE 'return
 END FUNCTION

FUNCTION INCHECK (X)
 DIM XX(27), YY(27), NDX
 FOR B = 0 TO 7
   FOR A = 0 TO 7
     IF BOARD(B, A) >= 0 THEN GOTO 6
     CALL MOVELIST(A, B, XX(), YY(), NDX)
     FOR I = 0 TO NDX STEP 1
       X = XX(I)
       Y = YY(I)
       IF BOARD(Y, X) = 5000 THEN
         PRINT "YOU ARE IN CHECK!"
         PRINT " "
         PRINT " "
         INCHECK = 1
         EXIT FUNCTION
       END IF
     NEXT
6 NEXT
   NEXT
   INCHECK = 0
END FUNCTION

'Routine to get player move
SUB IO (A, B, X, Y, RESULT)
 DIM XX(0 TO 26), YY(0 TO 26)
 CLS
 IF A >= 0 THEN
   IF RESULT < -2500 THEN
     PRINT "I RESIGN"
     SLEEP
     SYSTEM
   END IF
   PIECE = BOARD(Y, X)
   CALL MAKEMOVE(A, B, X, Y)
   PRINT "MY MOVE: "; CHR$(65 + A); 8 - B; "-"; CHR$(65 + X); 8 - Y 'show computer move
   IF PIECE THEN
     PRINT "I TOOK YOUR ";
     IF PIECE = 100 THEN PRINT "PAWN"
     IF PIECE = 270 THEN PRINT "KNIGHT"
     IF PIECE = 300 THEN PRINT "BISHOP"
     IF PIECE = 500 THEN PRINT "ROOK"
     IF PIECE = 900 THEN PRINT "QUEEN"
     IF PIECE = 5000 THEN PRINT "KING"
   END IF
   NULL = INCHECK(0)
 END IF
 DO
   CALL SHOWBD
   LOCATE 24, 1
   INPUT "YOUR MOVE (ex: E2-E4): ", IN$
   IF UCASE$(IN$) = "QUIT" THEN CLS : END
   IF UCASE$(IN$) = "O-O" OR IN$ = "0-0" THEN
     IF CFLAG THEN GOTO 16
     IF BOARD(7, 7) <> 500 THEN GOTO 16
     IF BOARD(7, 6) OR BOARD(7, 5) THEN GOTO 16
     BOARD(7, 6) = 5000
     BOARD(7, 4) = 0
     BOARD(7, 5) = 500
     BOARD(7, 7) = 0
     CFLAG = 1
     EXIT SUB
   END IF
   IF UCASE$(IN$) = "O-O-O" OR IN$ = "0-0-0" THEN
     IF CFLAG THEN GOTO 16
     IF BOARD(7, 0) <> 500 THEN GOTO 16
     IF BOARD(7, 1) OR BOARD(7, 2) OR BOARD(7, 3) THEN GOTO 16
     BOARD(7, 2) = 5000
     BOARD(7, 4) = 0
     BOARD(7, 3) = 500
     BOARD(7, 0) = 0
     CFLAG = 1
     EXIT SUB
   END IF
   IF LEN(IN$) < 5 THEN GOTO 16
   B = 8 - (ASC(MID$(IN$, 2, 1)) - 48)
   A = ASC(UCASE$(MID$(IN$, 1, 1))) - 65
   X = ASC(UCASE$(MID$(IN$, 4, 1))) - 65
   Y = 8 - (ASC(MID$(IN$, 5, 1)) - 48)
   IF B > 7 OR B < 0 OR A > 7 OR A < 0 OR X > 7 OR X < 0 OR Y > 7 OR Y < 0 THEN GOTO 16
   IF BOARD(B, A) <= 0 THEN GOTO 16
   CALL MOVELIST(A, B, XX(), YY(), NDX)
   FOR K = 0 TO NDX STEP 1 'validate move
     IF X = XX(K) AND Y = YY(K) THEN
       MOVER = BOARD(B, A)
       TARGET = BOARD(Y, X)
       CALL MAKEMOVE(A, B, X, Y)
       LOCATE 1, 1
       IF INCHECK(0) = 0 THEN EXIT SUB 'make sure move out of check
       BOARD(B, A) = MOVER  'if not move out of check reset board
       BOARD(Y, X) = TARGET
       GOTO 16
     END IF
   NEXT
16 CLS
 LOOP
END SUB

'gen for king
SUB KING (A, B, XX(), YY(), NDX)
 ID = SGN(BOARD(B, A))
 FOR DY = -1 TO 1 'go through each of 8 king moves, checking for same color and off board
   IF B + DY < 0 OR B + DY > 7 THEN GOTO 12
   FOR DX = -1 TO 1
     IF A + DX < 0 OR A + DX > 7 THEN GOTO 11
     IF ID <> SGN(BOARD(B + DY, A + DX)) THEN
       NDX = NDX + 1
       XX(NDX) = A + DX
       YY(NDX) = B + DY
     END IF
11 NEXT
12 NEXT
END SUB

' Move list generator for knight
SUB KNIGHT (A, B, XX(), YY(), NDX)
 ID = SGN(BOARD(B, A)) 'get color
 X = A - 1 'work out each of the knight's eight moves
 Y = B - 2
 GOSUB 5
 X = A - 2
 Y = B - 1
 GOSUB 5
 X = A + 1
 Y = B - 2
 GOSUB 5
 X = A + 2
 Y = B - 1
 GOSUB 5
 X = A - 1
 Y = B + 2
 GOSUB 5
 X = A - 2
 Y = B + 1
 GOSUB 5
 X = A + 1
 Y = B + 2
 GOSUB 5
 X = A + 2
 Y = B + 1
 GOSUB 5
 EXIT SUB
5 IF X < 0 OR X > 7 OR Y < 0 OR Y > 7 THEN RETURN 'make sure on board
 IF ID <> SGN(BOARD(Y, X)) THEN NDX = NDX + 1: XX(NDX) = X: YY(NDX) = Y 'make sure no piece of same color
 RETURN
END SUB

'Routine to make a move on the chessboard
SUB MAKEMOVE (A, B, X, Y)
 BOARD(Y, X) = BOARD(B, A) 'the piece moves to the target square
 BOARD(B, A) = 0 'the old square is now empty
 IF Y = 0 AND BOARD(Y, X) = 100 THEN BOARD(Y, X) = 900 'simple pawn promotion routine
 IF Y = 7 AND BOARD(Y, X) = -100 THEN BOARD(Y, X) = -900
END SUB

'This generates a list of moves
SUB MOVELIST (A, B, XX(), YY(), NDX)
 PIECE = INT(ABS(BOARD(B, A))) 'get value corresponding to piece
 NDX = -1
 IF PIECE = 100 THEN
   CALL PAWN(A, B, XX(), YY(), NDX) 'call proper move listing routine depending on piece
 ELSEIF PIECE = 270 THEN CALL KNIGHT(A, B, XX(), YY(), NDX)
 ELSEIF PIECE = 300 THEN CALL BISHOP(A, B, XX(), YY(), NDX)
 ELSEIF PIECE = 500 THEN CALL ROOK(A, B, XX(), YY(), NDX)
 ELSEIF PIECE = 900 THEN CALL QUEEN(A, B, XX(), YY(), NDX)
 ELSE CALL KING(A, B, XX(), YY(), NDX)
 END IF
END SUB

' This is the move-list generator for the pawn
SUB PAWN (A, B, XX(), YY(), NDX)
 ID = SGN(BOARD(B, A)) 'get color
 IF (A - 1) >= 0 AND (A - 1) <= 7 AND (B - ID) >= 0 AND (B - ID) <= 7 THEN 'capture
   IF SGN(BOARD((B - ID), (A - 1))) = -ID THEN 'make sure there is piece to capture
     NDX = NDX + 1
     XX(NDX) = A - 1
     YY(NDX) = B - ID
   END IF
 END IF
 IF (A + 1) >= 0 AND (A + 1) <= 7 AND (B - ID) >= 0 AND (B - ID) <= 7 THEN  'capture
   IF SGN(BOARD((B - ID), (A + 1))) = -ID THEN 'make sure there is piece to capture
     NDX = NDX + 1
     XX(NDX) = A + 1
     YY(NDX) = B - ID
   END IF
 END IF
 IF A >= 0 AND A <= 7 AND (B - ID) >= 0 AND (B - ID) <= 7 THEN 'one square forward
   IF BOARD((B - ID), A) = 0 THEN 'make sure square is empty
     NDX = NDX + 1
     XX(NDX) = A
     YY(NDX) = B - ID
     IF (ID < 0 AND B = 1) OR (ID > 0 AND B = 6) THEN '2 squares forward
       IF BOARD((B - ID - ID), A) = 0 THEN  'make sure square is empty
         NDX = NDX + 1
         XX(NDX) = A
         YY(NDX) = B - 2 * ID
       END IF
     END IF
   END IF
 END IF
END SUB

'gen for queen
SUB QUEEN (A, B, XX(), YY(), NDX)
 CALL BISHOP(A, B, XX(), YY(), NDX) 'queen's move = bishop + rook
 CALL ROOK(A, B, XX(), YY(), NDX)
END SUB

'Gen for rook
SUB ROOK (A, B, XX(), YY(), NDX)
 ID = SGN(BOARD(B, A))
 FOR X = A - 1 TO 0 STEP -1 'work out vert/horiz each dir.
   IF ID <> SGN(BOARD(B, X)) THEN 'make sure no piece of same color
     NDX = NDX + 1
     XX(NDX) = X
     YY(NDX) = B
   END IF
   IF (BOARD(B, X)) THEN EXIT FOR
 NEXT
 FOR X = A + 1 TO 7 STEP 1
   IF ID <> SGN(BOARD(B, X)) THEN
     NDX = NDX + 1
     XX(NDX) = X
     YY(NDX) = B
   END IF
   IF (BOARD(B, X)) THEN EXIT FOR
 NEXT
 FOR Y = B - 1 TO 0 STEP -1
   IF ID <> SGN(BOARD(Y, A)) THEN
     NDX = NDX + 1
     XX(NDX) = A
     YY(NDX) = Y
   END IF
   IF (BOARD(Y, A)) THEN EXIT FOR
 NEXT
 FOR Y = B + 1 TO 7 STEP 1
   IF ID <> SGN(BOARD(Y, A)) THEN
     NDX = NDX + 1
     XX(NDX) = A
     YY(NDX) = Y
   END IF
   IF (BOARD(Y, A)) THEN EXIT FOR
 NEXT
END SUB

'Show board
SUB SHOWBD
 LOCATE 3, 30
 COLOR 7, 0
 PRINT "A  B  C  D  E  F  G  H"
 FOR K = 0 TO 25
   LOCATE 4, 28 + K
   COLOR 6, 0
   PRINT CHR$(220)
 NEXT
 FOR B = 0 TO 7
   LOCATE 2 * B + 5, 26
   COLOR 7, 0
   PRINT CHR$(56 - B)
   LOCATE 2 * B + 5, 28
   COLOR 6, 0
   PRINT CHR$(219)
   LOCATE 2 * B + 6, 28
   COLOR 6, 0
   PRINT CHR$(219)
   FOR A = 0 TO 7
     IF ((A + B) MOD 2) THEN
       COLOUR = 8
     ELSE COLOUR = 12
     END IF
     CALL SQUARE(3 * A + 31, 2 * B + 5, COLOUR)
   NEXT
   LOCATE 2 * B + 5, 53
   COLOR 6, 0
   PRINT CHR$(219)
   LOCATE 2 * B + 6, 53
   COLOR 6, 0
   PRINT CHR$(219)
   LOCATE 2 * B + 6, 55
   COLOR 7, 0
   PRINT CHR$(56 - B)
 NEXT
 FOR K = 0 TO 25
   LOCATE 21, 28 + K
   COLOR 6, 0
   PRINT CHR$(223)
 NEXT
 LOCATE 22, 30
 COLOR 7, 0
 PRINT "A  B  C  D  E  F  G  H"
 FOR B = 0 TO 7
   FOR A = 0 TO 7
     CALL SHOWMAN(A, B, 0)
   NEXT
 NEXT
 COLOR 7, 0
END SUB

'Show piece
SUB SHOWMAN (A, B, FLAG)
 IF BOARD(B, A) < 0 THEN BACK = 0
 IF BOARD(B, A) > 0 THEN BACK = 7
 FORE = 7 - BACK + FLAG
 IF BOARD(B, A) = 0 THEN
   IF (A + B) AND 1 THEN BACK = 8 ELSE BACK = 12
   FORE = BACK + -1 * (FLAG > 0)
 END IF
 N$ = " "
 PIECE = INT(ABS(BOARD(B, A)))
 IF PIECE = 0 THEN N$ = CHR$(219)
 IF PIECE = 100 THEN N$ = "P"
 IF PIECE = 270 THEN N$ = "N"
 IF PIECE = 300 THEN N$ = "B"
 IF PIECE = 500 THEN N$ = "R"
 IF PIECE = 900 THEN N$ = "Q"
 IF PIECE = 5000 OR PIECE = 7500 THEN N$ = "K"
 LOCATE 2 * B + 5 - (BOARD(B, A) > 0), 3 * A + 30
 COLOR FORE, BACK
 PRINT N$
 LOCATE 1, 1
 COLOR 7, 0
END SUB

'Display a square
SUB SQUARE (A, B, C)
 MT$ = CHR$(219)
 MT$ = MT$ + MT$ + MT$
 LOCATE B, A - 2
 COLOR C, C
 PRINT MT$
 LOCATE B + 1, A - 2
 COLOR C, C
 PRINT MT$
 COLOR 7, 0
END SUB

