#ifndef SD_GTK_H
#define SD_GTK_H

#include <gtk/gtk.h>

#ifdef WIN32
#define SD_GTK_BUILDER_FILE_1  ".\\ui.xml"
#else
#define SD_GTK_BUILDER_FILE_1  "./ui.xml"
#define SD_GTK_BUILDER_FILE_2  "/usr/share/sdispatch/ui.xml"
#endif

extern gboolean gtk_ui_idle_cb(gpointer data);
extern void gtk_widget_show_name(const char *n);
extern void gtk_widget_hide_name(const char *n);
extern char *gtk_entry_get_text_name(const char *n);
extern void gtk_entry_set_text_name(const char *n, const char *v);
extern void gtk_add_line_to_text_view_name(char *n, const char *l,
    char scroll_to_end);
extern void gtk_clear_all_text_view_name(char *n);
extern void gtk_widget_set_sensitive_name(const char *n, gboolean c);
extern int gtk_toggle_button_get_active_name(const char *n);
extern char *gtk_get_filechooser_filename(const char *n);
extern int gtk_get_spinbutton_int_value_name(const char *n);
extern int gtk_remove_selected_treeview_entries_name(char *name);
extern void *gtk_get_pointer_selected(const char *n, int i);
extern void *gtk_get_pointer_combobox_selected(const char *n);
extern void gtk_set_toggle_button_active_name(const char *n, gboolean c);
extern void gtk_set_filechooser_filename(const char *n, const char *v);
extern void gtk_set_spinbutton_int_value_name(const char *n, int v);

extern GtkBuilder *sd_gtkbuilder;
extern gint statusbar_context_id;

#endif


// vim:ts=2:expandtab
