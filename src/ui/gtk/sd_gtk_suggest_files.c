/*
   GTK+ UI suggest files
 
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

#include <inttypes.h>

#include <gtk/gtk.h>
#include "sd_ui.h"
#include "sd_gtk.h"
#include "sd_globals.h"
#include "sd_error.h"
#include "sd_net.h"
#include "sd_ui.h"
#include "sd_dynamic_memory.h"
#include "sd_gtk_suggest_files.h"
#include "sd_protocol.h"
#include "sd_protocol_commands.h"

GtkListStore *added_files_list_store;
GtkListStore *peers_combobox_list_store;

G_MODULE_EXPORT void 
file_suggest_cancel_button_clicked_cb(GtkObject *object, gpointer user_data)
{
  gtk_widget_hide_name("suggest_files_dialog");
}

G_MODULE_EXPORT void 
add_toolbutton_clicked_cb(GtkObject *object, gpointer user_data)
{
  /* reset exisitng files */
  gtk_list_store_clear(added_files_list_store);
  gtk_update_peers_combobox();
  gtk_update_listen_addresses_combobox();
  gtk_widget_show_name("suggest_files_dialog");
}

G_MODULE_EXPORT void 
listen_local_address_togglebutton_toggled_cb(GtkObject *object, gpointer user_data)
{
#if 0
  GtkWidget *w;
  w = GTK_WIDGET(gtk_builder_get_object(sd_gtkbuilder,
        "listen_local_address_togglebutton"));
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w))) {
    gtk_widget_set_sensitive_name("listen_address_select_combobox", TRUE);
  }
  else {
    gtk_widget_set_sensitive_name("listen_address_select_combobox", FALSE);
  }
#endif
}

/* -------- security settings begin -------- */
G_MODULE_EXPORT void 
transfer_security_settings_cancel_button_clicked_cb(GtkObject *object, gpointer user_data)
{
  gtk_widget_hide_name("transfer_security_settings_dialog");
}

G_MODULE_EXPORT void 
transfer_ssl_togglebutton_toggled_cb(GtkObject *object, gpointer user_data)
{
  if (gtk_toggle_button_get_active_name("transfer_ssl_togglebutton") == TRUE)
    gtk_widget_set_sensitive_name("transfer_security_settings_button", TRUE);
  else
    gtk_widget_set_sensitive_name("transfer_security_settings_button", FALSE);

}

G_MODULE_EXPORT void 
transfer_security_settings_use_certificate_togglebutton_toggled_cb
(GtkObject *object, gpointer user_data)
{
  if (gtk_toggle_button_get_active_name("transfer_security_settings_use_certificate_togglebutton")
          == TRUE)
  {
    gtk_widget_set_sensitive_name("transfer_security_settings_cert_filechooserbutton", TRUE);
    gtk_widget_set_sensitive_name("transfer_security_settings_prkey_filechooserbutton", TRUE);
  }
  else
  {
    gtk_widget_set_sensitive_name("transfer_security_settings_cert_filechooserbutton", FALSE);
    gtk_widget_set_sensitive_name("transfer_security_settings_prkey_filechooserbutton", FALSE);
  }
}

G_MODULE_EXPORT void 
transfer_security_settings_button_clicked_cb(GtkObject *object, gpointer user_data)
{
  gtk_widget_show_name("transfer_security_settings_dialog");
}

G_MODULE_EXPORT void 
transfer_security_settings_verify_peer_togglebutton_toggled_cb
(GtkObject *object, gpointer user_data)
{
  if (gtk_toggle_button_get_active_name("transfer_security_settings_verify_peer_togglebutton")
          == TRUE)
  {
    gtk_widget_set_sensitive_name("transfer_security_settings_verify_depth_spinbutton", TRUE);
  }
  else
  {
    gtk_widget_set_sensitive_name("transfer_security_settings_verify_depth_spinbutton", FALSE);
  }
}

G_MODULE_EXPORT void 
transfer_security_settings_ok_button_clicked_cb(GtkObject *object, gpointer user_data)
{

  gtk_widget_hide_name("transfer_security_settings_dialog");
}

void init_file_suggest_security_settings_dialog()
{
  gtk_set_toggle_button_active_name("transfer_security_settings_use_certificate_togglebutton",
      gbls->conf->ssl_verify.ssl_use_cert);
  gtk_set_filechooser_filename("transfer_security_settings_cert_filechooserbutton",
      gbls->conf->ssl_verify.ssl_cert_path);
  gtk_set_filechooser_filename("transfer_security_settings_prkey_filechooserbutton",
      gbls->conf->ssl_verify.ssl_pr_key_path);
  gtk_set_toggle_button_active_name("transfer_security_settings_verify_peer_togglebutton",
      gbls->conf->ssl_verify.ssl_verify_peer);
  gtk_set_spinbutton_int_value_name("transfer_security_settings_verify_depth_spinbutton",
      gbls->conf->ssl_verify.ssl_verify_depth);

}

/* -------- security settings end -------- */


G_MODULE_EXPORT void 
active_radiobutton_toggled_cb(GtkObject *object, gpointer user_data)
{
  gtk_widget_set_sensitive_name("passive_method_connect_ip_address_entry", FALSE);
  gtk_widget_set_sensitive_name("passive_method_connect_port_entry", FALSE);
  gtk_widget_set_sensitive_name("active_method_connect_ip_address_entry", TRUE);
  gtk_widget_set_sensitive_name("active_method_connect_wan_ip_address_entry", TRUE);
  gtk_widget_set_sensitive_name("listen_address_select_combobox", FALSE);
  //gtk_widget_set_sensitive_name("listen_local_address_togglebutton", FALSE);
  gtk_widget_set_sensitive_name("listen_address_select_combobox", FALSE);
  gtk_widget_set_sensitive_name("passive_allow_any_port_checkbutton", FALSE);

  gtk_widget_set_sensitive_name("transfer_ssl_togglebutton", TRUE);
  transfer_ssl_togglebutton_toggled_cb(NULL, NULL);
}
G_MODULE_EXPORT void 
passive_radiobutton_toggled_cb(GtkObject *object, gpointer user_data)
{
  gtk_widget_set_sensitive_name("passive_method_connect_ip_address_entry", TRUE);
  gtk_widget_set_sensitive_name("passive_method_connect_port_entry", TRUE);
  gtk_widget_set_sensitive_name("active_method_connect_ip_address_entry", FALSE);
  gtk_widget_set_sensitive_name("active_method_connect_wan_ip_address_entry", FALSE);
  gtk_widget_set_sensitive_name("listen_address_select_combobox", TRUE);
  //gtk_widget_set_sensitive_name("listen_local_address_togglebutton", TRUE);
  gtk_widget_set_sensitive_name("listen_address_select_combobox", TRUE);
  gtk_widget_set_sensitive_name("passive_allow_any_port_checkbutton", TRUE);

  gtk_widget_set_sensitive_name("transfer_ssl_togglebutton", FALSE);
  gtk_widget_set_sensitive_name("transfer_security_settings_button", FALSE);
}

void gtk_init_listen_addresses_combobox()
{
  GtkWidget *w;
  GtkCellRenderer *renderer;
  GtkListStore *ls;

  w = GTK_WIDGET(gtk_builder_get_object(sd_gtkbuilder,
        "listen_address_select_combobox"));
  renderer = gtk_cell_renderer_text_new();

  gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(w), renderer, TRUE);
  gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(w), renderer, "text", 0, NULL);

  ls = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_POINTER);
  gtk_combo_box_set_model(GTK_COMBO_BOX(w), GTK_TREE_MODEL(ls));
}


void gtk_init_suggest_files_dialog(void)
{
  GtkWidget *w;
  w = GTK_WIDGET(gtk_builder_get_object(sd_gtkbuilder, "suggest_files_dialog"));
  gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(w), TRUE);
  gtk_init_added_files_treeview();
  gtk_init_peers_combobox();
  gtk_init_listen_addresses_combobox("listen_address_select_combobox");

  gtk_set_toggle_button_active_name("listen_local_address_togglebutton", TRUE);
  gtk_set_toggle_button_active_name("active_radiobutton", TRUE);
  gtk_set_toggle_button_active_name("transfer_ssl_togglebutton", FALSE);

  /* set deafault textboxes */
  gtk_entry_set_text_name("active_method_connect_ip_address_entry",
      gbls->conf->data_local_net_address);
  gtk_entry_set_text_name("active_method_connect_wan_ip_address_entry",
      gbls->conf->data_wide_net_address);
  gtk_entry_set_text_name("passive_method_connect_ip_address_entry",
      gbls->conf->data_wide_net_address);
  gtk_entry_set_text_name("passive_method_connect_port_entry",
      gbls->conf->data_wide_service);
}

G_MODULE_EXPORT void 
refresh_select_peer_button_clicked_cb(GtkObject *object, gpointer user_data)
{
  gtk_update_peers_combobox();
  gtk_update_listen_addresses_combobox();
}

void gtk_init_added_files_treeview(void)
{
  GtkWidget *w;
  GtkCellRenderer *renderer;

  w = GTK_WIDGET(gtk_builder_get_object(sd_gtkbuilder, "added_files_treeview"));
  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(
      GTK_TREE_VIEW(w), -1, "File", renderer, "text", 0, NULL);

  /* create model */
  added_files_list_store = gtk_list_store_new(1, G_TYPE_STRING);
  gtk_tree_view_set_model(GTK_TREE_VIEW(w), GTK_TREE_MODEL(added_files_list_store));

  gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(w)),
      GTK_SELECTION_MULTIPLE);
}

void gtk_init_peers_combobox(void)
{
  GtkWidget *w;
  GtkCellRenderer *renderer;

  w = GTK_WIDGET(gtk_builder_get_object(sd_gtkbuilder, "peer_select_combobox"));
  renderer = gtk_cell_renderer_text_new();

  gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(w), renderer, TRUE);
  gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(w), renderer, "text", 0, NULL);

  peers_combobox_list_store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_POINTER);
  gtk_combo_box_set_model(GTK_COMBO_BOX(w), GTK_TREE_MODEL(peers_combobox_list_store));
}

static int gtk_combobox_add_listen_address_iter_cb(void *v, int i)
{
  char *saddr;
  GtkWidget *w;
  GtkTreeIter iter;
  GtkTreeModel *m;
  

  if (((struct sd_serv_info *)v)->state == SERVER_STATE_LISTENING)
  {

    if (((struct sd_serv_info *)v)->type == CON_TYPE_DATA)
    {
      w = GTK_WIDGET(gtk_builder_get_object(sd_gtkbuilder,
            "listen_address_select_combobox"));
      m = gtk_combo_box_get_model(GTK_COMBO_BOX(w));

      saddr = get_sockaddr_storage_string(
        (struct sockaddr_storage *)((struct sd_serv_info *)v)->servinfo.ai_addr
          );

      gtk_list_store_append(GTK_LIST_STORE(m), &iter);
      gtk_list_store_set(GTK_LIST_STORE(m), &iter, 0, saddr,
          1, (gpointer)v, -1);

      SAFE_FREE(saddr);
    }
  }
  return 0;
}

/* used for peer list in combobox */
static int gtk_combobox_add_peer_iter_cb(void *v, int i)
{
  char *saddr;
  GtkWidget *w;
  GtkTreeIter iter;

  if (((struct sd_peer_info *)(v))->ctl_con.state == CON_STATE_ESTABLISHED)
  {

    w = GTK_WIDGET(gtk_builder_get_object(sd_gtkbuilder, "peer_select_combobox"));
    saddr = get_sockaddr_storage_string(
      &(((struct sd_peer_info *)(v))->ctl_con.dst_sa)
        );

    if (saddr != NULL) {
      gtk_list_store_append(peers_combobox_list_store, &iter);
      //gtk_combo_box_append_text(GTK_COMBO_BOX(w), saddr);
      gtk_list_store_set(peers_combobox_list_store, &iter, 0, saddr,
          1, (gpointer)v, -1);
      SAFE_FREE(saddr);
    }
  }
  return 0;
}

void gtk_update_listen_addresses_combobox()
{
  GtkWidget *w;
  GtkTreeModel *m;
  
  w = GTK_WIDGET(gtk_builder_get_object(sd_gtkbuilder,
        "listen_address_select_combobox"));
  m = gtk_combo_box_get_model(GTK_COMBO_BOX(w));
  gtk_list_store_clear(GTK_LIST_STORE(m));

  linked_list_iterate(&gbls->net->con_servers, gtk_combobox_add_listen_address_iter_cb);
}

void gtk_update_peers_combobox(void)
{
  GtkWidget *w;
  GtkTreeModel *m;
  
  w = GTK_WIDGET(gtk_builder_get_object(sd_gtkbuilder, "peer_select_combobox"));
  m = gtk_combo_box_get_model(GTK_COMBO_BOX(w));
  gtk_list_store_clear(GTK_LIST_STORE(m));

  linked_list_iterate(&gbls->net->peers, gtk_combobox_add_peer_iter_cb);
}


G_MODULE_EXPORT void 
add_file_toolbutton_clicked_cb(GtkObject *object, gpointer user_data)
{
  GtkTreeIter titer;
  GtkWidget *w;
  GSList *files;
  GSList *iter;
  int nfiles;

  w = GTK_WIDGET(gtk_builder_get_object(sd_gtkbuilder, "suggest_files_dialog"));
  files = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER (w));
  nfiles = 0;

  for (iter = files; iter != NULL; iter = iter->next, nfiles++)
  {
    gtk_list_store_append(added_files_list_store, &titer);
    gtk_list_store_set(added_files_list_store, &titer, 0, iter->data, -1);
  }

  g_slist_free(files);
  
  if (!nfiles)
    ui_sd_err("No files selected.");
}


G_MODULE_EXPORT void 
remove_file_toolbutton_clicked_cb(GtkObject *object, gpointer user_data)
{
  if (!gtk_remove_selected_treeview_entries_name("added_files_treeview"))
    ui_sd_err("No files selected.");
}

G_MODULE_EXPORT void 
suggest_button_clicked_cb(GtkObject *object, gpointer user_data)
{
  /* server */
  char lla;
  char aap;
  struct sd_serv_info *si;

  /* command args begin */
  char *na_str = NULL;
  char *ns_str = NULL;
  char *nwa_str = NULL;
  char *nws_str = NULL;

  char c_cm;

  /* ssl */
  char c_essl;
  char *cert, *prkey, vp, uc;
  int vdepth;
  struct sd_ssl_verify_info vi, *vi_ptr;
  /* command args end */

  /* error checking */
  struct sd_peer_info *pi;

  pi = (struct sd_peer_info *)gtk_get_pointer_combobox_selected("peer_select_combobox");
  
  if (pi == NULL) {
    ui_sd_err("No peer selected.");
    return;
  }
  else if (pi->ctl_con.state == CON_STATE_CLOSED) {
    ui_sd_err("This connection is not active.");
    return;
  }
  else {
  }

  /* using an active method */
  if (gtk_toggle_button_get_active_name("active_radiobutton"))
    c_cm = CON_METH_ACTIVE;
  else
    c_cm = CON_METH_PASSIVE;

  switch (c_cm)
  {
    case CON_METH_PASSIVE:
      {
      /* listening on local address */
      if ((lla = gtk_toggle_button_get_active_name("listen_local_address_togglebutton")))
      {
        si = (struct sd_serv_info *)
          gtk_get_pointer_combobox_selected("listen_address_select_combobox");

        if (si == NULL) {
          ui_sd_err("If listening on local address, must select server.");
          return;
        }
        else if (si->state != SERVER_STATE_LISTENING) {
          ui_sd_err("The selected listening address must be in listening state.");
          return;
        }

      } 
      else
      {
      }

      /* setup wan info for peer to connect to */
      nwa_str = gtk_entry_get_text_name("passive_method_connect_ip_address_entry");
      nws_str = gtk_entry_get_text_name("passive_method_connect_port_entry");
      
      aap = gtk_toggle_button_get_active_name("passive_allow_any_port_checkbutton");

      c_essl = 0;
      vi_ptr = NULL;
      }
    break;
  case CON_METH_ACTIVE:
    {
    /* setup wan info for peer to connect to */
    na_str = gtk_entry_get_text_name("active_method_connect_ip_address_entry");
    nwa_str = gtk_entry_get_text_name("active_method_connect_wan_ip_address_entry");

    /* get verify info */
    c_essl = gtk_toggle_button_get_active_name("transfer_ssl_togglebutton");
    uc = gtk_toggle_button_get_active_name("transfer_security_settings_use_certificate_togglebutton");
    cert = gtk_get_filechooser_filename("transfer_security_settings_cert_filechooserbutton");
    prkey = gtk_get_filechooser_filename("transfer_security_settings_prkey_filechooserbutton");
    vp = gtk_toggle_button_get_active_name("transfer_security_settings_verify_peer_togglebutton");
    vdepth =
      gtk_get_spinbutton_int_value_name("transfer_security_settings_verify_depth_spinbutton");
    ssl_set_verify_info(&vi, uc, cert, prkey, (char) vp, vdepth, NULL, SSL_HANDSHAKE_ACTION_CONNECT);

    vi_ptr = &vi;
  
    g_free(cert);
    g_free(prkey);
    }
    break;
  }



  /* for each selected file */
  GtkTreeIter iter;
  GtkTreeModel *m;
  GtkWidget *w;
  char *filepath;
  struct file_info fi;

  w = GTK_WIDGET(gtk_builder_get_object(sd_gtkbuilder, "added_files_treeview"));
  m = gtk_tree_view_get_model(GTK_TREE_VIEW(w));
  
  if (gtk_tree_model_get_iter_first(m, &iter) == FALSE) {
    ui_sd_err("No files to suggest.");
    return;
  }

  struct sd_data_transfer_info *dti;
  
  for (;;)
  {
    gtk_tree_model_get(m, &iter, 0, &filepath, -1);

    if (file_set_info(filepath, &fi) != -1)
    {
      dti = data_transfer_init(
          pi,
          c_essl,
          vi_ptr, /* if NULL we can set later */
          &fi, DATA_TRANSFER_DIRECTION_OUTGOING);

      data_transfer_set_id(dti, DATA_TRANSFER_DIRECTION_OUTGOING, 0);
      data_transfer_set_wan(dti, nwa_str, nws_str);
      
      dti->peer_using_ssl = c_essl;

      switch (c_cm)
      {
        case CON_METH_PASSIVE:
          data_transfer_set_passive(dti, lla, si, aap);
          file_suggest_command_pack_and_send(dti);
          break;
        case CON_METH_ACTIVE:
          data_transfer_set_active(dti, na_str, ns_str);
          data_transfer_setup_active_resolve_source(dti);
          break;
      }

    }

    if (gtk_tree_model_iter_next(m, &iter) == FALSE)
      break;
  }
  

  gtk_widget_hide_name("suggest_files_dialog");
  
}


// vim:ts=2:expandtab
