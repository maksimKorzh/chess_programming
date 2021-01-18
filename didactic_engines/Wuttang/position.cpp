#include <iostream>
#include "position.h"
#include "hashKey.h"

void Position::resetBoard()
{
    for (int i = 0; i < sq_no; ++i)
    {
        pieces[i] = OFFBOARD;
    }
    for (int sq = 0; sq < 64; ++sq)
    {
        pieces[sq120(sq)] = EMPTY;
    }
    for (int i = white; i < both; ++i)
    {
        bigPce[i] = 0;
        majPce[i] = 0;
        minPce[i] = 0;
        material[i] = 0;
        kingSq[i] = NO_SQ;
    }

    for (int i = white; i <= both; ++i)
    {
        pawns[i] = 0ull;
    }
    for (int i = wP; i <= bK; ++i)
    {
        pceNum[i] = 0;
    }

    side = both;
    enPass_sq = NO_SQ;
    fifty_move = 0;

    ply = hisPly = 0;

    castleRights = NO_CASTLING;

    posKey = 0ULL;
}

void Position::updateListMaterial()
{
    int piece, color;

    for (int sq = 0; sq < sq_no; ++sq)
    {
        piece = pieces[sq];
        if (piece != OFFBOARD && piece != EMPTY)
        {
            color = pceCol[piece];

            if (pceBig[piece])
            {
                ++bigPce[color];
                if (pceMaj[piece])
                    ++majPce[color];
                else
                    ++minPce[color];
            }

            material[color] += pceVal[piece];

            pList[piece][pceNum[piece]] = sq;
            ++pceNum[piece];

            if (piece == wK)
            {
                kingSq[white] = sq;
            }
            else if (piece == bK)
            {
                kingSq[black] = sq;
            }
            else if (piece == wP)
            {
                SETBIT(pawns[white], sq64(sq));
                SETBIT(pawns[both], sq64(sq));
            }
            else if (piece == bP)
            {
                SETBIT(pawns[black], sq64(sq));
                SETBIT(pawns[both], sq64(sq));
            }

        }
    }
}

void Position::parseFEN(const std::string fenStr)
{
    const char *fen = &(fenStr[0]);

    assert(fen != nullptr);

    int rank = rank_8, file = file_A, cnt = 1, piece = EMPTY;

    resetBoard();

    while (rank >= rank_1)
    {
        cnt = 1;
        switch(*fen)
        {
            case 'r' : piece = bR; break;
            case 'n' : piece = bN; break;
            case 'b' : piece = bB; break;
            case 'q' : piece = bQ; break;
            case 'k' : piece = bK; break;
            case 'p' : piece = bP; break;
            case 'R' : piece = wR; break;
            case 'N' : piece = wN; break;
            case 'B' : piece = wB; break;
            case 'Q' : piece = wQ; break;
            case 'K' : piece = wK; break;
            case 'P' : piece = wP; break;

            case '1' :
            case '2' :
            case '3' :
            case '4' :
            case '5' :
            case '6' :
            case '7' :
            case '8' :
                piece = EMPTY;
                cnt = *fen - '0';
                break;

            case ' ' :
            case '/' :
                rank--;
                file = file_A;
                ++fen;
                continue;

            default :
                std::cout << "FEN error\n";
                return;
        }
        while (cnt > 0)
        {
            if (piece != EMPTY)
                pieces[fr2sq(file, rank)] = piece;
            ++file;
            --cnt;
        }
        ++fen;
    }

    assert(*fen == 'w' || *fen == 'b');

    side = (*fen == 'w' ? white : black);
    fen += 2;

    for (int i = 0; i < 4; ++i)
    {
        if (*fen == ' ')
        {
            break;
        }
        switch (*fen)
        {
            case 'K' : castleRights |= WKCA; break;
            case 'Q' : castleRights |= WQCA; break;
            case 'k' : castleRights |= BKCA; break;
            case 'q' : castleRights |= BQCA; break;
            default : break;
        }
        ++fen;
    }
    ++fen;
    assert (castleRights >= 0 && castleRights < 16);

    if (*fen != '-')
    {
        file = fen[0] - 'a';
        rank = fen[1] - '1';

        assert(file >= file_A && file <= file_H);
        assert(rank >= rank_1 && rank <= rank_8);

        enPass_sq = fr2sq(file, rank);
    }

    posKey = generatePosKey(*this);

    updateListMaterial();
}

void Position::display() const
{
    int sq;

    std::cout << "\n\n";
    for (int rank = rank_8; rank >= rank_1; --rank)
    {
        std::cout << ' ' << rank+1 << "  ";
        for (int file = file_A; file <= file_H; ++file)
        {
            sq = fr2sq(file, rank);
            std::cout << pceChar[pieces[sq]] << "  ";
        }
        std::cout << "\n\n";
    }
    std::cout << "    a  b  c  d  e  f  g  h\n\n";

    std::cout << "Side : " << sideChar[side] << '\n'
              << "EnPassSq : " << enPass_sq << '\n'
              << "CastleRights : " << (castleRights & WKCA ? 'K' : '-') << (castleRights & WQCA ? 'Q' : '-') << (castleRights & BKCA ? 'k' : '-') << (castleRights & BQCA ? 'q' : '-') << '\n'
              << "PosKey : " << std::hex << posKey << '\n' << std::dec;
}

bool Position::checkBoard() const
{
    int t_pceNum[13] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    int t_bigPce[2] = {0, 0};
    int t_majPce[2] = {0, 0};
    int t_minPce[2] = {0, 0};
    int t_material[2] = {0, 0};

    BitBoard t_pawns[3] = {0ull, 0ull, 0ull};

    t_pawns[white] = pawns[white];
    t_pawns[black] = pawns[black];
    t_pawns[both] = pawns[both];

    //Check piece list
    for (int piece = wP; piece <= bK; ++piece)
    {
        for (int num = 0; num < pceNum[piece]; ++num)
        {
            assert(pieces[pList[piece][num]] == piece);
        }
    }

    //Check piece count and other parameters
    int piece, color;
    for (int sq64 = 0; sq64 < 64; ++sq64)
    {
        piece = pieces[sq120(sq64)];
        color = pceCol[piece];
        ++t_pceNum[piece];

        if (pceBig[piece])
        {
            ++t_bigPce[color];
            if (pceMaj[piece])
            {
                ++t_majPce[color];
            }
            else
            {
                ++t_minPce[color];
            }
        }

        t_material[color] += pceVal[piece];
    }

    for (piece = wP; piece <= bK; ++piece)
    {
        assert(t_pceNum[piece] == pceNum[piece]);
    }

    //Check BitBoard count
    int cnt = countBits(t_pawns[white]);
    assert(cnt == pceNum[wP]);
    cnt = countBits(t_pawns[black]);
    assert(cnt == pceNum[bP]);
    cnt = countBits(t_pawns[both]);
    assert(cnt == (pceNum[wP]+pceNum[bP]));

    //Check BitBoards square
    int sq_64;
    while (t_pawns[white])
    {
        sq_64 = popBit(t_pawns[white]);
        assert(pieces[sq120(sq_64)] == wP);
    }
    while (t_pawns[black])
    {
        sq_64 = popBit(t_pawns[black]);
        assert(pieces[sq120(sq_64)] == bP);
    }
    while (t_pawns[both])
    {
        sq_64 = popBit(t_pawns[both]);
        assert(pieces[sq120(sq_64)] == wP || pieces[sq120(sq_64)] == bP);
    }

    assert(t_material[white] == material[white] && t_material[black] == material[black]);
    assert(t_majPce[white] == majPce[white] && t_majPce[black] == majPce[black]);
    assert(t_minPce[white] == minPce[white] && t_minPce[black] == minPce[black]);
    assert(t_bigPce[white] == bigPce[white] && t_bigPce[black] == bigPce[black]);

    assert(side == white || side == black);
    assert(generatePosKey(*this) == posKey);

    assert(enPass_sq == NO_SQ || (rankSq(enPass_sq) == rank_6 && side == white) || (rankSq(enPass_sq) == rank_3 && side == black));

    assert(pieces[kingSq[white]] == wK);
    assert(pieces[kingSq[black]] == bK);

    return true;
}

bool Position::isSqAttacked(const int sq, const int side) const
{
    assert(SqOnBoard(sq));
    assert(SideValid(side));
    assert(checkBoard());

    //pawns
    if (side == white)
    {
        if (pieces[sq-11] == wP || pieces[sq-9] == wP)
            return true;
    }
    else
    {
        if (pieces[sq+11] == bP || pieces[sq+9] == bP)
            return true;
    }

    //knights
    int piece;
    for (int i = 0; i < 8; ++i)
    {
        piece = pieces[sq+KnDir[i]];
        if (piece != OFFBOARD && IsKn(piece) && pceCol[piece] == side)
        {
            return true;
        }
    }

    //rooks, queens
    int dir, t_sq;
    for (int i = 0; i < 4; ++i)
    {
        dir = RkDir[i];
        t_sq = sq+dir;
        piece = pieces[t_sq];
        while (piece != OFFBOARD)
        {
            if (piece != EMPTY)
            {
                if (IsRQ(piece) && pceCol[piece] == side)
                {
                    return true;
                }
                break;
            }
            t_sq += dir;
            piece = pieces[t_sq];
        }
    }

    //bishops, queens
    for (int i = 0; i < 4; ++i)
    {
        dir = BiDir[i];
        t_sq = sq + dir;
        piece = pieces[t_sq];
        while (piece != OFFBOARD)
        {
            if (piece != EMPTY)
            {
                if (IsBQ(piece) && pceCol[piece] == side)
                {
                    return true;
                }
                break;
            }
            t_sq += dir;
            piece = pieces[t_sq];
        }
    }

    //kings
    for (int i = 0; i < 8; ++i)
    {
        piece = pieces[sq+KiDir[i]];
        if (piece != OFFBOARD && IsKi(piece) && pceCol[piece] == side)
        {
            return true;
        }
    }

    return false;
}
