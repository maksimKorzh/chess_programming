/**************************************************
 *   Faile version 0.6                            *
 *   Author: Adrien Regimbald                     *
 *   E-mail: adrien@gpu.srv.ualberta.ca           *
 *   Web Page: http://www.ualberta.ca/~adrien/    *
 *                                                *
 *   File: Protos.h                               *
 *   Purpose: includes all prototypes             *
 **************************************************/

#ifndef PROTOS_H
#define PROTOS_H

int eval(void);
int try_move(move_s move[], int *num_moves, int from, int target);
int generate_move(move_s move[], int *n_moves);
int make_move(move_s move[], int a);
int unmake_move(move_s move[], int a);
Bool is_attacked(int location, int color);
int check_legal(move_s move[], int a);
int search(int alpha, int beta, int depth);
move_s search_root(int alpha, int beta, int depth);
void print_tree (int depth, int indent); /* call with print_tree (1, 0) */
void print_move (move_s move[], int a);
void print_pos(void);
int play_move(move_s move_to_play);
void print_history(int start_num);
void print_move2 (int from, int target, int promoted_to);
Bool parse_epd(void);
void rinput (char str[], int n);
void rdelay (int time_in_s);
void rclrscr (void);
void file_output(int move_num);
void get_user_input(int move_num, char input[]);
void refresh_screen(int move_num);
void init_game(void);
void parse_user_input(void);
void comp_to_coord (move_s comp_move, char new_move[]);
void order_moves(move_s move[], int num_moves);
Bool in_check(void);
void qsort_moves(move_s moves[], int values[], int start, int end);
int qsearch(int alpha, int beta, int depth);
move_s think(void);
Bool verify_board(void);
void xboard (void);
void init_piece_squares (void);
void init_eval (void);
double allocate_time(void);
Bool init_book (void);
move_s choose_book_move (void);
Bool is_rep_kludge (void);

#endif