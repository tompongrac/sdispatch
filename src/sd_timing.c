/*
   Timing operations
 
   Copyright (c) Thomas Pongrac
 
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
 
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
*/

#include <time.h>
#include <inttypes.h>

#include "sd_timing.h"
#include "sd_error.h"
#include "sd_dynamic_memory.h"


void reset_frame_info(frame *f)
{
  gettimeofday(&f->prev_t, NULL);
  f->current_fps = 0;
  f->prev_d = 0.0;
}

int elapsed_time(frame *f)
{
  struct timeval ctime;
  int tdiff;

  gettimeofday(&ctime, NULL);
  tdiff = (int)((((double) ctime.tv_sec + (ctime.tv_usec / 1000000.0) ) -
          ((double) f->prev_t.tv_sec + (f->prev_t.tv_usec / 1000000.0) ))
          * 1000);

  return tdiff;
}

void set_next_frame(frame *f)
{
  int t_d;

  t_d = elapsed_time(f);
  gettimeofday(&f->prev_t, NULL);

  f->current_fps = (!t_d ? 0 : 1000 / t_d);
  f->prev_d = (double) t_d / 1000;
}

char *get_time_string()
{
  char *tstr;
  time_t rt;
  struct tm *ti;

  rt = time(NULL);
  ti = localtime(&rt);
  SAFE_CALLOC(tstr, 1, 64);

  strftime(tstr, 64, "%H:%M", ti);

  return tstr;
}

/* in ms */
int time_diff(struct timeval *current, struct timeval *previous)
{
  return (((double) current->tv_sec + ((double) current->tv_usec / 1000000.0) ) -
          ((double) previous->tv_sec + ((double) previous->tv_usec / 1000000.0) ))
          * 1000;
}

#define DAY_NUM_SECONDS     86400
#define HOUR_NUM_SECONDS     3600
#define MINUTE_NUM_SECONDS     60

/* parameter is in seconds */
char *time_get_time_remaining_string(uint64_t *r)
{
#define MAX_TIME_REM_STRING_LEN     128
  char *r_str;
  char tmp[MAX_TIME_REM_STRING_LEN];

  uint64_t td, th, tm, ts;

  /* init */
  ts = *r;
  SAFE_CALLOC(r_str, 1, MAX_TIME_REM_STRING_LEN);

  /* days */
  td =  ts / DAY_NUM_SECONDS;
  if (td > 0)
    ts %= DAY_NUM_SECONDS;

  /* hours */
  th =  ts / HOUR_NUM_SECONDS;
  if (th > 0)
    ts %= DAY_NUM_SECONDS;
  
  /* minutes */
  tm =  ts / MINUTE_NUM_SECONDS;
  if (tm > 0)
    ts %= MINUTE_NUM_SECONDS;

  /* seconds should be ready */

  /* make the string */
  snprintf(tmp, MAX_TIME_REM_STRING_LEN, td?"%"PRIu64"d":"", td);
  snprintf(r_str, MAX_TIME_REM_STRING_LEN, th?"%s %"PRIu64"h":"", tmp, th);
  snprintf(tmp, MAX_TIME_REM_STRING_LEN, tm?"%s %"PRIu64"m":"", r_str, tm);
  snprintf(r_str, MAX_TIME_REM_STRING_LEN, "%s %"PRIu64"s", tmp, ts);

  return r_str;
}


// vim:ts=2:expandtab
