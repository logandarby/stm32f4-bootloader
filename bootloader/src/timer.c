#include "timer.h"

#include "system.h"

void timer_init(timer_t* timer, uint64_t interval_ms, bool auto_reset) {
  if (!timer) return;

  timer->interval_ms = interval_ms;
  timer->auto_reset = auto_reset;
  timer->disabled = false;
  timer->start_ms = system_get_ms();
}

bool timer_has_elapsed(timer_t* timer) {
  if (!timer || timer->disabled) return false;

  uint64_t now = system_get_ms();

  /*
   * Unsigned subtraction handles uint64_t wraparound naturally.
   * If the elapsed time is less than interval_ms, the timer hasn't
   * expired.
   */
  if ((now - timer->start_ms) < timer->interval_ms) {
    return false;
  }

  if (timer->auto_reset) {
    // Advance start_ms by interval_ms to keep periodic alignment
    timer->start_ms += timer->interval_ms;
  }

  return true;
}

void timer_reset(timer_t* timer) {
  if (!timer) return;

  timer->start_ms = system_get_ms();
}

void timer_set_interval(timer_t* timer, uint64_t interval_ms) {
  if (!timer) return;

  timer->interval_ms = interval_ms;
  timer_reset(timer);
}

void timer_disable(timer_t* timer) {
  if (!timer) return;

  timer->disabled = true;
}

void timer_enable(timer_t* timer) {
  if (!timer) return;

  // Reset start time upon enabling so stale time doesn't trigger an
  // instant timeout
  timer->start_ms = system_get_ms();
  timer->disabled = false;
}