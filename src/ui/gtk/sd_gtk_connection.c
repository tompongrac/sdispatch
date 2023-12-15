/*
   GTK+ UI control connections setup
 
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
#include "sd_gtk_connection.h"
#include "sd_gtk.h"
#include "sd_globals.h"
#include "sd_error.h"
#include "sd_net.h"
#include "sd_dynamic_memory.h"

/* ---------- server [begin] ---------- */
G_MODULE_EXPORT void 
start_server_cancel_button_clicked_cb(GtkObject *object, gpointer user_data)
{
  gtk_widget_hide_name("start_server_dialog");
}

void start_server_dialog_init()
{
  gtk_entry_set_text_name("server_ip_entry",
      gbls->conf->control_server_net_address);
  gtk_entry_set_text_name("server_port_entry",
      gbls->conf->control_server_service);
}

G_MODULE_EXPORT void 
start_server_button_clicked_cb(GtkObject *object, gpointer user_data)
{
  char *ip, *port;
  char essl;
  
  /* ssl verify info begin */
  char *cert, *prkey, vp, uc, type;
  int vdepth;

  uc = gtk_toggle_button_get_active_name("server_security_settings_use_certificate_togglebutton");
  cert = gtk_get_filechooser_filename("server_security_settings_cert_filechooserbutton");
  prkey = gtk_get_filechooser_filename("server_security_settings_prkey_filechooserbutton");
  vp = gtk_toggle_button_get_active_name("server_security_settings_verify_peer_togglebutton");
  vdepth =
    gtk_get_spinbutton_int_value_name("server_security_settings_verify_depth_spinbutton");

  if (gtk_toggle_button_get_active_name("server_type_control_radiobutton"))
    type = SERVER_TYPE_CONTROL;
  else
    type = SERVER_TYPE_DATA;

  struct sd_ssl_verify_info vi;
  ssl_set_verify_info(&vi, uc, cert, prkey,
      vp, vdepth, NULL, SSL_HANDSHAKE_ACTION_ACCEPT);
  
  g_free(cert);
  g_free(prkey);
  /* ssl verify info end */

  ip = gtk_entry_get_text_name("server_ip_entry");
  port = gtk_entry_get_text_name("server_port_entry");
  essl = (char) gtk_toggle_button_get_active_name("server_ssl_togglebutton");

  /* store into server */
  struct sd_serv_info *si;
  si = server_init(ip, port, essl, &vi, type);
  server_start_state_machine(si, SERVER_STATE_RESOLVE_IP, SD_OPTION_ON);
  
  gtk_widget_hide_name("start_server_dialog");

}

G_MODULE_EXPORT void 
server_security_settings_cancel_button_clicked_cb(GtkObject *object, gpointer user_data)
{
  gtk_widget_hide_name("server_security_settings_dialog");
}

G_MODULE_EXPORT void 
server_ssl_togglebutton_toggled_cb(GtkObject *object, gpointer user_data)
{
  if (gtk_toggle_button_get_active_name("server_ssl_togglebutton") == TRUE)
    gtk_widget_set_sensitive_name("server_security_settings_button", TRUE);
  else
    gtk_widget_set_sensitive_name("server_security_settings_button", FALSE);

}

G_MODULE_EXPORT void 
server_security_settings_use_certificate_togglebutton_toggled_cb
(GtkObject *object, gpointer user_data)
{
  if (gtk_toggle_button_get_active_name("server_security_settings_use_certificate_togglebutton")
          == TRUE)
  {
    gtk_widget_set_sensitive_name("server_security_settings_cert_filechooserbutton", TRUE);
    gtk_widget_set_sensitive_name("server_security_settings_prkey_filechooserbutton", TRUE);
  }
  else
  {
    gtk_widget_set_sensitive_name("server_security_settings_cert_filechooserbutton", FALSE);
    gtk_widget_set_sensitive_name("server_security_settings_prkey_filechooserbutton", FALSE);
  }
}

G_MODULE_EXPORT void 
server_security_settings_button_clicked_cb(GtkObject *object, gpointer user_data)
{
  gtk_widget_show_name("server_security_settings_dialog");
}

G_MODULE_EXPORT void 
server_security_settings_verify_peer_togglebutton_toggled_cb
(GtkObject *object, gpointer user_data)
{
  if (gtk_toggle_button_get_active_name("server_security_settings_verify_peer_togglebutton")
          == TRUE)
  {
    gtk_widget_set_sensitive_name("server_security_settings_verify_depth_spinbutton", TRUE);
  }
  else
  {
    gtk_widget_set_sensitive_name("server_security_settings_verify_depth_spinbutton", FALSE);
  }
}

G_MODULE_EXPORT void 
server_security_settings_ok_button_clicked_cb(GtkObject *object, gpointer user_data)
{

  gtk_widget_hide_name("server_security_settings_dialog");
}
/* ---------- server [end] ---------- */

/* ---------- client [begin] ---------- */
G_MODULE_EXPORT void 
connect_cancel_button_clicked_cb(GtkObject *object, gpointer user_data)
{
  gtk_widget_hide_name("connect_dialog");
}

void connect_dialog_init()
{
  gtk_entry_set_text_name("connect_ip_entry",
      gbls->conf->control_client_net_address);
  gtk_entry_set_text_name("connect_port_entry",
      gbls->conf->control_server_service);
}

G_MODULE_EXPORT void 
connect_button_clicked_cb(GtkObject *object, gpointer user_data)
{
  char *ip, *port;
  char essl;
  
  /* verify info start */
  struct sd_ssl_verify_info vi;
  char *cert, *prkey, vp, uc;
  int vdepth;

  uc = gtk_toggle_button_get_active_name("client_security_settings_use_certificate_togglebutton");
  cert = gtk_get_filechooser_filename("client_security_settings_cert_filechooserbutton");
  prkey = gtk_get_filechooser_filename("client_security_settings_prkey_filechooserbutton");
  vp = gtk_toggle_button_get_active_name("client_security_settings_verify_peer_togglebutton");
  vdepth =
    gtk_get_spinbutton_int_value_name("client_security_settings_verify_depth_spinbutton");
  ssl_set_verify_info(&vi, uc, cert, prkey, (char) vp, vdepth, NULL, SSL_HANDSHAKE_ACTION_CONNECT);

  g_free(cert);
  g_free(prkey);
  /* verify info end */
  
  essl = (char) gtk_toggle_button_get_active_name("client_ssl_togglebutton");
  ip = gtk_entry_get_text_name("connect_ip_entry");
  port = gtk_entry_get_text_name("connect_port_entry");


  struct sd_peer_info *pi;

  pi = peer_init(ip, port, essl, &vi);
  con_start_state_machine(&pi->ctl_con, CON_STATE_RESOLVE_DST_IP, SD_OPTION_ON);
  
  gtk_widget_hide_name("connect_dialog");
}

G_MODULE_EXPORT void 
client_security_settings_cancel_button_clicked_cb(GtkObject *object, gpointer user_data)
{
  gtk_widget_hide_name("client_security_settings_dialog");
}

G_MODULE_EXPORT void 
client_ssl_togglebutton_toggled_cb(GtkObject *object, gpointer user_data)
{
  if (gtk_toggle_button_get_active_name("client_ssl_togglebutton") == TRUE)
    gtk_widget_set_sensitive_name("client_security_settings_button", TRUE);
  else
    gtk_widget_set_sensitive_name("client_security_settings_button", FALSE);
}

G_MODULE_EXPORT void 
client_security_settings_button_clicked_cb(GtkObject *object, gpointer user_data)
{
  gtk_widget_show_name("client_security_settings_dialog");
}

G_MODULE_EXPORT void 
client_security_settings_verify_peer_togglebutton_toggled_cb
(GtkObject *object, gpointer user_data)
{
  if (gtk_toggle_button_get_active_name("client_security_settings_verify_peer_togglebutton")
          == TRUE)
  {
    gtk_widget_set_sensitive_name("client_security_settings_verify_depth_spinbutton", TRUE);
  }
  else
  {
    gtk_widget_set_sensitive_name("client_security_settings_verify_depth_spinbutton", FALSE);
  }
}

G_MODULE_EXPORT void 
client_security_settings_use_certificate_togglebutton_toggled_cb
(GtkObject *object, gpointer user_data)
{
  if (gtk_toggle_button_get_active_name("client_security_settings_use_certificate_togglebutton")
          == TRUE)
  {
    gtk_widget_set_sensitive_name("client_security_settings_cert_filechooserbutton", TRUE);
    gtk_widget_set_sensitive_name("client_security_settings_prkey_filechooserbutton", TRUE);
  }
  else
  {
    gtk_widget_set_sensitive_name("client_security_settings_cert_filechooserbutton", FALSE);
    gtk_widget_set_sensitive_name("client_security_settings_prkey_filechooserbutton", FALSE);
  }
}

G_MODULE_EXPORT void 
client_security_settings_ok_button_clicked_cb(GtkObject *object, gpointer user_data)
{
  gtk_widget_hide_name("client_security_settings_dialog");
}

/* ---------- client [end] ---------- */

void init_server_security_settings_dialog()
{
  gtk_set_toggle_button_active_name("server_security_settings_use_certificate_togglebutton",
      gbls->conf->ssl_verify.ssl_use_cert);
  gtk_set_filechooser_filename("server_security_settings_cert_filechooserbutton",
      gbls->conf->ssl_verify.ssl_cert_path);
  gtk_set_filechooser_filename("server_security_settings_prkey_filechooserbutton",
      gbls->conf->ssl_verify.ssl_pr_key_path);
  gtk_set_toggle_button_active_name("server_security_settings_verify_peer_togglebutton",
      gbls->conf->ssl_verify.ssl_verify_peer);
  gtk_set_spinbutton_int_value_name("server_security_settings_verify_depth_spinbutton",
      gbls->conf->ssl_verify.ssl_verify_depth);
}

void init_client_security_settings_dialog()
{
  gtk_set_toggle_button_active_name("client_security_settings_use_certificate_togglebutton",
      gbls->conf->ssl_verify.ssl_use_cert);
  gtk_set_filechooser_filename("client_security_settings_cert_filechooserbutton",
      gbls->conf->ssl_verify.ssl_cert_path);
  gtk_set_filechooser_filename("client_security_settings_prkey_filechooserbutton",
      gbls->conf->ssl_verify.ssl_pr_key_path);
  gtk_set_toggle_button_active_name("client_security_settings_verify_peer_togglebutton",
      gbls->conf->ssl_verify.ssl_verify_peer);
  gtk_set_spinbutton_int_value_name("client_security_settings_verify_depth_spinbutton",
      gbls->conf->ssl_verify.ssl_verify_depth);

}



// vim:ts=2:expandtab
