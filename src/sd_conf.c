/*
   Parser for configuration file
 
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
#include <stdio.h>
#include <string.h>

#include "sd_conf.h"
#include "sd_globals.h"
#include "sd_protocol.h"

struct sd_conf_item config[] = {
  { "ssl_ca_cert_path",            SD_CONFIG_VALUE_TYPE_STRING,           SD_MAX_PATH_LEN,                 NULL },
  { "ssl_verify_peer",             SD_CONFIG_VALUE_TYPE_STRING_BOOLEAN,   SD_MAX_PROTOCOL_CONST_VALUE_LEN, NULL },
  { "ssl_verify_depth",            SD_CONFIG_VALUE_TYPE_INT,              sizeof(int),                     NULL },
  { "ssl_use_cert",                SD_CONFIG_VALUE_TYPE_STRING_BOOLEAN,   SD_MAX_PROTOCOL_CONST_VALUE_LEN, NULL },
  { "ssl_cert_path",               SD_CONFIG_VALUE_TYPE_STRING,           SD_MAX_PATH_LEN,                 NULL },
  { "ssl_pr_key_path",             SD_CONFIG_VALUE_TYPE_STRING,           SD_MAX_PATH_LEN,                 NULL },

  { "control_server_net_address",  SD_CONFIG_VALUE_TYPE_STRING,           LOOKUP_ADDRESS_LEN,              NULL },
  { "control_server_service",      SD_CONFIG_VALUE_TYPE_STRING,           LOOKUP_SERVICE_LEN,              NULL },
  { "control_client_net_address",  SD_CONFIG_VALUE_TYPE_STRING,           LOOKUP_ADDRESS_LEN,              NULL },
  { "control_client_service",      SD_CONFIG_VALUE_TYPE_STRING,           LOOKUP_SERVICE_LEN,              NULL },

  { "data_local_net_address",      SD_CONFIG_VALUE_TYPE_STRING,           LOOKUP_ADDRESS_LEN,              NULL },
  { "data_wide_net_address",       SD_CONFIG_VALUE_TYPE_STRING,           LOOKUP_ADDRESS_LEN,              NULL },
  { "data_wide_service",           SD_CONFIG_VALUE_TYPE_STRING,           LOOKUP_SERVICE_LEN,              NULL },
  { "data_output_path",            SD_CONFIG_VALUE_TYPE_STRING,           SD_MAX_PATH_LEN,                 NULL },

  { "logging_enabled",             SD_CONFIG_VALUE_TYPE_STRING_BOOLEAN,   sizeof(int),                     NULL },
  { "logging_path",                SD_CONFIG_VALUE_TYPE_STRING,           SD_MAX_PATH_LEN,                 NULL },
};

int conf_get_num_items(void)
{
  return sizeof(config) / sizeof (struct sd_conf_item);
}

void conf_set_conf_default_path()
{
#ifdef WIN32

#define SD_CONFIG_DEFAULT_PATH_1    ".\\sdispatch.conf"

#else

#define SD_CONFIG_DEFAULT_PATH_1    "/etc/sdispatch.conf"
#define SD_CONFIG_DEFAULT_PATH_2    "./sdispatch.conf"
  
  if (file_exists(SD_CONFIG_DEFAULT_PATH_2) == SD_OPTION_ON)
  {
    snprintf(gbls->conf->config_file_path, sizeof gbls->conf->config_file_path,
            "%s", SD_CONFIG_DEFAULT_PATH_2);
    return;
  }

#endif

  snprintf(gbls->conf->config_file_path, sizeof gbls->conf->config_file_path,
          "%s", SD_CONFIG_DEFAULT_PATH_1);

}

int conf_set_pointer(const char *n, void *p)
{
  int i, l, ret;

  l = conf_get_num_items();
  ret = -1;

  for (i = 0; i < l; i++)
  {
    if (!strcasecmp(n, config[i].name))
    {
      config[i].value = p;
      ret = i;
      break;
    }
  }

  return i;
}

void conf_set_all_pointers(void)
{
  /* general ssl */
  conf_set_pointer("ssl_ca_cert_path", &gbls->conf->ssl_verify.ssl_ca_cert_dir_path);
  conf_set_pointer("ssl_verify_peer", &gbls->conf->ssl_verify.ssl_verify_peer);
  conf_set_pointer("ssl_verify_depth", &gbls->conf->ssl_verify.ssl_verify_depth);
  conf_set_pointer("ssl_use_cert", &gbls->conf->ssl_verify.ssl_use_cert);
  conf_set_pointer("ssl_cert_path", &gbls->conf->ssl_verify.ssl_cert_path);
  conf_set_pointer("ssl_pr_key_path", &gbls->conf->ssl_verify.ssl_pr_key_path);

  /* control connection */
  conf_set_pointer("control_server_net_address", &gbls->conf->control_server_net_address);
  conf_set_pointer("control_server_service", &gbls->conf->control_server_service);
  conf_set_pointer("control_client_net_address", &gbls->conf->control_client_net_address);
  conf_set_pointer("control_client_service", &gbls->conf->control_client_service);

  /* data transfer */
  conf_set_pointer("data_local_net_address", &gbls->conf->data_local_net_address);
  conf_set_pointer("data_wide_net_address", &gbls->conf->data_wide_net_address);
  conf_set_pointer("data_wide_service", &gbls->conf->data_wide_service);
  conf_set_pointer("data_output_path", &gbls->conf->data_output_path);
  
  /* logging */
  conf_set_pointer("logging_enabled", &gbls->conf->logging_enabled);
  conf_set_pointer("logging_path", &gbls->conf->logging_path);
}

void conf_init(void)
{
  conf_set_all_pointers();
  conf_load_file();
}

int conf_load_file(void)
{
  FILE *f;
  char line[10240];
  char *p, *q;
  int nvalid = 0;
  int lineno = 0;

  if ((f = fopen(gbls->conf->config_file_path, "r")) == NULL) {
    fprintf(stderr, "Warning: Could not find configuration file.\n");
    return -1;
  }


  while (fgets(line, sizeof(line), f) != 0)
  {
    lineno++;

    /* trim comment */
    if ((p = strchr(line, '#')))
      *p = '\0';

    /* trim new line */
    if ((p = strchr(line, '\r')))
      *p = '\0';
    if ((p = strchr(line, '\n')))
      *p = '\0';

    q = line;

    /* trim initial space */
    while (q < line + sizeof(line) && *q == ' ')
      q++;

    /* ignore empty lines */
    if (line[0] == '\0' || *q == '\0')
      continue;



    /* make sure we have an equals symbol */
    if (!strchr(q, '=')) {
      fprintf(stderr, "Parse Error: Line %i of %s \"%s\". No '=' symbol. Ignoring line...\n",
              lineno, gbls->conf->config_file_path, q);
      continue;
    }
    
    p = q;

    /* seperate entry name and value */
    do {
      if (*p == ' ' || *p == '=') {
        *p = '\0';
        break;
      }
    } while (p++ < line + sizeof(line));
    
    /* check if entry exists */
    int i, l, ret;
    l = conf_get_num_items();
    ret = -1;
    for (i = 0; i < l; i++) {
      if (!strcasecmp(config[i].name, q)) {
        ret = 0;
        break;
      }
    }
    if (ret == -1) {
      fprintf(stderr, "Parse Error: Line %i of %s \"%s\". Entry name is invalid. Ignoring line...\n",
              lineno, gbls->conf->config_file_path, q);
      continue;
    }

    /* move to value */
    p++;
    do {
      if (*p != ' ' && *p != '=')
        break;
    } while (p++ < line + sizeof(line));

    /* type specific */
    switch (config[i].type)
    {
      case SD_CONFIG_VALUE_TYPE_STRING:
      case SD_CONFIG_VALUE_TYPE_STRING_BOOLEAN:
        {
        /* dealing with a string */
        if (*p == '\"') {
          p++;

          char *e;
          /* remove end */
          if (!(e = strrchr(p, '\"'))) {
            fprintf(stderr, "Parse Error: Line %i of %s \"%s\". Could not find tailing \". "
                    "Ignoring line...\n", lineno, gbls->conf->config_file_path, q);
            continue;
          }

          *e = '\0';
        }

        if (config[i].type == SD_CONFIG_VALUE_TYPE_STRING_BOOLEAN) {
          int b;
          if ((b = get_boolean_id_from_string(p)) == -1) {
            fprintf(stderr, "Parse Error: Line %i of %s \"%s\". Invalid value, must be boolean. "
                    "Ignoring line...\n", lineno, gbls->conf->config_file_path, q);
            continue;
          }
          *((char *)config[i].value) = (char) b;
        
        }
        else {
          //printf("before strncpy:\nname = %s\nvalue = %s\np = %s\n length = %i\n",
          //    config[i].name, (char *) config[i].value, p, config[i].length);
          strncpy(config[i].value, p, config[i].length);
          ((char *)config[i].value)[config[i].length - 1] = '\0';
        }

        }
        break;
      case SD_CONFIG_VALUE_TYPE_INT:
        if (sscanf(p, "%i", (int *)config[i].value) != 1) {
          fprintf(stderr, "Parse Error: Line %i of %s \"%s\". Invalid Value. Ignoring line...\n",
                  lineno, gbls->conf->config_file_path, q);
          continue;
        }

        break;
    }

    nvalid++;
  }

  return nvalid;
}


// vim:ts=2:expandtab
