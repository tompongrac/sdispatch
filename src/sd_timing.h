#ifndef SD_TIMING_H
#define SD_TIMING_H

#include <sys/time.h>

/*! \brief Holds timing information */
typedef struct
{
  struct timeval prev_t;
  int current_fps;
  double prev_d;
} frame;

/*! \brief Restart timer */
extern void reset_frame_info(frame *f);

/*! \brief Get elapsed time since last frame */
extern int elapsed_time(frame *f);

/*! \brief Record current frame */
extern void set_next_frame(frame *f);

/*! \brief Get current time string %H:%M */
extern char *get_time_string();

/*! \brief Get difference between two time periods in ms */
extern int time_diff(struct timeval *current, struct timeval *previous);

/*! \brief Get a string containing the time remaining based on time in seconds */
extern char *time_get_time_remaining_string(uint64_t *r);

#endif


// vim:ts=2:expandtab
