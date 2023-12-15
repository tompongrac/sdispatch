#ifndef SD_GTK_SUGGEST_FILES_H
#define SD_GTK_SUGGEST_FILES_H

extern void gtk_init_suggest_files_dialog(void);
extern void gtk_init_added_files_treeview(void);
extern void init_file_suggest_security_settings_dialog();
extern void gtk_reset_added_files_treeview(void);
extern void gtk_update_peer_information(void);
extern void gtk_update_peers_combobox(void);
extern void gtk_init_peers_combobox(void);

extern void gtk_init_listen_addresses_combobox();
extern void gtk_update_listen_addresses_combobox(void);

#endif
