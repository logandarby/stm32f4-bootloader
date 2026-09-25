#ifndef DC72349E_32DE_4DF5_86EA_3B93303C2321
#define DC72349E_32DE_4DF5_86EA_3B93303C2321

#include <common.h>

typedef struct {
  void (*callback)(void);
  uint32_t interval_ms;

  // Optional
  bool disabled;   // Change to either enable or disable a task
  bool singleton;  // If set to true, the task will only run once, and then
                   // set itself to disabled

  // Private fields
  uint32_t last_run_ms;  // Defaults to 0
} task_t;

/**
 * A co-operative, non-blocking scheduler.
 * Runs a number of tasks at a pre-defined interval.
 *
 * Requires that the system has already been setup
 */
NORETURN void scheduler_run(task_t* tasks, size_t tasks_len);

#endif /* DC72349E_32DE_4DF5_86EA_3B93303C2321 */
