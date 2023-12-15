#ifndef SD_GLOBALS_H
#define SD_GLOBALS_H

#include <openssl/ssl.h>

#include "sd_linked_list.h"
#include "sd_ssl.h"
#include "sd_peers.h"
#include "sd_timing.h"
#include "sd_thread.h"

/*! \brief Default values that are set with configuration file */
struct sd_conf
{
  /* configuration file location */
  char config_file_path[SD_MAX_PATH_LEN + SD_MAX_FILENAME_LEN];

  /* general ssl */
  struct sd_ssl_verify_info ssl_verify;

  /* control connection */
  char control_server_net_address[LOOKUP_ADDRESS_LEN];
  char control_server_service[LOOKUP_SERVICE_LEN];
  char control_client_net_address[LOOKUP_ADDRESS_LEN];
  char control_client_service[LOOKUP_SERVICE_LEN];

  /* data transfer */
  char data_local_net_address[LOOKUP_ADDRESS_LEN];
  char data_wide_net_address[LOOKUP_ADDRESS_LEN];
  char data_wide_service[LOOKUP_SERVICE_LEN];
  char data_output_path[SD_MAX_PATH_LEN];

  /* logging */
  char logging_enabled;
  char logging_path[SD_MAX_PATH_LEN];
};

/*! \brief Command line options */
struct sd_cl_options
{
};

/*! \brief Program information */
struct sd_prog_info
{
  char *name;
  char *version;
  char *authors;
  char *website;
};

/*! \brief User interface information */
struct sd_ui_info
{
  void (*init)(void);
  void (*begin)(void);
  void (*deinit)(void);
  void (*idle)(void);
  void (*sys_err)(int n, const char *f, const char *m);
  void (*sock_err)(int n, const char *f, const char *m);
  void (*gai_err)(int n, const char *f, const char *m);
  void (*sd_err)(const char *m);
  void (*ssl_err)(const char *f, const char *m);
  void (*wsa_err)(int n, const char *f);
  void (*notify)(const char *m);
  void (*state_set)(int os, int ns);
  void (*data_transfer_change)(
      struct sd_data_transfer_info *dti, char changetype);
  void (*data_transfer_progress_change)(
      struct sd_data_transfer_info *dti);
  void (*data_transfer_accepted)(struct sd_data_transfer_info *dti);
  int (*process_status_message)(void *v, int i);
  char initialized;
  char iface;
  linked_list status_backlog;
  struct sd_mutex_state_info mutex;

#define UI_GTK 0
};

/*! \brief Network information */
struct sd_net_info
{
  fd_set master_fd_set;
  int highest_fd;
  SSL_CTX *ssl_ctx; /* holds default values for SSL structs */

  /* servers */

  linked_list con_servers;
  
  /* peers */

  linked_list peers;
  
};

/*! \brief Logging info */
struct sd_logging_info
{
  struct file_info file;
};

/*! \brief Holder for globals */
struct sd_globals
{
  struct sd_conf *conf;
  struct sd_cl_options *cl_opts;
  struct sd_prog_info *prog;
  struct sd_ui_info *ui;
  struct sd_net_info *net;
  struct sd_logging_info *logging;
  frame frame;
};

extern struct sd_globals *gbls;

/*! \brief Allocate the globals */
extern void sd_globals_alloc();

/*! \brief Free the globals */
extern void sd_globals_free();

#endif


// vim:ts=2:expandtab
