/*
 * Program: usage
 *
 * Author:
 *  John Criswell
 *
 * Description:
 *  Report useful information about the time and memory usage of processes.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/*
 * Function: findtMemTickSize()
 *
 * Description:
 *  Find the memory size which is expected to be represented in
 *  kilobytes * ticks-of-execution (as returned by getrusage()).
 */
static inline long
findMemTickSize (long totalTime, long size) {
  /* Ticks per second */
  long ticksPerSecond;

  /* Total (average?) memory size in KB */
  long kbSize;

  /*
   * Find the number of ticks per second.
   */
  ticksPerSecond = sysconf(_SC_CLK_TCK);

  /*
   * Convert the size into KB * seconds into just KB by dividing by
   * ticks / second and then by the total amount of CPU time used by the
   * children processes.
   */
  kbSize = size / ticksPerSecond / totalTime;

  /*
   * Return the size.
   */
  return kbSize;
}

/*
 * Function: main()
 *
 * Description:
 *  Entry point for this program.
 *
 * Inputs:
 *  argc - The number of arguments given on the command line.
 *
 * Outputs:
 *  argv - A pointer to an array of strings for the arguments.  This pointer
 *         is assumed to be non-NULL.
 *
 * Return value:
 *  The return value is the exit status for when the program terminates.
 */
int
main (int argc, char ** argv) {
  /* Buffer for holding child usage information */
  struct rusage rusage;

  /* Child process ID */
  pid_t child;

  /* Status of child process */
  int status;

  /* Start and end times */
  time_t startTime;
  time_t endTime;

  /*
   * Create a child process and execute the command specified on the command
   * line.
   */
  switch (child = fork()) {
    case -1:
      perror("Fork failed: ");
      return -1;

    case 0:
      /*
       * The child process should attempt to execute the program.
       */
      if (execvp (argv[1], argv + 1) == -1) {
        perror ("Exec failed: ");
        return -1;
      }
      break;

    default:
      break;
  };

  /*
   * Get the current time.
   */
  if ((startTime = time (NULL)) == ((time_t)(-1))) {
    perror ("Failed to get start time: ");
    return -1;
  }

  /*
   * The parent process should wait on the child process.
   */
  waitpid (child, &status, 0);

  /*
   * Get the current time after the child has completed execution.
   */
  if ((endTime = time (NULL)) == ((time_t)(-1))) {
    perror ("Failed to get end time: ");
    return -1;
  }

  /*
   * Get and report the resource usage.
   */
  if ((getrusage (RUSAGE_CHILDREN, &rusage)) == -1) {
    perror ("Getrusage failed: ");
    return -1;
  }

  printf ("User CPU time (s): %ld\n", rusage.ru_utime.tv_sec);
  printf ("System CPU time (s): %ld\n", rusage.ru_stime.tv_sec);
  printf ("Total CPU time (s): %ld\n", rusage.ru_utime.tv_sec + rusage.ru_stime.tv_sec);
  printf ("Total Wall time (s): %6.2f\n", difftime (endTime,startTime));
  printf ("\n");
  printf ("Maximum memory (KB): %ld\n", rusage.ru_maxrss);
  printf ("Maximum memory (MB): %ld\n", rusage.ru_maxrss / 1024);
  printf ("Maximum memory (GB): %ld\n", rusage.ru_maxrss / 1024 / 1024);
  printf ("\n");

  /*
   * Compute the total amount of second the CPU was busy.  If it is less than
   * a second, round it up to a second.
   */
  long totalTime = rusage.ru_utime.tv_sec - rusage.ru_stime.tv_sec;
  if (totalTime < 1)
    totalTime = 1;

  /* Print code size */
  long codeSize = findMemTickSize (totalTime, rusage.ru_ixrss);
  printf ("Maximum code (KB): %ld\n", codeSize);
  printf ("Maximum code (MB): %ld\n", codeSize / 1024);
  printf ("\n");

  /* Print data size */
  long dataSize = findMemTickSize (totalTime, rusage.ru_idrss);
  printf ("Maximum data (KB): %ld\n", dataSize);
  printf ("Maximum data (MB): %ld\n", dataSize / 1024);
  printf ("\n");

  /* Print stack size */
  long stackSize = findMemTickSize (totalTime, rusage.ru_isrss);
  printf ("Maximum stack (KB): %ld\n", stackSize);
  printf ("Maximum stack (MB): %ld\n", stackSize / 1024);
  printf ("\n");

  /*
   * Print I/O statistics.
   */
  printf ("Number of FS Reads : %ld\n", rusage.ru_inblock);
  printf ("Number of FS Writes: %ld\n", rusage.ru_oublock);
  return 0;
}
