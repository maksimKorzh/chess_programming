#include "chess.h"
#include "evaluate.h"
#include "data.h"
/* last modified 08/03/16 */
/*
 *******************************************************************************
 *                                                                             *
 *   Evaluate() is used to evaluate the chess board.  Broadly, it addresses    *
 *   four (4) distinct areas:  (1) material score which is simply a summing of *
 *   piece types multiplied by piece values;  (2) pawn scoring which considers *
 *   placement of pawns and also evaluates passed pawns, particularly in end-  *
 *   game situations;  (3) piece scoring which evaluates the placement of each *
 *   piece as well as things like piece mobility;  (4) king safety which       *
 *   considers the pawn shelter around the king and enemy pieces and how close *
 *   they are to assist in a king-side attack.                                 *
 *                                                                             *
 *******************************************************************************
 */
int Evaluate(TREE * RESTRICT tree, int ply, int wtm, int alpha, int beta) {
  PAWN_HASH_ENTRY *ptable;
  PXOR *pxtable;
  int score, side, can_win = 3, phase, lscore, cutoff;

/*
 *************************************************************
 *                                                           *
 *  First thing we do is if -DSKILL was passed in to the     *
 *  compiler, we burn some time to slow the search down,     *
 *  then we fall into the normal evaluation code.            *
 *                                                           *
 *************************************************************
 */
#if defined(SKILL)
  if (skill < 100) {
    int i, j;
    for (i = 0; i < burnc[skill / 10] && !abort_search; i++)
      for (j = 1; j < 10 && !abort_search; j++)
        burner[j - 1] = burner[j - 1] * burner[j];
    if (TimeCheck(tree, 1))
      abort_search = 1;
  }
#endif
/*
 *************************************************************
 *                                                           *
 *  First lazy cutoff attempt.  If the material score is way *
 *  below alpha or way above beta (way above means so far    *
 *  above it is very unlikely the positional score can bring *
 *  the total score back into the alpha / beta window) then  *
 *  we take what is known as a "lazy evaluation exit" and    *
 *  avoid the computational cost of a full evaluation in a   *
 *  position where one side is way ahead or behind.          *
 *                                                           *
 *************************************************************
 */
  cutoff = (TotalPieces(white, occupied) && TotalPieces(black, occupied))
      ? KNIGHT_VALUE : ROOK_VALUE;
  lscore = MaterialSTM(wtm);
  if (lscore + cutoff < alpha)
    return alpha;
  if (lscore - cutoff > beta)
    return beta;
/*
 *************************************************************
 *                                                           *
 *  Check for draws where one side seems to be ahead, but    *
 *  has no actual winning chances.  One simple example is a  *
 *  king, bishop and rook pawn, with the wrong colored       *
 *  bishop and the enemy king too close to the promotion     *
 *  square.                                                  *
 *                                                           *
 *  The variable "can_win" uses 2 bits.  If White can        *
 *  actually win in this position, bit 1 is set.  If Black   *
 *  can actually win in this position, bit 0 is set.  If     *
 *  both sides can win, both bits are set.  This is used     *
 *  later to drag the score closer to a draw score if the    *
 *  side with the better score can't actually win.           *
 *                                                           *
 *  Note that we only set these bits in minimal material     *
 *  positions (both sides have < 13 points of material       *
 *  total).  Otherwise we assume normal scoring should       *
 *  apply.                                                   *
 *                                                           *
 *************************************************************
 */
  tree->evaluations++;
  tree->score_mg = 0;
  tree->score_eg = 0;
  EvaluateMaterial(tree, wtm);
  if (TotalPieces(white, occupied) < 13 && TotalPieces(black, occupied) < 13)
    for (side = black; side <= white; side++)
      if (!EvaluateWinningChances(tree, side, wtm))
        can_win ^= (1 << side);
/*
 *************************************************************
 *                                                           *
 *  Determine if this position should be evaluated to force  *
 *  mate (neither side has pawns) or if it should be         *
 *  evaluated normally.                                      *
 *                                                           *
 *  Note the special case of no pawns, one side is ahead in  *
 *  total material, but the game is a hopeless draw.  KRN vs *
 *  KR is one example.  If EvaluateWinningChances()          *
 *  determines that the side with extra material can not     *
 *  win, the score is pulled closer to a draw although it    *
 *  can not collapse completely to the drawscore as it is    *
 *  possible to lose KRB vs KR if the KR side lets the king  *
 *  get trapped on the edge of the board.                    *
 *                                                           *
 *************************************************************
 */
  tree->all_pawns = Pawns(black) | Pawns(white);
  if (!tree->all_pawns) {
    if (TotalPieces(white, occupied) > 3 || TotalPieces(black, occupied) > 3) {
      if (Material > 0)
        EvaluateMate(tree, white);
      else if (Material < 0)
        EvaluateMate(tree, black);
      if (tree->score_eg > DrawScore(1) && !(can_win & 2))
        tree->score_eg = tree->score_eg / 16;
      if (tree->score_eg < DrawScore(1) && !(can_win & 1))
        tree->score_eg = tree->score_eg / 16;
#if defined(SKILL)
      if (skill < 100)
        tree->score_eg =
            skill * tree->score_eg / 100 + ((100 -
                skill) * PAWN_VALUE * (uint64_t) Random32() /
            0x100000000ull) / 100;
#endif
      return (wtm) ? tree->score_eg : -tree->score_eg;
    }
  }
/*
 *************************************************************
 *                                                           *
 *  Now evaluate pawns.  If the pawn hash signature has not  *
 *  changed from the last entry to Evaluate() then we        *
 *  already have everything we need in the pawn hash entry.  *
 *  In this case, we do not need to call EvaluatePawns() at  *
 *  all.  EvaluatePawns() does all of the analysis for       *
 *  information specifically regarding only pawns.  In many  *
 *  cases, it merely records the presence/absence of         *
 *  positional pawn features because those features also     *
 *  depends on pieces.                                       *
 *                                                           *
 *  Note that anything put into EvaluatePawns() can only     *
 *  consider the placement of pawns.  Kings or other pieces  *
 *  can not influence the score because those pieces are not *
 *  hashed into the pawn hash signature.  Violating this     *
 *  principle leads to lots of very difficult and            *
 *  challenging debugging problems.                          *
 *                                                           *
 *************************************************************
 */
  else {
    if (PawnHashKey == tree->pawn_score.key) {
      tree->score_mg += tree->pawn_score.score_mg;
      tree->score_eg += tree->pawn_score.score_eg;
    }
/*
 *************************************************************
 *                                                           *
 *  First check to see if this position has been handled     *
 *  before.  If so, we can skip the work saved in the pawn   *
 *  hash table.                                              *
 *                                                           *
 *************************************************************
 */
    else {
      ptable = pawn_hash_table + (PawnHashKey & pawn_hash_mask);
      pxtable = (PXOR *) & (tree->pawn_score);
      tree->pawn_score = *ptable;
      tree->pawn_score.key ^= pxtable->entry[1] ^ pxtable->entry[2];
      if (tree->pawn_score.key != PawnHashKey) {
        tree->pawn_score.key = PawnHashKey;
        tree->pawn_score.score_mg = 0;
        tree->pawn_score.score_eg = 0;
        for (side = black; side <= white; side++)
          EvaluatePawns(tree, side);
        ptable->key =
            pxtable->entry[0] ^ pxtable->entry[1] ^ pxtable->entry[2];
        memcpy((char *) ptable + 8, (char *) &(tree->pawn_score) + 8,
            sizeof(PAWN_HASH_ENTRY) - 8);
      }
      tree->score_mg += tree->pawn_score.score_mg;
      tree->score_eg += tree->pawn_score.score_eg;
    }
/*
 *************************************************************
 *                                                           *
 *  If there are any passed pawns, first call                *
 *  EvaluatePassedPawns() to evaluate them.  Then, if one    *
 *  side has a passed pawn and the other side has no pieces, *
 *  call EvaluatePassedPawnRaces() to see if the passed pawn *
 *  can be stopped from promoting.                           *
 *                                                           *
 *************************************************************
 */
    if (tree->pawn_score.passed[black] || tree->pawn_score.passed[white]) {
      for (side = black; side <= white; side++)
        if (tree->pawn_score.passed[side])
          EvaluatePassedPawns(tree, side, wtm);
      if ((TotalPieces(white, occupied) == 0 &&
              tree->pawn_score.passed[black])
          || (TotalPieces(black, occupied) == 0 &&
              tree->pawn_score.passed[white]))
        EvaluatePassedPawnRaces(tree, wtm);
    }
  }
/*
 *************************************************************
 *                                                           *
 *  Call EvaluateCastling() to evaluate castling potential.  *
 *   Note we  only do this when that side has not castled at *
 *  the root.                                                *
 *                                                           *
 *************************************************************
 */
  for (side = black; side <= white; side++)
    if (Castle(1, side) > 0)
      EvaluateCastling(tree, ply, side);
/*
 *************************************************************
 *                                                           *
 *  The "dangerous" flag simply indicates whether that side  *
 *  has enough material to whip up a mating attack if the    *
 *  other side is careless (Q + minor or better, or RR + two *
 *  minors or better).                                       *
 *                                                           *
 *************************************************************
 */
  tree->dangerous[white] = (Queens(white) && TotalPieces(white, occupied) > 9)
      || (TotalPieces(white, rook) > 1 && TotalPieces(white, occupied) > 15);
  tree->dangerous[black] = (Queens(black) && TotalPieces(black, occupied) > 9)
      || (TotalPieces(black, rook) > 1 && TotalPieces(black, occupied) > 15);
/*
 *************************************************************
 *                                                           *
 *  Second lazy evaluation test.  We have computed the large *
 *  positional scores (except for king safety).  If the      *
 *  score is too far outside the alpha/beta window, we skip  *
 *  the piece scoring which is the most expensive of all the *
 *  evaluation terms, and simply use what we have at this    *
 *  point.                                                   *
 *                                                           *
 *************************************************************
 */
  phase =
      Min(62, TotalPieces(white, occupied) + TotalPieces(black, occupied));
  score = ((tree->score_mg * phase) + (tree->score_eg * (62 - phase))) / 62;
  lscore = (wtm) ? score : -score;
  int w_mat = (2 * TotalPieces(white, rook)) + TotalPieces(white,
      knight) + TotalPieces(white, bishop);
  int b_mat = (2 * TotalPieces(black, rook)) + TotalPieces(black,
      knight) + TotalPieces(black, bishop);
  cutoff = 72 + (w_mat + b_mat) * 8 + abs(w_mat - b_mat) * 16;
  if (tree->dangerous[white] || tree->dangerous[black])
    cutoff += 35;
/*
 *************************************************************
 *                                                           *
 *  Then evaluate pieces if the lazy eval test fails.        *
 *                                                           *
 *  Note:  We MUST evaluate kings last, since their scoring  *
 *  depends on the tropism scores computed by the other      *
 *  piece evaluators.  Do NOT try to collapse the following  *
 *  loops into one loop.  That will break things since it    *
 *  would violate the kings last rule.  More importantly     *
 *  there is no benefit as the loops below are unrolled by   *
 *  the compiler anyway.                                     *
 *                                                           *
 *************************************************************
 */
  if (lscore + cutoff > alpha && lscore - cutoff < beta) {
    tree->tropism[white] = 0;
    tree->tropism[black] = 0;
    for (side = black; side <= white; side++)
      if (Knights(side))
        EvaluateKnights(tree, side);
    for (side = black; side <= white; side++)
      if (Bishops(side))
        EvaluateBishops(tree, side);
    for (side = black; side <= white; side++)
      if (Rooks(side))
        EvaluateRooks(tree, side);
    for (side = black; side <= white; side++)
      if (Queens(side))
        EvaluateQueens(tree, side);
    for (side = black; side <= white; side++)
      EvaluateKing(tree, ply, side);
  }
/*
 *************************************************************
 *                                                           *
 *  Caclulate the final score, which is interpolated between *
 *  the middlegame score and endgame score based on the      *
 *  material left on the board.                              *
 *                                                           *
 *  Adjust the score if one side can't win, but the score    *
 *  actually favors that side significantly.                 *
 *                                                           *
 *************************************************************
 */
  score = ((tree->score_mg * phase) + (tree->score_eg * (62 - phase))) / 62;
  score = EvaluateDraws(tree, ply, can_win, score);
#if defined(SKILL)
  if (skill < 100)
    score =
        skill * score / 100 + ((100 -
            skill) * PAWN_VALUE * (uint64_t) Random32() / 0x100000000ull) /
        100;
#endif
  return (wtm) ? score : -score;
}

/* last modified 10/19/15 */
/*
 *******************************************************************************
 *                                                                             *
 *  EvaluateBishops() is used to evaluate bishops.                             *
 *                                                                             *
 *******************************************************************************
 */
void EvaluateBishops(TREE * RESTRICT tree, int side) {
  uint64_t temp, moves;
  int square, special, i, mobility;
  int score_eg = 0, score_mg = 0, enemy = Flip(side), tpawns;
/*
 ************************************************************
 *                                                          *
 *  First, locate each bishop and add in its piece/square   *
 *  table score.                                            *
 *                                                          *
 ************************************************************
 */
  for (temp = Bishops(side); temp; temp &= temp - 1) {
    square = LSB(temp);
    score_mg += bval[mg][side][square];
    score_eg += bval[eg][side][square];
/*
 ************************************************************
 *                                                          *
 *  Evaluate for "outposts" which is a bishop that can't be *
 *  driven off by an enemy pawn, and which is supported by  *
 *  a friendly pawn.                                        *
 *                                                          *
 *  If the enemy has NO minor to take this bishop, then     *
 *  increase the bonus.                                     *
 *                                                          *
 ************************************************************
 */
    special = bishop_outpost[side][square];
    if (special) {
      if (!(mask_pattacks[enemy][square] & Pawns(enemy))) {
        if (pawn_attacks[enemy][square] & Pawns(side)) {
          special += special / 2;
          if (!Knights(enemy) && !(Color(square) & Bishops(enemy)))
            special += bishop_outpost[side][square];
        }
        score_eg += special;
        score_mg += special;
      }
    }
/*
 ************************************************************
 *                                                          *
 *  Next we count the number of friendly pawns on the same  *
 *  color squares as the bishop.  This is a bad thing since *
 *  it restricts the bishop's ability to move.  We only do  *
 *  this if there is only one bishop for this side.         *
 *                                                          *
 ************************************************************
 */
    if (TotalPieces(side, bishop) == 1) {
      if (dark_squares & SetMask(square))
        tpawns = PopCnt(dark_squares & Pawns(side));
      else
        tpawns = PopCnt(~dark_squares & Pawns(side));
      score_mg -= tpawns * bishop_pawns_on_color[mg];
      score_eg -= tpawns * bishop_pawns_on_color[eg];
    }
/*
 ************************************************************
 *                                                          *
 *  Mobility counts the number of squares the bishop        *
 *  attacks, excluding squares with friendly pieces, and    *
 *  weighs each square according to centralization.         *
 *                                                          *
 ************************************************************
 */
    mobility = BishopMobility(square, OccupiedSquares);
    if (mobility < 0 && (pawn_attacks[enemy][square] & Pawns(side))
        && (File(square) == FILEA || File(square) == FILEH))
      mobility -= 8;
    score_mg += mobility;
    score_eg += mobility;
/*
 ************************************************************
 *                                                          *
 *  Adjust the tropism count for this piece.                *
 *                                                          *
 ************************************************************
 */
    if (tree->dangerous[side]) {
      moves = king_attacks[KingSQ(enemy)];
      i = ((bishop_attacks[square] & moves) &&
          ((BishopAttacks(square, OccupiedSquares & ~Queens(side))) & moves))
          ? 1 : Distance(square, KingSQ(enemy));
      tree->tropism[side] += king_tropism_b[i];
    }
  }
/*
 ************************************************************
 *                                                          *
 *  Add a bonus if this side has a pair of bishops, which   *
 *  can become very strong in open positions.               *
 *                                                          *
 ************************************************************
 */
  if (TotalPieces(side, bishop) > 1) {
    score_mg += bishop_pair[mg];
    score_eg += bishop_pair[eg];
  }
/*
 ************************************************************
 *                                                          *
 *  Check for pawns on both wings, which makes a bishop     *
 *  even more valuable against an enemy knight              *
 *                                                          *
 ************************************************************
 */
  else {
    if (tree->all_pawns & mask_fgh && tree->all_pawns & mask_abc) {
      score_mg += bishop_wing_pawns[mg];
      score_eg += bishop_wing_pawns[eg];
    }
  }
  tree->score_mg += sign[side] * score_mg;
  tree->score_eg += sign[side] * score_eg;
}

/* last modified 01/03/15 */
/*
 *******************************************************************************
 *                                                                             *
 *   EvaluateCastling() is called when "side" has not castled at the root.     *
 *   Its main purpose is to determine if it has either castled somewhere in    *
 *   the tree, or else has lost all (or some) castling rights, which reduces   *
 *   options significantly.                                                    *
 *                                                                             *
 *******************************************************************************
 */
void EvaluateCastling(TREE * RESTRICT tree, int ply, int side) {
  int enemy = Flip(side), oq, score_mg = 0;;

/*
 ************************************************************
 *                                                          *
 *  If the king castled during the search, we are done and  *
 *  we leave it to EvaluateKing() to figure out how safe it *
 *  is.  If it has not castled, we give a significant       *
 *  penalty if the king moves since that loses all castling *
 *  rights, otherwise we give a smaller penalty for moving  *
 *  a rook and giving up castling rights to that side of    *
 *  the board.  The penalty is always increased if the      *
 *  opponent has a queen since the position is much more    *
 *  dangerous.                                              *
 *                                                          *
 ************************************************************
 */
  oq = (Queens(enemy)) ? 3 : 1;
  if (Castle(ply, side) != Castle(1, side)) {
    if (Castle(ply, side) == 0)
      score_mg -= oq * development_losing_castle;
    else if (Castle(ply, side) > 0)
      score_mg -= (oq * development_losing_castle) / 2;
  } else
    score_mg -= oq * development_not_castled;
  tree->score_mg += sign[side] * score_mg;
}

/* last modified 01/03/15 */
/*
 *******************************************************************************
 *                                                                             *
 *   EvaluateDraws() is used to adjust the score based on whether the side     *
 *   that appears to be better according the computed score can actually win   *
 *   the game or not.  If the answer is "no" then the score is reduced         *
 *   significantly to reflect the lack of winning chances.                     *
 *                                                                             *
 *******************************************************************************
 */
int EvaluateDraws(TREE * RESTRICT tree, int ply, int can_win, int score) {
/*
 ************************************************************
 *                                                          *
 *  If the ending has only bishops of opposite colors, the  *
 *  score is pulled closer to a draw.                       *
 *                                                          *
 *  If this is a pure BOC ending, it is very drawish unless *
 *  one side has at least 4 pawns.  More pawns makes it     *
 *  harder for a bishop and king to stop them all from      *
 *  advancing.                                              *
 *                                                          *
 *  If the following are both true:                         *
 *                                                          *
 *    black and white have less than a queen left (pieces   *
 *    only).                                                *
 *                                                          *
 *    both have one bishop and they are opposite colored.   *
 *                                                          *
 *  then either                                             *
 *                                                          *
 *    (a) both have just one bishop, both have less than 4  *
 *    pawns or one side has only one more pawn than the     *
 *    other side then score is divided by 2 with draw score *
 *    added in; or                                          *
 *                                                          *
 *    (b) pieces are equal, then score is reduced by 25%    *
 *    with draw score added in.                             *
 *                                                          *
 ************************************************************
 */
  if (TotalPieces(white, occupied) <= 8 && TotalPieces(black, occupied) <= 8) {
    if (TotalPieces(white, bishop) == 1 && TotalPieces(black, bishop) == 1)
      if (square_color[LSB(Bishops(black))] !=
          square_color[LSB(Bishops(white))]) {
        if (TotalPieces(white, occupied) == 3 &&
            TotalPieces(black, occupied) == 3 &&
            ((TotalPieces(white, pawn) < 4 && TotalPieces(black, pawn) < 4)
                || Abs(TotalPieces(white, pawn) - TotalPieces(black,
                        pawn)) < 2))
          score = score / 2 + DrawScore(1);
        else if (TotalPieces(white, occupied) == TotalPieces(black, occupied))
          score = 3 * score / 4 + DrawScore(1);
      }
  }
/*
 ************************************************************
 *                                                          *
 *  Final score adjustment.  If the score says white is     *
 *  better, but can_win says white can not win, or if the   *
 *  score says black is better, but can_win says black can  *
 *  not win, then we divide the score by 16, and then add   *
 *  in the draw score.  If the can_win says neither side    *
 *  can win, we just set the score to draw score and exit.  *
 *                                                          *
 ************************************************************
 */
  if (can_win != 3) {
    if (can_win & 1) {
      if (score > DrawScore(1))
        score = score / 16 + DrawScore(1);
    } else if (can_win & 2) {
      if (score < DrawScore(1))
        score = score / 16 + DrawScore(1);
    } else
      score = DrawScore(1);
  }
/*
 ************************************************************
 *                                                          *
 *  If we are running into the 50-move rule, then start     *
 *  dragging the score toward draw.  This is the idea of a  *
 *  "weariness factor" as mentioned by Dave Slate many      *
 *  times.  This avoids slamming into a draw at move 50 and *
 *  having to move something quickly, rather than slowly    *
 *  discovering that the score is dropping and that pushing *
 *  a pawn or capturing something will cause it to go back  *
 *  to its correct value a bit more smoothly.               *
 *                                                          *
 ************************************************************
 */
  if (Reversible(ply) > 80) {
    int closeness = 101 - Reversible(ply);

    score = DrawScore(1) + score * closeness / 20;
  }
  return score;
}

/* last modified 01/03/15 */
/*
 *******************************************************************************
 *                                                                             *
 *   EvaluateHasOpposition() is used to determine if one king stands in        *
 *   "opposition" to the other.  If the kings are opposed on the same file or  *
 *   else are opposed on the same diagonal, then the side not-to-move has the  *
 *   opposition and the side-to-move must give way.                            *
 *                                                                             *
 *******************************************************************************
 */
int EvaluateHasOpposition(int on_move, int king, int enemy_king) {
  int file_distance, rank_distance;

  file_distance = FileDistance(king, enemy_king);
  rank_distance = RankDistance(king, enemy_king);
  if (rank_distance < 2)
    return 1;
  if (on_move) {
    if (rank_distance & 1)
      rank_distance--;
    if (file_distance & 1)
      file_distance--;
  }
  if (!(file_distance & 1) && !(rank_distance & 1))
    return 1;
  return 0;
}

/* last modified 01/03/15 */
/*
 *******************************************************************************
 *                                                                             *
 *   EvaluateKing() is used to evaluate a king.                                *
 *                                                                             *
 *******************************************************************************
 */
void EvaluateKing(TREE * RESTRICT tree, int ply, int side) {
  int score_eg = 0, score_mg = 0, defects;
  int ksq = KingSQ(side), enemy = Flip(side);

/*
 ************************************************************
 *                                                          *
 *  First, check for where the king should be if this is an *
 *  endgame.  The basic idea is to centralize unless the    *
 *  king is needed to deal with a passed enemy pawn.        *
 *                                                          *
 ************************************************************
 */
  score_eg += kval[side][ksq];
/*
 ************************************************************
 *                                                          *
 *  Do castle scoring, if the king has castled, the pawns   *
 *  in front are important.  If not castled yet, the pawns  *
 *  on the kingside should be preserved for this.           *
 *                                                          *
 ************************************************************
 */
  if (tree->dangerous[enemy]) {
    defects = 0;
    if (Castle(ply, side) <= 0) {
      if (File(ksq) > FILEE)
        defects = tree->pawn_score.defects_k[side];
      else if (File(ksq) < FILED)
        defects = tree->pawn_score.defects_q[side];
      else
        defects = tree->pawn_score.defects_m[side];
    } else {
      if (Castle(ply, side) == 3)
        defects =
            Min(Min(tree->pawn_score.defects_k[side],
                tree->pawn_score.defects_m[side]),
            tree->pawn_score.defects_q[side]);
      else if (Castle(ply, side) == 1)
        defects =
            Min(tree->pawn_score.defects_k[side],
            tree->pawn_score.defects_m[side]);
      else
        defects =
            Min(tree->pawn_score.defects_q[side],
            tree->pawn_score.defects_m[side]);
      if (defects < 3)
        defects = 3;
    }
/*
 ************************************************************
 *                                                          *
 *  Fold in the king tropism and king pawn shelter scores   *
 *  together.                                               *
 *                                                          *
 ************************************************************
 */
    if (tree->tropism[enemy] < 0)
      tree->tropism[enemy] = 0;
    else if (tree->tropism[enemy] > 15)
      tree->tropism[enemy] = 15;
    if (defects > 15)
      defects = 15;
    score_mg -= king_safety[defects][tree->tropism[enemy]];
  }
  tree->score_mg += sign[side] * score_mg;
  tree->score_eg += sign[side] * score_eg;
}

/* last modified 01/03/15 */
/*
 *******************************************************************************
 *                                                                             *
 *   EvaluateKingsFile computes defects for a file, based on whether the file  *
 *   is open or half-open.  If there are friendly pawns still on the file,     *
 *   they are penalized for advancing in front of the king.                    *
 *                                                                             *
 *******************************************************************************
 */
int EvaluateKingsFile(TREE * RESTRICT tree, int side, int first, int last) {
  int defects = 0, file, enemy = Flip(side);

  for (file = first; file <= last; file++)
    if (!(file_mask[file] & tree->all_pawns))
      defects += open_file[file];
    else {
      if (!(file_mask[file] & Pawns(enemy)))
        defects += half_open_file[file] / 2;
      else
        defects +=
            pawn_defects[side][Rank(MostAdvanced(enemy,
                    file_mask[file] & Pawns(enemy)))];
      if (!(file_mask[file] & Pawns(side)))
        defects += half_open_file[file];
      else if (!(Pawns(side) & SetMask(sqflip[side][A2] + file))) {
        defects++;
        if (!(Pawns(side) & SetMask(sqflip[side][A3] + file)))
          defects++;
      }
    }
  return defects;
}

/* last modified 10/19/15 */
/*
 *******************************************************************************
 *                                                                             *
 *   EvaluateKnights() is used to evaluate knights.                            *
 *                                                                             *
 *******************************************************************************
 */
void EvaluateKnights(TREE * RESTRICT tree, int side) {
  uint64_t temp;
  int square, special, i, score_eg = 0, score_mg = 0, enemy = Flip(side);

/*
 ************************************************************
 *                                                          *
 *  First fold in centralization score from the piece/      *
 *  square table "nval".                                    *
 *                                                          *
 ************************************************************
 */
  for (temp = Knights(side); temp; temp &= temp - 1) {
    square = LSB(temp);
    score_mg += nval[mg][side][square];
    score_eg += nval[eg][side][square];
/*
 ************************************************************
 *                                                          *
 *  Evaluate for "outposts" which is a knight that can't    *
 *  be driven off by an enemy pawn, and which is supported  *
 *  by a friendly pawn.                                     *
 *                                                          *
 *  If the enemy has NO minor to take this knight, then     *
 *  increase the bonus.                                     *
 *                                                          *
 ************************************************************
 */
    special = knight_outpost[side][square];
    if (special && !(mask_pattacks[enemy][square] & Pawns(enemy))) {
      if (pawn_attacks[enemy][square] & Pawns(side)) {
        special += special / 2;
        if (!Knights(enemy) && !(Color(square) & Bishops(enemy)))
          special += knight_outpost[side][square];
      }
      score_eg += special;
      score_mg += special;
    }
/*
 ************************************************************
 *                                                          *
 *  Adjust the tropism count for this piece.                *
 *                                                          *
 ************************************************************
 */
    if (tree->dangerous[side]) {
      i = Distance(square, KingSQ(enemy));
      tree->tropism[side] += king_tropism_n[i];
    }
  }
  tree->score_mg += sign[side] * score_mg;
  tree->score_eg += sign[side] * score_eg;
}

/* last modified 03/30/15 */
/*
 *******************************************************************************
 *                                                                             *
 *   EvaluateMate() is used to evaluate positions where neither side has pawns *
 *   and one side has enough material to force checkmate.  It simply trys to   *
 *   force the losing king to the edge of the board, and then to the corner    *
 *   where mates are easier to find.                                           *
 *                                                                             *
 *******************************************************************************
 */
void EvaluateMate(TREE * RESTRICT tree, int side) {
  int mate_score = 0, enemy = Flip(side);

/*
 ************************************************************
 *                                                          *
 *  If the winning side has a bishop + knight and the other *
 *  side has no pieces or pawns, then use the special       *
 *  bishop_knight scoring board for the losing king to      *
 *  force it to the right corner for mate.                  *
 *                                                          *
 ************************************************************
 */
  if (!TotalPieces(enemy, occupied) && TotalPieces(side, bishop) == 1 &&
      TotalPieces(side, knight) == 1) {
    if (dark_squares & Bishops(side))
      mate_score = b_n_mate_dark_squares[KingSQ(enemy)];
    else
      mate_score = b_n_mate_light_squares[KingSQ(enemy)];
  }
/*
 ************************************************************
 *                                                          *
 *  The winning side has to force the losing king to the    *
 *  edge of the board.                                      *
 *                                                          *
 ************************************************************
 */
  else
    mate_score = mate[KingSQ(enemy)];
/*
 ************************************************************
 *                                                          *
 *  And for either, it is important to bring the winning    *
 *  king to help force mate.                                *
 *                                                          *
 ************************************************************
 */
  mate_score -= Distance(KingSQ(side), KingSQ(enemy)) * king_king_tropism;
  tree->score_eg += sign[side] * mate_score;
}

/* last modified 10/19/15 */
/*
 *******************************************************************************
 *                                                                             *
 *  EvaluateMaterial() is used to evaluate material on the board.  It really   *
 *  accomplishes detecting cases where one side has made a 'bad trade' as the  *
 *  comments below show.                                                       *
 *                                                                             *
 *******************************************************************************
 */
void EvaluateMaterial(TREE * RESTRICT tree, int wtm) {
  int score_mg, score_eg, majors, minors;

/*
 *************************************************************
 *                                                           *
 *  We start with the raw Material balance for the current   *
 *  position, then adjust this with a small bonus for the    *
 *  side on move.                                            *
 *                                                           *
 *************************************************************
 */
  score_mg = Material + ((wtm) ? wtm_bonus[mg] : -wtm_bonus[mg]);
  score_eg = Material + ((wtm) ? wtm_bonus[eg] : -wtm_bonus[eg]);
/*
 *************************************************************
 *                                                           *
 *  test 1.  if Majors or Minors are not balanced, then if   *
 *  one side is only an exchange up or down, we give a       *
 *  penalty to the side that is an exchange down, but not as *
 *  big a penalty as the bad trade case below.               *
 *                                                           *
 *  test 2.  if Majors or Minors are not balanced, then if   *
 *  one side has more piece material points than the other   *
 *  (using normal piece values of 3, 3, 5, 9 for N, B, R     *
 *  and Q) then the side that is behind in piece material    *
 *  gets a penalty.                                          *
 *                                                           *
 *************************************************************
 */
  majors =
      TotalPieces(white, rook) + 2 * TotalPieces(white,
      queen) - TotalPieces(black, rook) - 2 * TotalPieces(black, queen);
  minors =
      TotalPieces(white, knight) + TotalPieces(white,
      bishop) - TotalPieces(black, knight) - TotalPieces(black, bishop);
  if (majors || minors) {
    if (Abs(TotalPieces(white, occupied) - TotalPieces(black, occupied)) != 2
        && TotalPieces(white, occupied) - TotalPieces(black, occupied) != 0) {
      score_mg +=
          Sign(TotalPieces(white, occupied) - TotalPieces(black,
              occupied)) * bad_trade;
      score_eg +=
          Sign(TotalPieces(white, occupied) - TotalPieces(black,
              occupied)) * bad_trade;
    }
  }
  tree->score_mg += score_mg;
  tree->score_eg += score_eg;
}

/* last modified 11/27/15 */
/*
 *******************************************************************************
 *                                                                             *
 *   EvaluatePassedPawns() is used to evaluate passed pawns and the danger     *
 *   they produce.  This code considers pieces as well, so it MUST NOT be done *
 *   in the normal EvaluatePawns() code since that hashes information based    *
 *   only on the position of pawns.                                            *
 *                                                                             *
 *   This is a significant rewrite of passed pawn evaluation, with the primary *
 *   change being to collect the passed pawn scoring into one place, rather    *
 *   than have it scattered around all over the place.  One example is the old *
 *   rook_behind_passed_pawn scoring term that was done in rook scoring.  It   *
 *   is now done here along with other passed pawn terms such as blockaded and *
 *   the ability to advance or not.                                            *
 *                                                                             *
 *******************************************************************************
 */
void EvaluatePassedPawns(TREE * RESTRICT tree, int side, int wtm) {
  uint64_t behind, forward, backward, attacked, defended, thispawn;
  int file, square, score, score_mg = 0, score_eg = 0, next_sq;
  int pawns, rank, mg_base, eg_base, bonus, enemy = Flip(side);
  uint64_t fsliders = Queens(side) | Rooks(side);
  uint64_t esliders = Queens(enemy) | Rooks(enemy);

/*
 ************************************************************
 *                                                          *
 *  Initialize.  The base value "passed_pawn[rank]" is      *
 *  almost the "square" of the rank.  That got a bit too    *
 *  big, so a hand-tuned set of values, one per rank,       *
 *  proved to be a better value.                            *
 *                                                          *
 ************************************************************
 */
  for (pawns = tree->pawn_score.passed[side]; pawns; pawns &= pawns - 1) {
    file = LSB8Bit(pawns);
    thispawn = Pawns(side) & file_mask[file];
    if (thispawn) {
      square = MostAdvanced(side, thispawn);
      rank = rankflip[side][Rank(square)];
      score = passed_pawn[rank];
/*
 ************************************************************
 *                                                          *
 *  For endgame only, we add in a bonus based on how close  *
 *  our king is to this pawn and a penalty based on how     *
 *  close the enemy king is.  We also try to keep our king  *
 *  ahead of the pawn so it can escort it to promotion.  We *
 *  only do this for passed pawns whose base score value is *
 *  greater than zero (ie pawns on ranks 4-7 since those    *
 *  are the ones threatening to become a major problem.)    *
 *  Also, if you happen to think that a small bonus for a   *
 *  passed pawn on the 3rd rank might be useful, consider   *
 *  speed.  If the 3rd rank score is non-zero, that will    *
 *  trigger a significant amount of work below.  In testing *
 *  the additional cost more than offset the gain and so it *
 *  is basically ignored unless rank > 3.                   *
 *                                                          *
 ************************************************************
 */
      if (score) {
        mg_base = score * passed_pawn_base[mg];
        eg_base = score * passed_pawn_base[eg];
        next_sq = square + direction[side];
        eg_base +=
            Distance(KingSQ(enemy),
            next_sq) * 2 * score - Distance(KingSQ(side), next_sq) * score;
        if (rank < RANK7)
          eg_base -=
              Distance(KingSQ(side), next_sq + direction[side]) * score / 2;
/*
 ************************************************************
 *                                                          *
 *  If the pawn is not blockaded, we need to see whether it *
 *  can actually advance or not.  Note that this directly   *
 *  gives a bonus for blockading a passed pawn since the    *
 *  mobility evaluation below will not be applied when the  *
 *  pawn is blockaded by any piece.                         *
 *                                                          *
 *  Step one is to determine if the squares in front of the *
 *  pawn are attacked by the enemy.  If not, we add in a    *
 *  significant score bonus.  If some are attacked, we look *
 *  to see if at least the square directly in front of the  *
 *  pawn is not attacked so that we can advance one square, *
 *  anyway.  This gets a smaller score bonus.               *
 *                                                          *
 ************************************************************
 */
        if (!(OccupiedSquares & SetMask(next_sq))) {
          bonus = 0;
          if (Pawns(side) & pawn_attacks[enemy][next_sq])
            bonus = passed_pawn_free_advance;
          else {
            attacked = 0;
            forward = (side) ? plus8dir[square] : minus8dir[square];
            backward = (side) ? minus8dir[square] : plus8dir[square];
            if ((behind = backward & esliders) &&
                (FileAttacks(square) & behind))
              attacked = forward;
            else
              attacked = Attacked(tree, enemy, forward);
            if (!attacked)
              bonus = passed_pawn_free_advance;
            else if (!(attacked & SetMask(next_sq)))
              bonus = passed_pawn_free_advance_1;
/*
 ************************************************************
 *                                                          *
 *  Step two is to determine if the squares in front of the *
 *  pawn are are defended by the friendly side.  If all are *
 *  defended (such as with a rook or queen behind the pawn  *
 *  or the king in front and to one side of the pawn, then  *
 *  we give a bonus (but smaller than the previous cases).  *
 *  As a last resort, if we at least defend the square in   *
 *  front of the pawn, we give a small bonus.               *
 *                                                          *
 ************************************************************
 */
            if ((behind = backward & fsliders) &&
                (FileAttacks(square) & behind))
              defended = forward;
            else
              defended = Attacked(tree, side, forward);
            if (defended == forward)
              bonus += passed_pawn_defended;
            else if (defended & SetMask(next_sq))
              bonus += passed_pawn_defended_1;
          }
/*
 ************************************************************
 *                                                          *
 *  Fold in the bonus for this pawn and move on to the next *
 *  (if there is one).  Note that the bonus computed above  *
 *  is multiplied by the base passed pawn score for this    *
 *  particular rank.                                        *
 *                                                          *
 ************************************************************
 */
          mg_base += bonus * score;
          eg_base += bonus * score;
        }
        score_mg += mg_base;
        score_eg += eg_base;
      } else
        score_eg += 4;
    }
  }
/*
 ************************************************************
 *                                                          *
 *  All pawns done, add score to the two evaluation values  *
 *  and return.                                             *
 *                                                          *
 ************************************************************
 */
  tree->score_mg += sign[side] * score_mg;
  tree->score_eg += sign[side] * score_eg;
}

/* last modified 11/27/15 */
/*
 *******************************************************************************
 *                                                                             *
 *   EvaluatePassedPawnRaces() is used to evaluate passed pawns when one       *
 *   side has passed pawns and the other side (or neither) has pieces.  In     *
 *   such a case, the critical question is can the defending king stop the     *
 *   pawn from queening or is it too far away?  If only one side has pawns     *
 *   that can "run" then the situation is simple.  When both sides have pawns  *
 *   that can "run" it becomes more complex as it then becomes necessary to    *
 *   see if one side can use a forced king move to stop the other side, while  *
 *   the other side doesn't have the same ability to stop ours.                *
 *                                                                             *
 *   In the case of king and pawn endings with exactly one pawn, the simple    *
 *   evaluation rules are used:  if the king is two squares in front of the    *
 *   pawn then it is a win, if the king is one one square in front with the    *
 *   opposition, then it is a win,  if the king is on the 6th rank with the    *
 *   pawn close by, it is a win.  Rook pawns are handled separately and are    *
 *   more difficult to queen because the king can get trapped in front of the  *
 *   pawn blocking promotion.                                                  *
 *                                                                             *
 *******************************************************************************
 */
void EvaluatePassedPawnRaces(TREE * RESTRICT tree, int wtm) {
  uint64_t pawns, thispawn;
  int file, square, queen_distance, pawnsq, passed, side, enemy;
  int queener[2] = { 8, 8 };
/*
 ************************************************************
 *                                                          *
 *  Check to see if side has one pawn and neither side has  *
 *  any pieces.  If so, use the simple pawn evaluation      *
 *  logic.                                                  *
 *                                                          *
 ************************************************************
 */
  for (side = black; side <= white; side++) {
    enemy = Flip(side);
    if (Pawns(side) && !Pawns(enemy) && TotalPieces(white, occupied) == 0 &&
        TotalPieces(black, occupied) == 0) {
      for (pawns = Pawns(side); pawns; pawns &= pawns - 1) {
        pawnsq = LSB(pawns);
/*
 ************************************************************
 *                                                          *
 *  King must be in front of the pawn or we go no further.  *
 *                                                          *
 ************************************************************
 */
        if (sign[side] * Rank(KingSQ(side)) <= sign[side] * Rank(pawnsq))
          continue;
/*
 ************************************************************
 *                                                          *
 *  First a special case.  If this is a rook pawn, then the *
 *  king must be on the adjacent file, and be closer to the *
 *  queening square than the opposing king.                 *
 *                                                          *
 ************************************************************
 */
        if (File(pawnsq) == FILEA) {
          if (File(KingSQ(side)) == FILEB &&
              Distance(KingSQ(side),
                  sqflip[side][A8]) < Distance(KingSQ(enemy),
                  sqflip[side][A8])) {
            tree->score_eg += sign[side] * pawn_can_promote;
            return;
          }
          continue;
        } else if (File(pawnsq) == FILEH) {
          if (File(KingSQ(side)) == FILEG &&
              Distance(KingSQ(side),
                  sqflip[side][H8]) < Distance(KingSQ(enemy),
                  sqflip[side][H8])) {
            tree->score_eg += sign[side] * pawn_can_promote;
            return;
          }
          continue;
        }
/*
 ************************************************************
 *                                                          *
 *  If king is two squares in front of the pawn then it's a *
 *  win immediately.  If the king is on the 6th rank and    *
 *  closer to the pawn than the opposing king, it's also a  *
 *  win.                                                    *
 *                                                          *
 ************************************************************
 */
        if (Distance(KingSQ(side), pawnsq) < Distance(KingSQ(enemy), pawnsq)) {
          if (sign[side] * Rank(KingSQ(side)) >
              sign[side] * (Rank(pawnsq) - 1 + 2 * side)) {
            tree->score_eg += sign[side] * pawn_can_promote;
            return;
          }
          if (Rank(KingSQ(side)) == rank6[side]) {
            tree->score_eg += sign[side] * pawn_can_promote;
            return;
          }
/*
 ************************************************************
 *                                                          *
 *  Last chance:  if the king is one square in front of the *
 *  pawn and has the opposition, then it's still a win.     *
 *                                                          *
 ************************************************************
 */
          if (Rank(KingSQ(side)) == Rank(pawnsq) - 1 + 2 * side &&
              EvaluateHasOpposition(wtm == side, KingSQ(side),
                  KingSQ(enemy))) {
            tree->score_eg += sign[side] * pawn_can_promote;
            return;
          }
        }
      }
    }
/*
 ************************************************************
 *                                                          *
 *  Check to see if enemy is out of pieces and stm has      *
 *  passed pawns.  If so, see if any of these passed pawns  *
 *  can outrun the defending king and promote.              *
 *                                                          *
 ************************************************************
 */
    if (TotalPieces(enemy, occupied) == 0 && tree->pawn_score.passed[side]) {
      passed = tree->pawn_score.passed[side];
      for (; passed; passed &= passed - 1) {
        file = LSB8Bit(passed);
        thispawn = Pawns(side) & file_mask[file];
        if (thispawn) {
          square = MostAdvanced(side, thispawn);
          if (!(pawn_race[side][wtm][square] & Kings(enemy))) {
            queen_distance = Abs(rank8[side] - Rank(square));
            if (Kings(side) & ((side) ? plus8dir[square] : minus8dir[square])) {
              if (file == FILEA || file == FILEH)
                queen_distance = 99;
              queen_distance++;
            }
            if (Rank(square) == rank2[side])
              queen_distance--;
            if (queen_distance < queener[side])
              queener[side] = queen_distance;
          }
        }
      }
    }
  }
/*
 ************************************************************
 *                                                          *
 *  Now that we know which pawns can outrun the kings for   *
 *  each side, we need to do determine if one side queens   *
 *  before the other.  If so, that side wins.  If they      *
 *  queen at the same time, then we will have to rely on    *
 *  the search to handle queening with check or queening    *
 *  and attacking the opponent's queening square.           *
 *                                                          *
 ************************************************************
 */
  if (queener[white] < queener[black])
    tree->score_eg += pawn_can_promote + (5 - queener[white]) * 10;
  else if (queener[black] < queener[white])
    tree->score_eg -= pawn_can_promote + (5 - queener[black]) * 10;
}

/* last modified 11/27/15 */
/*
 *******************************************************************************
 *                                                                             *
 *   EvaluatePawns() is used to evaluate pawns.  It evaluates pawns for only   *
 *   one side, and fills in the pawn hash entry information.  It requires two  *
 *   calls to evaluate all pawns on the board.  Comments below indicate the    *
 *   particular pawn structure features that are evaluated.                    *
 *                                                                             *
 *   This procedure also flags which pawns are passed, since this is scored in *
 *   another part of the evaluation because that includes piece information    *
 *   which can not be used here since the pawn hash signature does not include *
 *   piece information of any kind.                                            *
 *                                                                             *
 *   Note that a pawn is not penalized for two different reasons.  If it is    *
 *   isolated, it is not backward.  Etc.  This simplifies evaluation tuning    *
 *   not to mention eliminating the overlap and interaction that was happening *
 *   previously when multiple penalties could be applied.                      *
 *                                                                             *
 *******************************************************************************
 */
void EvaluatePawns(TREE * RESTRICT tree, int side) {
  uint64_t pawns, attackers, defenders;
  uint64_t doubled, supported, connected, passed, backward;
  int square, file, rank, score_eg = 0, score_mg = 0, enemy = Flip(side);
  unsigned int isolated, pawn_files = 0;

/*
 ************************************************************
 *                                                          *
 *  Loop through all pawns for this side.                   *
 *                                                          *
 ************************************************************
 */
  tree->pawn_score.passed[side] = 0;
  for (pawns = Pawns(side); pawns; pawns &= pawns - 1) {
    square = LSB(pawns);
    file = File(square);
    rank = rankflip[side][Rank(square)];
    pawn_files |= 1 << file;
/*
 ************************************************************
 *                                                          *
 *  Evaluate pawn advances.  Center pawns are encouraged to *
 *  occupy central squares, edge pawns are penalized on all *
 *  edge squares to encourage capture toward the center,    *
 *  the rest are neutral.                                   *
 *                                                          *
 ************************************************************
 */
    score_mg += pval[side][square];
/*
 ************************************************************
 *                                                          *
 *  Evaluate isolated pawns, which are penalized based on   *
 *  which file they occupy.                                 *
 *                                                          *
 ************************************************************
 */
    isolated = !(Pawns(side) & mask_pawn_isolated[square]);
    if (isolated) {
      score_mg -= pawn_isolated[mg][file];
      score_eg -= pawn_isolated[eg][file];
    }
/*
 ************************************************************
 *                                                          *
 *  Evaluate unsupported pawns, which provide a target      *
 *  since they are undefended by a pawn.  We exclude pawns  *
 *  that are isolated since they have already been given a  *
 *  penalty.                                                *
 *                                                          *
 ************************************************************
 */
    supported = Pawns(side) & pawn_attacks[enemy][square];
    if (!isolated && !supported) {
      score_mg += pawn_unsupported[mg];
      score_eg += pawn_unsupported[eg];
    }
/*
 ************************************************************
 *                                                          *
 *  Evaluate doubled pawns.  If there are other pawns on    *
 *  this file in front of this pawn, penalize this pawn.    *
 *  Note that this will NOT penalize both pawns, just the   *
 *  most rearward one that is really almost worthless.      *
 *                                                          *
 *  The farther apart two doubled pawns (same file) are,    *
 *  the less weak they are, so the penalty is reduced as    *
 *  this distance increases.                                *
 *                                                          *
 ************************************************************
 */
    doubled = Pawns(side) & ((side) ? plus8dir[square] : minus8dir[square]);
    if (doubled) {
      score_mg -=
          pawn_doubled[mg][file] / RankDistance(square, MostAdvanced(side,
              doubled));
      score_eg -=
          pawn_doubled[eg][file] / RankDistance(square, MostAdvanced(side,
              doubled));
    }
/*
 ************************************************************
 *                                                          *
 *  Test the pawn to see if it is connected to a neighbor   *
 *  which makes it easier to defend.                        *
 *                                                          *
 ************************************************************
 */
    connected = Pawns(side) & mask_pawn_connected[side][square];
    if (connected) {
      score_mg += pawn_connected[mg][rank][file];
      score_eg += pawn_connected[eg][rank][file];
    }
/*
 ************************************************************
 *                                                          *
 *  Flag passed pawns for use later when we finally call    *
 *  EvaluatePassedPawns.                                    *
 *                                                          *
 ************************************************************
 */
    passed = !(Pawns(enemy) & mask_passed[side][square]);
    if (passed)
      tree->pawn_score.passed[side] |= 1 << file;
/*
 ************************************************************
 *                                                          *
 *  Test the pawn to see if it is backward which makes it a *
 *  target that ties down pieces to defend it.              *
 *                                                          *
 ************************************************************
 */
    backward = 0;
    if (!(passed | isolated | connected | (Pawns(side) &
                mask_pattacks[side][square]) | (Pawns(enemy) &
                PawnAttacks(side, square))))
      backward = Pawns(enemy) & PawnAttacks(side, square + direction[side]);
    if (backward) {
      score_mg -= pawn_backward[mg][file];
      score_eg -= pawn_backward[eg][file];
    }
/*
 ************************************************************
 *                                                          *
 *  Determine if this pawn is a candidate passed pawn,      *
 *  which is a pawn on a file with no enemy pawns in front  *
 *  of it, and if it advances until it contacts an enemy    *
 *  pawn, and it is defended at least as many times as it   *
 *  is attacked when it reaches that pawn, then it will end *
 *  up passed.                                              *
 *                                                          *
 ************************************************************
 */
    if (!(passed | backward | isolated) &&
        !(Pawns(enemy) & ((side) ? plus8dir[square] : minus8dir[square]))) {
      defenders = mask_pattacks[side][square + direction[side]] & Pawns(side);
      attackers = mask_pattacks[enemy][square] & Pawns(enemy);
      if (PopCnt(defenders) >= PopCnt(attackers)) {
        score_mg += passed_pawn_candidate[mg][rank];
        score_eg += passed_pawn_candidate[eg][rank];
      }
    }
  }
/*
 ************************************************************
 *                                                          *
 *  Give a bonus for distance between left-most pawn and    *
 *  right-most pawn.  The idea is that the wider the gap    *
 *  between the pawns, the harder they are for a lone king  *
 *  to control in the endgame.  Botvinnik referred to this  *
 *  concept as "trousers" (pants with two legs, the farther *
 *  the legs are apart, the better for the side with those  *
 *  pawns).                                                 *
 *                                                          *
 ************************************************************
 */
  score_eg += pawn_file_width * (MSB8Bit(pawn_files) - LSB8Bit(pawn_files));
/*
 ************************************************************
 *                                                          *
 *  Evaluate king safety.                                   *
 *                                                          *
 *  This uses the function EvaluateKingsFile() and looks at *
 *  three possible positions for the king, either castled   *
 *  kingside, queenside or else standing on the d or e file *
 *  stuck in the middle.  This essentially is about the     *
 *  pawns in front of the king and what kind of "shelter"   *
 *  they provide for the king during the middlegame.        *
 *                                                          *
 ************************************************************
 */
  tree->pawn_score.defects_q[side] =
      EvaluateKingsFile(tree, side, FILEA, FILEC);
  tree->pawn_score.defects_m[side] =
      EvaluateKingsFile(tree, side, FILEC, FILEF);
  tree->pawn_score.defects_k[side] =
      EvaluateKingsFile(tree, side, FILEF, FILEH);
/*
 ************************************************************
 *                                                          *
 *  Done.  Add mg/eg scores to final result (sign-corrected *
 *  so that black = -, white = +) and return.               *
 *                                                          *
 ************************************************************
 */
  tree->pawn_score.score_mg += sign[side] * score_mg;
  tree->pawn_score.score_eg += sign[side] * score_eg;
}

/* last modified 10/19/15 */
/*
 *******************************************************************************
 *                                                                             *
 *   EvaluateQueens() is used to evaluate queens.                              *
 *                                                                             *
 *******************************************************************************
 */
void EvaluateQueens(TREE * RESTRICT tree, int side) {
  uint64_t temp;
  int square, i, score_mg = 0, score_eg = 0, enemy = Flip(side);

/*
 ************************************************************
 *                                                          *
 *  First locate each queen and obtain it's centralization  *
 *  score from the static piece/square table for queens.    *
 *                                                          *
 ************************************************************
 */
  for (temp = Queens(side); temp; temp &= temp - 1) {
    square = LSB(temp);
/*
 ************************************************************
 *                                                          *
 *  Then, add in the piece/square table value for the       *
 *  queen.                                                  *
 *                                                          *
 ************************************************************
*/
    score_mg += qval[mg][side][square];
    score_eg += qval[eg][side][square];
/*
 ************************************************************
 *                                                          *
 *  Adjust the tropism count for this piece.                *
 *                                                          *
 ************************************************************
 */
    if (tree->dangerous[side]) {
      i = KingSQ(enemy);
      tree->tropism[side] += king_tropism_q[Distance(square, i)];
      i = 8 - (RankDistance(square, i) + FileDistance(square, i));
      score_mg += i;
      score_eg += i;
    }
  }
  tree->score_mg += sign[side] * score_mg;
  tree->score_eg += sign[side] * score_eg;
}

/* last modified 10/19/15 */
/*
 *******************************************************************************
 *                                                                             *
 *   EvaluateRooks() is used to evaluate rooks.                                *
 *                                                                             *
 *******************************************************************************
 */
void EvaluateRooks(TREE * RESTRICT tree, int side) {
  uint64_t temp, moves;
  int square, rank, file, i, mobility, score_mg = 0, score_eg = 0;
  int enemy = Flip(side);

/*
 ************************************************************
 *                                                          *
 *  Initialize.                                             *
 *                                                          *
 ************************************************************
 */
  for (temp = Rooks(side); temp; temp &= temp - 1) {
    square = LSB(temp);
    file = File(square);
    rank = Rank(square);
/*
 ************************************************************
 *                                                          *
 *  Determine if the rook is on an open file or on a half-  *
 *  open file, either of which increases its ability to     *
 *  attack important squares.                               *
 *                                                          *
 ************************************************************
 */
    if (!(file_mask[file] & Pawns(side))) {
      if (!(file_mask[file] & Pawns(enemy))) {
        score_mg += rook_open_file[mg];
        score_eg += rook_open_file[eg];
      } else {
        score_mg += rook_half_open_file[mg];
        score_eg += rook_half_open_file[eg];
      }
    }
/*
 ************************************************************
 *                                                          *
 *  Mobility counts the number of squares the rook attacks, *
 *  excluding squares with friendly pieces, and weighs each *
 *  square according to a complex formula that includes     *
 *  files as well as total number of squares attacked.      *
 *                                                          *
 ************************************************************
 */
    mobility = RookMobility(square, OccupiedSquares);
    score_mg += mobility;
    score_eg += mobility;
/*
 ************************************************************
 *                                                          *
 *  Check to see if the king has been forced to move and    *
 *  has trapped a rook at a1/b1/g1/h1, if so, then penalize *
 *  the trapped rook to help extricate it.  We only need to *
 *  check this if the rooks mobility is very low.           *
 *                                                          *
 ************************************************************
 */
    if (mobility < 0 && rank == rank1[side] && rank == Rank(KingSQ(side))) {
      i = File(KingSQ(side));
      if (i > FILEE) {
        if (file > i) {
          score_mg += mobility * 3;
          score_eg += mobility * 3;
        }
      } else if (i < FILED && file < i) {
        score_mg += mobility * 3;
        score_eg += mobility * 3;
      }
    }
/*
 ************************************************************
 *                                                          *
 *   finally check to see if any rooks are on the 7th rank, *
 *   with the opponent having pawns on that rank or the     *
 *   opponent's king being hemmed in on the 7th/8th rank.   *
 *   If so, we give a bonus for the strong rook.  If there  *
 *   is another rook or queen on the 7th that is connected  *
 *   with this one, then the positional advantage is even   *
 *   stronger.                                              *
 *                                                          *
 ************************************************************
 */
    else if (rank == rank7[side] && (Rank(KingSQ(enemy)) == rank8[side]
            || Pawns(enemy) & rank_mask[rank])) {
      score_mg += rook_on_7th[mg];
      score_eg += rook_on_7th[eg];
      if (RankAttacks(square) & (Queens(side) | Rooks(side))) {
        score_mg += rook_connected_7th[mg];
        score_eg += rook_connected_7th[eg];
      }
    }
/*
 ************************************************************
 *                                                          *
 *  Adjust the tropism count for this piece.                *
 *                                                          *
 ************************************************************
 */
    if (tree->dangerous[side]) {
      moves = king_attacks[KingSQ(enemy)];
      i = (rook_attacks[square] & moves &&
          RookAttacks(square,
              OccupiedSquares & ~(Queens(side) | Rooks(side))) & moves) ? 1 :
          Distance(square, KingSQ(enemy));
      tree->tropism[side] += king_tropism_r[i];
    }
  }
  tree->score_mg += sign[side] * score_mg;
  tree->score_eg += sign[side] * score_eg;
}

/* last modified 01/03/15 */
/*
 *******************************************************************************
 *                                                                             *
 *   EvaluateWinningChances() is used to determine if one side has reached a   *
 *   position which can not be won, period, even though side may be ahead in   *
 *   material in some way.                                                     *
 *                                                                             *
 *   Return values:                                                            *
 *          0    ->     side on move can not win.                              *
 *          1    ->     side on move can win.                                  *
 *                                                                             *
 *******************************************************************************
 */
int EvaluateWinningChances(TREE * RESTRICT tree, int side, int wtm) {
  int square, ekd, promote, majors, minors, enemy = Flip(side);

  if (!Pawns(side)) {
/*
 ************************************************************
 *                                                          *
 *  If side has a piece and no pawn, it can not possibly    *
 *  win.  If side is a piece ahead, the only way it can win *
 *  is if the enemy is already trapped on the edge of the   *
 *  board (special case to handle KRB vs KR which can be    *
 *  won if the king gets trapped).                          *
 *                                                          *
 ************************************************************
 */
    if (TotalPieces(side, occupied) <= 3)
      return 0;
    if (TotalPieces(side, occupied) - TotalPieces(enemy, occupied) <= 3 &&
        mask_not_edge & Kings(enemy))
      return 0;
/*
 ************************************************************
 *                                                          *
 *  If one side is an exchange up, but has no pawns, then   *
 *  that side can not possibly win.                         *
 *                                                          *
 ************************************************************
 */
    majors =
        TotalPieces(white, rook) + 2 * TotalPieces(white,
        queen) - TotalPieces(black, rook) - 2 * TotalPieces(black, queen);
    if (Abs(majors) == 1) {
      minors =
          TotalPieces(black, knight) + TotalPieces(black,
          bishop) - TotalPieces(white, knight) - TotalPieces(white, bishop);
      if (majors == minors)
        return 0;
    }
  } else {
/*
 ************************************************************
 *                                                          *
 *  If neither side has any pieces, and both sides have     *
 *  non-rookpawns, then either side can win.                *
 *                                                          *
 ************************************************************
 */
    if (TotalPieces(white, occupied) == 0 && TotalPieces(black, occupied) == 0
        && Pawns(white) & not_rook_pawns && Pawns(black) & not_rook_pawns)
      return 1;
  }
/*
 ************************************************************
 *                                                          *
 *  If "side" has a pawn, then either the pawn had better   *
 *  not be a rook pawn, or else side had better have the    *
 *  right color bishop or any other piece, otherwise it is  *
 *  not winnable if the enemy king can get to the queening  *
 *  square first.                                           *
 *                                                          *
 ************************************************************
 */
  if (!(Pawns(side) & not_rook_pawns))
    do {
      if (TotalPieces(side, occupied) > 3 || (TotalPieces(side, occupied) == 3
              && Knights(side)))
        continue;
      if (file_mask[FILEA] & Pawns(side) && file_mask[FILEH] & Pawns(side))
        continue;
      if (Bishops(side)) {
        if (Bishops(side) & dark_squares) {
          if (file_mask[dark_corner[side]] & Pawns(side))
            continue;
        } else if (file_mask[light_corner[side]] & Pawns(side))
          continue;
      }
      if (Pawns(side) & file_mask[FILEA])
        promote = A8;
      else
        promote = H8;
      ekd = Distance(KingSQ(enemy), sqflip[side][promote]) - (wtm != side);
      if (ekd <= 1)
        return 0;
    } while (0);
/*
 ************************************************************
 *                                                          *
 *  Check to see if this is a KRP vs KR ending.  If so, and *
 *  the losing king is in front of the passer, then this is *
 *  a drawish ending.                                       *
 *                                                          *
 ************************************************************
 */
  if (TotalPieces(side, pawn) == 1 && TotalPieces(enemy, pawn) == 0 &&
      TotalPieces(side, occupied) == 5 && TotalPieces(enemy, occupied) == 5) {
    square = LSB(Pawns(side));
    if (FileDistance(KingSQ(enemy), square) <= 1 &&
        InFront(side, Rank(KingSQ(enemy)), Rank(square)))
      return 0;
  }
/*
 ************************************************************
 *                                                          *
 *  If this side has pawns, and we have made it through the *
 *  previous tests, then this side has winning chances.     *
 *                                                          *
 ************************************************************
 */
  if (TotalPieces(side, pawn))
    return 1;
/*
 ************************************************************
 *                                                          *
 *  If this side has two bishops, and the enemy has only a  *
 *  single kinght, the two bishops win.                     *
 *                                                          *
 ************************************************************
 */
  if (TotalPieces(side, occupied) == 6)
    if (TotalPieces(enemy, occupied) == 3 && (Knights(side)
            || !Knights(enemy)))
      return 0;
/*
 ************************************************************
 *                                                          *
 *  If one side is two knights ahead and the opponent has   *
 *  no remaining material, it is a draw.                    *
 *                                                          *
 ************************************************************
 */
  if (TotalPieces(side, occupied) == 6 && !Bishops(side)
      && TotalPieces(enemy, occupied) + TotalPieces(enemy, pawn) == 0)
    return 0;
/*
 ************************************************************
 *                                                          *
 *  If we make it through all the above tests, then "side"  *
 *  can win so we return 1.                                 *
 *                                                          *
 ************************************************************
 */
  return 1;
}

/*
 *******************************************************************************
 *                                                                             *
 *   InitializeKingSafety() is used to initialize the king safety matrix.      *
 *   This is set so that the matrix, indexed by king safety pawn structure     *
 *   index and by king safety piece tropism, combines the two indices to       *
 *   produce a single score.  As either index rises, the king safety score     *
 *   tracks along, but as both rise, the king safety score rises much more     *
 *   quickly.                                                                  *
 *                                                                             *
 *******************************************************************************
 */
void InitializeKingSafety() {
  int safety, tropism;

  for (safety = 0; safety < 16; safety++) {
    for (tropism = 0; tropism < 16; tropism++) {
      king_safety[safety][tropism] =
          180 * ((safety_vector[safety] + 100) * (tropism_vector[tropism] +
              100) / 100 - 100) / 100;
    }
  }
}
