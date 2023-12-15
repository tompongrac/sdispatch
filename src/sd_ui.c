/*
   User interface operations
 
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

#ifdef WIN32
#include <windows.h>
#endif

#include <stdlib.h>
#include "sd_ui.h"
#include "sd_globals.h"
#include "sd_error.h"
#include "sd_logging.h"

extern void register_gtk_ui(void);

void ui_init(void)
{
  if (gbls->ui->initialized)
    ON_ERROR_EXIT("ui_init called when allready init.");
  
  (*gbls->ui->init)();
  sd_thread_init(&gbls->ui->mutex.cs_mutex);
  linked_list_init(&gbls->ui->status_backlog);
  gbls->ui->initialized = 1;
}

void ui_begin(void)
{
  if (gbls->ui->initialized)
    (*gbls->ui->begin)();
  else
    ON_ERROR_EXIT("ui_begin called before init.");
}

void ui_deinit(void)
{
  if (!gbls->ui->initialized)
    ON_ERROR_EXIT("ui_deinit called without init.");
  
  (*gbls->ui->deinit)();
  sd_thread_deinit(&gbls->ui->mutex.cs_mutex);
  gbls->ui->initialized = 0;
}

/* ------- these must be thread safe begin -----*/
void ui_sys_err(int n, const char *f)
{
  sd_cs_lock(&gbls->ui->mutex.cs_mutex);

  (*gbls->ui->sys_err)(n, f, strerror(n));

  sd_cs_unlock(&gbls->ui->mutex.cs_mutex);
}
void ui_sock_err(const char *f)
{
  sd_cs_lock(&gbls->ui->mutex.cs_mutex);
#ifdef WIN32
  (*gbls->ui->sock_err)(WSAGetLastError(), f, NULL);
#else
  (*gbls->ui->sock_err)(errno, f, strerror(errno));
#endif
  sd_cs_unlock(&gbls->ui->mutex.cs_mutex);
}
void ui_gai_err(int n, const char *f)
{
  sd_cs_lock(&gbls->ui->mutex.cs_mutex);

  (*gbls->ui->gai_err)(n, f, gai_strerror(n));

  sd_cs_unlock(&gbls->ui->mutex.cs_mutex);
}
void ui_sd_err(const char *m)
{
  sd_cs_lock(&gbls->ui->mutex.cs_mutex);

  (*gbls->ui->sd_err)(m);

  sd_cs_unlock(&gbls->ui->mutex.cs_mutex);
}
void ui_ssl_err(const char *f)
{
  sd_cs_lock(&gbls->ui->mutex.cs_mutex);

  (*gbls->ui->ssl_err)(f, ERR_error_string(ERR_get_error(), NULL));

  sd_cs_unlock(&gbls->ui->mutex.cs_mutex);
}
void ui_wsa_err(const char *f)
{
  sd_cs_lock(&gbls->ui->mutex.cs_mutex);

#if WIN32
  (*gbls->ui->wsa_err)(WSAGetLastError(), f);
#endif

  sd_cs_unlock(&gbls->ui->mutex.cs_mutex);
}
void ui_notify(const char *m)
{
  sd_cs_lock(&gbls->ui->mutex.cs_mutex);

  (*gbls->ui->notify)(m);

  sd_cs_unlock(&gbls->ui->mutex.cs_mutex);
}
int ui_notify_printf(const char *m, ...)
{
  int ret;
  va_list args;
  char b[strlen(m) + 512];
  
  sd_cs_lock(&gbls->ui->mutex.cs_mutex);

  va_start(args, m);

  ret = vsnprintf(b, sizeof b, m, args);

  va_end(args);

  (*gbls->ui->notify)(b);

  sd_cs_unlock(&gbls->ui->mutex.cs_mutex);

  return ret;
}
/* ------- these must be thread safe end -----*/

void ui_state_set(int os, int ns)
{
  if (gbls->ui->initialized)
    (*gbls->ui->state_set)(os, ns);
}


void set_gtk_ui(void)
{
  register_gtk_ui();
}

void ui_data_transfer_change(struct sd_data_transfer_info *dti,
    char changetype)
{
  gbls->ui->data_transfer_change(dti, changetype);
}

void ui_data_transfer_accepted(struct sd_data_transfer_info *dti)
{
  gbls->ui->data_transfer_accepted(dti);
}

void ui_data_transfer_progress_change(struct sd_data_transfer_info *dti)
{
  gbls->ui->data_transfer_progress_change(dti);
}

int ui_process_status_backlog()
{
  /* print to ui */
  linked_list_iterate(&gbls->ui->status_backlog,
      gbls->ui->process_status_message);
  /* print to log */
  if (gbls->conf->logging_enabled == SD_OPTION_ON) {
    linked_list_iterate(&gbls->ui->status_backlog,
        &logging_print_line_iter);
  }

  int n;
  n = linked_list_rem_all_entries(&gbls->ui->status_backlog, 1);

  return n;
}

// vim:ts=2:expandtab
