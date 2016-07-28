#ifndef __METER_H__
#define __METER_H__

#include <stdint.h>
#include <stdbool.h>

#include "hdr_atomic.h"
#include "hdr_thread.h"

#include "utils.h"

/**
 * Structure calculating exponentially weighted moving averages
 */
typedef struct _performance_metrics_ExponentiallyWeightedMovingAvergage {
  /**
	 * Alpha decay for exponential weighted moving average
	 */
	double alpha;
	/**
	 * Tick interval for metered rate (in nanoseconds)
	 */
	uint64_t tick_interval_ns;
  /**
	 * Tick interval for metered rate (seconds)
	 */
	uint64_t tick_interval_s;
	/**
	 * Flag to determine if the EWMA has been initialized
	 */
	bool is_initialized;
	/**
	 * Total number of uncounted events
	 */
	int64_t uncounted;
  /**
   * Total number of events
   */
  int64_t total_count;
  /**
   * Time of the last tick interval
   */
  uint64_t last_tick;
  /**
   * Starting time (first mark)
   */
  uint64_t start_time;
	/**
	 * EWMA rate
	 */
	double rate;
  /**
   * Mutex for ensuring synchronization
   */
  hdr_mutex mutex;
} EWMA;

/* Create an alias (Meter) */
typedef EWMA Meter;

/* Macro initialization for EWMA */
#define EWMA_INIT(ewma, seconds, interval_value) \
  ewma.tick_interval_ns = interval_value * NANOSEC; \
  ewma.tick_interval_s = interval_value; \
  ewma.alpha = calculate_alpha_decay(seconds, interval_value); \
  ewma.is_initialized = false; \
  ewma.uncounted = 0; \
  ewma.total_count = 0; \
  ewma.last_tick = 0; \
  ewma.start_time = 0; \
  ewma.rate = 0.0; \
  hdr_mutex_init(&ewma.mutex);
#define EWMA_CLEANUP(ewma) \
  hdr_mutex_destroy(&ewma.mutex);
#define EWMA_STANDARD_ONE_MINUTE_INIT(ewma) EWMA_INIT(ewma, 60, 5)
#define EWMA_STANDARD_FIVE_MINUTE_INIT(ewma) EWMA_INIT(ewma, 300, 5)
#define EWMA_STANDARD_FIFTEEN_MINUTE_INIT(ewma) EWMA_INIT(ewma, 900, 5)
#define METER_INIT(meter, seconds, interval) EWMA_INIT(meter, seconds, interval)
#define METER_CLEANUP(meter) EWMA_CLEANUP(meter)
#define METER_STANDARD_ONE_MINUTE_INIT(meter) EWMA_STANDARD_ONE_MINUTE_INIT(meter)
#define METER_STANDARD_FIVE_MINUTE_INIT(meter) EWMA_STANDARD_FIVE_MINUTE_INIT(meter)
#define METER_STANDARD_FIFTEEN_MINUTE_INIT(meter) EWMA_STANDARD_FIFTEEN_MINUTE_INIT(meter)


/* {{{ calculate_alpha_decay(time, interval)
 * Calculate the alpha decay based on the time (in seconds) to be used for the
 * EWMA and the tick interval for the rate calculation
 */
static inline double calculate_alpha_decay(uint64_t time, uint64_t tick_interval) {
  return 1.0 - exp((-(double) tick_interval) / ((double) time));
}

/* {{{ increment_ewma(ewma) | increment_meter(meter)
 *     mark_ewma(ewma) | mark_meter(meter)
 * Increment the number of uncounted marks
 */
static inline void increment_ewma(EWMA* ewma) {
  ewma->uncounted = hdr_atomic_add_fetch_64(&ewma->uncounted, 1);
  ewma->total_count = hdr_atomic_add_fetch_64(&ewma->total_count, 1);
}
#define increment_meter(meter) increment_ewma(meter)
#define mark_ewma(ewma) increment_ewma(ewma)
#define mark_meter(meter) increment_ewma(meter)
/* }}} */

/* {{{ mean_rate(ewma|meter)
 * Calculate the mean rate (over time)
 */
static inline double mean_rate(EWMA* ewma) {
  uint64_t elapsed;

  /* Determine if the mean rate can be calculated */
  if (ewma->total_count <= 0) {
    return 0.0;
  }

  /* Calculate and return the mean rate */
  elapsed = get_fasttime() - ewma->start_time;
  return ewma->total_count / (((double) elapsed) / NANOSEC);
}
/* }}} */

/* {{{ tick_ewma(ewma)|tick_meter(meter)
 * Perform the tick for the rate if the interval rate has occurred
 *
 * NOTE: Standard tick interval initialization is 5 seconds for rate
 *       initializations
 */
static inline void tick_ewma(EWMA* ewma) {
  uint64_t now;
  uint64_t elapsed;

  /* Calculate the instant rate and update the uncounted marks */
  double instant_rate = ((double) ewma->uncounted) / ((double) ewma->tick_interval_s);

  /* Lock the mutex before updating the rate */
  hdr_mutex_lock(&ewma->mutex);

  /* Determine if the rate should be ticked/updated */
  now = get_fasttime();
  elapsed = (now - ewma->last_tick);
  if (elapsed > ewma->tick_interval_ns) {
    /* Determine the amount of ticks to perform */
    uint64_t ticks = (uint64_t) (elapsed / ewma->tick_interval_ns);
    int i = 0;
    for (i; i < ticks; ++i) {
      ewma->uncounted = 0;

      if (ewma->is_initialized) {
        ewma->rate += (ewma->alpha * (instant_rate - ewma->rate));
      } else {
        ewma->rate = instant_rate;
        ewma->last_tick = ewma->start_time = get_fasttime();
        ewma->is_initialized = true;
      }
    }

    /* Update the last tick */
    ewma->last_tick = now;
  }

  /* Unlock the mutex */
  hdr_mutex_unlock(&ewma->mutex);
}
#define tick_meter(meter) tick_ewma(meter)
/* }}} */

#endif /* __METER_H__ */
