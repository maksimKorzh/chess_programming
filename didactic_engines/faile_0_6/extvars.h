/**************************************************
 *   Faile version 0.6                            *
 *   Author: Adrien Regimbald                     *
 *   E-mail: adrien@gpu.srv.ualberta.ca           *
 *   Web Page: http://www.ualberta.ca/~adrien/    *
 *                                                *
 *   File: Extvars.h                              *
 *   Purpose: declare external variables          *
 **************************************************/

#ifdef lines
extern int line[20][2];
extern int line_depth;
extern FILE *lines_out;
#endif

extern int white_to_move;
extern Bool game_end;
extern int castle_flag;
extern long int nodes,qnodes;
extern double search_time;
extern Bool white_castled, black_castled;
extern int wking_loc, bking_loc;
extern int comp_color;
extern End_of_game result;
extern Bool new_game;
extern int move_num;
extern white_black history[];
extern int ply;
extern Bool captures;
extern Bool time_failure, time_exit;
extern time_t time0;
extern double time_for_move;
extern time_t search_start;
extern double qnodes_perc;
extern int pv_length[100];
extern move_s pv[100][100];
extern Bool searching_pv;
extern int history_h[144][144];
extern int iterative_depth;
extern clock_t clock0;
extern double clock_elapsed;
extern move_s pv_empty[100][100];
extern Bool xboard_mode;
extern int num_pieces;
extern int pieces[33];
extern int squares[144];
extern int current_score;
extern Bool post;
extern double time_left, increment;
extern long int moves_to_tc, minutes_per_game;
extern double time_cushion;
extern double opp_time;
extern char book[5000][81];
extern int num_book_lines;
extern int book_ply;
extern Bool use_book;
extern char opening_history[81];
extern int rep_kludge[500][144];
extern int rep_ply;
extern int fifty_move[500];
extern int fifty;
extern move_s junk_move;
extern move_s killer1[100];
extern move_s killer2[100];
extern int killer_scores[100];
extern Bool is_endgame;


extern FILE *stream;

extern int board[144];

extern int moved[144];

