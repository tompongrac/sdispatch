/*
   Text-based protocol command handling
 
   Copyright (c) Thomas Pongrac
 
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
 
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
*/

#ifdef WIN32

#include <windows.h>

#else

#include <unistd.h>

#endif

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>

#include "sd_protocol.h"
#include "sd_globals.h"
#include "sd_ui.h"
#include "sd_net.h"
#include "sd_peers.h"
#include "sd_error.h"
#include "sd_dynamic_memory.h"
#include "sd_version.h"
#include "sd_protocol_commands.h"

/* 
 * structure:
 *   name
 *   unpack format
 *   pack format
 *   num args
 *   unpack callback
 *   process callback
 *   
 */
struct sd_protocol_command_info control_command[] = {
  { "VERSION",
    /* args:
     *   version_string */
    "%"SD_TOSTRING(SD_MAX_PROTOCOL_VERSION_LEN)"s",
    "%."SD_TOSTRING(SD_MAX_PROTOCOL_VERSION_LEN)"s",
    1,
    &version_command_unpack_cb,
    &version_command_process_cb },

  { "TALK",
    /* args:
     *   message */
    "%"SD_TOSTRING(SD_MAX_PROTOCOL_TALK_MESSAGE_LEN)"s",
    "%."SD_TOSTRING(SD_MAX_PROTOCOL_TALK_MESSAGE_LEN)"s",
    1,
    &talk_command_unpack_cb,
    &talk_command_process_cb },

  { "FILE-SUGGEST",
    /* args:
     *   file_id
     *   file_name
     *   file_size
     *   file_modification_time
     *   enable_ssl
     *   data_connection_method
     *   net_address
     *   port
     */
    "%"PRIu64" "
    "%"SD_TOSTRING(SD_MAX_FILENAME_LEN)"s "
    "%"PRIu64" "
    "%"SD_TOSTRING(SD_MAX_MODIFICATION_TIME_LEN)"s "
    "%"SD_TOSTRING(SD_MAX_PROTOCOL_CONST_VALUE_LEN)"s "
    "%"SD_TOSTRING(SD_MAX_PROTOCOL_CONST_VALUE_LEN)"s "
    "%"SD_TOSTRING(LOOKUP_SERVICE_LEN)"s "
    "%"SD_TOSTRING(LOOKUP_SERVICE_LEN)"s",

    "%"PRIu64" "
    "%."SD_TOSTRING(SD_MAX_FILENAME_LEN)"s "
    "%"PRIu64" "
    "%."SD_TOSTRING(SD_MAX_MODIFICATION_TIME_LEN)"s "
    "%."SD_TOSTRING(SD_MAX_PROTOCOL_CONST_VALUE_LEN)"s "
    "%."SD_TOSTRING(SD_MAX_PROTOCOL_CONST_VALUE_LEN)"s "
    "%."SD_TOSTRING(LOOKUP_SERVICE_LEN)"s "
    "%."SD_TOSTRING(LOOKUP_SERVICE_LEN)"s",

    8,
    &file_suggest_command_unpack_cb,
    &file_suggest_command_process_cb },

  { "FILE-VERDICT",
    /* args:
     *   file_id
     *   verdict string
     *   net_address
     *   port
     */
    "%"PRIu64" "
    "%"SD_TOSTRING(SD_MAX_PROTOCOL_CONST_VALUE_LEN)"s "
    "%"SD_TOSTRING(LOOKUP_SERVICE_LEN)"s "
    "%"SD_TOSTRING(LOOKUP_SERVICE_LEN)"s "
    "%"PRIu64"",

    "%"PRIu64" "
    "%."SD_TOSTRING(SD_MAX_PROTOCOL_CONST_VALUE_LEN)"s "
    "%."SD_TOSTRING(LOOKUP_SERVICE_LEN)"s "
    "%."SD_TOSTRING(LOOKUP_SERVICE_LEN)"s "
    "%"PRIu64"",

    5,
    &file_verdict_command_unpack_cb,
    &file_verdict_command_process_cb },

  { "FILE-PREPARED",
    /* args:
     *   file id 
     *   direction
     *   */
    "%"PRIu64" "
    "%"SD_TOSTRING(SD_MAX_PROTOCOL_CONST_VALUE_LEN)"s",

    "%"PRIu64" "
    "%."SD_TOSTRING(SD_MAX_PROTOCOL_CONST_VALUE_LEN)"s",

    2,
    &file_prepared_command_unpack_cb,
    &file_prepared_command_process_cb },
};

int get_control_command_qty()
{
  return sizeof control_command /
    sizeof(struct sd_protocol_command_info);
}

/* **** command process callbacks **** */

#if 0
/* a template cb */
void template_command_process_cb(struct sd_peer_info *pi, linked_list *args)
{
  int na;
  void **a;

  na = linked_list_get_all_values(args, &a);
  ui_notify((char *)((*a)+0));

  /* note: no need to dealloc indev args as they are done
   * after this returns */

  SAFE_FREE(a);
}
#endif



/* ------------------- version command begin ---------------------- */
int version_command_unpack_cb(struct sd_peer_info *pi,
    struct sd_protocol_command_info *pci, const char *args)
{
  char vstr[SD_MAX_PROTOCOL_VERSION_LEN], *nstr;
  if (sscanf(args, pci->unpack_arg_fmt, vstr) != pci->nargs)
    return -1;
  SAFE_CALLOC(nstr, 1, SD_MAX_PROTOCOL_VERSION_LEN);
  strncpy(nstr, vstr, SD_MAX_PROTOCOL_VERSION_LEN);
  
  init_protocol_command_entry(pi, pci, nstr);
  return 0;
}

void con_send_protocol_version(struct sd_con_info *ci)
{
  struct sd_peer_info *pi;

  pi = con_get_peer(ci);

  if (!pi)
    ON_ERROR_EXIT("Could not find peer.\n");

  send_protocol_command(pi, "VERSION", SD_VERSION);
}

void version_command_process_cb(struct sd_peer_info *pi, linked_list *args)
{
  int na;
  void **a;

  /* check if allready verified */
  if (pi->ctl_con_verified == SD_OPTION_ON)
    return;

  na = linked_list_get_all_values(args, &a);
  
  snprintf(pi->peer_protocol_version, sizeof pi->peer_protocol_version,
      "%s", (char *)(a[0]));

  SAFE_FREE(a);

  /* get peer info */
  char *saddr;
  saddr = get_sockaddr_storage_string(&pi->ctl_con.dst_sa);

  ui_notify_printf("Recieved protocol version [%s] from %s.",
      pi->peer_protocol_version, saddr);
  SAFE_FREE(saddr);
  
  /* is protocol version valid? */
  if (!strncmp(SD_VERSION, pi->peer_protocol_version, strlen(SD_VERSION)))
  {
    ui_notify("Protocol versions match.");
  }
  else
  {
    ui_sd_err("Protocol version did not match. It is recommended you use the "
              "same protocol version otherwise effects are undefined.");
  }

  pi->ctl_con_verified = SD_OPTION_ON;
}
/* -------------------- version command end ------------------------ */

/* --------------------- talk command begin ------------------------ */
int talk_command_unpack_cb(struct sd_peer_info *pi,
    struct sd_protocol_command_info *pci, const char *args)
{
  char tstr[SD_MAX_PROTOCOL_TALK_MESSAGE_LEN], *nstr;
  if (sscanf(args, pci->unpack_arg_fmt, tstr) != pci->nargs)
    return -1;
  SAFE_CALLOC(nstr, 1, SD_MAX_PROTOCOL_TALK_MESSAGE_LEN);

  string_url_decode(nstr, tstr, sizeof tstr);
  //strncpy(nstr, vstr, SD_MAX_PROTOCOL_TALK_MESSAGE_LEN);
  
  init_protocol_command_entry(pi, pci, nstr);
  return 0;
}
void talk_command_process_cb(struct sd_peer_info *pi, linked_list *args)
{
  int na;
  void **a;
  
  if (pi->ctl_con_verified == SD_OPTION_OFF)
  {
    ui_sd_err("Did not recieve version string.. Closing connection.");
    ctl_con_close(pi);
    return;
  }

  na = linked_list_get_all_values(args, &a);
  
  /* get peer info */
  char *saddr;
  saddr = get_sockaddr_storage_string(&pi->ctl_con.dst_sa);

  ui_notify_printf("Message from %s: %s", saddr, (char *)(a[0]));

  SAFE_FREE(saddr);
  SAFE_FREE(a);
}
/* --------------------- talk command end ------------------------ */

/* ----------------- file-suggest command begin ------------------ */
int file_suggest_command_unpack_cb(struct sd_peer_info *pi,
    struct sd_protocol_command_info *pci, const char *args)
{
  char fn_str[SD_MAX_FILENAME_LEN], *n_fn_str;
  char mt_str[SD_MAX_MODIFICATION_TIME_LEN], *n_mt_str;
  char na_str[LOOKUP_ADDRESS_LEN], *n_na_str;
  char ns_str[LOOKUP_SERVICE_LEN], *n_ns_str;
  char essl_str[SD_MAX_PROTOCOL_CONST_VALUE_LEN], *n_essl_str;
  char cm_str[SD_MAX_PROTOCOL_CONST_VALUE_LEN], *n_cm_str;

  uint64_t *id;
  uint64_t *size;


  /* allocate all arguments */
  SAFE_CALLOC(id, 1, sizeof(uint64_t));
  SAFE_CALLOC(size, 1, sizeof(uint64_t));

  if (sscanf(args, pci->unpack_arg_fmt,
      /* args */
      id, fn_str, size, mt_str, essl_str, cm_str, na_str, ns_str
      ) != pci->nargs)
  {
    SAFE_FREE(id);
    SAFE_FREE(size);
    return -1;
  }
  
  SAFE_CALLOC(n_fn_str, 1, SD_MAX_FILENAME_LEN);
  SAFE_CALLOC(n_mt_str, 1, SD_MAX_MODIFICATION_TIME_LEN);
  SAFE_CALLOC(n_na_str, 1, LOOKUP_ADDRESS_LEN);
  SAFE_CALLOC(n_ns_str, 1, LOOKUP_SERVICE_LEN);
  SAFE_CALLOC(n_essl_str, 1, SD_MAX_PROTOCOL_CONST_VALUE_LEN);
  SAFE_CALLOC(n_cm_str, 1, SD_MAX_PROTOCOL_CONST_VALUE_LEN);

  string_url_decode(n_fn_str, fn_str, sizeof fn_str);
  string_url_decode(n_mt_str, mt_str, sizeof mt_str);
  string_url_decode(n_na_str, na_str, sizeof na_str);
  string_url_decode(n_ns_str, ns_str, sizeof ns_str);
  string_url_decode(n_cm_str, cm_str, sizeof cm_str);
  string_url_decode(n_essl_str, essl_str, sizeof essl_str);
  
  init_protocol_command_entry(pi, pci,
      /* args */
      id, n_fn_str, size, n_mt_str, n_essl_str, n_cm_str, n_na_str, n_ns_str
      );
  return 0;
}

/* this is used as a lot of packing is required */
int file_suggest_command_pack_and_send(struct sd_data_transfer_info *dti)
{
  char *enc_a;
  char *enc_s;
  char *cm;
  char *e_ssl;
  char *enc_f_name, *enc_m_time;

  SAFE_CALLOC(enc_a, 1, LOOKUP_ADDRESS_LEN);
  SAFE_CALLOC(enc_s, 1, LOOKUP_SERVICE_LEN);
  SAFE_CALLOC(enc_f_name, 1, sizeof dti->file.name);
  SAFE_CALLOC(enc_m_time, 1, sizeof dti->file.modtime);

  string_url_encode(enc_f_name, dti->file.name, sizeof dti->file.name);
  string_url_encode(enc_m_time, dti->file.modtime, sizeof dti->file.modtime);

  switch(dti->con_meth)
  {
    case CON_METH_PASSIVE:
      string_url_encode(enc_a, dti->wan_address, LOOKUP_ADDRESS_LEN);
      string_url_encode(enc_s, dti->wan_service, LOOKUP_SERVICE_LEN);
      if (dti->using_local_address)
        e_ssl = get_boolean_string(dti->data_server->enable_ssl);
      else
        e_ssl = get_boolean_string(SD_OPTION_OFF);

      cm = SD_PROTOCOL_VALUE_PASSIVE;
      break;
    case CON_METH_ACTIVE:
      {
      char *local_port;
      local_port = get_sockaddr_storage_port_string(&dti->data_con.src_sa);
      string_url_encode(enc_a, dti->wan_address, LOOKUP_ADDRESS_LEN);
      string_url_encode(enc_s, local_port, LOOKUP_SERVICE_LEN);
      e_ssl = get_boolean_string(dti->data_con.enable_ssl);
      cm = SD_PROTOCOL_VALUE_ACTIVE;
      SAFE_FREE(local_port);
      }
      break;
  }

  int ret;
  
  if (!(ret = send_protocol_command(dti->parent_peer, "FILE-SUGGEST",
      /* args */
      dti->id,
      enc_f_name, dti->file.size, enc_m_time,
      e_ssl,
      cm, enc_a, enc_s)))
  {
  }

  SAFE_FREE(enc_a);
  SAFE_FREE(enc_s);
  SAFE_FREE(e_ssl);
  SAFE_FREE(enc_f_name);
  SAFE_FREE(enc_m_time);
  
  data_transfer_set_state(dti, DATA_TRANSFER_STATE_VERDICT_PENDING);

  return ret;
}

void file_suggest_command_process_cb(struct sd_peer_info *pi, linked_list *args)
{
  int na;
  void **a;
  
  na = linked_list_get_all_values(args, &a);

  /* do something with data */
  //struct sd_data_transfer_info *dti;

  char *saddr;
  saddr = get_sockaddr_storage_string(&pi->ctl_con.dst_sa);

  /* if file id alread exists, ignore */
  if (get_data_transfer_from_id(pi, DATA_TRANSFER_DIRECTION_INCOMING,
        (uint64_t *)a[0]))
  {
    ui_notify_printf("Recieved a file suggestion with occupied ID from %s", saddr);
    goto file_suggest_cleanup;
  }

  int e_ssl;
  if ((e_ssl = get_boolean_id_from_string((const char *)a[4])) == -1)
  {
    ui_notify_printf("Recieved a file suggestion with invalid boolean value from %s",
        saddr);
    goto file_suggest_cleanup;
  }

  int cm;
  if ((cm = get_con_meth_id_from_string((const char *)a[5])) == -1)
  {
    ui_notify_printf(
        "Recieved a file suggestion with invalid connection method value from %s",
        saddr);
    goto file_suggest_cleanup;
  }
    /* args:
     *   file_id
     *   file_name
     *   file_size
     *   file_modification_time
     *   enable_ssl
     *   data_connection_method
     *   net_address
     *   port
     */
  

  /* init incomming transfer */
  struct file_info fi;
  file_manual_set_info(&fi,
      (const char *)a[1],
      NULL,
      (const char *)a[3],
      NULL,
      (uint64_t *)a[2]);

  struct sd_data_transfer_info *dti;
  dti = data_transfer_init(
      pi,
      SD_OPTION_OFF, NULL, /* wait till setup */
      &fi, DATA_TRANSFER_DIRECTION_INCOMING);


  /* semi-set con meth */
  dti->con_meth = cm == CON_METH_ACTIVE ? CON_METH_PASSIVE : CON_METH_ACTIVE;
  dti->peer_using_ssl = e_ssl;

  data_transfer_set_id(dti, DATA_TRANSFER_DIRECTION_INCOMING, (uint64_t *)a[0]);

  /* method specific */
  switch (dti->con_meth)
  {
    case CON_METH_PASSIVE:
      data_transfer_setup_passive_remote_address(dti, (const char *)a[6], (const char *)a[7]);
      break;
    case CON_METH_ACTIVE:
      data_transfer_setup_active_remote_address(dti, (const char *)a[6], (const char *)a[7]);
      break;
  }


file_suggest_cleanup:
  SAFE_FREE(saddr);
  SAFE_FREE(a);
  
  ui_data_transfer_change(dti, SD_DATA_TRANSFER_CHANGE_TYPE_UPDATE);
}
/* ----------------- file-suggest command end ------------------ */

/* ----------------- file-verdict command begin ------------------ */
int file_verdict_command_unpack_cb(struct sd_peer_info *pi,
    struct sd_protocol_command_info *pci, const char *args)
{
  char ver_str[SD_MAX_PROTOCOL_CONST_VALUE_LEN], *n_ver_str;
  char na_str[LOOKUP_ADDRESS_LEN], *n_na_str;
  char ns_str[LOOKUP_SERVICE_LEN], *n_ns_str;

  uint64_t *id;
  uint64_t *size;

  /* allocate all arguments */
  SAFE_CALLOC(id, 1, sizeof(uint64_t));
  SAFE_CALLOC(size, 1, sizeof(uint64_t));

  if (sscanf(args, pci->unpack_arg_fmt,
      /* args */
      id, ver_str, na_str, ns_str, size
      ) != pci->nargs)
  {
    SAFE_FREE(id);
    return -1;
  }
  
  SAFE_CALLOC(n_ver_str, 1, SD_MAX_PROTOCOL_CONST_VALUE_LEN);
  SAFE_CALLOC(n_na_str, 1, LOOKUP_ADDRESS_LEN);
  SAFE_CALLOC(n_ns_str, 1, LOOKUP_SERVICE_LEN);

  string_url_decode(n_na_str, na_str, sizeof na_str);
  string_url_decode(n_ns_str, ns_str, sizeof ns_str);
  string_url_decode(n_ver_str, ver_str, sizeof ver_str);
  
  init_protocol_command_entry(pi, pci,
      /* args */
      id, n_ver_str, n_na_str, n_ns_str, size
      );

  return 0;
}

/* this is used as a lot of packing is required */
int file_verdict_command_pack_and_send(struct sd_data_transfer_info *dti, char verdict)
{
  char *enc_a;
  char *enc_s;
  char *enc_ver;

  SAFE_CALLOC(enc_a, 1, LOOKUP_ADDRESS_LEN);
  SAFE_CALLOC(enc_s, 1, LOOKUP_SERVICE_LEN);
  SAFE_CALLOC(enc_ver, 1, SD_MAX_PROTOCOL_CONST_VALUE_LEN);

  switch (verdict)
  {
    case DATA_TRANSFER_VERDICT_ACCEPTED: {
      string_url_encode(enc_ver, SD_PROTOCOL_VALUE_ACCEPT, SD_MAX_PROTOCOL_CONST_VALUE_LEN);
      string_url_encode(enc_a, dti->wan_address, sizeof dti->wan_address);

      if (dti->con_meth == CON_METH_PASSIVE) {
        string_url_encode(enc_s, dti->wan_service, LOOKUP_SERVICE_LEN);
      }
      else { /* active */
        char *local_port;
        local_port = get_sockaddr_storage_port_string(&dti->data_con.src_sa);
        string_url_encode(enc_s, local_port, LOOKUP_SERVICE_LEN);
        SAFE_FREE(local_port);
      }
                                         }
      break;
    case DATA_TRANSFER_VERDICT_DECLINDED: {
      string_url_encode(enc_ver, SD_PROTOCOL_VALUE_DECLINE, SD_MAX_PROTOCOL_CONST_VALUE_LEN);
      string_url_encode(enc_a, SD_PROTOCOL_VALUE_NULL, strlen(SD_PROTOCOL_VALUE_NULL) + 1);
      string_url_encode(enc_s, SD_PROTOCOL_VALUE_NULL, strlen(SD_PROTOCOL_VALUE_NULL) + 1);
                                          }
      break;
    default:
      /* should cleanup */
      return -1;
  }

  int ret;
  
  ret = send_protocol_command(dti->parent_peer, "FILE-VERDICT",
      /* args */
      dti->id,
      enc_ver,
      enc_a,
      enc_s,
      dti->file.position);

  if (ret == -1 || verdict == DATA_TRANSFER_VERDICT_DECLINDED)
  {
    data_transfer_abort(dti);
  }
  else
  {
    /* finished setting up */
    //data_transfer_set_state(dti, DATA_TRANSFER_STATE_PREPARATION_PENDING);
  }

  SAFE_FREE(enc_a);
  SAFE_FREE(enc_s);
  SAFE_FREE(enc_ver);
  
  data_transfer_set_verdict(dti, verdict);

  return ret;
}

void file_verdict_command_process_cb(struct sd_peer_info *pi, linked_list *args)
{
  int na;
  void **a;
  
  na = linked_list_get_all_values(args, &a);

  /* do something with data */
  char *saddr;
  saddr = get_sockaddr_storage_string(&pi->ctl_con.dst_sa);

  /* if file id doesn't exist, ignore message */
  struct sd_data_transfer_info *dti;

  dti = get_data_transfer_from_id(pi, DATA_TRANSFER_DIRECTION_OUTGOING,
        (uint64_t *)a[0]);
  if (!dti)
  {
    ui_notify_printf("Recieved a transfer verdict with invalid ID from %s", saddr);
    goto file_verdict_cleanup;
  }

  if (dti->state != DATA_TRANSFER_STATE_VERDICT_PENDING)
  {
    ui_notify_printf("Recieved a transfer verdict for transfer not "
       "waiting for verdict from %s", saddr);
    goto file_verdict_cleanup;

  }

  int ver;
  ver = get_verdict_id_from_string((const char *)a[1]);
  if (ver == -1 || ver == DATA_TRANSFER_STATE_VERDICT_PENDING)
  {
    ui_notify_printf("Recieved a transfer verdict with invalid verdict parameter "
        "from %s", saddr);
    goto file_verdict_cleanup;
  }
  
  data_transfer_set_verdict(dti, ver);

  switch (dti->verdict)
  {
    case DATA_TRANSFER_VERDICT_ACCEPTED:
      /* update file info */
      dti->file.position = *((uint64_t *)a[4]);

      switch (dti->con_meth)
      {
        case CON_METH_PASSIVE:
          /* add recived address to allow connection from */
          data_transfer_setup_passive_remote_address(
              dti,
              (const char *)a[2],
              (const char *)a[3]);
          data_transfer_setup_passive_accept(dti);
          break;
        case CON_METH_ACTIVE:
          /* add recived address to connect to */
          data_transfer_setup_active_remote_address(
              dti,
              (const char *)a[2],
              (const char *)a[3]);
          /* finished setting up */
          data_transfer_set_state(dti, DATA_TRANSFER_STATE_PREPARATION_PENDING);

          break;
      }
      break;
    case DATA_TRANSFER_VERDICT_DECLINDED:
      data_transfer_abort(dti);
      break;
  }


file_verdict_cleanup:
  SAFE_FREE(saddr);
  SAFE_FREE(a);
  
  ui_data_transfer_change(dti, SD_DATA_TRANSFER_CHANGE_TYPE_UPDATE);
}
/* ----------------- file-verdict command end ------------------ */

/* --------------------- file-prepared command begin ------------------------ */
int file_prepared_command_pack_and_send(struct sd_data_transfer_info *dti)
{
  char *dir;

  switch (dti->direction)
  {
    case DATA_TRANSFER_DIRECTION_INCOMING: dir = SD_PROTOCOL_VALUE_INCOMING; break;
    case DATA_TRANSFER_DIRECTION_OUTGOING: dir = SD_PROTOCOL_VALUE_OUTGOING; break;
    default: return -1;
  }
  
  send_protocol_command(dti->parent_peer, "FILE-PREPARED", dti->id, dir);
  return 0;
}
int file_prepared_command_unpack_cb(struct sd_peer_info *pi,
    struct sd_protocol_command_info *pci, const char *args)
{
  char dir_str[SD_MAX_PROTOCOL_CONST_VALUE_LEN], *n_dir_str;
  uint64_t *id;

  SAFE_CALLOC(id, 1, sizeof(uint64_t));

  if (sscanf(args, pci->unpack_arg_fmt, id, dir_str) != pci->nargs)
    return -1;

  SAFE_CALLOC(n_dir_str, 1, SD_MAX_PROTOCOL_CONST_VALUE_LEN);
  string_url_decode(n_dir_str, dir_str, sizeof dir_str);
  
  init_protocol_command_entry(pi, pci, id, n_dir_str);
  return 0;
}
void file_prepared_command_process_cb(struct sd_peer_info *pi, linked_list *args)
{
  int na;
  void **a;
  
  na = linked_list_get_all_values(args, &a);
  
  char *saddr;
  saddr = get_sockaddr_storage_string(&pi->ctl_con.dst_sa);

  /* get direction id */
  int dir;
  dir = get_direction_id_from_string((const char *)a[1]);
  if (dir == -1)
  {
    ui_notify_printf("Recieved a transfer prepared message with invalid direction parameter "
        "from %s", saddr);
    goto file_prepared_cleanup;
  }
  /* flip the direction */
  dir = dir == DATA_TRANSFER_DIRECTION_OUTGOING ? DATA_TRANSFER_DIRECTION_INCOMING :
    DATA_TRANSFER_DIRECTION_OUTGOING;


  /* check if id is valid */
  struct sd_data_transfer_info *dti;
  dti = get_data_transfer_from_id(pi, dir, (uint64_t *)a[0]);
  if (!dti)
  {
    ui_notify_printf("Recieved a transfer prepared message with invalid ID from %s",
        saddr);
    goto file_prepared_cleanup;
  }

  /* check if already prepared */
  if (dti->peer_prepared == SD_OPTION_ON)
  {
    ui_notify_printf("Recieved transfer prepared message where transfer is already "
        "prepared from %s", saddr);
    goto file_prepared_cleanup;
  }

  /* set prepared */
  dti->peer_prepared = SD_OPTION_ON;
  
file_prepared_cleanup:
  SAFE_FREE(saddr);
  SAFE_FREE(a);
}
/* --------------------- file-prepared command end ------------------------ */

// vim:ts=2:expandtab
