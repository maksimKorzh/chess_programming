/* Fruit reloaded, a UCI chess playing engine derived from Fruit 2.1
 *
 * Copyright (C) 2012-2014 Daniel Mehrmann
 * Copyright (C) 2013-2014 Ryan Benitez
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

// thread.cpp

// includes

#ifdef _WIN32
#  include <windows.h>
#else
#  include <cstring>
#  include <signal.h>
#  include <pthread.h>
#endif

#include "board.h"
#include "move.h"
#include "mutex.h"
#include "option.h"
#include "search.h"
#include "search_full.h"
#include "smp.h"
#include "sort.h"
#include "thread.h"
#include "trans.h"
#include "util.h"
#include "value.h"

// types

enum stage_t {
   STAGE_START,
   STAGE_SLEEP,
   STAGE_IDLE,
   STAGE_BOOKED,
   STAGE_WORK,
   STAGE_SEARCH,
   STAGE_EXIT,
   STAGE_END,
};

struct thread_t {

   split_t * volatile split_used;
   split_t split[SplitMax];

   handle_t wait;

   volatile int split_nb;
   volatile int stage;
};

// variables

static int ThreadNb = 1;

static mutex_t ThreadLock[1];
static mutex_t SleepLock[1];

static thread_t Thread[ThreadMax];

// prototypes

static void          mutex_init        ();
static void          mutex_close       ();

static void          thread_create     ();
static void          thread_exit       ();

#ifdef _WIN32
static DWORD WINAPI  thread_start      (LPVOID id);
#else
static void *        thread_start      (void * id);
#endif

static void          thread_loop       (int id, split_t * split);
static void          thread_booking    (int master, split_t * split);

static void          thread_update     (const split_t * split, int old_alpha);

// functions

void smp_init(void) {

   mutex_init();
   thread_create();
}

void smp_close(void) {

   thread_exit();
   mutex_close();
}

// smp_wakeup()

void smp_wakeup(void) {

   int id;

   ThreadNb = option_get_int("Threads");

   for (id = 1; id < ThreadNb; id++) { // HACK: no main thread

      ASSERT(Thread[id].stage==STAGE_SLEEP);

      Thread[id].split_nb = 0;
      Thread[id].split_used = NULL;

#ifdef _WIN32
      SetEvent(Thread[id].wait);
#else
      pthread_cond_broadcast(&Thread[id].wait);
#endif

      while(Thread[id].stage==STAGE_SLEEP);
   }

   // main thread(to help debugging)

   Thread[ThreadMain].stage = STAGE_SEARCH;
   Thread[ThreadMain].split_nb = 0;
   Thread[ThreadMain].split_used = NULL;
}

// smp_sleep()

void smp_sleep(void) {

   int id;

   for (id = 1; id < ThreadNb; id++) { // HACK: no main thread

      ASSERT(Thread[id].stage==STAGE_IDLE);

      Thread[id].stage = STAGE_START;
      while(Thread[id].stage==STAGE_START);
   }
}

// thread_number()

int thread_number(void) { // debug only

   return ThreadNb;
}

// thread_is_ok()

bool thread_is_ok(int id) {

   return id >= ThreadMain && id < ThreadMax;
}

// thread_is_free()

bool thread_is_free(int master) {

   int id;
   int split_nb;

   ASSERT(thread_is_ok(master));

   // free threads

   for (id = 0; id < ThreadNb; id++) {

      if (Thread[id].stage != STAGE_IDLE) continue;

      split_nb = Thread[id].split_nb;
      if (split_nb == 0) return true;

      ASSERT(split_nb>0&&split_nb<SplitMax); // HACK: Might happen, but it's no bug

      if (Thread[id].split[split_nb-1].slave[master]) return true;
   }

   return false;
}

// thread_stop()

bool thread_is_stop(int id) {

   split_t * split;

   ASSERT(thread_is_ok(id));

   for (split = Thread[id].split_used; split != NULL; split = split->split_nb) {
      if (split->cut) return true;
   }

   return false;
}

// split()

bool do_split(const board_t * board, int * alpha, int old_alpha, int beta, int depth, int height, mv_t pv[], int node_type,
   int * best_value, int * played_nb, int * best_move, sort_t * sort,
   mv_t played[],int eval_value, bool in_check, int master) {

   int id;
   split_t * split_ptr;
   thread_t * thread_ptr;

   MUTEX_LOCK(ThreadLock);

   ASSERT(board!=NULL);
   ASSERT(alpha!=NULL);
   ASSERT(value_is_ok(old_alpha));
   ASSERT(depth_is_ok(depth));
   ASSERT(height_is_ok(height));
   ASSERT(pv!=NULL);
   ASSERT(node_type>=-1&&node_type<=1);
   ASSERT(best_value!=NULL);
   ASSERT(played_nb!=NULL);
   ASSERT(best_move!=NULL);
   ASSERT(sort!=NULL);
   ASSERT(played!=NULL);
   ASSERT(in_check==true||in_check==false);
   ASSERT(thread_is_ok(master));

   ASSERT(in_check||value_is_ok(eval_value));
   ASSERT(Thread[master].stage==STAGE_SEARCH);

   ASSERT(board_is_ok(board));
   ASSERT(board_is_legal(board));
   ASSERT(range_is_ok(*alpha,beta));
   ASSERT(value_is_ok(*best_value));
   ASSERT(*played_nb>0);
   ASSERT(*best_move==MoveNone||move_is_ok(*best_move));
   ASSERT(sort->pos>0);
   ASSERT(played[*played_nb-1]==LIST_MOVE(sort->list,sort->pos-1));

   if (Thread[master].split_nb >= SplitMax || !thread_is_free(master)) {
      MUTEX_FREE(ThreadLock);
      return false;
   }

   // init

   thread_ptr = &Thread[master];
   split_ptr = &thread_ptr->split[thread_ptr->split_nb++];

   for (id = 0; id < SlaveMax; id++) split_ptr->slave[id] = false;

   // booking

   thread_booking(master,split_ptr);

   MUTEX_FREE(ThreadLock);

   // copy

   split_ptr->split_nb = thread_ptr->split_used; // previous split
   split_ptr->master = master;

   split_ptr->end = false;
   split_ptr->cut = false;

   split_ptr->alpha = *alpha;
   split_ptr->beta = beta;
   split_ptr->depth = depth;
   split_ptr->height = height;
   split_ptr->node_type = node_type;

   split_ptr->played_nb = *played_nb;
   split_ptr->best_value = *best_value;
   split_ptr->best_move = *best_move;

   split_ptr->eval_value = eval_value;
   split_ptr->in_check = in_check;
   split_ptr->played = played;
   split_ptr->pv = pv;

   split_ptr->board = board;
   split_ptr->sort = sort;

   // master

   thread_ptr->split_used = split_ptr; // current split

   // slaves

   for (id = 0; id < ThreadNb; id++) {
      if (split_ptr->slave[id]) {

         ASSERT(id!=master);
         ASSERT(Thread[id].stage==STAGE_BOOKED);

         Thread[id].split_used = split_ptr;
         killer_copy(id,master,height+1);
      }
   }

   // start

   for (id = 0; id < ThreadNb; id++) {

      if (id == master || split_ptr->slave[id]) {
        ASSERT(Thread[id].stage==STAGE_BOOKED);
        Thread[id].stage = STAGE_WORK;
      }
   }

   // master search

   thread_loop(master,split_ptr);

   // update

   *best_value = split_ptr->best_value;

   thread_update(split_ptr,old_alpha);

   // master

   MUTEX_LOCK(ThreadLock);

   ASSERT(thread_ptr->stage==STAGE_SEARCH);

   thread_ptr->split_nb--;
   thread_ptr->split_used = split_ptr->split_nb;

   ASSERT(thread_ptr->split_nb>=0);

   MUTEX_FREE(ThreadLock);

   return true;
}

// mutex_init()

static void mutex_init(void) {

   int i, j;

   // global

   MUTEX_INIT(ThreadLock,NULL);
   MUTEX_INIT(SleepLock,NULL);

   // split

   for (i = 0; i < ThreadMax; i++) {
      for (j = 0; j < SplitMax; j++) {
         MUTEX_INIT(Thread[i].split[j].lock,NULL);
      }
   }
}

// mutex_close()

static void mutex_close(void) {

   int i, j;

   // global

   MUTEX_CLOSE(ThreadLock);
   MUTEX_CLOSE(SleepLock);

   // split

   for (i = 0; i < ThreadMax; i++) {
      for (j = 0; j < SplitMax; j++) {
         MUTEX_CLOSE(Thread[i].split[j].lock);
      }
   }
}

// thread_create()

static void thread_create(void) {

   volatile int i;
   bool thread_is_ok;

#ifndef _WIN32
   pthread_t pthread[1];
#endif

   // init

   for (i = 0; i < ThreadMax; i++) { // with main thread

      Thread[i].stage = STAGE_START;
      Thread[i].split_used = NULL;
      Thread[i].split_nb = 0;

#ifdef _WIN32
      Thread[i].wait = CreateEvent(0,false,false,0);
#else
      pthread_cond_init(&Thread[i].wait,NULL);
#endif
   }

   // create

   for (i = 1; i < ThreadMax; i++) { // no main thread

#ifdef _WIN32
      thread_is_ok = (CreateThread(NULL,0,thread_start,(LPVOID)(&i),0,NULL) != NULL);
#else
      thread_is_ok = (pthread_create(pthread,NULL,thread_start,(void*)(&i)) == 0);
#endif

      if (!thread_is_ok) my_fatal("thread_create(): can't create thread %d\n",i);
      while(Thread[i].stage==STAGE_START);
   }
}

// thread_exit()

static void thread_exit(void) {

   int id;

   for (id = 1; id < ThreadMax; id++) { // no main thread

      ASSERT(Thread[id].stage==STAGE_SLEEP);

      Thread[id].stage = STAGE_EXIT;

#ifdef _WIN32
      SetEvent(Thread[id].wait);
#else
      pthread_cond_broadcast(&Thread[id].wait);
#endif

      while(Thread[id].stage == STAGE_EXIT);
   }
}

// thread_start()

#ifdef _WIN32
   static DWORD WINAPI thread_start(LPVOID id) {
#else
   static void *       thread_start(void * id) {
#endif

   thread_loop(*(int*)id,NULL);

   return NULL;
}

// thread_loop()

static void thread_loop(int id, split_t * split) {

   split_t * sp;
   board_t board[1];

   ASSERT(thread_is_ok(id));

   // loop

   while (true) {

      // exit

      if (Thread[id].stage == STAGE_EXIT) break;

      // sleep

      if (Thread[id].stage == STAGE_START || id > ThreadNb) {

         ASSERT(id>ThreadMain);

         Thread[id].stage = STAGE_SLEEP;

#ifdef _WIN32
         WaitForSingleObject(Thread[id].wait, INFINITE);
#else
         MUTEX_LOCK(SleepLock);
         pthread_cond_wait(&Thread[id].wait,SleepLock);
         MUTEX_FREE(SleepLock);
#endif
         Thread[id].stage = STAGE_IDLE;
      }

      ASSERT(id<=ThreadNb||Thread[id].stage==STAGE_EXIT);

      // work

      if (Thread[id].stage == STAGE_WORK) {

         sp = Thread[id].split_used;
         memcpy(board,sp->board,sizeof(board_t));

         // search

         Thread[id].stage = STAGE_SEARCH;
         full_thread(board,sp->alpha,sp->beta,sp->depth,sp->height,sp->pv,sp->node_type,id,sp);

         Thread[id].stage = STAGE_IDLE;
      }

      // master

      if (split != NULL && split->thread_nb == 0) {

            ASSERT(Thread[id].stage==STAGE_IDLE);
            Thread[id].stage = STAGE_SEARCH;

            return;
      }
   }

   Thread[id].stage = STAGE_END;
}

static void thread_booking(int master, split_t * split) {

   int id;
   int thread_nb;

   ASSERT(thread_is_ok(master));
   ASSERT(split!=NULL);

   ASSERT(Thread[master].stage==STAGE_SEARCH);

   // init

   thread_nb = 1; // master
   Thread[master].stage = STAGE_BOOKED;

   // booking

   for (id = 0; id < ThreadNb && thread_nb <= SlaveMax; id++) {

      ASSERT(split->slave[id]==false);

      if (Thread[id].stage != STAGE_IDLE) continue;

      if (Thread[id].split_nb > 0 &&
         !Thread[id].split[Thread[id].split_nb-1].slave[master]) continue;

      Thread[id].stage = STAGE_BOOKED;
      split->slave[id] = true;
      thread_nb++;
   }

   ASSERT(thread_nb>1);
   split->thread_nb = thread_nb;
}

// thread_update()

static void thread_update(const split_t * split, int old_alpha) {

   int i;
   int move, best_move;
   int best_value;
   int played_nb;
   int depth;
   const board_t * board;
   int trans_min_value, trans_max_value;

   ASSERT(split!=NULL);
   ASSERT(value_is_ok(old_alpha));

   // init

   best_move = split->best_move;
   best_value = split->best_value;

   played_nb = split->played_nb;
   depth = split->depth;
   board = split->board;

   ASSERT(best_move==MoveNone||move_is_ok(best_move));
   ASSERT(value_is_ok(best_value));
   ASSERT(played_nb>0);
   ASSERT(depth_is_ok(depth));

   // move ordering

   if (best_move != MoveNone) {

      good_move(best_move,board,depth,split->height,split->master,best_value >=split->beta);

      if (best_value >= split->beta && !MOVE_IS_TACTICAL(best_move,board)) {

         ASSERT(played_nb>0&&split->played[played_nb-1]==best_move);

         for (i = 0; i < played_nb-1; i++) {
            move = split->played[i];
            ASSERT(move!=best_move);

            bad_move(move,board,depth);
         }
      }
   }

   // transposition table

   if (UseTrans && depth >= TransDepth) {

      trans_min_value = (best_value > old_alpha)   ? value_to_trans(best_value,split->height) : -ValueInf;
      trans_max_value = (best_value < split->beta) ? value_to_trans(best_value,split->height) : +ValueInf;

      trans_store(board->key,best_move,depth,trans_min_value,trans_max_value,split->eval_value);
   }
}

// end of thread.cpp

