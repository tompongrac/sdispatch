/*
   The beginning
 
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

#include <gtk/gtk.h>

#include <stdio.h>

#include "sd.h"
#include "sd_net.h"
#include "sd_ssl.h"
#include "sd_peers.h"
#include "sd_protocol.h"
#include "sd_globals.h"
#include "sd_conf.h"
#include "sd_cl_parser.h"
#include "sd_ui.h"
#include "sd_logging.h"

int main(int argc, char **argv)
{

  sd_globals_alloc();

  cl_parse(argc, argv);

  conf_init();

  logging_init();

  net_init();
  
  ui_init();

  ssl_init();

  ui_begin();
  
  
  /****************************************
   * wait for ui to end 
   ****************************************/

  ssl_deinit();

  ui_deinit();
  
  net_deinit();

  logging_deinit();

  sd_globals_free();

  return 0;
}


// vim:ts=2:expandtab
