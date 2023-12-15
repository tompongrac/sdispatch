/*
   GTK+ UI menubar & toolbar
 
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
#include "sd_gtk.h"
#include "sd_globals.h"
#include "sd_ui.h"
#include "sd_net.h"
#include "sd_gtk_peers.h"


G_MODULE_EXPORT void 
quit_menuitem_activate_cb(GtkObject *object, gpointer user_data)
{
  /* break out of main loop */
  gtk_main_quit();
}

G_MODULE_EXPORT void 
start_server_menuitem_activate_cb(GtkObject *object, gpointer user_data)
{
  gtk_set_toggle_button_active_name("server_type_control_radiobutton", TRUE);

  gtk_widget_show_name("start_server_dialog");
}

G_MODULE_EXPORT void 
stop_server_menuitem_activate_cb(GtkObject *object, gpointer user_data)
{
#if 0
  switch (gbls->net->ctl_con_server.state)
  {
    case SERVER_STATE_CLOSED:
      ui_sd_err("No server to stop");
      return;
    case SERVER_STATE_RESOLVE_IP:
      ui_sd_err("You have to wait till address resolution is complete before stopping.");
      return;
    case SERVER_STATE_LISTENING:
      break;
  }

  if (server_deinit(&gbls->net->ctl_con_server) == -1) {
    ui_notify("Server stopped with errors.");
  }
  else {
    ui_notify("Server stopped successfully.");
  }
#endif

}

G_MODULE_EXPORT void 
connect_menuitem_activate_cb(GtkObject *object, gpointer user_data)
{
  gtk_widget_show_name("connect_dialog");
}

G_MODULE_EXPORT void 
about_menuitem_activate_cb(GtkObject *object, gpointer user_data)
{
  gtk_widget_show_name("about_dialog");
}

G_MODULE_EXPORT void 
configure_close_button_clicked_cb(GtkObject *object, gpointer user_data)
{
  gtk_widget_hide_name("configure_dialog");
}

G_MODULE_EXPORT void 
configure_toolbutton_clicked_cb(GtkObject *object, gpointer user_data)
{
  gtk_widget_show_name("configure_dialog");
}


// vim:ts=2:expandtab
