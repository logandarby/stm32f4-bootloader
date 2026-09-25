#ifndef A3B8A2E2_F1BB_467F_8520_24072C382C5B
#define A3B8A2E2_F1BB_467F_8520_24072C382C5B

#include <stdbool.h>
#include <stdint.h>

/*
 * Timer configuration/state.
 *
 * auto_reset:
 *   If true, timer automatically schedules its next expiration when
 *   timer_has_elapsed() observes an expiration.
 *
 *   If false, the timer remains elapsed until timer_reset() is called.
 *
 * interval_ms:
 *   Timer interval in milliseconds.
 */
typedef struct {
  uint64_t interval_ms;
  uint64_t start_ms;
  bool auto_reset;
  bool disabled;
} timer_t;

/*
 * Initialize a timer.
 *
 * The timer starts counting immediately from the current system time.
 */
void timer_init(timer_t* timer, uint64_t interval_ms, bool auto_reset);

/*
 * Returns whether the timer has elapsed.
 */
bool timer_has_elapsed(timer_t* timer);

/*
 * Reset the timer
 */
void timer_reset(timer_t* timer);

/*
 * Change the timer interval.
 *
 * The timer is reset when the interval changes.
 */
void timer_set_interval(timer_t* timer, uint64_t interval_ms);

/**
 * Disable/enable the timer
 */
void timer_disable(timer_t* timer);
void timer_enable(timer_t* timer);

#endif /* A3B8A2E2_F1BB_467F_8520_24072C382C5B */
