/*
   Global variable handling
 
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
#include "sd_globals.h"
#include "sd_dynamic_memory.h"
#include "sd_error.h"
#include "sd_timing.h"

struct sd_globals *gbls;

void sd_globals_alloc()
{
  SAFE_CALLOC(gbls, 1, sizeof(struct sd_globals));
  SAFE_CALLOC(gbls->conf, 1, sizeof(struct sd_conf));
  SAFE_CALLOC(gbls->ui, 1, sizeof(struct sd_ui_info));
  SAFE_CALLOC(gbls->cl_opts, 1, sizeof(struct sd_cl_options));
  SAFE_CALLOC(gbls->prog, 1, sizeof(struct sd_prog_info));
  SAFE_CALLOC(gbls->net, 1, sizeof(struct sd_net_info));
  SAFE_CALLOC(gbls->logging, 1, sizeof(struct sd_logging_info));

  reset_frame_info(&gbls->frame);
}

void sd_globals_free()
{
  SAFE_FREE(gbls->conf);
  SAFE_FREE(gbls->cl_opts);
  SAFE_FREE(gbls->ui);
  SAFE_FREE(gbls->prog);
  SAFE_FREE(gbls->net);
  SAFE_FREE(gbls->logging);
  SAFE_FREE(gbls);
}


// vim:ts=2:expandtab
