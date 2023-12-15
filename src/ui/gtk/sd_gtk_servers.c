/*
   GTK+ UI servers
 
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
#include "sd_ui.h"
#include "sd_gtk.h"
#include "sd_globals.h"
#include "sd_error.h"
#include "sd_net.h"
#include "sd_dynamic_memory.h"
#include "sd_gtk_servers.h"

G_MODULE_EXPORT void 
servers_close_button_clicked_cb(GtkObject *object, gpointer user_data)
{
  gtk_widget_hide_name("servers_dialog");
}

G_MODULE_EXPORT void 
servers_toolbutton_clicked_cb(GtkObject *object, gpointer user_data)
{
  gtk_update_servers_treeview();
  gtk_widget_show_name("servers_dialog");
}

G_MODULE_EXPORT void 
refresh_servers_button_clicked_cb(GtkObject *object, gpointer user_data)
{
  gtk_update_servers_treeview();
}

G_MODULE_EXPORT void
servers_treeview_cursor_changed_cb(GtkObject *object, gpointer user_data)
{
  gtk_update_server_information();
}

G_MODULE_EXPORT void
stop_server_button_clicked_cb(GtkObject *object, gpointer user_data)
{
  struct sd_serv_info *si;

  si = (struct sd_serv_info *) gtk_get_pointer_selected("servers_treeview", 1);
  if (si == NULL)
    ui_sd_err("No server selected.");
  else if (si->state == SERVER_STATE_CLOSED)
    ui_sd_err("This server is allready closed.");
  else
  {
    server_close(si);
    char *saddr;
    saddr = get_sockaddr_storage_string((struct sockaddr_storage *) si->servinfo.ai_addr);
    ui_notify_printf("Server %s was closed.", saddr);
    SAFE_FREE(saddr);
  }
    
}

G_MODULE_EXPORT void
restart_server_button_clicked_cb(GtkObject *object, gpointer user_data)
{
  struct sd_serv_info *si;

  si = (struct sd_serv_info *) gtk_get_pointer_selected("servers_treeview", 1);
  if (si == NULL)
    ui_sd_err("No server selected.");
  else if (si->state != SERVER_STATE_CLOSED)
    ui_sd_err("Server must be in CLOSED state before restarting.");
  else
  {
    server_start_state_machine(si, SERVER_STATE_RESOLVE_IP, SD_OPTION_ON);
  }
    
}

void gtk_init_servers_dialog(void)
{
  gtk_init_servers_treeview();
}

void gtk_init_servers_treeview(void)
{
  GtkWidget *w;
  GtkCellRenderer *renderer;
  GtkListStore *ls;


  w = GTK_WIDGET(gtk_builder_get_object(sd_gtkbuilder, "servers_treeview"));
  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(
      GTK_TREE_VIEW(w), -1, "Server", renderer, "text", 0, NULL);

  /* create model */
  ls = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_POINTER);
  gtk_tree_view_set_model(GTK_TREE_VIEW(w), GTK_TREE_MODEL(ls));

  gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(w)),
      GTK_SELECTION_SINGLE);
}

int add_server_to_treeview_cb(void *v, int i)
{
  char *saddr;
  GtkTreeIter iter;
  GtkWidget *w;
  GtkTreeModel *m;

  w = GTK_WIDGET(gtk_builder_get_object(sd_gtkbuilder, "servers_treeview"));
  m = gtk_tree_view_get_model(GTK_TREE_VIEW(w));

  saddr = get_sockaddr_storage_string(
    (struct sockaddr_storage *)(((struct sd_serv_info *)v)->servinfo.ai_addr)
      );

  /* add socket addr */
  /* add reference */
  gtk_list_store_append(GTK_LIST_STORE(m), &iter);
  gtk_list_store_set(GTK_LIST_STORE(m), &iter, 0, saddr,
          1, (gpointer)v, -1);
  
  SAFE_FREE(saddr);
  return 0;
}

void gtk_update_servers_treeview(void)
{
  GtkWidget *w;
  GtkTreeModel *m;

  w = GTK_WIDGET(gtk_builder_get_object(sd_gtkbuilder, "servers_treeview"));
  m = gtk_tree_view_get_model(GTK_TREE_VIEW(w));

  gtk_list_store_clear(GTK_LIST_STORE(m));
  linked_list_iterate(&gbls->net->con_servers, add_server_to_treeview_cb);

  /* update info */
  gtk_update_server_information();
}


void gtk_update_server_information(void)
{
  GtkWidget *w;
  struct sd_serv_info *si;

  w = GTK_WIDGET(gtk_builder_get_object(sd_gtkbuilder, "servers_treeview"));

  /* clear buffer */
  gtk_clear_all_text_view_name("servers_textview");

  si = (struct sd_serv_info *) gtk_get_pointer_selected("servers_treeview", 1);
  
  if (si != NULL)
  {
    char b[2048];

    /* show server info */
    gtk_add_line_to_text_view_name("servers_textview", 
        "------------------ Begin Server Information ----------------", SD_OPTION_OFF);

    /* current state */
    char *state;
    switch (si->state)
    {
      case SERVER_STATE_CLOSED: state = "CLOSED"; break;
      case SERVER_STATE_RESOLVE_IP: state = "RESOLVING IP"; break;
      case SERVER_STATE_LISTENING: state = "LISTENING"; break;
    }
    snprintf(b, sizeof(b), "State: %s", state);
    gtk_add_line_to_text_view_name("servers_textview", b, SD_OPTION_OFF);
    
    /* type */
    char *type;
    switch (si->type)
    {
      case SERVER_TYPE_CONTROL: type = "Control"; break;
      case SERVER_TYPE_DATA: type = "Data"; break;
    }
    snprintf(b, sizeof(b), "Type: %s", type);
    gtk_add_line_to_text_view_name("servers_textview", b, SD_OPTION_OFF);

    /* listening fd */
    snprintf(b, sizeof(b), "File descriptor: %i",
        si->list_sock_fd);
    gtk_add_line_to_text_view_name("servers_textview", b, SD_OPTION_OFF);
    
    /* ssl enabled */
    snprintf(b, sizeof(b), "SSL enabled: %s", si->enable_ssl == SD_OPTION_ON ?
        "Yes" : "No");
    gtk_add_line_to_text_view_name("servers_textview", b, SD_OPTION_OFF);


    gtk_add_line_to_text_view_name("peer_textview", 
        "------------------- End Server Information -----------------", SD_OPTION_OFF);
    
    
  }
  else
  {
    /* no row is selected */
  }
}


// vim:ts=2:expandtab
