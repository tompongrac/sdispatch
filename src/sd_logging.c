/*
   Logging handling
 
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

#include <stdio.h>
#include <time.h>

#include "sd_globals.h"
#include "sd_logging.h"
#include "sd_dynamic_memory.h"
#include "sd_error.h"

void logging_init(void)
{
  if (gbls->conf->logging_enabled == SD_OPTION_ON)
  {
    file_set_path_from_fullpath(&gbls->logging->file,
        gbls->conf->logging_path);

    if (file_open(&gbls->logging->file, SD_TO_LOG_FILE) == -1)
      gbls->conf->logging_enabled = SD_OPTION_OFF;

    /* prepared for logging */

  }
}

void logging_deinit(void)
{
  file_close(&gbls->logging->file);
}

int logging_print_line(const char *m)
{
  char *nline;
  char t[128];
  int l, ret;

  time_t rawtime;
  struct tm *ti;

  l = strlen(m) + 256;
  SAFE_CALLOC(nline, 1, l);
  
  time(&rawtime);
  ti = localtime(&rawtime);
  strftime(t, sizeof t, SD_LOG_FILE_FORMAT, ti);

  snprintf(nline, l, "%s %s\n", t, m);

  if (fputs(nline, gbls->logging->file.file) == EOF) {
    ret = -1;
  }
  else {
    ret = 0;
    
    if (fflush(gbls->logging->file.file) == EOF)
      ret = -1;
  }


  SAFE_FREE(nline);

  return ret;
}

int logging_print_line_iter(void *v, int i)
{
  if (logging_print_line((const char *)v) == -1)
  {
    gbls->conf->logging_enabled = SD_OPTION_OFF;
    return 1;
  }
  return 0;
}


// vim:ts=2:expandtab
