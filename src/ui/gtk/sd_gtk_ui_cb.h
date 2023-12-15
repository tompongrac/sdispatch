#ifndef SD_UI_CB_H
#define SD_UI_CB_H

#include <gtk/gtk.h>
#include "sd_peers.h"

extern void gtk_ui_init(void);
extern void gtk_ui_begin(void);
extern void gtk_ui_deinit(void);
extern void gtk_ui_idle(void);
extern void gtk_ui_sys_err(int n, const char *f, const char *m);
extern void gtk_ui_sock_err(int n, const char *f, const char *m);
extern void gtk_ui_gai_err(int n, const char *f, const char *m);
extern void gtk_ui_sd_err(const char *m);
extern void gtk_ui_ssl_err(const char *f, const char *m);
extern void gtk_ui_wsa_err(int n, const char *f);
extern void gtk_ui_notify(const char *m);
extern void gtk_ui_state_set(int os, int ns);
extern void gtk_ui_data_transfer_change(struct sd_data_transfer_info *dti,
    char change_type);
extern void gtk_ui_data_transfer_accepted(struct sd_data_transfer_info *dti);
extern int gtk_ui_process_status_message(void *v, int i);
extern void gtk_ui_data_transfer_progress_change(struct sd_data_transfer_info *dti);

#endif


// vim:ts=2:expandtab
