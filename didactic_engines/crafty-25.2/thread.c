#include "chess.h"
#include "data.h"
#include "epdglue.h"
/* modified 08/03/16 */
/*
 *******************************************************************************
 *                                                                             *
 *   Split() is the driver for the threaded parallel search in Crafty.  The    *
 *   basic idea is that whenever we notice that one (or more) threads are in   *
 *   their idle loop, we drop into Split(), from Search(), and begin a new     *
 *   parallel search from this node.  This is simply a problem of establishing *
 *   a new split point, and then letting each thread join this split point and *
 *   copy whatever data they need.                                             *
 *                                                                             *
 *   This is generation II of Split().  The primary differences address two    *
 *   specific performance-robbing issues.  (1) Excessive waiting for a split   *
 *   to be done, and (b) excessive waiting on specific locks.  Generation II   *
 *   addresses both of these to significantly improve performance.             *
 *                                                                             *
 *   The main difference between Gen I and Gen II is the effort required to    *
 *   split the tree and which thread(s) expend this effort.  In generation I,  *
 *   the parent thread was responsible for allocating a split block for each   *
 *   child thread, and then copying the necessary data from the parent split   *
 *   block to these child split blocks.  When all of this was completed, the   *
 *   child processes were released to start the parallel search after being    *
 *   held while the split / copy operations were done.  In the generation II   *
 *   Split() we now simply allocate a new split block for THIS thread, flag    *
 *   the parent split block as joinable, and then go directly to ThreadWait()  *
 *   which will drop us back in to the search immediately.  The idle threads   *
 *   are continually looping on Join() which will jump them right into this    *
 *   split block letting them do ALL the work of allocating a split block,     *
 *   filling it in, and then copying the data to their local split block.      *
 *   This distributes the split overhead among all the threads that split,     *
 *   rather than this thread having to do all the work while the other threads *
 *   sit idle.                                                                 *
 *                                                                             *
 *   Generation II is also much more lightweight, in that it copies only the   *
 *   bare minimum from parent to child.  Generation I safely copied far too    *
 *   much since this code was being changed regularly, but that is no longer   *
 *   necessary overhead.                                                       *
 *                                                                             *
 *   Generation II has a zero contention split algorithm.  In the past, when a *
 *   thread became idle, it posted a global split request and any thread that  *
 *   was at an appropriate point would try to split.  But while it was doing   *
 *   the splitting, other threads that were also willing to split would "get   *
 *   in line" because Crafty used a global lock to prevent two threads from    *
 *   attempting to split at the same instant in time.  They got in line, and   *
 *   waited for the original splitter to release the lock, but now they have   *
 *   no idle threads to split with.  A waste of time.  Now we allow ANY thread *
 *   to attempt to split at the current ply.  When we do what might be called  *
 *   a "gratuitous split" the only restriction is that if we have existing     *
 *   "gratuitous split points" (split blocks that are joinable but have not    *
 *   yet been joined), then we limit the number of such splits (per thread) to *
 *   avoid excessive overhead.                                                 *
 *                                                                             *
 *   Generation II takes another idea from DTS, the idea of "late-join".  The  *
 *   idea is fairly simple.  If, when a thread becomes idle, there are already *
 *   other split points being searched in parallel, then we will try to join   *
 *   one of them rather than waiting for someone to ask us to help.  We use    *
 *   some simple criteria:  (1) The split point must be joinable, which simply *
 *   means that no processor has exited the split point yet (which would let   *
 *   us know there is no more work here and a join would be futile);  (2) We   *
 *   compute an "interest" value which is a simple formula based on depth at   *
 *   the split point, and the number of moves already searched.  It seems less *
 *   risky to choose a split point with max depth AND minimum moves already    *
 *   searched so that there is plenty to do.  This was quite simple to add     *
 *   after the rest of the Generation II rewrite.  In fact, this is now THE    *
 *   way threads join a split point, period, which further simplifies the code *
 *   and improves efficiency.  IE, a thread can split when idle threads are    *
 *   noticed, or if it is far enough away from the tips to make the cost       *
 *   negligible.  At that point any idle thread(s) can join immediately, those *
 *   that become idle later can join when they are ready.                      *
 *                                                                             *
 *   There are a number of settable options via the command-line or .craftyrc  *
 *   initialization file.  Here's a concise explanation for each option and an *
 *   occasional suggestion for testing/tuning.                                 *
 *                                                                             *
 *   smp_affinity (command = smpaffinity=<n> <p> is used to enable or disable  *
 *      processor affinity.  -1 disables affinity and lets threads run on any  *
 *      available core.  If you use an integer <n> then thread zero will bind  *
 *      itself to cpu <n> and each additional thread will bind to the next     *
 *      higher cpu number.  This is useful if you try to run two copies of     *
 *      crafty on the same machine, now you can cause one to bind to the first *
 *      <n> cores, and the second to the last <n> cores.  For the first        *
 *      instance of Crafty, you would use smpaffinity=0, and for the second    *
 *      smpaffinity=8, assuming you are running 8 threads per copy on a 16 cpu *
 *      machine.  If you get this wrong, you can have more than one thread on  *
 *      the same cpu which will significantly impact performance.              *
 *                                                                             *
 *   smp_max_threads (command = smpmt=n) sets the total number of allowable    *
 *      threads for the search.  The default is one (1) as Crafty does not     *
 *      assume it should use all available resources.  For optimal performance *
 *      this should be set to the number of physical cores your machine has,   *
 *      which does NOT include hyperthreaded cores.                            *
 *                                                                             *
 *   smp_split_group (command = smpgroup=n) sets the maximum number of threads *
 *      at any single split point, with the exception of split points fairly   *
 *      close to the root where ALL threads are allowed to split together,     *
 *      ignoring this limit.  Note that this is ignored in the first 1/2 of    *
 *      the tree (the nodes closer to the root).  There it is actually good to *
 *      split and get all active threads involved.                             *
 *                                                                             *
 *   smp_min_split_depth (command = smpmin=n) avoids splitting when remaining  *
 *      depth < n.  This is used to balance splitting overhead cost against    *
 *      the speed gain the parallel search produces.  The default is currently *
 *      5 (which could change with future generations of Intel hardware) but   *
 *      values between 4 and 8 will work.  Larger values allow somewhat fewer  *
 *      splits, which reduces overhead, but it also increases the percentage   *
 *      of the time where a thread is waiting on work.                         *
 *                                                                             *
 *   smp_split_at_root (command = smproot=0 or 1) enables (1) or disables (0)  *
 *      splitting the tree at the root.  This defaults to 1 which produces the *
 *      best performance by a signficiant margin.  But it can be disabled if   *
 *      you are playing with code changes.                                     *
 *                                                                             *
 *   smp_gratuitous_depth (command = smpgd=<n>) controls " gratuitous splits"  *
 *      which are splits that are done without any idle threads.  This sets a  *
 *      depth limit (remaining depth) that must be present before such a split *
 *      can be done.  Making this number larger will reduce the number of      *
 *      these splits.  Making it too small will increase overhead slightly and *
 *      increase split block usage significantly.                              *
 *                                                                             *
 *   smp_gratuitous_limit (command = smpgl=<n>) limits the number of these     *
 *      splits that a thread can do.  Once a thread has this number of         *
 *      unjoined split points, it will not be allowed to split any more until  *
 *      one or more threads join at least one of the existing split points.    *
 *      In the smp search statistics, where you see output that looks like     *
 *      this:                                                                  *
 *                                                                             *
 *        splits=m(n) ...                                                      *
 *                                                                             *
 *      m is the total splits done, n is the number of "wasted splits" which   *
 *      are basically gratuitous splits where no thread joined before this     *
 *      split point was completed and deallocated.                             *
 *                                                                             *
 *   The best way to tune all of these paramenters is to use the "autotune"    *
 *   command (see autotune.c and help autotune) which will automatically run   *
 *   tests and optimize the parameters.  More details are in the autotune.c    *
 *   source file.                                                              *
 *                                                                             *
 *   A few basic "rules of the road" for anyone interested in changing or      *
 *   adding to any of this code.                                               *
 *                                                                             *
 *   1.  If, at any time, you want to modify your private split block, no lock *
 *       is required.                                                          *
 *                                                                             *
 *   2.  If, at any time, you want to modify another split block, such as the  *
 *       parent split block shared move list, you must acquire the lock in the *
 *       split block first.  IE tree->parent->lock to lock the parent split    *
 *       block during NextMove() and such.                                     *
 *                                                                             *
 *   3.  If you want to modify any SMP-related data that spans multiple split  *
 *       blocks, such as telling sibling processes to stop, etc, then you must *
 *       acquire the global "lock_smp" lock first.  This prevents a deadlock   *
 *       caused by two different threads walking the split block chain from    *
 *       different directions, and acquiring the split block locks in          *
 *       different orders, which could cause a catastrophic deadlock to occur. *
 *       This is an infrequent event so the overhead is not significant.       *
 *                                                                             *
 *   4.  If you want to do any sort of I/O operation, you must acquire the     *
 *       "lock_io" lock first.  Since threads share descriptors, there are     *
 *       lots of potential race conditions, from the simple tty-output getting *
 *       interlaced from different threads, to outright data corruption in the *
 *       book or log files.                                                    *
 *                                                                             *
 *   Some of the bugs caused by failing to acquire the correct lock will only  *
 *   occur infrequently, and they are extremely difficult to find.  Some only  *
 *   show up in a public game where everyone is watching, to cause maximum     *
 *   embarassment and causes the program to do something extremely stupid.     *
 *                                                                             *
 *******************************************************************************
 */
int Split(TREE * RESTRICT tree) {
  TREE *child;
  int tid, tstart, tend;

/*
 ************************************************************
 *                                                          *
 *  Here we prepare to split the tree.  All we really do in *
 *  the Generation II threading is grab a split block for   *
 *  this thread, then flag the parent as "joinable" and     *
 *  then jump right to ThreadWait() to resume where we left *
 *  off, with the expectation (but not a requirement) that  *
 *  other threads will join us to help.                     *
 *                                                          *
 *  Idle threads are sitting in ThreadWait() repeatedly     *
 *  calling Join() to find them a split point, which we are *
 *  fixing to provide.  They will then join as quickly as   *
 *  they can, and other threads that become idle later can  *
 *  also join without any further splitting needed.         *
 *                                                          *
 *  If we are unable to allocate a split block, we simply   *
 *  abort this attempted split and return to the search     *
 *  since other threads will also split quickly.            *
 *                                                          *
 ************************************************************
 */
  tstart = ReadClock();
  tree->nprocs = 0;
  for (tid = 0; tid < smp_max_threads; tid++)
    tree->siblings[tid] = 0;
  child = GetBlock(tree, tree->thread_id);
  if (!child)
    return 0;
  CopyFromParent(child);
  thread[tree->thread_id].tree = child;
  tree->joined = 0;
  tree->joinable = 1;
  parallel_splits++;
  smp_split = 0;
  tend = ReadClock();
  thread[tree->thread_id].idle += tend - tstart;
/*
 ************************************************************
 *                                                          *
 *  We have successfully created a split point, which means *
 *  we are done.  The instant we set the "joinable" flag,   *
 *  idle threads may begin to join in at this split point   *
 *  to help.  Since this thread may finish before any or    *
 *  all of the other parallel threads, this thread is sent  *
 *  to ThreadWait() which will immediately send it to       *
 *  SearchMoveList() like the other threads; however, going *
 *  to ThreadWait() allows this thread to join others if it *
 *  runs out of work to do.  We do pass ThreadWait() the    *
 *  address of the parent split block, so that if this      *
 *  thread becomes idle, and this thread block shows no     *
 *  threads are still busy, then this thread can return to  *
 *  here and then back up into the previous ply as it       *
 *  should.  Note that no other thread can back up to the   *
 *  previous ply since their recursive call stacks are not  *
 *  set for that, while this call stack will bring us back  *
 *  to this point where we return to the normal search,     *
 *  which we just completed.                                *
 *                                                          *
 ************************************************************
 */
  ThreadWait(tree->thread_id, tree);
  if (!tree->joined)
    parallel_splits_wasted++;
  return 1;
}

/* modified 08/03/16 */
/*
 *******************************************************************************
 *                                                                             *
 *   Join() is called just when we enter the usual spin-loop waiting for work. *
 *   We take a quick look at all active split blocks to see if any look        *
 *   "joinable".  If so, we compute an "interest" value, which will be defined *
 *   below.  We then join the most interesting split point directly. This      *
 *   split point might have been created specifically for this thread to join, *
 *   or it might be one that was already active when this thread became idle,  *
 *   which allows us to join that existing split point and not request a new   *
 *   split operation, saving time.                                             *
 *                                                                             *
 *******************************************************************************
 */
int Join(int64_t tid) {
  TREE *tree, *join_block, *child;
  int interest, best_interest, current, pass = 0;

/*
 ************************************************************
 *                                                          *
 *  First we pass over ALL split blocks, looking for those  *
 *  flagged as "joinable" (which means they are actually    *
 *  active split points and that no processor at that split *
 *  point has run out of work (there is no point in joining *
 *  a split point with no remaining work) and no fail high  *
 *  has been found which would raise the "stop" flag.) This *
 *  is "racy" because we do not acquire any locks, which    *
 *  means that the busy threads continue working, and there *
 *  is a small probability that the split point will        *
 *  disappear while we are in this loop.  To resolve the    *
 *  potential race, after we find the most attractive split *
 *  point, we acquire the lock for that split block and     *
 *  test again, but this time if the block is joinable, we  *
 *  can safely join under control of the lock, which is not *
 *  held for very long at all.  If the block is not         *
 *  joinable once we acquire the lock, we abort joining     *
 *  since it is futile.  Note that if this happens, we will *
 *  try to find an existing split point we can join three   *
 *  times before we exit, setting split to 1 to ask other   *
 *  threads to produce more candidate split points.         *
 *                                                          *
 *  Special case:  We don't want to join a split point that *
 *  was created by this thread.  While it works, it can add *
 *  overhead since we can encounter a later split point     *
 *  that originated at the current split point, and we      *
 *  would continue searching even though most of the work   *
 *  has already been completed.  The hash table would help  *
 *  avoid most (if not all) of this overhead, but there is  *
 *  no good reason to take the chance of this happening.    *
 *                                                          *
 ************************************************************
 */
  for (pass = 0; pass < 3; pass++) {
    best_interest = -999999;
    join_block = 0;
    for (current = 0; current <= smp_max_threads * 64; current++) {
      tree = block[current];
      if (tree->joinable && (tree->ply <= tree->depth / 2 ||
              tree->nprocs < smp_split_group) && tree->thread_id != tid) {
        interest = tree->depth * 2 - tree->searched[0];
        if (interest > best_interest) {
          best_interest = interest;
          join_block = tree;
        }
      }
    }
/*
 ************************************************************
 *                                                          *
 *  Now we acquire the lock for this split block, and then  *
 *  check to see if the block is still flagged as joinable. *
 *  If so, we set things up, and then we get pretty tricky  *
 *  as we then release the lock, and then copy the data     *
 *  from the parent to our split block.  There is a chance  *
 *  that while we are copying this data, the split point    *
 *  gets completed by other threads.  Which would leave an  *
 *  apparent race condition exposed where we start copying  *
 *  data here, the split point is completed, the parent     *
 *  block is released and then reacquired and we continue   *
 *  if nothing has happened here, getting data copied from  *
 *  two different positions.                                *
 *                                                          *
 *  Fortunately, we linked this new split block to the old  *
 *  (original parent).  If that split block is released, we *
 *  will discover this because that operation will also set *
 *  our "stop" flag which will prevent us from using this   *
 *  data and breaking things.  We allow threads to copy     *
 *  this data without any lock protection to eliminate a    *
 *  serialization (each node would copy the data serially,  *
 *  rather than all at once) with the only consequence to   *
 *  this being the overhead of copying and then throwing    *
 *  the data away, which can happen on occasion even if we  *
 *  used a lock for protection, since once we release the   *
 *  lock it still takes time to get into the search and we  *
 *  could STILL find that this split block has already been *
 *  completed, once again.  Less contention and serial      *
 *  computing improves performance.                         *
 *                                                          *
 ************************************************************
 */
    if (join_block) {
      Lock(join_block->lock);
      if (join_block->joinable) {
        child = GetBlock(join_block, tid);
        Unlock(join_block->lock);
        if (child) {
          CopyFromParent(child);
          thread[tid].tree = child;
          parallel_joins++;
          return 1;
        }
      } else {
        Unlock(join_block->lock);
        break;
      }
    }
  }
/*
 ************************************************************
 *                                                          *
 *  We did not acquire a split point to join, so we set     *
 *  smp_split to 1 to ask busy threads to create joinable   *
 *  split points.                                           *
 *                                                          *
 ************************************************************
 */
  smp_split = 1;
  return 0;
}

/* modified 08/03/16 */
/*
 *******************************************************************************
 *                                                                             *
 *   ThreadAffinity() is called to "pin" a thread to a specific processor.  It *
 *   is a "noop" (no-operation) if Crafty was not compiled with -DAFFINITY, or *
 *   if smp_affinity is negative (smpaffinity=-1 disables affinity).  It       *
 *   simply sets the affinity for the current thread to the requested CPU and  *
 *   returns.  NOTE:  If hyperthreading is enabled, there is no guarantee that *
 *   this will work as expected and pin one thread per physical core.  It      *
 *   depends on how the O/S numbers the SMT cores.                             *
 *                                                                             *
 *******************************************************************************
 */
void ThreadAffinity(int cpu) {
#if defined(AFFINITY)
  cpu_set_t cpuset;
  pthread_t current_thread = pthread_self();

  if (smp_affinity >= 0) {
    CPU_ZERO(&cpuset);
    CPU_SET(smp_affinity_increment * (cpu + smp_affinity), &cpuset);
    pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
  }
#endif
}

/* modified 08/03/16 */
/*
 *******************************************************************************
 *                                                                             *
 *   ThreadInit() is called after a process is created.  Its main task is to   *
 *   initialize the process local memory so that it will fault in and be       *
 *   allocated on the local node rather than the node where the original       *
 *   (first) process was running.  All threads will hang here via a custom     *
 *   WaitForALlThreadsInitialized() procedure so that all the local thread     *
 *   blocks are usable before the search actually begins.                      *
 *                                                                             *
 *******************************************************************************
 */
void *STDCALL ThreadInit(void *t) {
  int tid = (int64_t) t;

  ThreadAffinity(tid);
#if !defined(UNIX)
  ThreadMalloc((uint64_t) tid);
#endif
  thread[tid].blocks = 0xffffffffffffffffull;
  Lock(lock_smp);
  initialized_threads++;
  Unlock(lock_smp);
  WaitForAllThreadsInitialized();
  ThreadWait(tid, (TREE *) 0);
  Lock(lock_smp);
  smp_threads--;
  Unlock(lock_smp);
  return 0;
}

/* modified 08/03/16 */
/*
 *******************************************************************************
 *                                                                             *
 *   ThreadSplit() is used to determine if we should split at the current ply. *
 *   There are some basic constraints on when splits can be done, such as the  *
 *   depth remaining in the search (don't split to near the tips), and have we *
 *   searched at least one move to get a score or bound (YBW condition).       *
 *                                                                             *
 *   If those conditions are satisfied, AND either a thread has requested a    *
 *   split OR we are far enough away from the tips of the tree to justify a    *
 *   "gratuitout split" then we return "success."  A "gratuitout split" is a   *
 *   split done without any idle threads.  Since splits are not free, we only  *
 *   do this well away from tips to limit overhead.  We do this so that when a *
 *   thread becomes idle, it will find these split points immediately and not  *
 *   have to wait for a split after the fact.                                  *
 *                                                                             *
 *******************************************************************************
 */
int ThreadSplit(TREE * tree, int ply, int depth, int alpha, int o_alpha,
    int done) {
  TREE *used;
  int64_t tblocks;
  int temp, unused = 0;

/*
 ************************************************************
 *                                                          *
 *  First, we see if we meet the basic criteria to create a *
 *  split point, that being that we must not be too far     *
 *  from the root (smp_min_split_depth).                    *
 *                                                          *
 ************************************************************
 */
  if (depth < smp_min_split_depth)
    return 0;
/*
 ************************************************************
 *                                                          *
 *  If smp_split is NOT set, we are checking to see if it   *
 *  is acceptable to do a gratuitous split here.            *
 *                                                          *
 *  (1) if we are too far from the root we do not do        *
 *      gratuitous splits to avoid the overhead.            *
 *                                                          *
 *  (2) if we have searched more than one move at this ply, *
 *      we don't do any further tests to see if a           *
 *      gratuitous split is acceptable, since we have       *
 *      previously done this test at this ply and decided   *
 *      one should not be done.  That condition has likely  *
 *      not changed.                                        *
 *                                                          *
 *  (3) if we have pre-existing gratuitous split points for *
 *      this thread, we make certain we don't create more   *
 *      than the gratuitous split limit as excessive splits *
 *      just add to the overhead with no benefit.           *
 *                                                          *
 ************************************************************
 */
  if (!smp_split) {
    if (depth < smp_gratuitous_depth || done > 1)
      return 0;
    tblocks = ~thread[tree->thread_id].blocks;
    while (tblocks) {
      temp = LSB(tblocks);
      used = block[temp + tree->thread_id * 64 + 1];
      if (used->joinable && !used->joined)
        unused++;
      Clear(temp, tblocks);
    }
    if (unused > smp_gratuitous_limit)
      return 0;
  }
/*
 ************************************************************
 *                                                          *
 *  If smp_split IS set, we are checking to see if it is    *
 *  acceptable to do a split because there are idle threads *
 *  that need work to do.                                   *
 *                                                          *
 *  The only reason this would be false is if we have a     *
 *  pre-existing split point that is joinable but has not   *
 *  been joined. If one exists, there is no need to split   *
 *  again as there is already an accessible split point.    *
 *  Otherwise, if we are at the root and we are either not  *
 *  allowed to split at the root, or we have additional     *
 *  root moves that have to be searched one at a time using *
 *  all available threads we also can not split here.       *
 *                                                          *
 ************************************************************
 */
  else {
    if (ply == 1 && (!smp_split_at_root || !NextRootMoveParallel() ||
            alpha == o_alpha))
      return 0;
    tblocks = ~thread[tree->thread_id].blocks;
    while (tblocks) {
      temp = LSB(tblocks);
      used = block[temp + tree->thread_id * 64 + 1];
      if (used->joinable && !used->joined)
        unused++;
      Clear(temp, tblocks);
    }
    if (unused > smp_gratuitous_limit)
      return 0;
  }
  return 1;
}

/* modified 08/03/16 */
/*
 *******************************************************************************
 *                                                                             *
 *   ThreadStop() is called from SearchMoveList() when it detects a beta       *
 *   cutoff (fail high) at a node that is being searched in parallel.  We need *
 *   to stop all threads here, and since this threading algorithm is recursive *
 *   it may be necessary to stop other threads that are helping search this    *
 *   branch further down into the tree.  This function simply sets appropriate *
 *   tree->stop variables to 1, which will stop those particular threads       *
 *   instantly and return them to the idle loop in ThreadWait().               *
 *                                                                             *
 *******************************************************************************
 */
void ThreadStop(TREE * RESTRICT tree) {
  int proc;

  Lock(tree->lock);
  tree->stop = 1;
  tree->joinable = 0;
  for (proc = 0; proc < smp_max_threads; proc++)
    if (tree->siblings[proc])
      ThreadStop(tree->siblings[proc]);
  Unlock(tree->lock);
}

/* modified 08/03/16 */
/*
 *******************************************************************************
 *                                                                             *
 *   ThreadTrace() is a debugging tool that simply walks the split block tree  *
 *   and displays interesting data to help debug the parallel search whenever  *
 *   changes break things.                                                     *
 *                                                                             *
 *******************************************************************************
 */
void ThreadTrace(TREE * RESTRICT tree, int depth, int brief) {
  int proc, i;

  Lock(tree->lock);
  Lock(lock_io);
  if (!brief) {
    for (i = 0; i < 4 * depth; i++)
      Print(4095, " ");
    depth++;
    Print(4095, "block[%d]  thread=%d  ply=%d  nprocs=%d  ",
        FindBlockID(tree), tree->thread_id, tree->ply, tree->nprocs);
    Print(4095, "joined=%d  joinable=%d  stop=%d  nodes=%d", tree->joined,
        tree->joinable, tree->stop, tree->nodes_searched);
    Print(4095, "  parent=%d\n", FindBlockID(tree->parent));
  } else {
    if (tree->nprocs > 1) {
      for (i = 0; i < 4 * depth; i++)
        Print(4095, " ");
      depth++;
      Print(4095, "(ply %d)", tree->ply);
    }
  }
  if (tree->nprocs) {
    if (!brief) {
      for (i = 0; i < 4 * depth; i++)
        Print(4095, " ");
      Print(4095, "          parent=%d  sibling threads=",
          FindBlockID(tree->parent));
      for (proc = 0; proc < smp_max_threads; proc++)
        if (tree->siblings[proc])
          Print(4095, " %d(%d)", proc, FindBlockID(tree->siblings[proc]));
      Print(4095, "\n");
    } else {
      if (tree->nprocs > 1) {
        Print(4095, " helping= ");
        for (proc = 0; proc < smp_max_threads; proc++)
          if (tree->siblings[proc]) {
            if (proc == tree->thread_id)
              Print(4095, "[");
            Print(4095, "%d", proc);
            if (proc == tree->thread_id)
              Print(4095, "]");
            Print(4095, " ");
          }
        Print(4095, "\n");
      }
    }
  }
  Unlock(lock_io);
  for (proc = 0; proc < smp_max_threads; proc++)
    if (tree->siblings[proc])
      ThreadTrace(tree->siblings[proc], depth, brief);
  Unlock(tree->lock);
}

/* modified 08/03/16 */
/*
 *******************************************************************************
 *                                                                             *
 *   ThreadWait() is the idle loop for the N threads that are created at the   *
 *   beginning when Crafty searches.  Threads are "parked" here waiting on a   *
 *   pointer to something they should search (a parameter block built in the   *
 *   function Split() in this case.  When this pointer becomes non-zero, each  *
 *   thread "parked" here will immediately call SearchMoveList() and begin the *
 *   parallel search as directed.                                              *
 *                                                                             *
 *   Upon entry, all threads except for the "master" will arrive here with a   *
 *   value of zero (0) in the waiting parameter below.  This indicates that    *
 *   they will search and them be done.  The "master" will arrive here with a  *
 *   pointer to the parent split block in "waiting" which says I will sit here *
 *   waiting for work OR when the waiting split block has no threads working   *
 *   on it, at which point I should return which will let me "unsplit" here    *
 *   and clean things up.  The call to here in Split() passes this block       *
 *   address while threads that are helping get here with a zero.              *
 *                                                                             *
 *******************************************************************************
 */
int ThreadWait(int tid, TREE * RESTRICT waiting) {
  int value, tstart, tend;

/*
 ************************************************************
 *                                                          *
 *  When we reach this point, one of three possible         *
 *  conditions is true (1) we already have work to do, as   *
 *  we are the "master thread" and we have already split    *
 *  the tree, we are coming here to join in;  (2) we are    *
 *  the master, and we are waiting on our split point to    *
 *  complete, so we come here to join and help currently    *
 *  active threads;  (3) we have no work to do, so we will  *
 *  spin until Join() locates a split pont we can join to   *
 *  help out.                                               *
 *                                                          *
 *  Note that when we get here, the parent already has a    *
 *  split block and does not need to call Join(), it simply *
 *  falls through the while spin loop below because its     *
 *  "tree" pointer is already non-zero.                     *
 *                                                          *
 ************************************************************
 */
  while (FOREVER) {
    tstart = ReadClock();
    while (!thread[tid].tree && (!waiting || waiting->nprocs) && !Join(tid) &&
        !thread[tid].terminate);
    tend = ReadClock();
    if (!thread[tid].tree)
      thread[tid].tree = waiting;
    thread[tid].idle += tend - tstart;
    if (thread[tid].tree == waiting || thread[tid].terminate)
      return 0;
/*
 ************************************************************
 *                                                          *
 *  Once we get here, we have a good split point, so we are *
 *  ready to participate in a parallel search.  Once we     *
 *  return from SearchMoveList() we copy our results back   *
 *  to the parent via CopyToParent() before we look for a   *
 *  new split point.  If we are a parent, we will slip out  *
 *  of the spin loop at the top and return to the normal    *
 *  serial search to finish up here.                        *
 *                                                          *
 *  When we return from SearchMoveList(), we need to        *
 *  decrement the "nprocs" value since there is now one     *
 *  less thread working at this split point.                *
 *                                                          *
 *  Note:  CopyToParent() marks the current split block as  *
 *  unused once the copy is completed, so we don't have to  *
 *  do anything about that here.                            *
 *                                                          *
 ************************************************************
 */
    value =
        SearchMoveList(thread[tid].tree, thread[tid].tree->ply,
        thread[tid].tree->depth, thread[tid].tree->wtm,
        thread[tid].tree->alpha, thread[tid].tree->beta,
        thread[tid].tree->searched, thread[tid].tree->in_check, 0, parallel);
    tstart = ReadClock();
    Lock(thread[tid].tree->parent->lock);
    thread[tid].tree->parent->joinable = 0;
    CopyToParent((TREE *) thread[tid].tree->parent, thread[tid].tree, value);
    thread[tid].tree->parent->nprocs--;
    thread[tid].tree->parent->siblings[tid] = 0;
    Unlock(thread[tid].tree->parent->lock);
    thread[tid].tree = 0;
    tend = ReadClock();
    thread[tid].idle += tend - tstart;
  }
}

/* modified 08/03/16 */
/*
 *******************************************************************************
 *                                                                             *
 *   CopyFromParent() is used to copy data from a parent thread to a child     *
 *   thread.  This only copies the appropriate parts of the TREE structure to  *
 *   avoid burning memory bandwidth by copying everything.                     *
 *                                                                             *
 *******************************************************************************
 */
void CopyFromParent(TREE * RESTRICT child) {
  TREE *parent = child->parent;
  int i, ply;

/*
 ************************************************************
 *                                                          *
 *  We have allocated a split block.  Now we copy the tree  *
 *  search state from the parent block to the child in      *
 *  preparation for starting the parallel search.           *
 *                                                          *
 ************************************************************
 */
  ply = parent->ply;
  child->ply = ply;
  child->position = parent->position;
  for (i = 0; i <= rep_index + parent->ply; i++)
    child->rep_list[i] = parent->rep_list[i];
  for (i = ply - 1; i < MAXPLY; i++)
    child->killers[i] = parent->killers[i];
  for (i = 0; i < 4096; i++) {
    child->counter_move[i] = parent->counter_move[i];
    child->move_pair[i] = parent->move_pair[i];
  }
  for (i = ply - 1; i <= ply; i++) {
    child->curmv[i] = parent->curmv[i];
    child->pv[i] = parent->pv[i];
  }
  child->in_check = parent->in_check;
  child->last[ply] = child->move_list;
  child->status[ply] = parent->status[ply];
  child->status[1] = parent->status[1];
  child->save_hash_key[ply] = parent->save_hash_key[ply];
  child->save_pawn_hash_key[ply] = parent->save_pawn_hash_key[ply];
  child->nodes_searched = 0;
  child->fail_highs = 0;
  child->fail_high_first_move = 0;
  child->evaluations = 0;
  child->egtb_probes = 0;
  child->egtb_hits = 0;
  child->extensions_done = 0;
  child->qchecks_done = 0;
  child->moves_fpruned = 0;
  child->moves_mpruned = 0;
  for (i = 0; i < 16; i++) {
    child->LMR_done[i] = 0;
    child->null_done[i] = 0;
  }
  child->alpha = parent->alpha;
  child->beta = parent->beta;
  child->value = parent->value;
  child->wtm = parent->wtm;
  child->depth = parent->depth;
  child->searched = parent->searched;
  strcpy(child->root_move_text, parent->root_move_text);
  strcpy(child->remaining_moves_text, parent->remaining_moves_text);
}

/* modified 08/03/16 */
/*
 *******************************************************************************
 *                                                                             *
 *   CopyToParent() is used to copy data from a child thread to a parent       *
 *   thread.  This only copies the appropriate parts of the TREE structure to  *
 *   avoid burning memory bandwidth by copying everything.                     *
 *                                                                             *
 *******************************************************************************
 */
void CopyToParent(TREE * RESTRICT parent, TREE * RESTRICT child, int value) {
  int i, ply = parent->ply, which;

/*
 ************************************************************
 *                                                          *
 *  The only concern here is to make sure that the info is  *
 *  only copied to the parent if our score is > than the    *
 *  parent value, and that we were not stopped for any      *
 *  reason which could produce a partial score that is      *
 *  worthless and dangerous to use.                         *
 *                                                          *
 *  One important special case.  If we get here with the    *
 *  thread->stop flag set, and ply is 1, then we need to    *
 *  clear the "this move has been searched" flag in the ply *
 *  1 move list since we did not complete the search.  If   *
 *  we fail to do this, then a move being searched in       *
 *  parallel at the root will be "lost" for this iteration  *
 *  and won't be searched again until the next iteration.   *
 *                                                          *
 *  In any case, we add our statistical counters to the     *
 *  parent's totals no matter whether we finished or not    *
 *  since the total nodes searched and such should consider *
 *  everything searched, not just the "useful stuff."       *
 *                                                          *
 *  After we finish copying everything, we mark this split  *
 *  block as free in the split block bitmap.                *
 *                                                          *
 ************************************************************
 */
  if (child->nodes_searched && !child->stop && value > parent->value &&
      !abort_search) {
    parent->pv[ply] = child->pv[ply];
    parent->value = value;
    parent->cutmove = child->curmv[ply];
    for (i = 0; i < 4096; i++) {
      parent->counter_move[i] = child->counter_move[i];
      parent->move_pair[i] = child->move_pair[i];
    }
  }
  if (child->stop && ply == 1)
    for (which = 0; which < n_root_moves; which++)
      if (root_moves[which].move == child->curmv[ply]) {
        root_moves[which].status &= 7;
        break;
      }
  parent->nodes_searched += child->nodes_searched;
  parent->fail_highs += child->fail_highs;
  parent->fail_high_first_move += child->fail_high_first_move;
  parent->evaluations += child->evaluations;
  parent->egtb_probes += child->egtb_probes;
  parent->egtb_hits += child->egtb_hits;
  parent->extensions_done += child->extensions_done;
  parent->qchecks_done += child->qchecks_done;
  parent->moves_fpruned += child->moves_fpruned;
  parent->moves_mpruned += child->moves_mpruned;
  for (i = 1; i < 16; i++) {
    parent->LMR_done[i] += child->LMR_done[i];
    parent->null_done[i] += child->null_done[i];
  }
  which = FindBlockID(child) - 64 * child->thread_id - 1;
  Set(which, thread[child->thread_id].blocks);
}

/* modified 08/03/16 */
/*
 *******************************************************************************
 *                                                                             *
 *   GetBlock() is used to allocate a split block and fill in only SMP-        *
 *   critical information.  The child process will copy the rest of the split  *
 *   block information as needed.                                              *
 *                                                                             *
 *   When we arrive here, the parent split block must be locked since we are   *
 *   going to change data in that block as well as copy data from that block   *
 *   the current split block.  The only exception is when this is the original *
 *   split operation, since this is done "out of sight" of other threads which *
 *   means no locks are needed until after the "joinable" flag is set, which   *
 *   exposes this split point to other threads instantly.                      *
 *                                                                             *
 *******************************************************************************
 */
TREE *GetBlock(TREE * RESTRICT parent, int tid) {
  TREE *child;
  static int warnings = 0;
  int i, unused;
/*
 ************************************************************
 *                                                          *
 *  One NUMA-related trick is that we only allocate a split *
 *  block in the thread's local memory.  Each thread has a  *
 *  group of split blocks that were first touched by the    *
 *  correct CPU so that the split blocks page-faulted into  *
 *  local memory for that specific processor.  If we can't  *
 *  find an optimally-placed block, we return a zero which  *
 *  will prevent this thread from joining the split point.  *
 *  This is highly unlikely as it would mean the current    *
 *  thread has 64 active split blocks where it is waiting   *
 *  on other threads to complete the last bit of work at    *
 *  each.  This is extremely unlikely.                      *
 *                                                          *
 *  Here we use a simple 64 bit bit-map per thread that     *
 *  indicates which blocks are free (1) and which blocks    *
 *  used (0).  We simply use LSB() to find the rightmost    *
 *  one-bit set and that is the local block number.  We     *
 *  convert that to a global block number by doing the      *
 *  simple computation:                                     *
 *                                                          *
 *     global = local + 64 * tid + 1                        *
 *                                                          *
 *  Each thread has exactly 64 split blocks, and block 0    *
 *  is the "master block" that never gets allocated or      *
 *  freed.  Once we find a free block for the current       *
 *  thread, we zero that bit so that the block won't be     *
 *  used again until it is released.                        *
 *                                                          *
 ************************************************************
 */
  if (thread[tid].blocks) {
    unused = LSB(thread[tid].blocks);
    Clear(unused, thread[tid].blocks);
    Set(unused, thread[tid].max_blocks);
  } else {
    if (++warnings < 6)
      Print(2048,
          "WARNING.  local SMP block cannot be allocated, thread %d\n", tid);
    return 0;
  }
  child = block[unused + tid * 64 + 1];
/*
 ************************************************************
 *                                                          *
 *  Found a split block.  Now we need to fill in only the   *
 *  critical information that can't be delayed due to race  *
 *  conditions.  When we get here, the parent split block   *
 *  must be locked, that lets us safely update the number   *
 *  of processors working here, etc, without any ugly race  *
 *  conditions that would corrupt this critical data.       *
 *                                                          *
 ************************************************************
 */
  for (i = 0; i < smp_max_threads; i++)
    child->siblings[i] = 0;
  child->nprocs = 0;
  child->stop = 0;
  child->joinable = 0;
  child->joined = 0;
  child->parent = parent;
  child->thread_id = tid;
  parent->nprocs++;
  parent->siblings[tid] = child;
  parent->joined = 1;
  return child;
}

/*
 *******************************************************************************
 *                                                                             *
 *   WaitForAllThreadsInitialized() waits until all smp_max_threads are        *
 *   initialized.  We have to initialize each thread and malloc() its split    *
 *   blocks before we start the actual parallel search.  Otherwise we will see *
 *   invalid memory accesses and crash instantly.                              *
 *                                                                             *
 *******************************************************************************
 */
void WaitForAllThreadsInitialized(void) {
  while (initialized_threads < smp_max_threads); /* Do nothing */
}

#if !defined (UNIX)
/* modified 08/03/16 */
/*
 *******************************************************************************
 *                                                                             *
 *   ThreadMalloc() is called from the ThreadInit() function.  It malloc's the *
 *   split blocks in the local memory for the processor associated with the    *
 *   specific thread that is calling this code.                                *
 *                                                                             *
 *******************************************************************************
 */
extern void *WinMalloc(size_t, int);
void ThreadMalloc(int64_t tid) {
  int i;

  for (i = tid * 64 + 1; i < tid * 64 + 65; i++) {
    if (block[i] == NULL)
      block[i] =
          (TREE *) ((~(size_t) 127) & (127 + (size_t) WinMalloc(sizeof(TREE) +
                  127, tid)));
    block[i]->parent = NULL;
    LockInit(block[i]->lock);
  }
}
#endif
