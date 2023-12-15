/*
   GTK+ UI callbacks
 
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
#include <stdlib.h>
#include "sd_gtk_ui_cb.h"
#include "sd_gtk.h"
#include "sd_gtk_peers.h"
#include "sd_ui.h"
#include "sd_globals.h"
#include "sd_gtk_connection.h"
#include "sd_gtk_suggest_files.h"
#include "sd_dynamic_memory.h"
#include "sd_error.h"
#include "sd_net.h"
#include "sd_gtk_unapproved_transfers.h"
#include "sd_gtk_approved_transfers.h"
#include "sd_gtk_servers.h"

void gtk_ui_init(void)
{
  GtkWidget *w;

  gtk_init(0, NULL);

  sd_gtkbuilder = gtk_builder_new();

  /* find the file */
  char *fpath;

  if (file_exists(SD_GTK_BUILDER_FILE_1) == SD_OPTION_ON)
    fpath = SD_GTK_BUILDER_FILE_1;
#ifdef WIN32
#else
  else if(file_exists(SD_GTK_BUILDER_FILE_2) == SD_OPTION_ON)
    fpath = SD_GTK_BUILDER_FILE_2;
#endif
  else
    ON_ERROR_EXIT("Could not find ui.xml, exiting...");

  gtk_builder_add_from_file(sd_gtkbuilder, fpath, NULL);



  gtk_builder_connect_signals(sd_gtkbuilder, NULL);
  

  /* init dialogs */
  start_server_dialog_init();
  connect_dialog_init();
  init_server_security_settings_dialog();
  init_client_security_settings_dialog();
  init_incoming_transfer_security_settings_dialog();
  gtk_init_suggest_files_dialog();
  gtk_init_peers_treeview();
  gtk_init_transfers_treeview();
  init_file_suggest_security_settings_dialog();
  gtk_init_servers_dialog();
  transfer_settings_dialog_init();

  /* init data transfer treeviews */
  gtk_init_data_transfers_unapproved_treeview();
  gtk_init_data_transfers_approved_treeview();
  
  /* init statusbar */
  w = GTK_WIDGET(gtk_builder_get_object(sd_gtkbuilder, "main_statusbar"));
  statusbar_context_id = gtk_statusbar_get_context_id(
      GTK_STATUSBAR(w), "Statusbar for main window.");

  /* initial state */
  gtk_ui_state_set(SERVER_STATE_CLOSED, SERVER_STATE_CLOSED);
}

void gtk_ui_begin(void)
{
  gtk_widget_show_name("main_window");

#ifdef WIN32
  /* g_idle_add causes the file chooser to lag on windows */
  gtk_timeout_add(15, &gtk_ui_idle_cb, NULL);

#else
#if 0
  g_idle_add(&gtk_ui_idle_cb, NULL);
#else
  gtk_timeout_add(15, &gtk_ui_idle_cb, NULL);
#endif
#endif

  gtk_main();
}

void gtk_ui_deinit(void)
{
  g_object_unref(G_OBJECT(sd_gtkbuilder));
}

void gtk_ui_idle(void)
{
  ui_idle();

  /* update gtk */
  //gtk_update_data_transfers_unapproved_treeview();
}

/* ------- mutex protected functions begin ------- */
void gtk_ui_sys_err(int n, const char *f, const char *m)
{
  char *b;
  char *t = get_time_string();
  int l = strlen(m) + 128;
  SAFE_CALLOC(b, 1, l);

  snprintf(b, l, "%s %s - %s [%i]", t, f, m, n);
  linked_list_add(&gbls->ui->status_backlog, b);
  SAFE_FREE(t);
}

void gtk_ui_sock_err(int n, const char *f, const char *m)
{
  char *b;
  char *t = get_time_string();
  int l = 128;
#ifndef WIN32
  l += strlen(m);
#endif
  SAFE_CALLOC(b, 1, l);

#ifdef WIN32
  snprintf(b, l, "%s %s [%i]", t, f, n);
#else
  snprintf(b, l, "%s %s - %s [%i]", t, f, m, n);
#endif
  linked_list_add(&gbls->ui->status_backlog, b);
  SAFE_FREE(t);
}

void gtk_ui_gai_err(int n, const char *f, const char *m)
{
  char *t = get_time_string();
  char *b;
  int l = strlen(m) + 128;
  SAFE_CALLOC(b, 1, l);

  snprintf(b, l, "%s %s - %s [%i]", t, f, m, n);
  linked_list_add(&gbls->ui->status_backlog, b);
  SAFE_FREE(t);
}

void gtk_ui_sd_err(const char *m)
{
  char *t = get_time_string();
  char *b;
  int l = strlen(m) + 128;
  SAFE_CALLOC(b, 1, l);
  
  snprintf(b, l, "%s %s", t, m);
  linked_list_add(&gbls->ui->status_backlog, b);
  SAFE_FREE(t);
}

void gtk_ui_ssl_err(const char *f, const char *m)
{
  char *t = get_time_string();
  char *b;
  int l = strlen(m) + 128;
  SAFE_CALLOC(b, 1, l);

  snprintf(b, l, "%s %s - %s", t, f, m);
  linked_list_add(&gbls->ui->status_backlog, b);
  SAFE_FREE(t);
}

void gtk_ui_wsa_err(int n, const char *f)
{
  char *t = get_time_string();
  char *b;
  int l = 128;
  SAFE_CALLOC(b, 1, l);

  snprintf(b, l, "%s %s [%i]", t, f, n);
  linked_list_add(&gbls->ui->status_backlog, b);
  SAFE_FREE(t);
}

void gtk_ui_notify(const char *m)
{
  char *t = get_time_string();
  char *b;
  int l = strlen(m) + 128;
  SAFE_CALLOC(b, 1, l);

  snprintf(b, l, "%s %s", t, m);
  linked_list_add(&gbls->ui->status_backlog, b);
  SAFE_FREE(t);
}
/* ------- mutex protected functions end ------- */

void gtk_ui_state_set(int os, int ns)
{
#if 0
  int cns;
  GtkWidget *w;
  char b[256];

  w = GTK_WIDGET(gtk_builder_get_object(sd_gtkbuilder, "main_statusbar"));
  gtk_statusbar_pop(GTK_STATUSBAR(w), statusbar_context_id);
  cns = gbls->net->ctl_con_server.state;


  switch (cns)
  {
    case SERVER_STATE_CLOSED:
    case SERVER_STATE_RESOLVE_IP:
      snprintf(b, sizeof(b), "P2P mode, %i peer(s)", get_est_peer_count());
      gtk_statusbar_push(GTK_STATUSBAR(w), statusbar_context_id, b);
      break;
    case SERVER_STATE_LISTENING:
      snprintf(b, sizeof(b), "P2P mode with Server, %i peer(s)",
          get_est_peer_count());
      gtk_statusbar_push(GTK_STATUSBAR(w), statusbar_context_id, b);
      break;
  }
#endif

}

void gtk_ui_data_transfer_change(struct sd_data_transfer_info *dti,
    char change_type)
{
  switch (dti->verdict)
  {
    case DATA_TRANSFER_VERDICT_PENDING:
    case DATA_TRANSFER_VERDICT_DECLINDED:
      switch (change_type)
      {
        case SD_DATA_TRANSFER_CHANGE_TYPE_ADD:
          treeview_add_unapproved_transfer_treeview(dti);
          break;
        case SD_DATA_TRANSFER_CHANGE_TYPE_REMOVE:
          treeview_remove_unapproved_transfer_treeview(dti);
          break;
        case SD_DATA_TRANSFER_CHANGE_TYPE_UPDATE:
          treeview_update_unapproved_transfer_treeview(dti);
          break;
      }
      break;
    case DATA_TRANSFER_VERDICT_ACCEPTED:
      switch (change_type)
      {
        case SD_DATA_TRANSFER_CHANGE_TYPE_ADD:
          treeview_add_approved_transfer_treeview(dti);
          break;
        case SD_DATA_TRANSFER_CHANGE_TYPE_REMOVE:
          treeview_remove_approved_transfer_treeview(dti);
          break;
        case SD_DATA_TRANSFER_CHANGE_TYPE_UPDATE:
          treeview_update_approved_transfer_treeview(dti);
          break;
      }
      break;
  }
}

void gtk_ui_data_transfer_accepted(struct sd_data_transfer_info *dti)
{
  treeview_remove_unapproved_transfer_treeview(dti);
  treeview_add_approved_transfer_treeview(dti);
}

void gtk_ui_data_transfer_progress_change(struct sd_data_transfer_info *dti)
{
  treeview_update_approved_transfer_progress_treeview(dti);
}

int gtk_ui_process_status_message(void *v, int i)
{
  gtk_add_line_to_text_view_name("status_textview",
      (const char *)v,
      SD_OPTION_ON);

  return 0;
}

// vim:ts=2:expandtab
