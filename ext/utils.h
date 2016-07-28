#ifndef __UTILS_H__
#define __UTILS_H__

#include "hdr_time.h"

#define SECONDS_TO_NANOSECONDS ((uint64_t) 1e9)
#define NANOSECONDS_TO_MICROSECONDS ((uint64_t) 1e3)

#ifdef _WIN32
# define EPOCH_DELTA 116444736000000000llu /* Delta since 1970/01/01 (Win32 epoch to Unix epoch) */
#endif

#define LOWEST_TRACKABLE_VALUE 1LL
#define HIGHEST_TRACKABLE_VALUE 3600LL * 1000LL * 1000LL /* Microseconds per hour */

/* {{{ gettime()
 * Get a fast time for calculating latencies; returns in nanoseconds
 */
static inline uint64_t get_fasttime() {
  /* Get the current timestamp */
  hdr_timespec t;
  hdr_gettime(&t);

  /* Convert the timestamp to nanoseconds and return the value */
  return (((uint64_t) t.tv_sec) * SECONDS_TO_NANOSECONDS + t.tv_nsec);
}
/* }}} */

/* {{{ get_timestamp()
 * Get the precise timestamp (in seconds)
 */
static uint64_t get_timestamp() {
  /* Get the current local time */
#ifndef _WIN32
  struct timeval localtime;
  gettimeofday(&localtime, NULL);
#else
  FILETIME localtime;
  GetSystemTimeAsFileTime(&localtime);
#endif

  /* Calculate the millisecond timestamp (Unix epoch) */
  uint64_t return_timestamp;
#ifndef _WIN32
  return_timestamp = (uint64_t) ((localtime.tv_sec * 1000) + (localtime.tv_usec / 1000));
#else
  return_timestamp |= (uint64_t) (localtime.dwHighDateTime);
  return_timestamp <<= 32llu;
  return_timestamp |= (uint64_t) (localtime.dwLowDateTime);
  return_timestamp -= EPOCH_DELTA;
  return_timestamp /= 10000;
#endif

  /* Return the timestamp in seconds */
  return return_timestamp / 1000;
}

#endif /* __UTILS_H__ */
