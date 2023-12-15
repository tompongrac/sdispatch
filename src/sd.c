/*
   Common variable handling
 
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

#include "sd.h"
#include "sd_ui.h"
#include "sd_protocol.h"
#include "sd_error.h"
#include "sd_dynamic_memory.h"

void sd_set_state(char *s, int ns)
{
  int os;

  os = *s;
  *s = ns;
  ui_state_set(os, ns);
  
}

int get_boolean_id_from_string(const char *str)
{
  if (!strncasecmp(SD_PROTOCOL_VALUE_TRUE, str, strlen(SD_PROTOCOL_VALUE_TRUE)))
    return SD_OPTION_ON;
  if (!strncasecmp(SD_PROTOCOL_VALUE_FALSE, str, strlen(SD_PROTOCOL_VALUE_FALSE)))
    return SD_OPTION_OFF;

  return -1;
}

char *get_boolean_string(char b)
{
  char *str;
  int len;

  if (b == SD_OPTION_ON)
  {
    len = strlen(SD_PROTOCOL_VALUE_TRUE) + 1;
    SAFE_CALLOC(str, 1, len);
    memcpy(str, SD_PROTOCOL_VALUE_TRUE, len);
  }
  else
  {
    len = strlen(SD_PROTOCOL_VALUE_FALSE) + 1;
    SAFE_CALLOC(str, 1, len);
    memcpy(str, SD_PROTOCOL_VALUE_FALSE, len);
  }

  return str;
}


// vim:ts=2:expandtab
