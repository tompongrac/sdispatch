#ifndef SD_UI_H
#define SD_UI_H

#include "sd_peers.h"

#define SD_DATA_TRANSFER_CHANGE_TYPE_ADD         0
#define SD_DATA_TRANSFER_CHANGE_TYPE_REMOVE      1
#define SD_DATA_TRANSFER_CHANGE_TYPE_UPDATE      2

/*! \brief Initialise the User Interface */
extern void ui_init(void);

/*! \brief Start the User Interface */
extern void ui_begin(void);

/*! \brief Cleanup the User Interface */
extern void ui_deinit(void);

/*! \brief Called when we have free processing time */
extern void ui_idle(void);

/*! \brief Called when a system error occurs */
extern void ui_sys_err(int n, const char *f);

/*! \brief Called when a socket error occurs */
extern void ui_sock_err(const char *f);

/*! \brief Called when a Gai error occurs */
extern void ui_gai_err(int n, const char *f);

/*! \brief Called when a general error occurs */
extern void ui_sd_err(const char *m);

/*! \brief Called when a SSL error occurs */
extern void ui_ssl_err(const char *f);

/*! \brief Called when a WSA error occurs */
extern void ui_wsa_err(const char *f);

/*! \brief Used to notify messages to the user */
extern void ui_notify(const char *m);

/*! \brief Used to notify messages to the user with printf form VA list */
extern int ui_notify_printf(const char *m, ...);

/*! \brief Used indicate a state has changed (not used) */
extern void ui_state_set(int os, int ns);

/*! \brief Called when a data transfer has changed */
extern void ui_data_transfer_change(struct sd_data_transfer_info *dti,
    char changetype);

/*! \brief Called when a data transfer was accepted */
extern void ui_data_transfer_accepted(struct sd_data_transfer_info *dti);

/*! \brief Called when a data transfers progress changes */
extern void ui_data_transfer_progress_change(struct sd_data_transfer_info *dti);

/*! \brief Runs in idle to process any status messages to UI */
extern int ui_process_status_backlog();

/*! \brief Sets the UI as GTK */
extern void set_gtk_ui();



#endif


// vim:ts=2:expandtab
