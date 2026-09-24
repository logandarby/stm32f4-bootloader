#include "scheduler.h"

#include "system.h"

NORETURN void scheduler_run(task_t* tasks, size_t tasks_len) {
  // Initialize tasks
  for (size_t i = 0; i < tasks_len; i++) {
    tasks[i].last_run_ms = system_get_ms();
  }
  uint64_t current_time = system_get_ms();
  while (1) {
    current_time = system_get_ms();
    for (size_t i = 0; i < tasks_len; i++) {
      task_t* task = &tasks[i];
      if (task->disabled || !task->callback) {
        continue;
      }
      if (current_time - task->last_run_ms >= task->interval_ms) {
        task->last_run_ms = current_time;
        task->callback();
      }
    }
  }
}
