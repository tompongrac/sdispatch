/*
   GTK+ UI approved transfers
 
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
#include <math.h>
#include <inttypes.h>

#include "sd_ui.h"
#include "sd_gtk.h"
#include "sd_globals.h"
#include "sd_error.h"
#include "sd_dynamic_memory.h"
#include "sd_gtk_approved_transfers.h"
#include "sd_protocol.h"
#include "sd_protocol_commands.h"

static struct sd_data_transfer_info *popup_menu_approved_transfer_selected;

enum
{
  COL_ID = 0,
  COL_DIRECTION,
  COL_PEER,
  COL_FILENAME,
  COL_PROGRESS_METER_VALUE,
  COL_PROGRESS_METER_PULSE,
  COL_PROGRESS_METER_TEXT,
  COL_THROUGHPUT,
  COL_ETA,
  COL_TRANSFER_STATE,
  COL_CON_METH,
  COL_STATE,
  COL_FILE_DIR,
  COL_POINTER,
  NUM_COLS
};  

void gtk_init_data_transfers_approved_treeview(void)
{
  GtkWidget *w;
  GtkCellRenderer *renderer;
  GtkListStore *ls;

  w = GTK_WIDGET(gtk_builder_get_object(sd_gtkbuilder,
        "data_transfers_approved_treeview"));

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
  
  /* --- progress meter ---*/
  renderer = gtk_cell_renderer_progress_new();
  gtk_tree_view_insert_column_with_attributes(
      GTK_TREE_VIEW(w), -1, "Progress", renderer,
      "value", COL_PROGRESS_METER_VALUE,
      "pulse", COL_PROGRESS_METER_PULSE,
      "text", COL_PROGRESS_METER_TEXT,
      NULL);
  
  /* --- throughput ---*/
  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(
      GTK_TREE_VIEW(w), -1, "Throughput", renderer, "text", COL_THROUGHPUT, NULL);
  
  /* --- ETA ---*/
  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(
      GTK_TREE_VIEW(w), -1, "ETA", renderer, "text", COL_ETA, NULL);
  
  /* --- transfer state ---*/
  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(
      GTK_TREE_VIEW(w), -1, "Tfr State", renderer, "text", COL_TRANSFER_STATE, NULL);
  
  /* --- connection method ---*/
  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(
      GTK_TREE_VIEW(w), -1, "Method", renderer, "text", COL_CON_METH, NULL);
  
  /* --- state ---*/
  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(
      GTK_TREE_VIEW(w), -1, "State", renderer, "text", COL_STATE, NULL);
  
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

      /* p meter */
      G_TYPE_INT,
      G_TYPE_INT,
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


void set_data_transfer_approved_list_store_row(
    struct sd_data_transfer_info *dti, GtkListStore *ls, GtkTreeIter *ti)
{
  char *s;
  char *ts;
  char *saddr;
  char *cm;
  char *dir;

  s = get_data_transfer_state_string(dti);
  ts = get_data_transfer_transfer_state_string(dti);
  s = get_data_transfer_state_string(dti);
  saddr = get_sockaddr_storage_string(&dti->parent_peer->ctl_con.dst_sa);
  dir = get_data_transfer_direction_arrow_string(dti);
  cm = get_data_transfer_con_method_string(dti);

  gtk_list_store_set(ls, ti,
      COL_ID, dti->id,
      COL_DIRECTION, dir,
      COL_PEER, saddr,
      COL_FILENAME, dti->file.name,

      /* do progress elsewhere */

      COL_TRANSFER_STATE, ts,
      COL_CON_METH, cm,
      COL_STATE, s,
      COL_FILE_DIR, dti->file.directory,
      COL_POINTER, dti,
      
      -1);
  
  SAFE_FREE(s);
  SAFE_FREE(ts);
  SAFE_FREE(saddr);
  SAFE_FREE(dir);
  SAFE_FREE(cm);
}

/* ------------- transfer progress support begin ----------- */
void set_data_transfer_approved_list_store_progress_row(
    struct sd_data_transfer_info *dti, GtkListStore *ls, GtkTreeIter *ti)
{
  int time_difference;
  uint64_t byte_difference;

  /* 0 length file */
  if (!dti->file.size)
    return;

  byte_difference = dti->io_total_bytes_current - dti->io_total_bytes_last;

  /* not going to be accurate */
  if (((!byte_difference) && (!dti->io_byte_diff_zero)) &&
      (dti->state != DATA_TRANSFER_STATE_COMPLETED))
    return;

  time_difference = time_diff(&dti->io_time_current, &dti->io_time_last);

#if 0
  printf("time diff : %i byte diff : %"PRIu64"\n",
      time_difference, byte_difference);
#endif

  /* calculate percentage */
  
  uint64_t percent_i;
  char percent_s[64];


  percent_i = (dti->io_total_bytes_current * (uint64_t)(100)) / dti->file.size;
  snprintf(percent_s, sizeof percent_s, "%"PRIu64"%%", percent_i);

  /* calculate throughput */
  
  uint64_t throughput;
  char throughput_s[128];

  if (time_difference) {
    throughput = byte_difference / (uint64_t) time_difference; // b / ms
    throughput *= (uint64_t) 1000; // b / s
    throughput /= (uint64_t) 1024; // kB / s
  }
  else {
    throughput = 0;
  }
  snprintf(throughput_s, sizeof throughput_s, "%"PRIu64"kB/s", throughput);

  /* calculate ETA */
  uint64_t byte_remaining;
  uint64_t time_remaining;
  //char eta_s[256];
  char *eta_s;

  byte_remaining = dti->file.size - dti->io_total_bytes_current;
  if (throughput)
    time_remaining = byte_remaining / (throughput * ((uint64_t)1024)); /* s */
  else
    time_remaining = 0;

  //snprintf(eta_s, sizeof eta_s, "%"PRIu64"m", time_remaining);
  eta_s = time_get_time_remaining_string(&time_remaining);

  gtk_list_store_set(ls, ti,
      COL_PROGRESS_METER_VALUE, (int) percent_i,
      COL_PROGRESS_METER_PULSE, 0,
      COL_PROGRESS_METER_TEXT, percent_s,

      COL_THROUGHPUT, throughput_s,
      COL_ETA, eta_s,

      -1);

  SAFE_FREE(eta_s);
}
void treeview_update_approved_transfer_progress_treeview(struct sd_data_transfer_info *dti)
{
  GtkWidget *w;
  GtkTreeModel *m;
  GtkTreeIter iter;
  gboolean  valid;
  void *ptr;

  w = GTK_WIDGET(gtk_builder_get_object(sd_gtkbuilder,
        "data_transfers_approved_treeview"));
  m = gtk_tree_view_get_model(GTK_TREE_VIEW(w));

  valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(m), &iter);

  while (valid)
  {
    gtk_tree_model_get(m, &iter, COL_POINTER, &ptr, -1);

    /* we have a match */
    if (ptr == dti) {
      set_data_transfer_approved_list_store_progress_row(dti, GTK_LIST_STORE(m), &iter);
      break;
    }
    valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(m), &iter);
  }
}
/* ------------- transfer progress support end ----------- */

/* add to treeview based on pointer */
void treeview_add_approved_transfer_treeview(struct sd_data_transfer_info *dti)
{
  GtkTreeIter iter;
  GtkWidget *w;
  GtkTreeModel *m;

  w = GTK_WIDGET(gtk_builder_get_object(sd_gtkbuilder,
        "data_transfers_approved_treeview"));
  m = gtk_tree_view_get_model(GTK_TREE_VIEW(w));

  gtk_list_store_append(GTK_LIST_STORE(m), &iter);
  set_data_transfer_approved_list_store_row(dti, GTK_LIST_STORE(m), &iter);
  set_data_transfer_approved_list_store_progress_row(dti, GTK_LIST_STORE(m), &iter);
}

void treeview_remove_approved_transfer_treeview(struct sd_data_transfer_info *dti)
{
  GtkWidget *w;
  GtkTreeModel *m;
  GtkTreeIter iter;
  gboolean  valid;
  void *ptr;

  w = GTK_WIDGET(gtk_builder_get_object(sd_gtkbuilder,
        "data_transfers_approved_treeview"));
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

void treeview_update_approved_transfer_treeview(struct sd_data_transfer_info *dti)
{
  GtkWidget *w;
  GtkTreeModel *m;
  GtkTreeIter iter;
  gboolean  valid;
  void *ptr;

  w = GTK_WIDGET(gtk_builder_get_object(sd_gtkbuilder,
        "data_transfers_approved_treeview"));
  m = gtk_tree_view_get_model(GTK_TREE_VIEW(w));

  valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(m), &iter);

  while (valid)
  {
    gtk_tree_model_get(m, &iter, COL_POINTER, &ptr, -1);

    /* we have a match */
    if (ptr == dti)
    {
      set_data_transfer_approved_list_store_row(dti, GTK_LIST_STORE(m), &iter);
      set_data_transfer_approved_list_store_progress_row(dti, GTK_LIST_STORE(m), &iter);
      break;
    }
    
    valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(m), &iter);
  }
}

/* ----------------------- popup menu begin ----------------------- */
/* right click popup menu */
G_MODULE_EXPORT gboolean data_transfers_approved_treeview_button_press_event_cb(
    GtkWidget *treeview, GdkEventButton *event, gpointer userdata)
{
  //GtkTreeSelection *s;
  GtkWidget *w, *w_tv;

  struct sd_data_transfer_info *dti;

  w = GTK_WIDGET(gtk_builder_get_object(sd_gtkbuilder,
        "data_transfers_approved_menu"));
  w_tv = GTK_WIDGET(gtk_builder_get_object(sd_gtkbuilder,
        "data_transfers_approved_treeview"));

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
        "data_transfers_approved_treeview", COL_POINTER);

    if (dti != NULL)
    {

      popup_menu_approved_transfer_selected = dti;
      /* set values appropriatly */
      gtk_widget_show_name("data_transfers_approved_menu");

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

/* transfer state PASUED */
G_MODULE_EXPORT void data_transfer_approved_state_pause_menuitem_activate_cb(
    GtkObject *object, gpointer user_data)
{
  data_transfer_set_transfer_state(popup_menu_approved_transfer_selected,
      DATA_TRANSFER_TRANSFER_STATE_PAUSED);
}

/* transfer state RESUME */
G_MODULE_EXPORT void data_transfer_approved_state_resume_menuitem_activate_cb(
    GtkObject *object, gpointer user_data)
{
  data_transfer_set_transfer_state(popup_menu_approved_transfer_selected,
      DATA_TRANSFER_TRANSFER_STATE_RESUMED);
}

/* abort */
G_MODULE_EXPORT void data_transfer_approved_abort_menuitem_activate_cb(
    GtkObject *object, gpointer user_data)
{
  data_transfer_abort(popup_menu_approved_transfer_selected);
}

/* clear from treeview */
G_MODULE_EXPORT void data_transfers_approved_clear_menuitem_activate_cb(
    GtkObject *object, gpointer user_data)
{
  ui_data_transfer_change(popup_menu_approved_transfer_selected, SD_DATA_TRANSFER_CHANGE_TYPE_REMOVE);
}


/* ----------------------- popup menu end ----------------------- */

// vim:ts=2:expandtab
