/*
   GTK+ UI unapproved transfers
 
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
#include <inttypes.h>

#include "sd_ui.h"
#include "sd_gtk.h"
#include "sd_globals.h"
#include "sd_error.h"
#include "sd_dynamic_memory.h"
#include "sd_gtk_unapproved_transfers.h"
#include "sd_protocol.h"
#include "sd_protocol_commands.h"
#include "sd_file.h"

/* popup menu begin */
static struct sd_data_transfer_info *popup_menu_unapproved_transfer_selected;

enum
{
  COL_ID = 0,
  COL_DIRECTION,
  COL_PEER,
  COL_FILENAME,
  COL_FILE_SIZE,
  COL_FILE_MOD_TIME,
  COL_VERDICT,
  COL_STATE,
  COL_USE_SSL,
  COL_CON_METH,
  COL_NET_ADDR,
  COL_NET_SERV,
  COL_FILE_DIR,
  COL_POINTER,
  NUM_COLS
};  

void gtk_init_data_transfers_unapproved_treeview(void)
{
  GtkWidget *w;
  GtkCellRenderer *renderer;
  GtkListStore *ls;

  w = GTK_WIDGET(gtk_builder_get_object(sd_gtkbuilder,
        "data_transfers_unapproved_treeview"));

  /* --- id ---*/
  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(
      GTK_TREE_VIEW(w), -1, "ID", renderer, "text", COL_ID, NULL);

  /* --- direction ---*/
  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(
      GTK_TREE_VIEW(w), -1, "Dir", renderer, "text", COL_DIRECTION, NULL);
  
  /* --- peer ---*/
  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(
      GTK_TREE_VIEW(w), -1, "Peer", renderer, "text", COL_PEER, NULL);
  
  /* --- filename ---*/
  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(
      GTK_TREE_VIEW(w), -1, "Filename", renderer, "text", COL_FILENAME, NULL);
  
  /* --- file size ---*/
  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(
      GTK_TREE_VIEW(w), -1, "Size", renderer, "text", COL_FILE_SIZE, NULL);
  
  /* --- mod time ---*/
  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(
      GTK_TREE_VIEW(w), -1, "Mod Time", renderer, "text", COL_FILE_MOD_TIME, NULL);
  
  /* --- verdict ---*/
  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(
      GTK_TREE_VIEW(w), -1, "Verdict", renderer, "text", COL_VERDICT, NULL);
  
  /* --- state ---*/
  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(
      GTK_TREE_VIEW(w), -1, "State", renderer, "text", COL_STATE, NULL);
  
  /* --- use ssl ---*/
  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(
      GTK_TREE_VIEW(w), -1, "SSL", renderer, "text", COL_USE_SSL, NULL);
  
  /* --- connection method ---*/
  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(
      GTK_TREE_VIEW(w), -1, "Method", renderer, "text", COL_CON_METH, NULL);
  
  /* --- net address ---*/
  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(
      GTK_TREE_VIEW(w), -1, "Address", renderer, "text", COL_NET_ADDR, NULL);
  
  /* --- net service ---*/
  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(
      GTK_TREE_VIEW(w), -1, "Service", renderer, "text", COL_NET_SERV, NULL);
  
  /* --- file dir ---*/
  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(
      GTK_TREE_VIEW(w), -1, "Directory", renderer, "text", COL_FILE_DIR, NULL);

  /* create model */
  ls = gtk_list_store_new(NUM_COLS,
      /* cols */
      G_TYPE_UINT64,
      G_TYPE_STRING,
      G_TYPE_STRING,
      G_TYPE_STRING,
      G_TYPE_UINT64,
      G_TYPE_STRING,
      G_TYPE_STRING,
      G_TYPE_STRING,
      G_TYPE_STRING,
      G_TYPE_STRING,
      G_TYPE_STRING,
      G_TYPE_STRING,
      G_TYPE_STRING,
      G_TYPE_POINTER
      );
  gtk_tree_view_set_model(GTK_TREE_VIEW(w), GTK_TREE_MODEL(ls));

  gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(w)),
      GTK_SELECTION_SINGLE);
}
void set_data_transfer_unapproved_list_store_row(
    struct sd_data_transfer_info *dti, GtkListStore *ls, GtkTreeIter *ti)
{
  char *s;
  char *v;
  char *saddr;
  char *dir;
  char *cm;
  char *essl;

  s = get_data_transfer_state_string(dti);
  v = get_data_transfer_verdict_string(dti);
  saddr = get_sockaddr_storage_string(&dti->parent_peer->ctl_con.dst_sa);
  dir = get_data_transfer_direction_arrow_string(dti);
  cm = get_data_transfer_con_method_string(dti);
  essl = get_boolean_string(dti->data_con.enable_ssl);

  gtk_list_store_set(ls, ti,
      COL_ID, dti->id,
      COL_DIRECTION, dir,
      COL_PEER, saddr,
      COL_FILENAME, dti->file.name,
      COL_FILE_SIZE, dti->file.size,
      COL_FILE_MOD_TIME, dti->file.modtime,
      COL_VERDICT, v,
      COL_STATE, s,
      COL_USE_SSL, essl,
      COL_CON_METH, cm,
      COL_NET_ADDR, dti->data_con.resolve_dst_addr.lookup_address,
      COL_NET_SERV, dti->data_con.resolve_dst_addr.lookup_service,
      COL_FILE_DIR, dti->file.directory,
      COL_POINTER, dti,
      
      -1);

  SAFE_FREE(s);
  SAFE_FREE(v);
  SAFE_FREE(saddr);
  SAFE_FREE(dir);
  SAFE_FREE(cm);
  SAFE_FREE(essl);
}

int treeview_add_unapproved_transfer_treeview_iter_cb(void *v, int i)
{
  GtkTreeIter iter;
  GtkWidget *w;
  GtkTreeModel *m;

  struct sd_data_transfer_info *dti;

  w = GTK_WIDGET(gtk_builder_get_object(sd_gtkbuilder,
        "data_transfers_unapproved_treeview"));
  m = gtk_tree_view_get_model(GTK_TREE_VIEW(w));
  dti = (struct sd_data_transfer_info *)v;

  /* add if unapproved */
  gtk_list_store_append(GTK_LIST_STORE(m), &iter);
  set_data_transfer_unapproved_list_store_row(dti, GTK_LIST_STORE(m), &iter);

  return 0;
}

/* add to treeview based on pointer */
void treeview_add_unapproved_transfer_treeview(struct sd_data_transfer_info *dti)
{
  GtkTreeIter iter;
  GtkWidget *w;
  GtkTreeModel *m;

  w = GTK_WIDGET(gtk_builder_get_object(sd_gtkbuilder,
        "data_transfers_unapproved_treeview"));
  m = gtk_tree_view_get_model(GTK_TREE_VIEW(w));

  gtk_list_store_append(GTK_LIST_STORE(m), &iter);
  set_data_transfer_unapproved_list_store_row(dti, GTK_LIST_STORE(m), &iter);
}

void treeview_remove_unapproved_transfer_treeview(struct sd_data_transfer_info *dti)
{
  GtkWidget *w;
  GtkTreeModel *m;
  GtkTreeIter iter;
  gboolean  valid;
  void *ptr;

  w = GTK_WIDGET(gtk_builder_get_object(sd_gtkbuilder,
        "data_transfers_unapproved_treeview"));
  m = gtk_tree_view_get_model(GTK_TREE_VIEW(w));

  valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(m), &iter);

  while (valid)
  {
    gtk_tree_model_get(m, &iter, COL_POINTER, &ptr, -1);

    /* we have a match */
    if (ptr == dti)
    {
      gtk_list_store_remove(GTK_LIST_STORE(m), &iter);
      break;
    }
    
    valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(m), &iter);
  }

}

void treeview_update_unapproved_transfer_treeview(struct sd_data_transfer_info *dti)
{
  GtkWidget *w;
  GtkTreeModel *m;
  GtkTreeIter iter;
  gboolean  valid;
  void *ptr;

  w = GTK_WIDGET(gtk_builder_get_object(sd_gtkbuilder,
        "data_transfers_unapproved_treeview"));
  m = gtk_tree_view_get_model(GTK_TREE_VIEW(w));

  valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(m), &iter);

  while (valid)
  {
    gtk_tree_model_get(m, &iter, COL_POINTER, &ptr, -1);

    /* we have a match */
    if (ptr == dti)
    {
      set_data_transfer_unapproved_list_store_row(dti, GTK_LIST_STORE(m), &iter);
      break;
    }
    
    valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(m), &iter);
  }
}

int treeview_peers_for_unapproved_transfer_treeview_iter_cb(void *v, int i)
{
  linked_list_iterate(&((struct sd_peer_info *)v)->data_transfers,
      &treeview_add_unapproved_transfer_treeview_iter_cb);
  return 0;
}

/* inefficient removes and readd's all to list */
void gtk_update_data_transfers_unapproved_treeview(void)
{
  GtkWidget *w;
  GtkTreeModel *m;

  w = GTK_WIDGET(gtk_builder_get_object(sd_gtkbuilder,
        "data_transfers_unapproved_treeview"));
  m = gtk_tree_view_get_model(GTK_TREE_VIEW(w));

  gtk_list_store_clear(GTK_LIST_STORE(m));
  linked_list_iterate(&gbls->net->peers,
      treeview_peers_for_unapproved_transfer_treeview_iter_cb);
}


G_MODULE_EXPORT void data_transfers_unapproved_edit_settings_menuitem_activate_cb(
    GtkObject *object, gpointer user_data)
{
  if (popup_menu_unapproved_transfer_selected->direction ==
      DATA_TRANSFER_DIRECTION_OUTGOING)
  {
    /* in case its already open */
    gtk_widget_hide_name("transfer_settings_dialog");
    ui_sd_err("You must edit settings for outgoing transfers from the "
        "suggest file dialog.");
  }
  else
  {
    transfer_settings_dialog_spawn();
    gtk_widget_show_name("transfer_settings_dialog");
  }
}

/* transfer ACCEPTED */
G_MODULE_EXPORT void data_transfers_unapproved_verdict_accept_menuitem_activate_cb(
    GtkObject *object, gpointer user_data)
{
  if (popup_menu_unapproved_transfer_selected->direction ==
      DATA_TRANSFER_DIRECTION_OUTGOING) {
    ui_sd_err("Only the peer can verify outgoing transfers.");
  }
  else {
    /* continue configuring */
    if (popup_menu_unapproved_transfer_selected->state ==
        DATA_TRANSFER_STATE_SETUP_PENDING)
    {
      switch (popup_menu_unapproved_transfer_selected->con_meth)
      {
        case CON_METH_PASSIVE:
          data_transfer_setup_passive_accept(popup_menu_unapproved_transfer_selected);
          break;
        case CON_METH_ACTIVE:
          data_transfer_setup_active_resolve_source(popup_menu_unapproved_transfer_selected);
          break;
      }
    }
    else
    {
      ui_sd_err("This transfer is not waiting for verification.");
    }
  }

}

/* transfer DECLINED */
G_MODULE_EXPORT void data_transfers_unapproved_verdict_decline_menuitem_activate_cb(
    GtkObject *object, gpointer user_data)
{
  if (popup_menu_unapproved_transfer_selected->direction ==
      DATA_TRANSFER_DIRECTION_OUTGOING) {
    ui_sd_err("Only the peer can verify outgoing transfers.");
  }
  else {
    if (popup_menu_unapproved_transfer_selected->state ==
        DATA_TRANSFER_STATE_SETUP_PENDING)
    {
      file_verdict_command_pack_and_send(popup_menu_unapproved_transfer_selected,
          DATA_TRANSFER_VERDICT_DECLINDED);
    }
  }
}

/* abort */
G_MODULE_EXPORT void data_transfers_unapproved_abort_menuitem_activate_cb(
    GtkObject *object, gpointer user_data)
{
  data_transfer_abort(popup_menu_unapproved_transfer_selected);
}

/* clear from treeview */
G_MODULE_EXPORT void data_transfers_unapproved_clear_menuitem_activate_cb(
    GtkObject *object, gpointer user_data)
{
  ui_data_transfer_change(popup_menu_unapproved_transfer_selected, SD_DATA_TRANSFER_CHANGE_TYPE_REMOVE);
}

/* right click popup menu */
G_MODULE_EXPORT gboolean data_transfers_unapproved_treeview_button_press_event_cb(
    GtkWidget *treeview, GdkEventButton *event, gpointer userdata)
{
  //GtkTreeSelection *s;
  GtkWidget *w, *w_tv;

  struct sd_data_transfer_info *dti;

  w = GTK_WIDGET(gtk_builder_get_object(sd_gtkbuilder,
        "data_transfers_unapproved_menu"));
  w_tv = GTK_WIDGET(gtk_builder_get_object(sd_gtkbuilder,
        "data_transfers_unapproved_treeview"));

  if (event->type == GDK_BUTTON_PRESS && event->button == 3)
  {
    /* make sure we get new right click row */
    GtkTreePath *path;
    GtkTreeSelection *selection;

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(w_tv));

    if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(w_tv),
                                     (gint) event->x, 
                                     (gint) event->y,
                                     &path, NULL, NULL, NULL))
    {
      gtk_tree_selection_unselect_all(selection);
      gtk_tree_selection_select_path(selection, path);
      gtk_tree_path_free(path);
    }

    /* get row id */
    dti = (struct sd_data_transfer_info *) gtk_get_pointer_selected(
        "data_transfers_unapproved_treeview", COL_POINTER);

    if (dti != NULL)
    {

      popup_menu_unapproved_transfer_selected = dti;
      /* set values appropriatly */
      gtk_widget_show_name("data_transfers_unapproved_menu");

      gtk_menu_popup(GTK_MENU(w), NULL, NULL, NULL, NULL,
                   event != NULL ? event->button : 0,
                   gdk_event_get_time((GdkEvent*)event)); 


      /* we handled event */
      return TRUE;
    }
  }
  /* we did not handle event */
   return FALSE;
}


/* popup menu end */

/* --------- file transfer settings dialog begin ----------*/
G_MODULE_EXPORT void 
transfer_settings_cancel_button_clicked_cb(GtkObject *object, gpointer user_data)
{
  gtk_widget_hide_name("transfer_settings_dialog");
}
G_MODULE_EXPORT void 
transfer_settings_passive_method_listen_local_address_togglebutton_toggled_cb(
    GtkObject *object, gpointer user_data)
{
  if (gtk_toggle_button_get_active_name(
        "transfer_settings_passive_method_listen_local_address_togglebutton")) {
    gtk_widget_set_sensitive_name(
        "transfer_settings_passive_method_listen_address_select_combobox", TRUE);
    gtk_widget_set_sensitive_name("transfer_settings_passive_allow_any_port_checkbutton", TRUE);
  }
  else {
    gtk_widget_set_sensitive_name(
        "transfer_settings_passive_method_listen_address_select_combobox", FALSE);
    gtk_widget_set_sensitive_name("transfer_settings_passive_allow_any_port_checkbutton", FALSE);
  }
}

void transfer_settings_dialog_init()
{
  gtk_init_transfer_settings_listen_addresses_combobox();
}

/* -------- security settings begin -------- */
G_MODULE_EXPORT void 
incoming_transfer_security_settings_cancel_button_clicked_cb(GtkObject *object, gpointer user_data)
{
  gtk_widget_hide_name("incoming_transfer_security_settings_dialog");
}

G_MODULE_EXPORT void 
incoming_transfer_ssl_togglebutton_toggled_cb(GtkObject *object, gpointer user_data)
{
  if (gtk_toggle_button_get_active_name("incoming_transfer_ssl_togglebutton") == TRUE)
    gtk_widget_set_sensitive_name("incoming_transfer_security_settings_button", TRUE);
  else
    gtk_widget_set_sensitive_name("incoming_transfer_security_settings_button", FALSE);

}

G_MODULE_EXPORT void 
incoming_transfer_security_settings_use_certificate_togglebutton_toggled_cb
(GtkObject *object, gpointer user_data)
{
  if (gtk_toggle_button_get_active_name("incoming_transfer_security_settings_use_certificate_togglebutton")
          == TRUE)
  {
    gtk_widget_set_sensitive_name("incoming_transfer_security_settings_cert_filechooserbutton", TRUE);
    gtk_widget_set_sensitive_name("incoming_transfer_security_settings_prkey_filechooserbutton", TRUE);
  }
  else
  {
    gtk_widget_set_sensitive_name("incoming_transfer_security_settings_cert_filechooserbutton", FALSE);
    gtk_widget_set_sensitive_name("incoming_transfer_security_settings_prkey_filechooserbutton", FALSE);
  }
}

G_MODULE_EXPORT void 
incoming_transfer_security_settings_button_clicked_cb(GtkObject *object, gpointer user_data)
{
  gtk_widget_show_name("incoming_transfer_security_settings_dialog");
}

G_MODULE_EXPORT void 
incoming_transfer_security_settings_verify_peer_togglebutton_toggled_cb
(GtkObject *object, gpointer user_data)
{
  if (gtk_toggle_button_get_active_name("incoming_transfer_security_settings_verify_peer_togglebutton")
          == TRUE)
  {
    gtk_widget_set_sensitive_name("incoming_transfer_security_settings_verify_depth_spinbutton", TRUE);
  }
  else
  {
    gtk_widget_set_sensitive_name("incoming_transfer_security_settings_verify_depth_spinbutton", FALSE);
  }
}

G_MODULE_EXPORT void 
incoming_transfer_security_settings_ok_button_clicked_cb(GtkObject *object, gpointer user_data)
{

  gtk_widget_hide_name("incoming_transfer_security_settings_dialog");
}

void init_incoming_transfer_security_settings_dialog(void)
{
  GtkWidget *w;
  w = GTK_WIDGET(gtk_builder_get_object(sd_gtkbuilder,
       "transfer_settings_output_directory_filechooserbutton"));
  gtk_file_chooser_set_action(GTK_FILE_CHOOSER(w), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);

  gtk_set_toggle_button_active_name("incoming_transfer_security_settings_use_certificate_togglebutton",
      gbls->conf->ssl_verify.ssl_use_cert);
  gtk_set_filechooser_filename("incoming_transfer_security_settings_cert_filechooserbutton",
      gbls->conf->ssl_verify.ssl_cert_path);
  gtk_set_filechooser_filename("incoming_transfer_security_settings_prkey_filechooserbutton",
      gbls->conf->ssl_verify.ssl_pr_key_path);
  gtk_set_toggle_button_active_name("incoming_transfer_security_settings_verify_peer_togglebutton",
      gbls->conf->ssl_verify.ssl_verify_peer);
  gtk_set_spinbutton_int_value_name("incoming_transfer_security_settings_verify_depth_spinbutton",
      gbls->conf->ssl_verify.ssl_verify_depth);

}

/* -------- security settings end -------- */


void transfer_settings_dialog_spawn()
{
  GtkWidget *w;

  w = GTK_WIDGET(gtk_builder_get_object(sd_gtkbuilder,
        "transfer_settings_dialog"));

  switch (popup_menu_unapproved_transfer_selected->con_meth)
  {
    case CON_METH_ACTIVE:
      gtk_set_toggle_button_active_name("transfer_settings_active_radiobutton", TRUE);
      gtk_entry_set_text_name("transfer_settings_active_method_connect_ip_address_entry",
          popup_menu_unapproved_transfer_selected->data_con.resolve_src_addr.lookup_address);
      gtk_entry_set_text_name("transfer_settings_active_method_connect_wan_ip_address_entry",
          popup_menu_unapproved_transfer_selected->wan_address);

      gtk_widget_set_sensitive_name("transfer_settings_passive_radiobutton", FALSE);
      gtk_widget_set_sensitive_name("transfer_settings_passive_method_connect_ip_address_entry", FALSE);
      gtk_widget_set_sensitive_name("transfer_settings_passive_method_connect_port_entry", FALSE);
      gtk_widget_set_sensitive_name("transfer_settings_passive_method_listen_local_address_togglebutton",
          FALSE);
      gtk_widget_set_sensitive_name("transfer_settings_passive_method_listen_address_select_combobox",
          FALSE);

      gtk_widget_set_sensitive_name("transfer_settings_active_radiobutton", TRUE);
      gtk_widget_set_sensitive_name("transfer_settings_active_method_connect_ip_address_entry", TRUE);
      gtk_widget_set_sensitive_name("transfer_settings_active_method_connect_wan_ip_address_entry", TRUE);

      gtk_widget_set_sensitive_name("incoming_transfer_ssl_togglebutton", TRUE);
      incoming_transfer_ssl_togglebutton_toggled_cb(NULL, NULL);

      break;
    case CON_METH_PASSIVE:
      gtk_set_toggle_button_active_name("transfer_settings_passive_radiobutton", TRUE);
      gtk_set_toggle_button_active_name(
          "transfer_settings_passive_method_listen_local_address_togglebutton", TRUE);
      gtk_entry_set_text_name("transfer_settings_passive_method_connect_ip_address_entry",
          popup_menu_unapproved_transfer_selected->wan_address);
      gtk_entry_set_text_name("transfer_settings_passive_method_connect_port_entry",
          popup_menu_unapproved_transfer_selected->wan_service);
      
      gtk_widget_set_sensitive_name("transfer_settings_passive_radiobutton", TRUE);
      gtk_widget_set_sensitive_name("transfer_settings_passive_method_connect_ip_address_entry", TRUE);
      gtk_widget_set_sensitive_name("transfer_settings_passive_method_connect_port_entry", TRUE);
      gtk_widget_set_sensitive_name("transfer_settings_passive_method_listen_local_address_togglebutton",
          TRUE);
      gtk_widget_set_sensitive_name("transfer_settings_passive_method_listen_address_select_combobox", TRUE);

      gtk_widget_set_sensitive_name("transfer_settings_active_radiobutton", FALSE);
      gtk_widget_set_sensitive_name("transfer_settings_active_method_connect_ip_address_entry", FALSE);
      gtk_widget_set_sensitive_name("transfer_settings_active_method_connect_wan_ip_address_entry", FALSE);
  
      gtk_widget_set_sensitive_name("incoming_transfer_ssl_togglebutton", FALSE);
      gtk_widget_set_sensitive_name("incoming_transfer_security_settings_button", FALSE);
      break;
  }

  /* set output dir */
  if (popup_menu_unapproved_transfer_selected->file.directory[0] != '\0')
  {
    w = GTK_WIDGET(gtk_builder_get_object(sd_gtkbuilder,
        "transfer_settings_output_directory_filechooserbutton"));
    gtk_file_chooser_set_current_folder(
        GTK_FILE_CHOOSER(w),
        popup_menu_unapproved_transfer_selected->file.directory);
  }
  
  gtk_update_transfer_settings_listen_addresses_combobox();
  
}
G_MODULE_EXPORT void transfer_settings_apply_button_clicked_cb(
    GtkObject *object, gpointer user_data)
{
  char lla;
  char aap;
  struct sd_serv_info *si;

  char *na_str = NULL;
  char *ns_str = NULL;
  char *nwa_str = NULL;
  char *nws_str = NULL;
  
  /* ssl */
  char c_essl;
  char *cert, *prkey, *capath, vp, uc;
  int vdepth;
  struct sd_ssl_verify_info vi;

  switch (popup_menu_unapproved_transfer_selected->con_meth)
  {
    case CON_METH_ACTIVE:
      na_str = gtk_entry_get_text_name("transfer_settings_active_method_connect_ip_address_entry");

      data_transfer_set_active(popup_menu_unapproved_transfer_selected, na_str, ns_str);

      /* wan */
      nwa_str = gtk_entry_get_text_name("transfer_settings_active_method_connect_wan_ip_address_entry");


      /* get verify info */
      c_essl = gtk_toggle_button_get_active_name("incoming_transfer_ssl_togglebutton");
      uc = gtk_toggle_button_get_active_name("incoming_transfer_security_settings_use_certificate_togglebutton");
      cert = gtk_get_filechooser_filename("incoming_transfer_security_settings_cert_filechooserbutton");
      prkey = gtk_get_filechooser_filename("incoming_transfer_security_settings_prkey_filechooserbutton");
      capath = gtk_get_filechooser_filename("incoming_transfer_security_settings_ca_path_filechooserbutton");
      vp = gtk_toggle_button_get_active_name("incoming_transfer_security_settings_verify_peer_togglebutton");
      vdepth =
        gtk_get_spinbutton_int_value_name("incoming_transfer_security_settings_verify_depth_spinbutton");

      ssl_set_verify_info(&vi, uc, cert, prkey, (char) vp, vdepth, capath, SSL_HANDSHAKE_ACTION_CONNECT);

      /* ssl */
      if (c_essl) {
        memcpy(&popup_menu_unapproved_transfer_selected->data_con.ssl_verify,
            &vi,
            sizeof vi);
        popup_menu_unapproved_transfer_selected->data_con.enable_ssl = SD_OPTION_ON;
      }
      else {
        popup_menu_unapproved_transfer_selected->data_con.enable_ssl = SD_OPTION_OFF;
      }

      break;
    case CON_METH_PASSIVE:
      /* listening on local address */
      if ((lla = gtk_toggle_button_get_active_name(
              "transfer_settings_passive_method_listen_local_address_togglebutton")))
      {
        si = (struct sd_serv_info *)
          gtk_get_pointer_combobox_selected(
              "transfer_settings_passive_method_listen_address_select_combobox");

        if (si == NULL) {
          ui_sd_err("If listening on local address, must select server.");
          return;
        }
        else if (si->state != SERVER_STATE_LISTENING) {
          ui_sd_err("The selected listening address must be in listening state.");
          return;
        }

      }
      aap = gtk_toggle_button_get_active_name("transfer_settings_passive_allow_any_port_checkbutton");

      data_transfer_set_passive(popup_menu_unapproved_transfer_selected, lla, si, aap);

      /* wan */
      nwa_str = gtk_entry_get_text_name("transfer_settings_passive_method_connect_ip_address_entry");
      nws_str = gtk_entry_get_text_name("transfer_settings_passive_method_connect_port_entry");
      break;
  }
  
  /* setup the wan */
  data_transfer_set_wan(popup_menu_unapproved_transfer_selected, nwa_str, nws_str);


  /* set the file info */
  char *directory;
  directory = gtk_get_filechooser_filename("transfer_settings_output_directory_filechooserbutton");
  snprintf(popup_menu_unapproved_transfer_selected->file.directory,
      sizeof popup_menu_unapproved_transfer_selected->file.directory,
      "%s", directory);

  char *pos = gtk_entry_get_text_name("transfer_settings_resume_position_entry");
  uint64_t ipos;
  if (sscanf(pos, "%"PRIu64, &ipos) != 1) {
    ui_sd_err("File position was not accepted, must be a positive integral.");
  }
  else {
    /* should we reject if too large? */

    popup_menu_unapproved_transfer_selected->file.position = ipos;
  }

  gtk_widget_hide_name("transfer_settings_dialog");
}

void gtk_init_transfer_settings_listen_addresses_combobox()
{
  GtkWidget *w;
  GtkCellRenderer *renderer;
  GtkListStore *ls;

  w = GTK_WIDGET(gtk_builder_get_object(sd_gtkbuilder,
        "transfer_settings_passive_method_listen_address_select_combobox"));
  renderer = gtk_cell_renderer_text_new();

  gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(w), renderer, TRUE);
  gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(w), renderer, "text", 0, NULL);

  ls = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_POINTER);
  gtk_combo_box_set_model(GTK_COMBO_BOX(w), GTK_TREE_MODEL(ls));
}
int gtk_combobox_add_transfer_settings_listen_address_iter_cb(void *v, int i)
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
            "transfer_settings_passive_method_listen_address_select_combobox"));
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

void gtk_update_transfer_settings_listen_addresses_combobox()
{
  GtkWidget *w;
  GtkTreeModel *m;
  
  w = GTK_WIDGET(gtk_builder_get_object(sd_gtkbuilder,
        "transfer_settings_passive_method_listen_address_select_combobox"));
  m = gtk_combo_box_get_model(GTK_COMBO_BOX(w));
  gtk_list_store_clear(GTK_LIST_STORE(m));

  linked_list_iterate(&gbls->net->con_servers,
      gtk_combobox_add_transfer_settings_listen_address_iter_cb);
}

G_MODULE_EXPORT void transfer_settings_refresh_addresses_button_clicked_cb(
    GtkObject *object, gpointer user_data)
{
  gtk_update_transfer_settings_listen_addresses_combobox();
}

G_MODULE_EXPORT void data_transfer_unapproved_scan_button_clicked_cb(
    GtkObject *object, gpointer user_data)
{
  char *filepath;
  uint64_t size;

  char *d = gtk_get_filechooser_filename("transfer_settings_output_directory_filechooserbutton");

  filepath = file_make_full_path(popup_menu_unapproved_transfer_selected->file.name, d);
  
  if (file_get_size(filepath, &size) == -1)
  {
    ui_notify_printf("Could not find %s.", filepath);
    gtk_entry_set_text_name("transfer_settings_resume_position_entry", "0");
  }
  else
  {
    ui_notify_printf("Found %s, updated offset.", filepath);
    char size_str[512];

    snprintf(size_str, sizeof size_str, "%"PRIu64, size);
    gtk_entry_set_text_name("transfer_settings_resume_position_entry",
      size_str);
  }
  
  SAFE_FREE(filepath);
}

/* --------- file transfer settings dialog end ----------*/


// vim:ts=2:expandtab
