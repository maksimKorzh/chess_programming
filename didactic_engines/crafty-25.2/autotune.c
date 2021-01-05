#include "chess.h"
#include "data.h"
/* last modified 08/26/15 */
/*
 *******************************************************************************
 *                                                                             *
 *   AutoTune() is used to tune the parallel search parameters and optimize    *
 *   them for the current hardware and a specific time per move target.  The   *
 *   syntax of the command is                                                  *
 *                                                                             *
 *         autotune time accuracy                                              *
 *                                                                             *
 *   "time" is the target time to optimize for.  Longer time limits require    *
 *   somewhat different tuning values, so this should be set to the typical    *
 *   time per move.  The default is 30 seconds per move if not specified.      *
 *                                                                             *
 *   "accuracy" is normally set to 4.  Since SMP search results and times are  *
 *   non-deterministic, running tests 1 time can be inaccurate.  This value is *
 *   used to determine how many times to run each test.  If you set it to two, *
 *   the entire test will take 1/2 as long.  Bigger numbers are better, but    *
 *   it is easy to make the test run for hours if you go too far.  A big value *
 *   will work best if allowed to run overnight.  Crafty will display a time   *
 *   estimate after determining the optimal benchmark settings.  If this time  *
 *   is excessive, a ^C will let you re-start Crafty and pick a more           *
 *   reasonable time/accuracy setting.                                         *
 *                                                                             *
 *   AutoTune() will tune the primary SMP controls, namely the values set by   *
 *   the commands smpgroup, smpmin, smpsd and smppsl.  It will NEVER change    *
 *   smpmt (max threads), smproot (split at root) and smpaffinity.  those are  *
 *   user choices and in general the default is optimal.  In general the       *
 *   values have min and max settings defined in data.c (search for autotune), *
 *   and this code will try multiple values in the given range to find an      *
 *   optimal setting.  For some of the values, it will test each value in the  *
 *   interval, but for values with a large range it will try reasonable        *
 *   (again, see data.c and the "tune" array) intervals.  If desired, the      *
 *   low/high limits can be changed along with the interval between samples,   *
 *   by modifying the autotune data in data.c.                                 *
 *                                                                             *
 *   Note that this command is best used before you go to eat or something as  *
 *   it will run a while.  If you ramp up the accuracy setting, it will take   *
 *   multiples of accuracy times longer.  Best results are likely obtained     *
 *   with a larger accuracy setting, but it needs to run overnight.            *
 *                                                                             *
 *******************************************************************************
 */
void AutoTune(int nargs, char *args[]) {
  unsigned int target_time = 3000, accuracy = 4, atstart, atend;
  unsigned int time, current, setting[64], times[64], last_time, stageii;
  int benchd, i, v, p, best, bestv, samples;
  FILE *craftyrc = fopen(".craftyrc", "a");

/*
 ************************************************************
 *                                                          *
 *  Initialize.                                             *
 *                                                          *
 ************************************************************
 */
  if (smp_max_threads < 2) {
    Print(4095, "ERROR: smpmt must be set to > 1 for tuning to work\n");
    fclose(craftyrc);
    return;
  }
  if (nargs > 1)
    target_time = atoi(args[1]) * 100;
  if (nargs > 2)
    accuracy = atoi(args[2]);
  Print(4095, "AutoTune()  time=%s  accuracy=%d\n",
      DisplayHHMMSS(target_time), accuracy);
/*
 ************************************************************
 *                                                          *
 *  First task is to find the benchmark setting that will   *
 *  run in the alotted time.  The Bench() command runs six  *
 *  positions, so we want the command to run in no more     *
 *  than six times the autotune time limit to average the   *
 *  specified time per move.  We break out of the loop when *
 *  bench takes more than 6x this time limit and use the    *
 *  previous value which just fit inside the limit.         *
 *                                                          *
 ************************************************************
 */
  atstart = ReadClock();
  stageii = 0;
  for (v = 0; v < autotune_params; v++)
    for (current = tune[v].min; current <= tune[v].max;
        current += tune[v].increment)
      stageii++;
  Print(4095, "Calculating optimal benchmark setting.\n");
  Print(4095, "Target time average = %s.\n", DisplayHHMMSS(6 * target_time));
  Print(4095, "Estimated run time (stage I) is %s.\n",
      DisplayHHMMSS(accuracy * 12 * target_time));
  Print(4095, "Estimated run time (stage II) is %s.\n",
      DisplayHHMMSS(accuracy * stageii * 4 * target_time));
  Print(4095, "\nBegin stage I (calibration)\n");
  last_time = 0;
  for (benchd = -5; benchd < 10; benchd++) {
    Print(4095, "bench %2d:", benchd);
    time = 0;
    for (v = 0; v < accuracy; v++)
      time += Bench(benchd, 1);
    time /= accuracy;
    Print(4095, " ->%s\n", DisplayHHMMSS(time));
    if (time > 6 * target_time)
      break;
    last_time = time;
  }
  benchd--;
  Print(4095, "Optimal setting is " "bench %d" "\n", benchd);
  atend = ReadClock();
  Print(4095, "Actual runtime for Stage I: %s\n",
      DisplayHHMMSS(atend - atstart));
  Print(4095, "New estimated run time (stage II) is %s.\n",
      DisplayHHMMSS(accuracy * stageii * last_time));
  Print(4095, "\nBegin stage II (SMP testing).\n");
  atstart = ReadClock();
/*
 ************************************************************
 *                                                          *
 *  Next we simply take each option, one by one, and try    *
 *  reasonable values between the min/max values as defined *
 *  in data.c.                                              *
 *                                                          *
 *  The process is fairly simple, but very time-consuming.  *
 *  We will start at the min value for a single paramenter, *
 *  and run bench "accuracy" times and compute the average  *
 *  of the times.  We then repeat for the next step in the  *
 *  parameter, and continue until we try the max value that *
 *  is allowed.  We choose the parameter value that used    *
 *  the least amount of time which optimizes this value for *
 *  minimum time-to-depth.                                  *
 *                                                          *
 ************************************************************
 */
  for (v = 0; v < autotune_params; v++) {
    Print(4095, "auto-tuning %s (%d ~ %d by %d)\n", tune[v].description,
        tune[v].min, tune[v].max, tune[v].increment);
    current = *tune[v].parameter;
    samples = 0;
    if (v == 0 && tune[v].min > smp_max_threads) {
      samples = 1;
      times[0] = 0;
      setting[0] = smp_max_threads;
    } else
      for (current = tune[v].min; current <= tune[v].max;
          current += tune[v].increment) {
        Print(4095, "Testing %d: ", current);
        *tune[v].parameter = current;
        time = 0;
        for (p = 0; p < accuracy; p++)
          time += Bench(benchd, 1);
        time /= accuracy;
        times[samples] = time;
        setting[samples++] = current;
        Print(4095, " ->%s\n", DisplayHHMMSS(time));
      }
    best = 0;
    bestv = times[0];
    for (i = 1; i < samples; i++)
      if (bestv > times[i]) {
        bestv = times[i];
        best = i;
      }
    fprintf(craftyrc, "%s=%d\n", tune[v].command, setting[best]);
    Print(4095, "adding " "%s=%d" " to .craftyrc file.\n", tune[v].command,
        setting[best]);
  }
  atend = ReadClock();
  Print(4095, "Runtime for StageII: %s\n", DisplayHHMMSS(atend - atstart));
  fclose(craftyrc);
}
