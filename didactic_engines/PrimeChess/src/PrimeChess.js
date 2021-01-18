//
//    +-----------------------------------------------+
//    |           0x88 REPRESENTATION           |
//    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  8 |00|01|02|03|04|05|06|07|08|09|0A|0B|0C|0D|0E|0F|
//  7 |10|11|12|13|14|15|16|17|18|19|1A|1B|1C|1D|1E|1F|
//  6 |20|21|22|23|24|25|26|27|28|29|2A|2B|2C|2D|2E|2F|
//  5 |30|31|32|33|34|35|36|37|38|39|3A|3B|3C|3D|3E|3F|
//  4 |40|41|42|43|44|45|46|47|48|49|4A|4B|4C|4D|4E|4F|
//  3 |50|51|52|53|54|55|56|57|58|59|5A|5B|5C|5D|5E|5F|
//  2 |60|61|62|63|64|65|66|67|68|69|6A|6B|6C|6D|6E|6F|
//  1 |70|71|72|73|74|75|76|77|78|79|7A|7B|7C|7D|7E|7F|
//    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//      A  B  C  D  E  F  G  H
//
//  +---------------------------------+   +------------------------------------+
//  |          PIECE ENCODING         |   |             MOVE FLAGS             |
//  +-----+-----+------+--------------+   +-----------+------+-----------------+
//  | DEC | HEX | BIN  | DESCRIPTION  |   | BINARY    | HEX  | DESCRIPTION     |
//  +-----+-----+------+--------------+   +-----------+------+-----------------+
//  |   0 |   0 | 0000 | NONE         |   | 0000 0001 | 0x01 | K-SIDE CASTLING |
//  |   1 |   1 | 0001 | WHITE KING   |   | 0000 0010 | 0x02 | Q-SIDE CASTLING |
//  |   2 |   2 | 0010 | WHITE PAWN   |   | 0000 0100 | 0x04 | PAWN 2 SQUARES  |
//  |   3 |   3 | 0011 | WHITE KNIGHT |   | 0000 1000 | 0x08 | EN PASSANT      |
//  |   4 |   4 | 0100 | WHITE BISHOP |   | 0001 0000 | 0x10 | PROMOTION       |
//  |   5 |   5 | 0101 | WHITE ROOK   |   | 0010 0000 | 0x20 | PAWN OR CAPTURE |
//  |   6 |   6 | 0110 | WHITE QUEEN  |   | 0100 0000 | 0x40 | NOT YET USED    |
//  |   7 |   7 | 0111 |              |   | 1000 0000 | 0x80 | NOT YET USED    |
//  |   8 |   8 | 1000 |              |   +-----------+------+-----------------+
//  |   9 |   9 | 1001 | BLACK KING   |
//  |  10 |   A | 1010 | BLACK PAWN   |
//  |  11 |   B | 1011 | BLACK KNIGHT |
//  |  12 |   C | 1100 | BLACK BISHOP |
//  |  13 |   D | 1101 | BLACK ROOK   |
//  |  14 |   E | 1110 | BLACK QUEEN  |
//  |  15 |   F | 1111 |              |
//  +-----+-----+------+--------------+
//  | PIECE_COLOR_MASK  = 1000 = 0x08 |
//  | PIECE_TYPE_MASK   = 0111 = 0x07 |
//  | PIECE_SLIDER_MASK = 0100 = 0x04 |
//  +---------------------------------+
//
////////////////////////////////////////////////////////////////
//  CONSTANTS                                                 //
////////////////////////////////////////////////////////////////
const STARTING_FEN = 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1';
const NULL = 0;
const KING = 1;
const PAWN = 2;
const KNIGHT = 3;
const BISHOP = 4;
const ROOK = 5;
const QUEEN = 6;
const WHITE = 0;
const BLACK = 1;
const WHITE_KING = makePiece(WHITE, KING);
const WHITE_PAWN = makePiece(WHITE, PAWN);
const WHITE_KNIGHT = makePiece(WHITE, KNIGHT);
const WHITE_BISHOP = makePiece(WHITE, BISHOP);
const WHITE_ROOK = makePiece(WHITE, ROOK);
const WHITE_QUEEN = makePiece(WHITE, QUEEN);
const BLACK_KING = makePiece(BLACK, KING);
const BLACK_PAWN = makePiece(BLACK, PAWN);
const BLACK_KNIGHT = makePiece(BLACK, KNIGHT);
const BLACK_BISHOP = makePiece(BLACK, BISHOP);
const BLACK_ROOK = makePiece(BLACK, ROOK);
const BLACK_QUEEN = makePiece(BLACK, QUEEN);
const UP = -16;
const RIGHT = +1;
const DOWN = +16;
const LEFT = -1;
const PIECE_COLOR_MASK = 0x08;
const PIECE_TYPE_MASK = 0x07;
const PIECE_SLIDER_MASK = 0x04;
const OUT_OF_BOARD_MASK = 0x88;
const FILE_MASK = 0x0F;
const RANK_MASK = 0xF0;
const RANK_1 = 0x70;
const RANK_2 = 0x60;
const RANK_7 = 0x10;
const RANK_8 = 0x00;
const PAWN_STARTING_RANK = [RANK_2, RANK_7];
const PAWN_PROMOTING_RANK = [RANK_8, RANK_1];
const SQUARE_NULL = 0x7F;
const A1 = 0x70;
const H1 = 0x77;
const A8 = 0x00;
const H8 = 0x07;
const KINGSIDE_CASTLING_BIT = 0x01;
const QUEENSIDE_CASTLING_BIT = 0x02;
const PAWN_PUSH_2_SQUARES_BIT = 0x04;
const EN_PASSANT_BIT = 0x08;
const PROMOTION_BIT = 0x10;
const PAWN_OR_CAPTURE_BIT = 0x20;
const MF_PAWN_PUSH_1_SQUARE = PAWN_OR_CAPTURE_BIT;
const MF_PAWN_PUSH_1_SQUARE_AND_PROMOTE = PAWN_OR_CAPTURE_BIT + PROMOTION_BIT;
const MF_PAWN_PUSH_2_SQUARES = PAWN_OR_CAPTURE_BIT + PAWN_PUSH_2_SQUARES_BIT;
const MF_PAWN_CAPTURE = PAWN_OR_CAPTURE_BIT;
const MF_PAWN_CAPTURE_AND_PROMOTE = PAWN_OR_CAPTURE_BIT + PROMOTION_BIT;
const MF_PAWN_CAPTURE_EN_PASSANT = PAWN_OR_CAPTURE_BIT + EN_PASSANT_BIT;
const MF_PIECE_NORMAL_MOVE = 0;
const MF_PIECE_CAPTURE_MOVE = PAWN_OR_CAPTURE_BIT;
const MF_KINGSIDE_CASTLING = KINGSIDE_CASTLING_BIT;
const MF_QUEENSIDE_CASTLING = QUEENSIDE_CASTLING_BIT;
const FEN_CHAR_TO_PIECE_CODE = new Map([
    ['P', WHITE_PAWN], ['N', WHITE_KNIGHT], ['B', WHITE_BISHOP], ['R', WHITE_ROOK], ['Q', WHITE_QUEEN], ['K', WHITE_KING],
    ['p', BLACK_PAWN], ['n', BLACK_KNIGHT], ['b', BLACK_BISHOP], ['r', BLACK_ROOK], ['q', BLACK_QUEEN], ['k', BLACK_KING]
]);
const PIECE_CODE_TO_PRINTABLE_CHAR = new Map([
    [WHITE_PAWN, '\u2659'], [WHITE_KNIGHT, '\u2658'], [WHITE_BISHOP, '\u2657'], [WHITE_ROOK, '\u2656'], [WHITE_QUEEN, '\u2655'], [WHITE_KING, '\u2654'],
    [BLACK_PAWN, '\u265F'], [BLACK_KNIGHT, '\u265E'], [BLACK_BISHOP, '\u265D'], [BLACK_ROOK, '\u265C'], [BLACK_QUEEN, '\u265B'], [BLACK_KING, '\u265A'],
    [NULL, '.']
]);
const MOVE_DIRECTIONS = [
    [],
    [UP, RIGHT, DOWN, LEFT, UP + LEFT, UP + RIGHT, DOWN + LEFT, DOWN + RIGHT],
    [],
    [UP + UP + LEFT, UP + UP + RIGHT, DOWN + DOWN + LEFT, DOWN + DOWN + RIGHT, LEFT + LEFT + UP, LEFT + LEFT + DOWN, RIGHT + RIGHT + UP, RIGHT + RIGHT + DOWN],
    [UP + LEFT, UP + RIGHT, DOWN + LEFT, DOWN + RIGHT],
    [UP, RIGHT, DOWN, LEFT],
    [UP, RIGHT, DOWN, LEFT, UP + LEFT, UP + RIGHT, DOWN + LEFT, DOWN + RIGHT] // QUEEN
];
////////////////////////////////////////////////////////////////
//  GLOBAL VARIABLES                                          //
////////////////////////////////////////////////////////////////
let BOARD = new Uint8Array(128);
let ACTIVE_COLOR = WHITE;
let MOVE_STACK = [];
let HISTORY_STACK = [];
let EN_PASSANT_SQUARE = SQUARE_NULL;
let CASTLING_RIGHTS = [NULL, NULL];
let KING_SQUARE = [SQUARE_NULL, SQUARE_NULL];
let PLY_CLOCK = 0;
let MOVE_NUMBER = 1;
////////////////////////////////////////////////////////////////
//  FUNCTIONS                                                 //
////////////////////////////////////////////////////////////////
function makePiece(color, pieceType) {
    return pieceType | (color << 3);
}
function getPieceColor(piece) {
    return (piece & PIECE_COLOR_MASK) >> 3;
}
function toSquareCoordinates(square) {
    let file = String.fromCharCode('a'.charCodeAt(0) + (square & FILE_MASK));
    let rank = 8 - ((square & RANK_MASK) >> 4);
    return file + rank;
}
function toSquare(squareCoordinates) {
    return (squareCoordinates.charCodeAt(0) - 'a'.charCodeAt(0))
        - 16 * (squareCoordinates.charCodeAt(1) - '8'.charCodeAt(0));
}
function clearBoard() {
    for (let square = 0; square < BOARD.length; square++) {
        BOARD[square] = NULL;
    }
    ACTIVE_COLOR = WHITE;
    MOVE_STACK = [];
    HISTORY_STACK = [];
    EN_PASSANT_SQUARE = SQUARE_NULL;
    CASTLING_RIGHTS = [NULL, NULL];
    KING_SQUARE = [SQUARE_NULL, SQUARE_NULL];
    PLY_CLOCK = 0;
    MOVE_NUMBER = 1;
}
function initBoardFromFen(fen) {
    let fenParts = fen.split(' ');
    let fenBoard = fenParts[0];
    let fenActiveColor = fenParts[1];
    let fenCastlingRights = fenParts[2];
    let fenEnPassantSquare = fenParts[3];
    let fenHalfMoveClock = fenParts[4];
    let fenFullMoveCount = fenParts[5];
    clearBoard();
    let index = 0;
    for (let c of fenBoard) {
        if (c == '/') {
            index += 8;
        }
        else if (isNaN(parseInt(c, 10))) {
            let piece = FEN_CHAR_TO_PIECE_CODE.get(c);
            if ((piece & PIECE_TYPE_MASK) == KING) {
                KING_SQUARE[getPieceColor(piece)] = index;
            }
            BOARD[index] = piece;
            index += 1;
        }
        else {
            index += parseInt(c, 10);
        }
    }
    if (fenActiveColor == 'b')
        ACTIVE_COLOR = BLACK;
    if (fenCastlingRights.includes('K'))
        CASTLING_RIGHTS[WHITE] |= KINGSIDE_CASTLING_BIT;
    if (fenCastlingRights.includes('Q'))
        CASTLING_RIGHTS[WHITE] |= QUEENSIDE_CASTLING_BIT;
    if (fenCastlingRights.includes('k'))
        CASTLING_RIGHTS[BLACK] |= KINGSIDE_CASTLING_BIT;
    if (fenCastlingRights.includes('q'))
        CASTLING_RIGHTS[BLACK] |= QUEENSIDE_CASTLING_BIT;
    if (fenEnPassantSquare != '-') {
        EN_PASSANT_SQUARE = toSquare(fenEnPassantSquare);
    }
    PLY_CLOCK = parseInt(fenHalfMoveClock, 10);
    MOVE_NUMBER = parseInt(fenFullMoveCount, 10);
}
function generateMoves() {
    let moveList = [];
    for (let s = 0; s < 64; s++) {
        let square = s + (s & 0x38);
        let piece = BOARD[square];
        if (piece == NULL)
            continue;
        let pieceColor = getPieceColor(piece);
        if (pieceColor != ACTIVE_COLOR)
            continue;
        let pieceType = piece & PIECE_TYPE_MASK;
        if (pieceType == PAWN) {
            let direction = UP * (1 - 2 * ACTIVE_COLOR);
            let targetSquare = square + direction;
            let targetPiece = BOARD[targetSquare];
            if (targetPiece == NULL) {
                if ((targetSquare & RANK_MASK) == PAWN_PROMOTING_RANK[ACTIVE_COLOR]) {
                    moveList.push(createMove(MF_PAWN_PUSH_1_SQUARE_AND_PROMOTE, square, targetSquare, NULL, makePiece(ACTIVE_COLOR, QUEEN)));
                    moveList.push(createMove(MF_PAWN_PUSH_1_SQUARE_AND_PROMOTE, square, targetSquare, NULL, makePiece(ACTIVE_COLOR, ROOK)));
                    moveList.push(createMove(MF_PAWN_PUSH_1_SQUARE_AND_PROMOTE, square, targetSquare, NULL, makePiece(ACTIVE_COLOR, BISHOP)));
                    moveList.push(createMove(MF_PAWN_PUSH_1_SQUARE_AND_PROMOTE, square, targetSquare, NULL, makePiece(ACTIVE_COLOR, KNIGHT)));
                }
                else {
                    moveList.push(createMove(MF_PAWN_PUSH_1_SQUARE, square, targetSquare));
                    if ((square & RANK_MASK) == PAWN_STARTING_RANK[ACTIVE_COLOR]) {
                        targetSquare += direction;
                        let targetPiece = BOARD[targetSquare];
                        if (targetPiece == NULL) {
                            moveList.push(createMove(MF_PAWN_PUSH_2_SQUARES, square, targetSquare));
                        }
                    }
                }
            }
            for (let lr = LEFT; lr <= RIGHT; lr += 2) {
                targetSquare = square + direction + lr;
                if (targetSquare & OUT_OF_BOARD_MASK)
                    continue;
                targetPiece = BOARD[targetSquare];
                if (targetPiece != NULL && getPieceColor(targetPiece) != ACTIVE_COLOR) {
                    if ((targetSquare & RANK_MASK) == PAWN_PROMOTING_RANK[ACTIVE_COLOR]) {
                        moveList.push(createMove(MF_PAWN_CAPTURE_AND_PROMOTE, square, targetSquare, targetPiece, makePiece(ACTIVE_COLOR, QUEEN)));
                        moveList.push(createMove(MF_PAWN_CAPTURE_AND_PROMOTE, square, targetSquare, targetPiece, makePiece(ACTIVE_COLOR, ROOK)));
                        moveList.push(createMove(MF_PAWN_CAPTURE_AND_PROMOTE, square, targetSquare, targetPiece, makePiece(ACTIVE_COLOR, BISHOP)));
                        moveList.push(createMove(MF_PAWN_CAPTURE_AND_PROMOTE, square, targetSquare, targetPiece, makePiece(ACTIVE_COLOR, KNIGHT)));
                    }
                    else {
                        moveList.push(createMove(MF_PAWN_CAPTURE, square, targetSquare, targetPiece));
                    }
                }
                if (targetSquare == EN_PASSANT_SQUARE) {
                    moveList.push(createMove(MF_PAWN_CAPTURE_EN_PASSANT, square, targetSquare));
                }
            }
        }
        else {
            let slide = pieceType & PIECE_SLIDER_MASK;
            let directions = MOVE_DIRECTIONS[pieceType];
            for (let d = 0; d < directions.length; d++) {
                let targetSquare = square;
                do {
                    targetSquare += directions[d];
                    if (targetSquare & OUT_OF_BOARD_MASK)
                        break;
                    let targetPiece = BOARD[targetSquare];
                    if (targetPiece != NULL) {
                        if (getPieceColor(targetPiece) != ACTIVE_COLOR) {
                            moveList.push(createMove(MF_PIECE_CAPTURE_MOVE, square, targetSquare, targetPiece));
                        }
                        break;
                    }
                    moveList.push(createMove(MF_PIECE_NORMAL_MOVE, square, targetSquare));
                } while (slide);
            }
        }
    }
    if (CASTLING_RIGHTS[ACTIVE_COLOR] & KINGSIDE_CASTLING_BIT) {
        let kingSquare = KING_SQUARE[ACTIVE_COLOR];
        if (BOARD[kingSquare + RIGHT] == NULL
            && BOARD[kingSquare + RIGHT + RIGHT] == NULL) {
            if (!isSquareAttacked(kingSquare, 1 - ACTIVE_COLOR)
                && !isSquareAttacked(kingSquare + RIGHT, 1 - ACTIVE_COLOR)) {
                moveList.push(createMove(MF_KINGSIDE_CASTLING, kingSquare, kingSquare + RIGHT + RIGHT));
            }
        }
    }
    if (CASTLING_RIGHTS[ACTIVE_COLOR] & QUEENSIDE_CASTLING_BIT) {
        let kingSquare = KING_SQUARE[ACTIVE_COLOR];
        if (BOARD[kingSquare + LEFT] == NULL
            && BOARD[kingSquare + LEFT + LEFT] == NULL
            && BOARD[kingSquare + LEFT + LEFT + LEFT] == NULL) {
            if (!isSquareAttacked(kingSquare, 1 - ACTIVE_COLOR)
                && !isSquareAttacked(kingSquare + LEFT, 1 - ACTIVE_COLOR)) {
                moveList.push(createMove(MF_QUEENSIDE_CASTLING, kingSquare, kingSquare + LEFT + LEFT));
            }
        }
    }
    return moveList;
}
function generateCaptures() {
    let moveList = [];
    for (let s = 0; s < 64; s++) {
        let square = s + (s & 0x38);
        let piece = BOARD[square];
        if (piece == NULL)
            continue;
        let pieceColor = getPieceColor(piece);
        if (pieceColor != ACTIVE_COLOR)
            continue;
        let pieceType = piece & PIECE_TYPE_MASK;
        if (pieceType == PAWN) {
            let direction = UP * (1 - 2 * ACTIVE_COLOR);
            for (let lr = LEFT; lr <= RIGHT; lr += 2) {
                let targetSquare = square + direction + lr;
                if (targetSquare & OUT_OF_BOARD_MASK)
                    continue;
                let targetPiece = BOARD[targetSquare];
                if (targetPiece != NULL && getPieceColor(targetPiece) != ACTIVE_COLOR) {
                    if ((targetSquare & RANK_MASK) == PAWN_PROMOTING_RANK[ACTIVE_COLOR]) {
                        moveList.push(createMove(MF_PAWN_CAPTURE_AND_PROMOTE, square, targetSquare, targetPiece, makePiece(ACTIVE_COLOR, QUEEN)));
                        moveList.push(createMove(MF_PAWN_CAPTURE_AND_PROMOTE, square, targetSquare, targetPiece, makePiece(ACTIVE_COLOR, ROOK)));
                        moveList.push(createMove(MF_PAWN_CAPTURE_AND_PROMOTE, square, targetSquare, targetPiece, makePiece(ACTIVE_COLOR, BISHOP)));
                        moveList.push(createMove(MF_PAWN_CAPTURE_AND_PROMOTE, square, targetSquare, targetPiece, makePiece(ACTIVE_COLOR, KNIGHT)));
                    }
                    else {
                        moveList.push(createMove(MF_PAWN_CAPTURE, square, targetSquare, targetPiece));
                    }
                }
                if (targetSquare == EN_PASSANT_SQUARE) {
                    moveList.push(createMove(MF_PAWN_CAPTURE_EN_PASSANT, square, targetSquare));
                }
            }
        }
        else {
            let slide = pieceType & PIECE_SLIDER_MASK;
            let directions = MOVE_DIRECTIONS[pieceType];
            for (let d = 0; d < directions.length; d++) {
                let targetSquare = square;
                do {
                    targetSquare += directions[d];
                    if (targetSquare & OUT_OF_BOARD_MASK)
                        break;
                    let targetPiece = BOARD[targetSquare];
                    if (targetPiece != NULL) {
                        if (getPieceColor(targetPiece) != ACTIVE_COLOR) {
                            moveList.push(createMove(MF_PIECE_CAPTURE_MOVE, square, targetSquare, targetPiece));
                        }
                        break;
                    }
                } while (slide);
            }
        }
    }
    return moveList;
}
function createMove(moveFlags, fromSquare, toSquare, capturedPiece = NULL, promotedPiece = NULL) {
    return moveFlags | (fromSquare << 8) | (toSquare << 15) | (capturedPiece << 22) | (promotedPiece << 26);
}
function decodeMove(move) {
    return {
        moveFlags: move & 0xFF,
        fromSquare: (move >> 8) & 0x7F,
        toSquare: (move >> 15) & 0x7F,
        capturedPiece: (move >> 22) & 0x0F,
        promotedPiece: (move >> 26) & 0x0F
    };
}
function createHistory(enPassantSquare, castlingRights, fiftyMoveClock) {
    return enPassantSquare | (castlingRights[WHITE] << 7) | (castlingRights[BLACK] << 9) | (fiftyMoveClock << 11);
}
function decodeHistory(history) {
    return {
        enPassantSquare: history & 0x7F,
        whiteCastlingRights: (history >> 7) & 0x03,
        blackCastlingRights: (history >> 9) & 0x03,
        plyClock: (history >> 11) & 0x7F
    };
}
function makeMove(encodedMove) {
    MOVE_STACK.push(encodedMove);
    HISTORY_STACK.push(createHistory(EN_PASSANT_SQUARE, CASTLING_RIGHTS, PLY_CLOCK));
    EN_PASSANT_SQUARE = SQUARE_NULL;
    PLY_CLOCK++;
    MOVE_NUMBER += ACTIVE_COLOR;
    let move = decodeMove(encodedMove);
    let moveFlags = move.moveFlags;
    let fromSquare = move.fromSquare;
    let toSquare = move.toSquare;
    let piece = BOARD[fromSquare];
    BOARD[fromSquare] = NULL;
    BOARD[toSquare] = piece;
    if (moveFlags & PAWN_OR_CAPTURE_BIT) {
        PLY_CLOCK = 0;
    }
    if ((piece & PIECE_TYPE_MASK) == PAWN) {
        if (moveFlags & PAWN_PUSH_2_SQUARES_BIT) {
            EN_PASSANT_SQUARE = (fromSquare + toSquare) / 2;
        }
        else if (moveFlags & PROMOTION_BIT) {
            BOARD[toSquare] = move.promotedPiece;
        }
        else if (moveFlags & EN_PASSANT_BIT) {
            BOARD[(fromSquare & RANK_MASK) + (toSquare & FILE_MASK)] = NULL;
        }
    }
    if ((piece & PIECE_TYPE_MASK) == KING) {
        KING_SQUARE[ACTIVE_COLOR] = toSquare;
        CASTLING_RIGHTS[ACTIVE_COLOR] = NULL;
        if (moveFlags & KINGSIDE_CASTLING_BIT) {
            BOARD[toSquare + RIGHT] = NULL;
            BOARD[toSquare + LEFT] = makePiece(ACTIVE_COLOR, ROOK);
        }
        else if (moveFlags & QUEENSIDE_CASTLING_BIT) {
            BOARD[toSquare + LEFT + LEFT] = NULL;
            BOARD[toSquare + RIGHT] = makePiece(ACTIVE_COLOR, ROOK);
        }
    }
    if (fromSquare == A1 || toSquare == A1)
        CASTLING_RIGHTS[WHITE] &= KINGSIDE_CASTLING_BIT;
    if (fromSquare == H1 || toSquare == H1)
        CASTLING_RIGHTS[WHITE] &= QUEENSIDE_CASTLING_BIT;
    if (fromSquare == A8 || toSquare == A8)
        CASTLING_RIGHTS[BLACK] &= KINGSIDE_CASTLING_BIT;
    if (fromSquare == H8 || toSquare == H8)
        CASTLING_RIGHTS[BLACK] &= QUEENSIDE_CASTLING_BIT;
    ACTIVE_COLOR = 1 - ACTIVE_COLOR;
}
function takeback() {
    ACTIVE_COLOR = 1 - ACTIVE_COLOR;
    let history = decodeHistory(HISTORY_STACK.pop());
    EN_PASSANT_SQUARE = history.enPassantSquare;
    CASTLING_RIGHTS[WHITE] = history.whiteCastlingRights;
    CASTLING_RIGHTS[BLACK] = history.blackCastlingRights;
    PLY_CLOCK = history.plyClock;
    MOVE_NUMBER -= ACTIVE_COLOR;
    let move = decodeMove(MOVE_STACK.pop());
    let moveFlags = move.moveFlags;
    let fromSquare = move.fromSquare;
    let toSquare = move.toSquare;
    let capturedPiece = move.capturedPiece;
    let piece = BOARD[toSquare];
    if (moveFlags & PROMOTION_BIT) {
        piece = makePiece(ACTIVE_COLOR, PAWN);
    }
    BOARD[fromSquare] = piece;
    BOARD[toSquare] = capturedPiece;
    if (moveFlags & EN_PASSANT_BIT) {
        BOARD[(fromSquare & RANK_MASK) + (toSquare & FILE_MASK)] = makePiece(1 - ACTIVE_COLOR, PAWN);
    }
    if ((piece & PIECE_TYPE_MASK) == KING) {
        KING_SQUARE[ACTIVE_COLOR] = fromSquare;
        if (moveFlags & KINGSIDE_CASTLING_BIT) {
            BOARD[toSquare + RIGHT] = makePiece(ACTIVE_COLOR, ROOK);
            BOARD[toSquare + LEFT] = NULL;
        }
        else if (moveFlags & QUEENSIDE_CASTLING_BIT) {
            BOARD[toSquare + LEFT + LEFT] = makePiece(ACTIVE_COLOR, ROOK);
            BOARD[toSquare + RIGHT] = NULL;
        }
    }
}

var attTime = 0;

// piece move offsets
var knightOffsets = [33, 31, 18, 14, -33, -31, -18, -14];
var bishopOffsets = [15, 17, -15, -17];
var rookOffsets = [16, -16, 1, -1];
var kingOffsets = [16, -16, 1, -1, 15, 17, -15, -17];

// slider piece to offset mapping
var sliderPieces = {
  bishop: { offsets: bishopOffsets, side: [[WHITE_BISHOP, WHITE_QUEEN], [BLACK_BISHOP, BLACK_QUEEN]] },
  rook: { offsets: rookOffsets, side: [[WHITE_ROOK, WHITE_QUEEN], [BLACK_ROOK, BLACK_QUEEN]] }
};

// leaper piece to offset mapping
var leaperPieces = {
  knight: { offsets: knightOffsets, side: [WHITE_KNIGHT, BLACK_KNIGHT] },
  king: { offsets: kingOffsets, side: [WHITE_KING, BLACK_KING] }
};

// pawn directions to side mapping
var pawnDirections = {
  offsets: [[17, 15], [-17, -15]],
  pawn: [WHITE_PAWN, BLACK_PAWN]
}

function isSquareAttacked(square, color) {
    /* by pawns
    for (let index = 0; index < 2; index++) {
      let targetSquare = square + pawnDirections.offsets[color][index] 
      if (((targetSquare) & 0x88) == 0 &&
           (BOARD[targetSquare] == pawnDirections.pawn[color])) return 1;
    }
    
    // by leaper pieces
    for (let piece in leaperPieces) {      
      for (let index = 0; index < 8; index++) {
        let targetSquare = square + leaperPieces[piece].offsets[index];
        let targetPiece = BOARD[targetSquare];
        if ((targetSquare & 0x88) == 0)
          if (targetPiece == leaperPieces[piece].side[color]) return 1;
      }
    }
    
    // by slider pieces
    for (let piece in sliderPieces) {
      for (let index = 0; index < 4; index++) {
        let targetSquare = square + sliderPieces[piece].offsets[index];
        while ((targetSquare & 0x88) == 0) {
          var targetPiece = BOARD[targetSquare];
          if (sliderPieces[piece].side[color].includes(targetPiece)) return 1;
          if (targetPiece) break;
          targetSquare += sliderPieces[piece].offsets[index];
        }
      }
    }*/
    
    
    
    
    
    
    
    
    
    
    
    //let startTime = Date.now();
    for (let pieceType = QUEEN; pieceType >= KING; pieceType--) {
        let piece = makePiece(color, pieceType);
        if (pieceType == PAWN) {
            let direction = DOWN * (1 - 2 * color);
            for (let lr = LEFT; lr <= RIGHT; lr += 2) {
                let targetSquare = square + direction + lr;
                if (!(targetSquare & OUT_OF_BOARD_MASK) && BOARD[targetSquare] == piece) {
                    return true;
                }
            }
        }
        else {
            let slide = pieceType & PIECE_SLIDER_MASK;
            let directions = MOVE_DIRECTIONS[pieceType];
            for (let d = 0; d < directions.length; d++) {
                let targetSquare = square;
                do {
                    targetSquare += directions[d];
                    if (targetSquare & OUT_OF_BOARD_MASK)
                        break;
                    let targetPiece = BOARD[targetSquare];
                    if (targetPiece != NULL) {
                        if (targetPiece == piece)
                            return true;
                        break;
                    }
                } while (slide);
            }
        }
    }
    //attTime += Date.now() - startTime;
    return false;
}
function perft(depth) {
    if (depth == 0)
        return 1;
    let nodes = 0;
    let moveList = generateMoves();
    for (let m = 0; m < moveList.length; m++) {
        let move = moveList[m];
        makeMove(move);
        if (!isSquareAttacked(KING_SQUARE[1 - ACTIVE_COLOR], ACTIVE_COLOR)) {
            nodes += perft(depth - 1);
        }
        takeback();
    }
    return nodes;
}
////////////////////////////////////////////////////////////////
//  DISPLAYING AND DEBUGGING FUNCTIONS                        //
////////////////////////////////////////////////////////////////
function printBoard() {
    let printableBoard = '';
    for (let square = 0; square < BOARD.length; square++) {
        if (square & OUT_OF_BOARD_MASK)
            continue;
        printableBoard += PIECE_CODE_TO_PRINTABLE_CHAR.get(BOARD[square]) + " ";
        if ((square + RIGHT) & 0x08)
            printableBoard += '\n';
    }
    console.log(printableBoard);
}
function testPerft() {
    let perftTests = new Map();
    perftTests.set('rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1', [1, 20, 400, 8902, 197281, 4865609, 119060324]); // Starting position
    perftTests.set('r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1', [1, 48, 2039, 97862, 4085603, 193690690]); // Position 2
    perftTests.set('8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1', [1, 14, 191, 2812, 43238, 674624, 11030083, 178633661]); // Position 3
    perftTests.set('r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1', [1, 6, 264, 9467, 422333, 15833292]); // Position 4
    perftTests.set('r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1', [1, 6, 264, 9467, 422333, 15833292]); // Position 4 mirrored
    perftTests.set('rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8', [1, 44, 1486, 62379, 2103487, 89941194]); // Position 5
    perftTests.set('r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10', [1, 46, 2079, 89890, 3894594, 164075551]); // Position 6
    console.log('========================================');
    perftTests.forEach((perftCounts, fenString) => {
        initBoardFromFen(fenString);
        printBoard();
        for (let depth = 0; depth < perftCounts.length; depth++) {
            let perftCount = perft(depth);
            console.log('Depth = ' + depth + ' ; Perft = ' + perftCount + ' ; ' + (perftCount == perftCounts[depth]));
        }
        console.log('========================================');
    });
}
function bench() {
    let benchPositions = [
        'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1',
        'r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1',
        '8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 0',
        'rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8'
    ];
    for (let run = 1; run <= 5; run++) {
        console.time('Run ' + run);
        for (let p = 0; p < benchPositions.length; p++) {
            initBoardFromFen(benchPositions[p]);
            perft(5);
        }
        console.timeEnd('Run ' + run);
    }
}
////////////////////////////////////////////////////////////////
//  MAIN                                                      //
////////////////////////////////////////////////////////////////
//initBoardFromFen('rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1');
//testPerft();
//bench();
//testPerft();
/*
for (let run = 1; run <= 5; run++) {
    initBoardFromFen(STARTING_FEN);
    console.time('Run ' + run);
    console.log(perft(6));
    console.timeEnd('Run ' + run);
}*/


/*
    CMK code below
*/

// array to convert board square indices to coordinates
const coordinates = [
  'a8', 'b8', 'c8', 'd8', 'e8', 'f8', 'g8', 'h8', 'i8', 'j8', 'k8', 'l8', 'm8', 'n8', 'o8', 'p8',
  'a7', 'b7', 'c7', 'd7', 'e7', 'f7', 'g7', 'h7', 'i7', 'j7', 'k7', 'l7', 'm7', 'n7', 'o7', 'p7',
  'a6', 'b6', 'c6', 'd6', 'e6', 'f6', 'g6', 'h6', 'i6', 'j6', 'k6', 'l6', 'm6', 'n6', 'o6', 'p6',
  'a5', 'b5', 'c5', 'd5', 'e5', 'f5', 'g5', 'h5', 'i5', 'j5', 'k5', 'l5', 'm5', 'n5', 'o5', 'p5',
  'a4', 'b4', 'c4', 'd4', 'e4', 'f4', 'g4', 'h4', 'i4', 'j4', 'k4', 'l4', 'm4', 'n4', 'o4', 'p4',
  'a3', 'b3', 'c3', 'd3', 'e3', 'f3', 'g3', 'h3', 'i3', 'j3', 'k3', 'l3', 'm3', 'n3', 'o3', 'p3',
  'a2', 'b2', 'c2', 'd2', 'e2', 'f2', 'g2', 'h2', 'i2', 'j2', 'k2', 'l2', 'm2', 'n2', 'o2', 'p2',
  'a1', 'b1', 'c1', 'd1', 'e1', 'f1', 'g1', 'h1', 'i1', 'j1', 'k1', 'l1', 'm1', 'n1', 'o1', 'p1'
];

var nodes = 0;

// perft driver
function perftDriver(depth) {
  if  (depth == 0) { nodes++; return; }
    
  let moveList = generateMoves();
  
  for (var count = 0; count < moveList.length; count++) {      
    let move = moveList[count];
    makeMove(move);
    if (!isSquareAttacked(KING_SQUARE[1 - ACTIVE_COLOR], ACTIVE_COLOR)) perftDriver(depth - 1);
    takeback();
  }
}

// perft test
function perftTest(depth) {
  nodes = 0;
  console.log('   Performance test:\n');
  resultString = '';
  let startTime = Date.now();
  
  let moveList = generateMoves();
  
  for (var count = 0; count < moveList.length; count++) {      
    let move = moveList[count];
    let cumNodes = nodes;
    makeMove(move);
    if (!isSquareAttacked(KING_SQUARE[1 - ACTIVE_COLOR], ACTIVE_COLOR)) perftDriver(depth - 1);
    takeback();
    let oldNodes = nodes - cumNodes;
    console.log('old nodes: ' + oldNodes);
  }
  
  resultString += '\n   Depth: ' + depth;
  resultString += '\n   Nodes: ' + nodes;
  resultString += '\n    Time: ' + (Date.now() - startTime) + ' ms\n';
  console.log(resultString);
}

initBoardFromFen('r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1');
//perftTest(4);
//console.log('attTime:', attTime);
console.log(BOARD[0x77]);







