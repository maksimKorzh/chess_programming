#include "chess.h"
#include "data.h"
/* last modified 08/03/16 */
/*
 *******************************************************************************
 *                                                                             *
 *   ResignOrDraw() is used to determine if the program should either resign   *
 *   or offer a draw.  This decision is based on two criteria:  (1) current    *
 *   search evaluation and (2) time remaining on opponent's clock.             *
 *                                                                             *
 *   The evaluation returned by the last search must be less than the resign   *
 *   threshold to trigger the resign code, or else must be exactly equal to    *
 *   the draw score to trigger the draw code.                                  *
 *                                                                             *
 *   The opponent must have enough time to be able to win or draw the game if  *
 *   it were played out as well.                                               *
 *                                                                             *
 *******************************************************************************
 */
void ResignOrDraw(TREE * RESTRICT tree, int value) {
  int v, result = 0;

/*
 ************************************************************
 *                                                          *
 *  If the game is a technical draw, where there are no     *
 *  pawns and material is balanced, then offer a draw.      *
 *                                                          *
 ************************************************************
 */
  if (Drawn(tree, value) == 1)
    result = 2;
/*
 ************************************************************
 *                                                          *
 *  First check to see if the increment is 2 seconds or     *
 *  more.  If so, then the game is being played slowly      *
 *  enough that a draw offer or resignation is worth        *
 *  consideration.  Otherwise, if the opponent has at least *
 *  30 seconds left, he can probably play the draw or win   *
 *  out.                                                    *
 *                                                          *
 *  If the value is below the resignation threshold, then   *
 *  Crafty should resign and get on to the next game. Note  *
 *  that it is necessary to have a bad score for            *
 *  <resign_count> moves in a row before resigning.         *
 *                                                          *
 *  Note that we don't resign for "deep mates" since we do  *
 *  not know if the opponent actually saw that result.  We  *
 *  play on until it becomes obvious he "sees it."          *
 *                                                          *
 ************************************************************
 */
  if ((tc_increment > 200) || (tc_time_remaining[Flip(root_wtm)] >= 3000)) {
    if (resign) {
      if (value < -(MATE - 15)) {
        if (++resign_counter >= resign_count)
          result = 1;
      } else if (value < -resign * 100 && value > -32000) {
        if (++resign_counter >= resign_count)
          result = 1;
      } else
        resign_counter = 0;
    }
  }
/*
 ************************************************************
 *                                                          *
 *  If the value is almost equal to the draw score, then    *
 *  Crafty should offer the opponent a draw.  Note that it  *
 *  is necessary that the draw score occur on exactly       *
 *  <draw_count> moves in a row before making the offer.    *
 *  Note also that the draw offer will be repeated every    *
 *  <draw_count> moves so setting this value too low can    *
 *  make the program behave "obnoxiously."                  *
 *                                                          *
 ************************************************************
 */
  if ((tc_increment > 200) || (tc_time_remaining[Flip(root_wtm)] >= 3000)) {
    if (Abs(Abs(value) - Abs(DrawScore(game_wtm))) < 2 &&
        moves_out_of_book > 3) {
      if (++draw_counter >= draw_count) {
        draw_counter = 0;
        result = 2;
      }
    } else
      draw_counter = 0;
  }
/*
 ************************************************************
 *                                                          *
 *  Now print the draw offer or resignation if appropriate  *
 *  but be sure and do it in a form that ICC/FICS will      *
 *  understand if the "xboard" flag is set.                 *
 *                                                          *
 *  Note that we also use the "speak" facility to verbally  *
 *  offer draws or resign if the "speech" variable has been *
 *  set to 1 by entering "speech on".                       *
 *                                                          *
 ************************************************************
 */
  if (result == 1) {
    learn_value = (crafty_is_white) ? -300 : 300;
    LearnBook();
    if (xboard)
      Print(4095, "resign\n");
    if (audible_alarm)
      printf("%c", audible_alarm);
    if (speech) {
      char announce[128];

      strcpy(announce, "./speak ");
      strcat(announce, "Resign");
      v = system(announce);
      if (v <= 0)
        perror("ResignOrDraw() system() error: ");
    }
    if (crafty_is_white) {
      Print(4095, "0-1 {White resigns}\n");
      strcpy(pgn_result, "0-1");
    } else {
      Print(4095, "1-0 {Black resigns}\n");
      strcpy(pgn_result, "1-0");
    }
  }
  if (offer_draws && result == 2) {
    draw_offered = 1;
    if (!xboard) {
      Print(1, "\nI offer a draw.\n\n");
      if (audible_alarm)
        printf("%c", audible_alarm);
      if (speech) {
        char announce[128];

        strcpy(announce, "./speak ");
        strcat(announce, "Drawoffer");
        v = system(announce);
        if (v <= 0)
          perror("ResignOrDraw() system() error: ");
      }
    } else if (xboard)
      Print(4095, "offer draw\n");
    else
      Print(4095, "\n*draw\n");
  } else
    draw_offered = 0;
}
