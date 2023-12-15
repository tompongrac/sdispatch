#ifndef SD_PROTOCOL_H
#define SD_PROTOCOL_H

#include <stdlib.h>

#include "sd_linked_list.h"
#include "sd_peers.h"

#define SD_MAX_PROTOCOL_COMMAND_NAME_LEN    64
#define SD_MAX_PROTOCOL_TALK_MESSAGE_LEN  1024

#define SEND_BUFFER_LEN                  10240

#define SD_PROTOCOL_ARGUMENT_DELIM         " "

#define SD_MAX_PROTOCOL_CONST_VALUE_LEN    128
#define SD_PROTOCOL_VALUE_TRUE          "TRUE"
#define SD_PROTOCOL_VALUE_FALSE        "FALSE"

#define SD_PROTOCOL_VALUE_PASSIVE    "PASSIVE"
#define SD_PROTOCOL_VALUE_ACTIVE      "ACTIVE"

#define SD_PROTOCOL_VALUE_ACCEPT      "ACCEPT"
#define SD_PROTOCOL_VALUE_DECLINE    "DECLINE"

#define SD_PROTOCOL_VALUE_OUTGOING  "OUTGOING"
#define SD_PROTOCOL_VALUE_INCOMING  "INCOMING"

#define SD_PROTOCOL_VALUE_NULL          "NULL"

/* partial declearations */
struct sd_protocol_command_info;

/*! \brief Encode string to url format (for protocol message) */
extern int string_url_encode(char *dst, const char *str, int nbytes);

/*! \brief Decode string to url format (for protocol message) */
extern int string_url_decode(char *dst, const char *str, int nbytes);

/*! \brief Add command to command backlog if unpacked successfully */
extern void init_protocol_command_entry(struct sd_peer_info *pi,
    struct sd_protocol_command_info *pci, ...);

/*! \brief Build a command string */
extern int command_pack(const char *name, char *b, int nbytes, va_list args);

/*! \brief Send command to remote peer */
extern int send_protocol_command(struct sd_peer_info *pi, const char *name, ...);

/*! \brief Unpack the command and pass it to unpack callback for specific command */
extern int process_protocol_command(struct sd_peer_info *pi, const char *msg);

/*! \brief Recieve data from control connection and process if valid */
extern int handle_ctl_recv(struct sd_peer_info *pi, fd_set *m, int mfd, int len);

/*! \brief Send a message over the control connection */
extern int handle_ctl_send(struct sd_peer_info *pi, char *b, int len);

/*! \brief Check if we have the end of the message and format message appropriatly */
extern int get_end_of_message(char *b);

/*! \brief Iterate function to recieve data over control connection in idle */
extern int ctl_recv_iter_cb(void *v, int index);

/*! \brief Close and cleanup control connection */
extern void ctl_con_close(struct sd_peer_info *pi);

/*! \brief Process all commands in backlog then remove them */
extern int ctl_process_cmd_iter_cb(void *v, int index);

/*! \brief Iterate function for ctl_process_cmd_iter_cb */
extern int ctl_process_cmd_entry_iter_cb(void *v, int index);


/* -[ data connection specific ] -------------------------------------- */

/*! \brief Handle network socket and file input for data transfer */
extern int handle_data_recv(struct sd_data_transfer_info *dti, fd_set *m, int mfd, int len);

/*! \brief Handle network socket and file output for data transfer */
extern int handle_data_send(struct sd_data_transfer_info *dti, char *b, int len);


#endif


// vim:ts=2:expandtab
