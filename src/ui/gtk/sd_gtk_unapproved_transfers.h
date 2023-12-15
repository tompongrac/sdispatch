#ifndef SD_GTK_UNAPPROVED_TRANSFERS_H
#define SD_GTK_UNAPPROVED_TRANSFERS_H

//extern void gtk_init_peers_treeview(void);
//extern void gtk_update_peers_treeview(void);
//extern void gtk_update_peer_information(void);

extern void gtk_init_data_transfers_unapproved_treeview(void);
extern void gtk_update_data_transfers_unapproved_treeview(void);
extern void init_incoming_transfer_security_settings_dialog(void);
extern void transfer_settings_dialog_init();
extern void transfer_settings_dialog_spawn();
extern void gtk_init_transfer_settings_listen_addresses_combobox();
extern void gtk_update_transfer_settings_listen_addresses_combobox();

extern void treeview_add_unapproved_transfer_treeview(struct sd_data_transfer_info *dti);
extern void treeview_remove_unapproved_transfer_treeview(struct sd_data_transfer_info *dti);
extern void treeview_update_unapproved_transfer_treeview(struct sd_data_transfer_info *dti);

#endif
