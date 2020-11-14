/**************************************************
 *   Faile version 0.6                            *
 *   Author: Adrien Regimbald                     *
 *   E-mail: adrien@gpu.srv.ualberta.ca           *
 *   Web Page: http://www.ualberta.ca/~adrien/    *
 *                                                *
 *   File: book.c                                 *
 *   Purpose: Book initialization, selection of   *
 *   book moves, etc...                           *
 **************************************************/

#include "faile.h"
#include "protos.h"
#include "extvars.h"

Bool init_book (void) {

   /* simply read all the book moves into a book array.  The book will be
      a simple char[5000][81] array (5000 max book lines, since you can't
      have *too* many when you're doing this slow strncpy method ;)  After
      all, this is really just a kludge 'till I add hashing, and support
      transpositions and such ;) Returns true if reading in the book
      was successful. */

   FILE *f_book;
   int ch, i = 0, j = 0;


   /* init our random numbers: */
   srand((unsigned) time (NULL));

   if ((f_book = fopen ("faile.opn", "r")) == NULL)
      return FALSE;

   while ((ch = getc(f_book)) != EOF) {
      if (ch == '\n') { /* end of book line */
         book[i++][j] = '\0';
         j = 0;
      }
      else if (j < 80 && i < 5000) /* middle of book line */
         book[i][j++] = ch;
      /* If j >= 80, then the book line was too long.  The rest will be
         read, but ignored, and will be null-character terminated when a
         '\n' is read.  If i >= 5000, then there are too many book lines.
         The remaining lines will be read, but ignored. */

   }

   num_book_lines = i;

   fclose(f_book);

   return TRUE;

}

move_s choose_book_move (void) {

   /* Since the book is sorted alphabetically, we can use strncpy, and hope
      to get early exits once we've found the first few moves in the book.
      Once we choose a book move, we'll make a variable indicate where
      it was found, so we can start our search for the next move there. */

   static int last_book_move = 0;
   Bool book_match = FALSE;
   int i, j, num_moves, random_number, num_replies;
   char possible_move[5], coord_move[5];
   move_s book_replies[5000], moves[200];
   /* make move_s structs for our opening moves as white (the other
      opening book first moves are really only meant for use to reply as
      black to that opening, or are a bit too risky ;) */
   move_s e2e4 = {42, 66, npiece, 0, 0, 0, 0};
   move_s d2d4 = {41, 65, npiece, 0, 0, 0, 0};
   move_s c2c4 = {40, 64, npiece, 0, 0, 0, 0};
   move_s g1f3 = {32, 55, npiece, 0, 0, 0, 0};

   if (!book_ply)
      last_book_move = 0;
   num_moves = 0;
   num_replies = 0;
   generate_move(&moves[0], &num_moves);

   /* if we're playing white (book_ply == 0), pick one of our first moves */

   if (!book_ply) {
      /* pick a random number from 0 to 3: */
      random_number = rand() % 4;
      if (!random_number)
         return e2e4;
      else if (random_number == 1)
         return d2d4;
      else if (random_number == 2)
         return c2c4;
      else
         return g1f3;
   }

   for (i = last_book_move; i < num_book_lines; i++) {
      /* check to see if opening line matches up to current book_ply: */
      if (!strncmp(opening_history, book[i], (book_ply * 4))) {
         /* if book move is legal, add it to possible list of book moves */
         strncpy(possible_move, book[i] + (book_ply * 4), 4);
         possible_move[4] = '\0';

         for (j = 0; j < num_moves; j++) {
            comp_to_coord(moves[j], coord_move);
            if (!strcmp(possible_move, coord_move)) {
               make_move(&moves[0], j);
               if (check_legal(&moves[0], j)) {
                  book_replies[num_replies++] = moves[j];
                  book_match = TRUE;
               }
               unmake_move(&moves[0], j);
            }
         }
      }
      /* we can exit the search for a book move early, if we've no longer
         have a match, but we have found at least one match */
      if (!book_match && num_replies)
         break;
   }

   /* now, if we have some book replies, pick our move randomly from
      book_replies: */
   if (!num_replies)
      return junk_move;

   random_number = rand() % num_replies;
   return book_replies[random_number];

}



