#ifndef SD_PROTOCOL_COMMANDS_H
#define SD_PROTOCOL_COMMANDS_H

#include <stdlib.h>

#include "sd_linked_list.h"
#include "sd_peers.h"
#include "sd_protocol.h"

/*! \brief Holds protocol command structure information */
struct sd_protocol_command_info
{
  char *name;
  char *unpack_arg_fmt;
  char *pack_arg_fmt;
  int nargs;
  int (*unpack)(struct sd_peer_info *pi,
      struct sd_protocol_command_info *pci, const char *args);
  void (*process)(struct sd_peer_info *pi, linked_list *args);
};

/* store all commands */
extern struct sd_protocol_command_info control_command[];

/*! \brief Holds protocol command instance information */
struct sd_protocol_command_entry
{
  char name[SD_MAX_PROTOCOL_COMMAND_NAME_LEN];
  linked_list args;
  struct sd_peer_info *peer;
};

/*! \brief Holds protocol command instance information */
extern int get_control_command_qty();

extern int version_command_unpack_cb(struct sd_peer_info *pi,
    struct sd_protocol_command_info *pci, const char *args);
extern void con_send_protocol_version(struct sd_con_info *ci);
extern void version_command_process_cb(struct sd_peer_info *pi, linked_list *args);

extern int talk_command_unpack_cb(struct sd_peer_info *pi,
    struct sd_protocol_command_info *pci, const char *args);
extern void talk_command_process_cb(struct sd_peer_info *pi, linked_list *args);

extern int file_suggest_command_unpack_cb(struct sd_peer_info *pi,
    struct sd_protocol_command_info *pci, const char *args);
extern int file_suggest_command_pack_and_send(struct sd_data_transfer_info *dti);
extern void file_suggest_command_process_cb(struct sd_peer_info *pi,
    linked_list *args);

extern int file_verdict_command_unpack_cb(struct sd_peer_info *pi,
    struct sd_protocol_command_info *pci, const char *args);
extern int file_verdict_command_pack_and_send(struct sd_data_transfer_info *dti,
    char verdict);
extern void file_verdict_command_process_cb(struct sd_peer_info *pi,
    linked_list *args);

extern int file_prepared_command_pack_and_send(struct sd_data_transfer_info *dti);
extern int file_prepared_command_unpack_cb(struct sd_peer_info *pi,
    struct sd_protocol_command_info *pci, const char *args);
extern void file_prepared_command_process_cb(struct sd_peer_info *pi, linked_list *args);

#endif


// vim:ts=2:expandtab
