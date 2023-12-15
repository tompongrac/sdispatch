/*
   GTK+ UI peers
 
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
#include "sd_net.h"
#include "sd_dynamic_memory.h"
#include "sd_gtk_peers.h"
#include "sd_protocol.h"

GtkListStore *peers_list_store;

G_MODULE_EXPORT void 
peers_close_button_clicked_cb(GtkObject *object, gpointer user_data)
{
  gtk_widget_hide_name("peers_dialog");
}

G_MODULE_EXPORT void 
peers_toolbutton_clicked_cb(GtkObject *object, gpointer user_data)
{
  gtk_update_peers_treeview();
  gtk_widget_show_name("peers_dialog");
}

G_MODULE_EXPORT void 
refresh_peers_button_clicked_cb(GtkObject *object, gpointer user_data)
{
  gtk_update_peers_treeview();
  gtk_update_transfers_treeview();
}

G_MODULE_EXPORT void
peers_treeview_cursor_changed_cb(GtkObject *object, gpointer user_data)
{
  gtk_update_peer_information();
  gtk_update_transfers_treeview();
}


G_MODULE_EXPORT void
disconnect_peer_button_clicked_cb(GtkObject *object, gpointer user_data)
{
  struct sd_peer_info *pi;

  pi = (struct sd_peer_info *) gtk_get_pointer_selected("peers_treeview", 1);
  if (pi == NULL)
    ui_sd_err("No peer selected.");
  else if (pi->ctl_con.state == CON_STATE_CLOSED)
    ui_sd_err("This control connection is allready closed.");
  else
    ctl_con_close(pi);
}

G_MODULE_EXPORT void
message_peer_button_clicked_cb(GtkObject *object, gpointer user_data)
{
  struct sd_peer_info *pi;
  char *msg, *encmsg;
  
  msg = gtk_entry_get_text_name("message_peer_entry");
  if (msg[0] == '\0')
    return;

  pi = (struct sd_peer_info *) gtk_get_pointer_selected("peers_treeview", 1);
  if (pi == NULL) {
    ui_sd_err("No peer selected.");
  }
  else if (pi->ctl_con.state == CON_STATE_CLOSED) {
    ui_sd_err("This connection is not active.");
  }
  else {
    SAFE_CALLOC(encmsg, 1, SD_MAX_PROTOCOL_TALK_MESSAGE_LEN);
    string_url_encode(encmsg, msg, SD_MAX_PROTOCOL_TALK_MESSAGE_LEN);
    send_protocol_command(pi, "TALK", encmsg);
    SAFE_FREE(encmsg);
    ui_notify("Message sent to peer.");
  }
}

void gtk_init_peers_treeview(void)
{
  GtkWidget *w;
  GtkCellRenderer *renderer;
  //GtkTreeModel *model;

  w = GTK_WIDGET(gtk_builder_get_object(sd_gtkbuilder, "peers_treeview"));
  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(
      GTK_TREE_VIEW(w), -1, "Peer", renderer, "text", 0, NULL);

  /* create model */
  peers_list_store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_POINTER);
  gtk_tree_view_set_model(GTK_TREE_VIEW(w), GTK_TREE_MODEL(peers_list_store));

  gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(w)),
      GTK_SELECTION_SINGLE);
}

int add_peer_to_treeview_cb(void *v, int i)
{
  char *saddr;
  GtkTreeIter iter;

  saddr = get_sockaddr_storage_string(
    &(((struct sd_peer_info *)(v))->ctl_con.dst_sa)
      );

  /* add socket addr */
  /* add reference */
  gtk_list_store_append(peers_list_store, &iter);
  if (saddr != NULL) {
    gtk_list_store_set(peers_list_store, &iter, 0, saddr,
        1, (gpointer)v, -1);
    SAFE_FREE(saddr);
  }
  return 0;
}

void gtk_update_peers_treeview(void)
{

  gtk_list_store_clear(peers_list_store);
  linked_list_iterate(&gbls->net->peers, add_peer_to_treeview_cb);

  /* update info */
  gtk_update_peer_information();
}

void gtk_update_peer_information(void)
{
  GtkWidget *w;
  struct sd_peer_info *pi;

  w = GTK_WIDGET(gtk_builder_get_object(sd_gtkbuilder, "peers_treeview"));

  /* clear buffer */
  gtk_clear_all_text_view_name("peer_textview");

  pi = (struct sd_peer_info *) gtk_get_pointer_selected("peers_treeview", 1);

  if (pi != NULL)
  {
    char b[2048];
    char *bptr;

    /* show peer info */
    gtk_add_line_to_text_view_name("peer_textview", 
        "------------------ Begin Control Connection ----------------", SD_OPTION_OFF);
    /* con state */
    char *state;
    state = get_con_state_string(&pi->ctl_con);
    snprintf(b, sizeof(b), "Connection state: %s", state);
    gtk_add_line_to_text_view_name("peer_textview", b, SD_OPTION_OFF);
    SAFE_FREE(state);

    /* endpoint addr */
    bptr = get_sockaddr_storage_string(&pi->ctl_con.dst_sa);
    snprintf(b, sizeof(b), "Remote endpoint address: %s", bptr);
    gtk_add_line_to_text_view_name("peer_textview", b, SD_OPTION_OFF);
    SAFE_FREE(bptr);
    /* ctl fd */
    snprintf(b, sizeof(b), "File descriptor: %i",
        pi->ctl_con.sock_fd);
    gtk_add_line_to_text_view_name("peer_textview", b, SD_OPTION_OFF);
    /* ctl read buffer offset */
    snprintf(b, sizeof(b), "Read buffer offset: %i",
        pi->ctl_buffer_offset);
    gtk_add_line_to_text_view_name("peer_textview", b, SD_OPTION_OFF);
    /* ctl con ssl enabled */
    snprintf(b, sizeof(b), "SSL enabled: %s",
        pi->ctl_con.enable_ssl == SD_OPTION_ON ? "Yes" : "No");
    gtk_add_line_to_text_view_name("peer_textview", b, SD_OPTION_OFF);
    
    /* get info about SSL if enabled */
    if (pi->ctl_con.enable_ssl && pi->ctl_con.ssl != NULL)
    {
      X509 *peercert;
      
      /* ssl cipher */
      snprintf(b, sizeof(b), "SSL session cipher: %s",
          SSL_get_cipher(pi->ctl_con.ssl));
      gtk_add_line_to_text_view_name("peer_textview", b, SD_OPTION_OFF);

      /* have a peer cert */
      peercert = SSL_get_peer_certificate(pi->ctl_con.ssl);
      snprintf(b, sizeof(b), "Recieved a certificate from peer: %s",
          peercert != NULL ? "Yes" : "No");
      gtk_add_line_to_text_view_name("peer_textview", b, SD_OPTION_OFF);
      
      /* display certificate info */
      if (peercert != NULL)
      {
        //char sslb[1024];

        gtk_add_line_to_text_view_name("peer_textview", "", SD_OPTION_OFF);
        gtk_add_line_to_text_view_name("peer_textview", 
            "------------------- Begin Peer Certificate ------------------", SD_OPTION_OFF);
        /* subject name */
        //X509_NAME_get_text_by_NID(X509_get_subject_name(peercert), 0,
        //    sslb, sizeof(sslb));
        snprintf(b, sizeof(b), "Subject: %s",
            X509_NAME_oneline(X509_get_subject_name(peercert), 0, 0));
        gtk_add_line_to_text_view_name("peer_textview", b, SD_OPTION_OFF);
        
        /* issuer name */
        //X509_NAME_get_text_by_NID(X509_get_issuer_name(peercert), 0,
        //    sslb, sizeof(sslb));
        snprintf(b, sizeof(b), "Issuer: %s",
            X509_NAME_oneline(X509_get_issuer_name(peercert), 0, 0));
        gtk_add_line_to_text_view_name("peer_textview", b, SD_OPTION_OFF);

        gtk_add_line_to_text_view_name("peer_textview", 
            "-------------------- End Peer Certificate -------------------", SD_OPTION_OFF);
        gtk_add_line_to_text_view_name("peer_textview", "", SD_OPTION_OFF);

      }
      
    }
    
    gtk_add_line_to_text_view_name("peer_textview", 
        "------------------- End Control Connection -----------------", SD_OPTION_OFF);
    
    
  }
  else
  {
    /* no row is selected */
  }
}

/* ------------------ data transfers ------------------ */

G_MODULE_EXPORT void
transfers_treeview_cursor_changed_cb(GtkObject *object, gpointer user_data)
{
  gtk_update_transfer_information();
}

void gtk_init_transfers_treeview(void)
{
  GtkWidget *w;
  GtkCellRenderer *renderer;
  GtkListStore *ls;

  w = GTK_WIDGET(gtk_builder_get_object(sd_gtkbuilder, "transfers_treeview"));
  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(
      GTK_TREE_VIEW(w), -1, "Tfr", renderer, "text", 0, NULL);

  /* create model */
  ls = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_POINTER);
  gtk_tree_view_set_model(GTK_TREE_VIEW(w), GTK_TREE_MODEL(ls));

  gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(w)),
      GTK_SELECTION_SINGLE);
}

int add_transfer_to_treeview_cb(void *v, int i)
{
  GtkWidget *w;
  GtkTreeModel *m;

  char *str;
  GtkTreeIter iter;

  SAFE_CALLOC(str, 1, 64);

  snprintf(str, 64, "%"PRIu64" %s",
      ((struct sd_data_transfer_info *)v)->id,
      
      (((struct sd_data_transfer_info *)v)->direction == DATA_TRANSFER_DIRECTION_INCOMING ?
      "<--" : "-->"));

  w = GTK_WIDGET(gtk_builder_get_object(sd_gtkbuilder, "transfers_treeview"));
  m = gtk_tree_view_get_model(GTK_TREE_VIEW(w));

  gtk_list_store_append(GTK_LIST_STORE(m), &iter);
  gtk_list_store_set(GTK_LIST_STORE(m), &iter, 0, str,
    1, (gpointer)v, -1);

  SAFE_FREE(str);

  return 0;
}

/* don't use */
int peers_add_transfer_iter_cb(void *v, int index)
{
  linked_list_iterate(&((struct sd_peer_info *)v)->data_transfers,
    add_transfer_to_treeview_cb);
  return 0;
}

void gtk_update_transfers_treeview(void)
{
  GtkWidget *w;
  GtkTreeModel *m;

  w = GTK_WIDGET(gtk_builder_get_object(sd_gtkbuilder, "transfers_treeview"));
  m = gtk_tree_view_get_model(GTK_TREE_VIEW(w));

  gtk_list_store_clear(GTK_LIST_STORE(m));

  struct sd_peer_info *pi;
  pi = (struct sd_peer_info *) gtk_get_pointer_selected("peers_treeview", 1);
  if (pi != NULL)
    linked_list_iterate(&pi->data_transfers, &add_transfer_to_treeview_cb);

}

void gtk_update_transfer_information(void)
{
  GtkWidget *w;
  struct sd_data_transfer_info *dti;

  w = GTK_WIDGET(gtk_builder_get_object(sd_gtkbuilder, "transfers_treeview"));

  /* clear buffer */
  gtk_clear_all_text_view_name("peer_textview");

  dti = (struct sd_data_transfer_info *) gtk_get_pointer_selected("transfers_treeview", 1);

  if (dti != NULL)
  {
    char b[2048];
    char *bptr;

    /* show transfer info */
    gtk_add_line_to_text_view_name("peer_textview",
        "------------------ Begin Data Transfer ----------------", SD_OPTION_OFF);
    
    /* data buffer offset */
    snprintf(b, sizeof(b), "Data buffer offset: %i",
        dti->data_buffer_lower_offset);
    gtk_add_line_to_text_view_name("peer_textview", b, SD_OPTION_OFF);
    
    /* data buffer window size */
    snprintf(b, sizeof(b), "Data buffer window size: %i",
        dti->data_buffer_window_size);
    gtk_add_line_to_text_view_name("peer_textview", b, SD_OPTION_OFF);
    
    /* connection method */
    char *cm;
    cm = get_data_transfer_con_method_string(dti);
    snprintf(b, sizeof(b), "Connection method: %s", cm);
    gtk_add_line_to_text_view_name("peer_textview", b, SD_OPTION_OFF);
    SAFE_FREE(cm);
    
    /* verdict */
    char *verdict;
    verdict = get_data_transfer_verdict_string(dti);
    snprintf(b, sizeof(b), "Verdict: %s", verdict);
    gtk_add_line_to_text_view_name("peer_textview", b, SD_OPTION_OFF);
    SAFE_FREE(verdict);
    
    /* transfer state */
    char *tstate;
    tstate = get_data_transfer_state_string(dti);
    snprintf(b, sizeof(b), "Transfer state: %s", tstate);
    gtk_add_line_to_text_view_name("peer_textview", b, SD_OPTION_OFF);
    SAFE_FREE(tstate);

    gtk_add_line_to_text_view_name("peer_textview",
        "------------------ Begin File Information ----------------", SD_OPTION_OFF);
    /* name */
    snprintf(b, sizeof(b), "Name: %s", dti->file.name);
    gtk_add_line_to_text_view_name("peer_textview", b, SD_OPTION_OFF);

    /* directory */
    snprintf(b, sizeof(b), "Directory: %s", dti->file.directory);
    gtk_add_line_to_text_view_name("peer_textview", b, SD_OPTION_OFF);
    
    /* size */
    snprintf(b, sizeof(b), "Size: %"PRIu64, dti->file.size);
    gtk_add_line_to_text_view_name("peer_textview", b, SD_OPTION_OFF);

    /* pos */
    snprintf(b, sizeof(b), "Position: %"PRIu64, dti->file.position);
    gtk_add_line_to_text_view_name("peer_textview", b, SD_OPTION_OFF);
    
    /* mod time */
    snprintf(b, sizeof(b), "Modification Time: %s", dti->file.modtime);
    gtk_add_line_to_text_view_name("peer_textview", b, SD_OPTION_OFF);

    gtk_add_line_to_text_view_name("peer_textview",
        "------------------ EndFile Information ----------------", SD_OPTION_OFF);

    gtk_add_line_to_text_view_name("peer_textview",
        "------------------ Begin Data Connection ----------------", SD_OPTION_OFF);

    /* con state */
    char *state;
    state = get_con_state_string(&dti->data_con);
    snprintf(b, sizeof(b), "Connection state: %s", state);
    gtk_add_line_to_text_view_name("peer_textview", b, SD_OPTION_OFF);
    SAFE_FREE(state);

    /* endpoint addr */
    bptr = get_sockaddr_storage_string(&dti->data_con.dst_sa);
    snprintf(b, sizeof(b), "Remote endpoint address: %s", bptr);
    gtk_add_line_to_text_view_name("peer_textview", b, SD_OPTION_OFF);
    SAFE_FREE(bptr);

    /* con fd */
    snprintf(b, sizeof(b), "File descriptor: %i", dti->data_con.sock_fd);
    gtk_add_line_to_text_view_name("peer_textview", b, SD_OPTION_OFF);

    /* con ssl enabled */
    snprintf(b, sizeof(b), "SSL enabled: %s",
        dti->data_con.enable_ssl == SD_OPTION_ON ? "Yes" : "No");
    gtk_add_line_to_text_view_name("peer_textview", b, SD_OPTION_OFF);
    
    /* get info about SSL if enabled */
    if (dti->data_con.enable_ssl && dti->data_con.ssl != NULL)
    {
      X509 *peercert;
      
      /* ssl cipher */
      snprintf(b, sizeof(b), "SSL session cipher: %s",
          SSL_get_cipher(dti->data_con.ssl));
      gtk_add_line_to_text_view_name("peer_textview", b, SD_OPTION_OFF);

      /* have a peer cert */
      peercert = SSL_get_peer_certificate(dti->data_con.ssl);
      snprintf(b, sizeof(b), "Recieved a certificate from peer: %s",
          peercert != NULL ? "Yes" : "No");
      gtk_add_line_to_text_view_name("peer_textview", b, SD_OPTION_OFF);
      
      /* display certificate info */
      if (peercert != NULL)
      {
        //char sslb[1024];

        gtk_add_line_to_text_view_name("peer_textview", "", SD_OPTION_OFF);
        gtk_add_line_to_text_view_name("peer_textview", 
            "------------------- Begin Peer Certificate ------------------", SD_OPTION_OFF);
        /* subject name */
        //X509_NAME_get_text_by_NID(X509_get_subject_name(peercert), 0,
        //    sslb, sizeof(sslb));
        snprintf(b, sizeof(b), "Subject: %s",
            X509_NAME_oneline(X509_get_subject_name(peercert), 0, 0));
        gtk_add_line_to_text_view_name("peer_textview", b, SD_OPTION_OFF);
        
        /* issuer name */
        //X509_NAME_get_text_by_NID(X509_get_issuer_name(peercert), 0,
        //    sslb, sizeof(sslb));
        snprintf(b, sizeof(b), "Issuer: %s",
            X509_NAME_oneline(X509_get_issuer_name(peercert), 0, 0));
        gtk_add_line_to_text_view_name("peer_textview", b, SD_OPTION_OFF);

        gtk_add_line_to_text_view_name("peer_textview", 
            "-------------------- End Peer Certificate -------------------", SD_OPTION_OFF);
        gtk_add_line_to_text_view_name("peer_textview", "", SD_OPTION_OFF);

      }
      
    }
    gtk_add_line_to_text_view_name("peer_textview",
        "------------------ End Data Connection ----------------", SD_OPTION_OFF);

    /* file shit */
    
    gtk_add_line_to_text_view_name("peer_textview", 
        "------------------- End Data Transfer -----------------", SD_OPTION_OFF);
    
    
  }
  else
  {
    /* no row is selected */
  }
}


// vim:ts=2:expandtab
