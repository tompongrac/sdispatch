#ifndef SD_PEERS_H
#define SD_PEERS_H

#include <stdlib.h>
#include <openssl/ssl.h>

#ifdef WIN32

#else

#endif

#include "sd_net.h"
#include "sd_linked_list.h"
#include "sd_file.h"
#include "sd_ssl.h"
#include "sd_thread.h"

/*! \brief Holds data transfer information */
struct sd_data_transfer_info
{
  /* required for efficiency */
  struct sd_peer_info *parent_peer;

  uint64_t id;

  /* data channel */
  struct sd_con_info data_con;

  /* external address to advertise to peer */
  char wan_address[LOOKUP_ADDRESS_LEN];
  char wan_service[LOOKUP_SERVICE_LEN];

  /* server info */
  struct sd_serv_info *data_server;
  char using_local_address;
  struct sd_serv_accept_info *accept_info;
  char allow_any_port;

  /* connection method for data channel
   * note: same notation as FTP */
  char con_meth;
#define CON_METH_ACTIVE   0 /* we listen */
#define CON_METH_PASSIVE  1

  /* direction of data flow */
  char direction;
#define DATA_TRANSFER_DIRECTION_INCOMING  0
#define DATA_TRANSFER_DIRECTION_OUTGOING  1

  /* just for display */
  char peer_using_ssl;

  char verdict;
#define DATA_TRANSFER_VERDICT_PENDING     0
#define DATA_TRANSFER_VERDICT_ACCEPTED    1
#define DATA_TRANSFER_VERDICT_DECLINDED   2

  /* data transfer state machine */
  char state;
#define DATA_TRANSFER_STATE_SETUP_PENDING                0 /* before settings applied */
#define DATA_TRANSFER_STATE_ACTIVE_RESOLVE_SRC_IP       1
#define DATA_TRANSFER_STATE_PASSIVE_RESOLVE_ACCEPT_IP     2
#define DATA_TRANSFER_STATE_VERDICT_PENDING              3
  char peer_prepared;  /* flag because state machine might not be ready */
#define DATA_TRANSFER_STATE_PREPARATION_PENDING          4 /* verified, still setting up */
#define DATA_TRANSFER_STATE_TRANSFERING                  5
#define DATA_TRANSFER_STATE_COMPLETED                    6 /* sent/recv all */
#define DATA_TRANSFER_STATE_ABORTED                      7 /* error */

  char transfer_state;
#define DATA_TRANSFER_TRANSFER_STATE_PENDING    0
#define DATA_TRANSFER_TRANSFER_STATE_PAUSED     1 /* inavtive */
#define DATA_TRANSFER_TRANSFER_STATE_RESUMED    2 /* active */

  struct file_info file;

  /* this value needs to be large for good speeds
   * on very fast networks */
#define DATA_BUFFER_LEN                      102400   /* 100 kB */
  char data_buffer[DATA_BUFFER_LEN];
  int data_buffer_lower_offset;
  int data_buffer_window_size;

  /* progress control */

#define UPDATE_PROGRESS_INTERVAL          700   // ms

  uint64_t io_total_bytes_current;
  uint64_t io_total_bytes_last;
  char     io_byte_diff_zero;

  struct timeval io_time_last;
  struct timeval io_time_current;
};


/*! \brief Holds peer information including all its data transfers */
struct sd_peer_info
{
  /* control connection */

  struct sd_con_info ctl_con;  /* control channel */
#define CTL_BUFFER_LEN                       10240  /* 10 kB */
  char ctl_buffer[CTL_BUFFER_LEN]; /* for incomming cmds */
  int ctl_buffer_offset;
  linked_list ctl_cmd_backlog; /* struct protocol_command_entry */
  char ctl_con_verified;
#define CTL_CON_VERIFY_PENDING                   2
#define SD_MAX_PROTOCOL_VERSION_LEN             64
  char peer_protocol_version[SD_MAX_PROTOCOL_VERSION_LEN];

  /* data connections */

  linked_list data_transfers; /* struct sd_data_transfer_info */
};



/* -[ peers ]---------------------------------------------------------- */

/*! \brief Create initialise a peer */
extern struct sd_peer_info *peer_init(const char *a, const char *p, char enable_ssl,
    struct sd_ssl_verify_info *vi);

/*! \brief Add peer to linked list */
extern struct list_item *peer_add();

/*! \brief Iterate function for peer_deinit */
extern int peer_get_state_delete_iterate(void *value, int index);

/*! \brief Remove all peers marked for deletion */
extern int peer_rem_all_state_delete();

/*! \brief Close connection and abort all unestablished transfers */
extern void peer_set_closed(struct sd_peer_info *pi);

/*! \brief Deinitialise a peer and its data transfers */
extern void peer_deinit(struct sd_peer_info *pi);


/* -[ peers : getters ] ----------------------------------------------- */

/*! \brief Get number of established peers */
extern int get_est_peer_count();

/*! \brief Find a peer based on a control connection pointer */
extern struct sd_peer_info *con_get_peer(struct sd_con_info *ci);


/* -[ peers : setters ] ----------------------------------------------- */




/* -[ data transfers ]------------------------------------------------- */

/*! \brief Create and initialise a data transfer */
extern struct sd_data_transfer_info *data_transfer_init(
    struct sd_peer_info *pi,
    char enable_ssl, struct sd_ssl_verify_info *vi,
    struct file_info *fi, char dir);

/*! \brief Add a data transfer to peers linked list */
extern struct list_item *data_transfer_add(struct sd_peer_info *pi);

/*! \brief Initialise the IO components of a data transfer */
extern void data_transfer_init_io(struct sd_data_transfer_info *dti);

/*! \brief Deinitialise a data transfer */
extern void data_transfer_deinit(struct sd_data_transfer_info *dti);

/*! \brief Abort the unestablished transfers for given */
extern int abort_unest_data_transfers(void *v, int i);

/*! \brief Runs in idle to handle peers data transfers */
extern int data_transfer_idle_iter(void *value, int index);

/*! \brief Abort the transfer appropriatly according to its current state */
extern int data_transfer_abort(struct sd_data_transfer_info *dti);

/*! \brief Reset the data transfer IO components */
extern void data_transfer_reset_io(struct sd_data_transfer_info *dti);

/*! \brief Handles the data transfer based on it state in idle */
extern int handle_data_transfer_state(void *v, int index);

/* -[ data transfers : getters ]--------------------------------------- */

/*! \brief Get the next id for peer, based on direction */
extern uint64_t *get_next_transfer_id(struct sd_peer_info *pi, char dir);

/*! \brief Get data transfer pointer based on id and direction */
extern struct sd_data_transfer_info *get_data_transfer_from_id(
    struct sd_peer_info *pi, char dir, uint64_t *id);

/*! \brief Get data transfer connection method numeric from asci string */
extern int get_con_meth_id_from_string(const char *str);

/*! \brief Get data transfer connection method asci string */
extern char *get_data_transfer_con_method_string(struct sd_data_transfer_info *dti);

/*! \brief Get data transfer state asci string */
extern char *get_data_transfer_state_string(struct sd_data_transfer_info *dti);

/*! \brief Get data transfer verdict asci string */
extern char *get_data_transfer_verdict_string(struct sd_data_transfer_info *dti);

/*! \brief Get data transfer transfer asci string */
extern char *get_data_transfer_transfer_state_string(struct sd_data_transfer_info *dti);

/*! \brief Get data transfer verdict numeric from asci string */
extern int get_verdict_id_from_string(const char *str);

/*! \brief Get data transfer direction numeric from asci string */
extern int get_direction_id_from_string(const char *str);

/*! \brief Get data transfer direction arrow from asci string */
extern char *get_data_transfer_direction_arrow_string(struct sd_data_transfer_info *dti);


/* -[ data transfers : setters ]--------------------------------------- */

/*! \brief Mark the data transfer as active and set source address */
extern void data_transfer_set_active(struct sd_data_transfer_info *dti,
    const char *src_a, const char *src_s);

/*! \brief On recieving remote source address set value to data transfer */
extern void data_transfer_setup_active_remote_address(
    struct sd_data_transfer_info *dti, const char *a, const char *s);

/*! \brief Start resolving the source address for the transfer */
extern void data_transfer_setup_active_resolve_source(
    struct sd_data_transfer_info *dti);




/*! \brief Mark the data transfer as passive and setup server */
extern void data_transfer_set_passive(struct sd_data_transfer_info *dti,
    char local_address, struct sd_serv_info *si, char any_port);

/*! \brief On recieving remote source address set value to data transfer */
extern void data_transfer_setup_passive_remote_address(
    struct sd_data_transfer_info *dti, const char *a, const char *s);

/*! \brief On recieving file-verdict or before sending file-verdict accept */
extern struct sd_serv_accept_info *data_transfer_setup_passive_accept(
    struct sd_data_transfer_info *dti);

/*! \brief Set the data transfer id appropriatly */
extern void data_transfer_set_id(struct sd_data_transfer_info *dti, char direction,
    uint64_t *id);



/*! \brief Set the data transfer wan information */
extern void data_transfer_set_wan(struct sd_data_transfer_info *dti,
    const char *send_a, const char *send_s);

/*! \brief Set the data transfer state */
extern void data_transfer_set_state(struct sd_data_transfer_info *dti, char nstate);

/*! \brief Set the data transfer verdict */
extern void data_transfer_set_verdict(struct sd_data_transfer_info *dti, char v);

/*! \brief Set the data transfer transfer state */
extern void data_transfer_set_transfer_state(struct sd_data_transfer_info *dti, char v);

/*! \brief Set the data transfer transfer based on last and current */
extern void data_transfer_set_io(struct sd_data_transfer_info *dti);


#endif


// vim:ts=2:expandtab
