/*
   Parser for command line arguments
 
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

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include "sd_cl_parser.h"
#include "sd_globals.h"
#include "sd_conf.h"
#include "sd_version.h"
#include "sd_ui.h"


void cl_parse(int argc, char **argv)
{
  int c, errflg;
  char use_config;

  errflg = 0;
  use_config = 0;
  
  while ((c = getopt(argc, argv, "Ghc:")) != -1)
  {
    switch (c)
    {
      case 'G':
        //set_gtk_ui();
        break;
      case 'h':
        sd_usage(argv[0]);
        exit(0);
        break;
      case 'c':
        snprintf(gbls->conf->config_file_path, sizeof gbls->conf->config_file_path,
            "%s", optarg);
        use_config = 1;
        break;
      case '?':
        /* invalid option */
        errflg++;
    }
  }

  /* only interface 
   * */
  set_gtk_ui();
  
  if (!use_config)
  {
    conf_set_conf_default_path();
  }

  if (errflg)
  {
    sd_usage(argv[0]);
    exit(1);
  }
  
}

void sd_usage(char *sd_name)
{
  printf("Usage: %s [-h] [-G]\n"
         "%s %s\n"
         "-h                : Shows this output and exit\n"
         "-G                : Register GTK interface (default)\n"
         "-c <config-file>  : Use <config-file> instead of default\n"
	 , sd_name, SD_NAME, SD_VERSION
  );
}


// vim:ts=2:expandtab
