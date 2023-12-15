/*
   Common GTK+ operations
 
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
#include "sd_gtk.h"
#include "sd_gtk_ui_cb.h"
#include "sd_ui.h"
#include "sd_globals.h"
#include "sd_gtk_suggest_files.h"


GtkBuilder *sd_gtkbuilder;
gint statusbar_context_id;

G_MODULE_EXPORT void main_window_destroy_cb(GtkObject *object, gpointer user_data)
{
  /* break out of main loop */
  gtk_main_quit();
}

gboolean gtk_ui_idle_cb(gpointer data)
{
  gtk_ui_idle();

  return TRUE;
}

void register_gtk_ui(void)
{
  gbls->ui->init = &gtk_ui_init;
  gbls->ui->begin = &gtk_ui_begin;
  gbls->ui->deinit = &gtk_ui_deinit;
  gbls->ui->idle = &gtk_ui_idle;
  gbls->ui->sys_err = &gtk_ui_sys_err;
  gbls->ui->sock_err = &gtk_ui_sock_err;
  gbls->ui->gai_err = &gtk_ui_gai_err;
  gbls->ui->sd_err = &gtk_ui_sd_err;
  gbls->ui->ssl_err = &gtk_ui_ssl_err;
  gbls->ui->wsa_err = &gtk_ui_wsa_err;
  gbls->ui->notify = &gtk_ui_notify;
  gbls->ui->state_set = &gtk_ui_state_set;
  gbls->ui->data_transfer_change = &gtk_ui_data_transfer_change;
  gbls->ui->data_transfer_accepted = &gtk_ui_data_transfer_accepted;
  gbls->ui->data_transfer_progress_change = &gtk_ui_data_transfer_progress_change;
  gbls->ui->process_status_message = &gtk_ui_process_status_message;

  gbls->ui->iface = UI_GTK;
}

void gtk_widget_hide_name(const char *n)
{
  GtkWidget *dialog;
  
  dialog = GTK_WIDGET(gtk_builder_get_object(sd_gtkbuilder, n));
  gtk_widget_hide(dialog);
}

void gtk_widget_show_name(const char *n)
{
  GtkWidget *dialog;

  dialog = GTK_WIDGET(gtk_builder_get_object(sd_gtkbuilder, n));
  gtk_widget_show_all(dialog);
}

char *gtk_entry_get_text_name(const char *n)
{
  GtkWidget *entry;

  entry = GTK_WIDGET(gtk_builder_get_object(sd_gtkbuilder, n));
  return (char *) gtk_entry_get_text(GTK_ENTRY(entry));
}

void gtk_entry_set_text_name(const char *n, const char *v)
{
  GtkWidget *entry;

  entry = GTK_WIDGET(gtk_builder_get_object(sd_gtkbuilder, n));
  gtk_entry_set_text(GTK_ENTRY(entry), v);
}

void gtk_widget_set_sensitive_name(const char *n, gboolean c)
{
  GtkWidget *w;

  w = GTK_WIDGET(gtk_builder_get_object(sd_gtkbuilder, n));
  gtk_widget_set_sensitive(w, c);
}

void gtk_add_line_to_text_view_name(char *n, const char *l,
    char scroll_to_end)
{
  GtkWidget *tv;
  GtkTextBuffer *buffer;
  GtkTextIter iter;
  GtkTextMark *insert_mark;

  tv = GTK_WIDGET(gtk_builder_get_object(sd_gtkbuilder, n));
  /* get text in buffer */
  buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tv));

  /* get end iter */
  gtk_text_buffer_get_end_iter(buffer, &iter);
  /* add text to the buffer */
  gtk_text_buffer_insert(buffer, &iter, l, -1);
  
  /* to add new line */
  gtk_text_buffer_insert(buffer, &iter, "\n", -1);
  gtk_text_buffer_get_end_iter(buffer, &iter);

  if (scroll_to_end)
  {
    /* scroll to the end */
    insert_mark = gtk_text_buffer_get_insert(buffer);
    gtk_text_buffer_place_cursor(buffer, &iter);
    gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(tv),
        insert_mark, 0.0, FALSE, 0, 0.0);
  }
}
void gtk_clear_all_text_view_name(char *n)
{
  GtkWidget *tv;
  GtkTextBuffer *buffer;
  GtkTextIter start_iter, end_iter;

  tv = GTK_WIDGET(gtk_builder_get_object(sd_gtkbuilder, n));
  /* get text in buffer */
  buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tv));

  /* get start iter */
  gtk_text_buffer_get_start_iter(buffer, &start_iter);
  /* get end iter */
  gtk_text_buffer_get_end_iter(buffer, &end_iter);

  /* remove the buffer */
  gtk_text_buffer_delete(buffer, &start_iter, &end_iter);
}

int gtk_toggle_button_get_active_name(const char *n)
{
  GtkWidget *tb;

  tb = GTK_WIDGET(gtk_builder_get_object(sd_gtkbuilder, n));
  return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tb));
}

char *gtk_get_filechooser_filename(const char *n)
{
  GtkWidget *w;

  w = GTK_WIDGET(gtk_builder_get_object(sd_gtkbuilder, n));
  return (char *) gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(w));
}

void gtk_set_filechooser_filename(const char *n, const char *v)
{
  GtkWidget *w;

  w = GTK_WIDGET(gtk_builder_get_object(sd_gtkbuilder, n));
  gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(w), v);
}

int gtk_get_spinbutton_int_value_name(const char *n)
{
  GtkWidget *w;

  w = GTK_WIDGET(gtk_builder_get_object(sd_gtkbuilder, n));
  return gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(w));
}

void gtk_set_spinbutton_int_value_name(const char *n, int v)
{
  GtkWidget *w;

  w = GTK_WIDGET(gtk_builder_get_object(sd_gtkbuilder, n));
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(w), v);
}

void gtk_set_toggle_button_active_name(const char *n, gboolean c)
{
  GtkWidget *w;
  w = GTK_WIDGET(gtk_builder_get_object(sd_gtkbuilder, n));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), c);
}

void *gtk_get_pointer_selected(const char *n, int i)
{
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter iter;
  GtkWidget *w;
  void *ptr;

  w = GTK_WIDGET(gtk_builder_get_object(sd_gtkbuilder, n));
  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(w));

  ptr = NULL;
  if (gtk_tree_selection_get_selected(selection, &model, &iter))
    gtk_tree_model_get(model, &iter, i, &ptr, -1);

  return ptr;
}

void *gtk_get_pointer_combobox_selected(const char *n)
{
  GtkWidget *w;
  void *v;
  GtkTreeIter iter;
  GtkTreeModel *m;

  w = GTK_WIDGET(gtk_builder_get_object(sd_gtkbuilder, n));
  m = gtk_combo_box_get_model(GTK_COMBO_BOX(w));
  
  v = NULL;
  if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(w), &iter))
    gtk_tree_model_get(m, &iter, 1, &v, -1);

  return v;
}

int gtk_remove_selected_treeview_entries_name(char *name)
{
  GtkWidget *w;
  GtkTreeModel *m;
  GtkTreeSelection *s;
  GList *tp_list, *node;
  GtkTreeIter iter;
  GtkTreeRowReference *ref;
  GtkTreePath *np;
  int nrem;

  w = GTK_WIDGET(gtk_builder_get_object(sd_gtkbuilder, name));

  s = gtk_tree_view_get_selection(GTK_TREE_VIEW(w));
  tp_list = gtk_tree_selection_get_selected_rows(s, &m);

  nrem = 0;
  for (node = tp_list; node != NULL; node = node->next)
  {
    ref = gtk_tree_row_reference_new(m, node->data);
    gtk_tree_path_free(node->data);
    node->data = ref;
    nrem++;
  }

  for (node = tp_list; node != NULL; node = node->next)
  {
    np = gtk_tree_row_reference_get_path((GtkTreeRowReference *)node->data);

    if (np)
    {
      if (gtk_tree_model_get_iter(m, &iter, np))
      {
        gtk_list_store_remove(GTK_LIST_STORE(m), &iter);
      }
    }
    gtk_tree_path_free(np);
  }

  g_list_free(tp_list);

  return nrem;
}

// vim:ts=2:expandtab
