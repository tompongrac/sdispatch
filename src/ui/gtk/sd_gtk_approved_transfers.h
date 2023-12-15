#ifndef SD_GTK_APPROVED_TRANSFERS_H
#define SD_GTK_APPROVED_TRANSFERS_H

extern void gtk_init_data_transfers_approved_treeview(void);
#if 0
extern void gtk_update_data_transfers_unapproved_treeview(void);
extern void transfer_settings_dialog_init();
extern void transfer_settings_dialog_spawn();
extern void gtk_init_transfer_settings_listen_addresses_combobox();
extern void gtk_update_transfer_settings_listen_addresses_combobox();
#endif

extern void treeview_add_approved_transfer_treeview(struct sd_data_transfer_info *dti);
extern void treeview_remove_approved_transfer_treeview(struct sd_data_transfer_info *dti);
extern void treeview_update_approved_transfer_treeview(struct sd_data_transfer_info *dti);

extern void treeview_update_approved_transfer_progress_treeview(struct sd_data_transfer_info *dti);

#endif
