/*
   Peer object handling
 
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

#include <time.h>

#include "sd.h"
#include "sd_globals.h"
#include "sd_peers.h"
#include "sd_error.h"
#include "sd_dynamic_memory.h"
#include "sd_net.h"
#include "sd_ui.h"
#include "sd_protocol.h"
#include "sd_protocol_commands.h"


/* -[ peers ]---------------------------------------------------------- */

struct sd_peer_info *peer_init(const char *a, const char *p, char enable_ssl,
    struct sd_ssl_verify_info *vi)
{
  struct sd_peer_info *new_peer;

  new_peer = (struct sd_peer_info *) peer_add()->value;
  
  /* set up control connection */
  con_init(&new_peer->ctl_con, a, p, enable_ssl, vi, CON_TYPE_CONTROL);

  /* set up rest */
  new_peer->ctl_buffer_offset = 0;
  new_peer->ctl_con_verified = SD_OPTION_OFF;
  
  linked_list_init(&new_peer->ctl_cmd_backlog);
  linked_list_init(&new_peer->data_transfers);

  new_peer->ctl_con_verified = SD_OPTION_OFF;

  return new_peer;
}

struct list_item *peer_add()
{
  list_item *new_peer_item;
  struct sd_peer_info *new_peer;

  /* allocate memory for new peer */
  SAFE_CALLOC(new_peer, 1, sizeof(struct sd_peer_info));
  /* add peer to linked list */
  new_peer_item = linked_list_add(&gbls->net->peers, (void *) new_peer);
  return new_peer_item;
}

int peer_get_state_delete_iterate(void *value, int index)
{
  if (((struct sd_peer_info *)value)->ctl_con.state ==
      CON_STATE_DELETE)
  {
    peer_deinit((struct sd_peer_info *)value);
    return 1; /* stop iterating */
  }
  return 0;
}

int peer_rem_all_state_delete()
{
  list_item *li;
  int nrem;

  nrem = 0;

  while ((li = linked_list_iterate(&gbls->net->peers,
          &peer_get_state_delete_iterate)) != NULL)
  {
    linked_list_rem(&gbls->net->peers, li, SD_OPTION_ON);
    nrem++;
  }

  return nrem;
}

void peer_set_closed(struct sd_peer_info *pi)
{
  linked_list_iterate(&pi->data_transfers, &abort_unest_data_transfers);

  sd_set_state(&pi->ctl_con.state, CON_STATE_CLOSED);
}

void peer_deinit(struct sd_peer_info *pi)
{
  linked_list_deinit_rem_all_entries(
      &pi->data_transfers,
      SD_OPTION_ON,
      (void (*)(void *)) &data_transfer_deinit);
}



/* -[ peers : getters ] ----------------------------------------------- */

/* get the number of established peers */
static int est_peer_count;
int est_peer_iter_cb(void *value, int index)
{
  if (((struct sd_peer_info *)(value))->ctl_con.state == CON_STATE_ESTABLISHED)
    est_peer_count++;
  return 0;
}

int get_est_peer_count() {
  est_peer_count = 0;
  linked_list_iterate(&gbls->net->peers, &est_peer_iter_cb);
  return est_peer_count;
}

/* for getting a pointer to peer from control connection */
static struct sd_con_info *con_for_get_peer;
int con_get_peer_iterate(void *value, int index) {

  if (&((struct sd_peer_info *)value)->ctl_con ==
      con_for_get_peer)
    return 1;
  return 0;
}
struct sd_peer_info *con_get_peer(struct sd_con_info *ci)
{
  list_item *li;

  con_for_get_peer = ci;
  li = linked_list_iterate(&gbls->net->peers,
          &con_get_peer_iterate);

  return (struct sd_peer_info *)(li->value);
}


/* -[ peers : setters ] ----------------------------------------------- */


/* -[ data transfers ]------------------------------------------------- */

struct sd_data_transfer_info *data_transfer_init(
    struct sd_peer_info *pi,
    char enable_ssl, struct sd_ssl_verify_info *vi,
    struct file_info *fi, char dir)
{
  struct sd_data_transfer_info *new_dt;

  new_dt = (struct sd_data_transfer_info *) data_transfer_add(pi)->value;
  
  new_dt->direction = dir;
  new_dt->peer_prepared = SD_OPTION_OFF;
  new_dt->verdict = DATA_TRANSFER_VERDICT_PENDING;
  new_dt->transfer_state = DATA_TRANSFER_TRANSFER_STATE_RESUMED;
  new_dt->state = DATA_TRANSFER_STATE_SETUP_PENDING;

  new_dt->data_buffer_lower_offset = 0;
  new_dt->data_buffer_window_size = 0;


  /* file */

  memcpy(&new_dt->file, fi, sizeof(new_dt->file));
  
  if (new_dt->direction == DATA_TRANSFER_DIRECTION_INCOMING)
  {
    /* default output */
    snprintf(
        new_dt->file.directory,
        sizeof new_dt->file.directory,
        "%s",
        gbls->conf->data_output_path);
  }

  /* set up data connection */
  con_init(
      &new_dt->data_con,
      NULL,
      NULL,
      enable_ssl, vi, CON_TYPE_DATA);

  /* set local */
  resolve_addr_set_info(
      &new_dt->data_con.resolve_src_addr,
      gbls->conf->data_local_net_address,
      NULL,
      SD_OPTION_OFF);

  /* set wan */
  data_transfer_set_wan(
      new_dt,
      gbls->conf->data_wide_net_address,
      gbls->conf->data_wide_service);

  new_dt->parent_peer = pi;

  ui_data_transfer_change(new_dt, SD_DATA_TRANSFER_CHANGE_TYPE_ADD);

  return new_dt;
}

struct list_item *data_transfer_add(struct sd_peer_info *pi)
{
  list_item *new_dt_item;
  struct sd_data_transfer_info *new_dt;

  /* allocate memory for new data transfer */
  SAFE_CALLOC(new_dt, 1, sizeof(struct sd_data_transfer_info));
  /* add tranfer to linked list */
  new_dt_item = linked_list_add(&pi->data_transfers, (void *) new_dt);
  return new_dt_item;
}

void data_transfer_init_io(struct sd_data_transfer_info *dti)
{
  dti->io_total_bytes_current = dti->file.position;
  dti->io_total_bytes_last = dti->file.position;

  data_transfer_reset_io(dti);
}

int data_transfer_idle_iter(void *value, int index)
{
  linked_list_iterate(&((struct sd_peer_info *)value)->data_transfers,
      &handle_data_transfer_state);
  return 0;
}

int data_transfer_abort(struct sd_data_transfer_info *dti)
{
  switch (dti->state)
  {
    case DATA_TRANSFER_STATE_ABORTED:
    case DATA_TRANSFER_STATE_COMPLETED:
      return -1;
      break;
  }

  /* close created, unconnected sockets */
  switch (dti->state)
  {
    case DATA_TRANSFER_STATE_VERDICT_PENDING:
    case DATA_TRANSFER_STATE_PREPARATION_PENDING:
      if (dti->con_meth == CON_METH_ACTIVE) {
        socket_close(&dti->data_con.sock_fd, SD_OPTION_OFF, SD_OPTION_OFF, NULL);
      }
      break;
  }
  
  switch (dti->state)
  {
    case DATA_TRANSFER_STATE_TRANSFERING:
      /* were connected */
      if (dti->data_con.state == CON_STATE_ESTABLISHED) {
	      socket_close(&dti->data_con.sock_fd, SD_OPTION_ON, dti->data_con.enable_ssl,
            dti->data_con.ssl);
      }

      break;
  }

  data_transfer_set_state(dti, DATA_TRANSFER_STATE_ABORTED);

  ui_notify_printf("Transfer was aborted.");

  return 0;
}

void data_transfer_deinit(struct sd_data_transfer_info *dti)
{

}


void data_transfer_reset_io(struct sd_data_transfer_info *dti)
{
  dti->io_byte_diff_zero = SD_OPTION_OFF;
  
  gettimeofday(&dti->io_time_last, NULL);
  gettimeofday(&dti->io_time_current, NULL);
  
  data_transfer_set_io(dti);
}


int abort_unest_data_transfers(void *v, int i)
{
  if (((struct sd_data_transfer_info *)v)->data_con.state !=
      CON_STATE_ESTABLISHED)
  {
    data_transfer_abort((struct sd_data_transfer_info *)v);
  }
  return 0;
}

int handle_data_transfer_state(void *v, int index)
{
  struct sd_data_transfer_info *dti = (struct sd_data_transfer_info *)v;

  switch (dti->state)
  {
    case DATA_TRANSFER_STATE_SETUP_PENDING:
      break;
    case DATA_TRANSFER_STATE_ACTIVE_RESOLVE_SRC_IP:
      switch (dti->data_con.state)
      {
        case CON_STATE_RESOLVED_SRC_IP:
          switch (dti->direction)
          {
            case DATA_TRANSFER_DIRECTION_INCOMING:
              file_verdict_command_pack_and_send(dti, DATA_TRANSFER_VERDICT_ACCEPTED);
              data_transfer_set_state(dti, DATA_TRANSFER_STATE_PREPARATION_PENDING);
              break;
            case DATA_TRANSFER_DIRECTION_OUTGOING:
              data_transfer_set_state(dti, DATA_TRANSFER_STATE_VERDICT_PENDING);
              file_suggest_command_pack_and_send(dti);
              break;
          }
          break;
        case CON_STATE_CLOSED:
          data_transfer_abort(dti);
          break;
      }
      break;
    case DATA_TRANSFER_STATE_VERDICT_PENDING:
      break;
    case DATA_TRANSFER_STATE_PASSIVE_RESOLVE_ACCEPT_IP:
      switch (dti->accept_info->state)
      {
        case SERVER_ACCEPT_STATE_ERROR:
          sd_set_state(&dti->accept_info->state, SERVER_ACCEPT_STATE_DELETE);
          data_transfer_abort(dti);
          break;
        case SERVER_ACCEPT_STATE_RESOLVED:
          switch (dti->direction)
          {
            case DATA_TRANSFER_DIRECTION_OUTGOING:
              /* prepared */
              data_transfer_set_state(dti, DATA_TRANSFER_STATE_PREPARATION_PENDING);
              break;
            case DATA_TRANSFER_DIRECTION_INCOMING:
              /* prepared */
              file_verdict_command_pack_and_send(dti, DATA_TRANSFER_VERDICT_ACCEPTED);
              data_transfer_set_state(dti, DATA_TRANSFER_STATE_PREPARATION_PENDING);
              break;
          }
          break;
      }
      break;
    case DATA_TRANSFER_STATE_PREPARATION_PENDING:
      if (dti->peer_prepared)
      {
        data_transfer_set_state(dti, DATA_TRANSFER_STATE_TRANSFERING);
        switch (dti->con_meth)
        {
          case CON_METH_ACTIVE:
            con_start_state_machine(&dti->data_con,
                CON_STATE_RESOLVE_DST_IP, SD_OPTION_ON);
            break;
          case CON_METH_PASSIVE:
            break;
        }
      }
      break;
    case DATA_TRANSFER_STATE_TRANSFERING:
      if (dti->data_con.state == CON_STATE_ESTABLISHED)
      {
        switch (dti->file.state)
        {
          case FILE_STATE_CLOSED:
            if (file_open(&dti->file, dti->direction) == -1)
              data_transfer_abort(dti);
            break;
          case FILE_STATE_OPENED:
            switch (dti->direction)
            {
              case DATA_TRANSFER_DIRECTION_OUTGOING:
                handle_data_send(dti,
                    dti->data_buffer,
                    sizeof dti->data_buffer);
                break;
              case DATA_TRANSFER_DIRECTION_INCOMING:
                handle_data_recv(dti,
                    &gbls->net->master_fd_set,
                    gbls->net->highest_fd,
                    sizeof dti->data_buffer);
                break;
            }
            break;
        }
      }
      else
      {
      }
      break;
    case DATA_TRANSFER_STATE_COMPLETED:
      break;
    case DATA_TRANSFER_STATE_ABORTED:
      break;
  }

  return 0;
}


/* -[ data transfers : getters ]--------------------------------------- */

/* get next transfer id */
static uint64_t next_transfer_id;
static char next_transfer_direction;
int next_transfer_id_iter_cb(void *value, int index)
{
  if (((struct sd_data_transfer_info *)(value))->direction == 
      next_transfer_direction)
  {
    if (((struct sd_data_transfer_info *)(value))->id > next_transfer_id)
      next_transfer_id = ((struct sd_data_transfer_info *)(value))->id;
  }
  return 0;
}

uint64_t *get_next_transfer_id(struct sd_peer_info *pi, char dir)
{
  uint64_t *id;

  next_transfer_id = 0;
  next_transfer_direction = dir;

  linked_list_iterate(&pi->data_transfers, &next_transfer_id_iter_cb);

  /* we have current heightest, inc for next id */
  next_transfer_id++;

  SAFE_CALLOC(id, 1, sizeof(uint64_t));
  *id = next_transfer_id;

  return id;
}

/* transfer id exists*/
static uint64_t data_transfer_id;
static char data_transfer_direction;
int data_transfer_id_iter_cb(void *value, int index)
{
  if (((struct sd_data_transfer_info *)(value))->direction == 
      data_transfer_direction)
  {
    if (((struct sd_data_transfer_info *)(value))->id == data_transfer_id)
      return 1;
  }
  return 0;
}

struct sd_data_transfer_info *get_data_transfer_from_id(
    struct sd_peer_info *pi, char dir, uint64_t *id)
{
  list_item *li;

  data_transfer_direction = dir;
  data_transfer_id = *id;

  li = linked_list_iterate(&pi->data_transfers, &data_transfer_id_iter_cb);

  if (!li)
    return NULL;

  return (struct sd_data_transfer_info *)li->value;
}

int get_verdict_id_from_string(const char *str)
{
  if (!strncasecmp(SD_PROTOCOL_VALUE_ACCEPT, str, strlen(SD_PROTOCOL_VALUE_ACCEPT)))
    return DATA_TRANSFER_VERDICT_ACCEPTED;
  if (!strncasecmp(SD_PROTOCOL_VALUE_DECLINE, str, strlen(SD_PROTOCOL_VALUE_DECLINE)))
    return DATA_TRANSFER_VERDICT_DECLINDED;

  return -1;
}

int get_con_meth_id_from_string(const char *str)
{
  if (!strncasecmp(SD_PROTOCOL_VALUE_ACTIVE, str, strlen(SD_PROTOCOL_VALUE_ACTIVE)))
    return CON_METH_ACTIVE;
  if (!strncasecmp(SD_PROTOCOL_VALUE_PASSIVE, str, strlen(SD_PROTOCOL_VALUE_PASSIVE)))
    return CON_METH_PASSIVE;

  return -1;
}

int get_direction_id_from_string(const char *str)
{
  if (!strncasecmp(SD_PROTOCOL_VALUE_OUTGOING, str, strlen(SD_PROTOCOL_VALUE_OUTGOING)))
    return DATA_TRANSFER_DIRECTION_OUTGOING;
  if (!strncasecmp(SD_PROTOCOL_VALUE_INCOMING, str, strlen(SD_PROTOCOL_VALUE_INCOMING)))
    return DATA_TRANSFER_DIRECTION_INCOMING;

  return -1;
}

char *get_data_transfer_state_string(struct sd_data_transfer_info *dti)
{
  char *str, *nstr;
  int len;

  switch (dti->state)
  {
    case DATA_TRANSFER_STATE_SETUP_PENDING:             str = "AWAITING SETUP"; break;
    case DATA_TRANSFER_STATE_ACTIVE_RESOLVE_SRC_IP:    str = "RESOLVING SOURCE ADDRESS"; break;
    case DATA_TRANSFER_STATE_PASSIVE_RESOLVE_ACCEPT_IP:  str = "RESOLVING ADDRESS TO ACCEPT"; break;
    case DATA_TRANSFER_STATE_VERDICT_PENDING:           str = "AWAITING VERDICT"; break;
    case DATA_TRANSFER_STATE_PREPARATION_PENDING:       str = "AWAITING PREPARATION"; break;
    case DATA_TRANSFER_STATE_TRANSFERING:               str = "TRANSFERING"; break;
    case DATA_TRANSFER_STATE_COMPLETED:                 str = "COMPLETED"; break;
    case DATA_TRANSFER_STATE_ABORTED:                   str = "ABORTED"; break;
    default:
      str = "ERROR";
  }
  
  len = strlen(str) + 1;
  SAFE_CALLOC(nstr, 1, len);
  memcpy(nstr, str, len);

  return nstr;
}

char *get_data_transfer_transfer_state_string(struct sd_data_transfer_info *dti)
{
  char *str, *nstr;
  int len;

  switch (dti->transfer_state)
  {
    case DATA_TRANSFER_TRANSFER_STATE_PENDING: str = "PENDING"; break;
    case DATA_TRANSFER_TRANSFER_STATE_PAUSED: str = "PAUSED"; break;
    case DATA_TRANSFER_TRANSFER_STATE_RESUMED: str = "RESUMED"; break;
    default:
      str = "ERROR";
  }
  
  len = strlen(str) + 1;
  SAFE_CALLOC(nstr, 1, len);
  memcpy(nstr, str, len);

  return nstr;
}

char *get_data_transfer_verdict_string(struct sd_data_transfer_info *dti)
{
  char *str, *nstr;
  int len;

  switch (dti->verdict)
  {
    case DATA_TRANSFER_VERDICT_PENDING: str = "PENDING"; break;
    case DATA_TRANSFER_VERDICT_ACCEPTED: str = "ACCEPTED"; break;
    case DATA_TRANSFER_VERDICT_DECLINDED: str = "DECLINED"; break;
    default: str = "ERROR";
  }
  
  len = strlen(str) + 1;
  SAFE_CALLOC(nstr, 1, len);
  memcpy(nstr, str, len);

  return nstr;
}

char *get_data_transfer_direction_string(struct sd_data_transfer_info *dti)
{
  char *str, *nstr;
  int len;

  switch (dti->direction)
  {
    case DATA_TRANSFER_DIRECTION_INCOMING: str = "INCOMING"; break;
    case DATA_TRANSFER_DIRECTION_OUTGOING: str = "OUTGOING"; break;
    default: str = "ERROR";
  }
  
  len = strlen(str) + 1;
  SAFE_CALLOC(nstr, 1, len);
  memcpy(nstr, str, len);

  return nstr;
}
char *get_data_transfer_direction_arrow_string(struct sd_data_transfer_info *dti)
{
  char *str, *nstr;
  int len;

  switch (dti->direction)
  {
    case DATA_TRANSFER_DIRECTION_INCOMING: str = "<--"; break;
    case DATA_TRANSFER_DIRECTION_OUTGOING: str = "-->"; break;
    default: str = "ERROR";
  }
  
  len = strlen(str) + 1;
  SAFE_CALLOC(nstr, 1, len);
  memcpy(nstr, str, len);

  return nstr;
}
char *get_data_transfer_con_method_string(struct sd_data_transfer_info *dti)
{
  char *str, *nstr;
  int len;

  switch (dti->con_meth)
  {
    case CON_METH_PASSIVE: str = "PASSIVE"; break;
    case CON_METH_ACTIVE: str = "ACTIVE"; break;
    default: str = "ERROR";
  }
  
  len = strlen(str) + 1;
  SAFE_CALLOC(nstr, 1, len);
  memcpy(nstr, str, len);

  return nstr;
}


/* -[ data transfers : setters ]--------------------------------------- */

void data_transfer_set_active(struct sd_data_transfer_info *dti,
    const char *src_a, const char *src_s) /* bind for connect */
{
  dti->con_meth = CON_METH_ACTIVE;

  resolve_addr_set_info(&dti->data_con.resolve_src_addr, src_a, src_s,
    SD_OPTION_ON /* don't worry about service */
  );
  
  
  ui_data_transfer_change(dti, SD_DATA_TRANSFER_CHANGE_TYPE_UPDATE);
}

void data_transfer_setup_active_remote_address(
    struct sd_data_transfer_info *dti, const char *a, const char *s)
{
  resolve_addr_set_info(&dti->data_con.resolve_dst_addr, a, s, SD_OPTION_OFF);

}

void data_transfer_setup_active_resolve_source(struct sd_data_transfer_info *dti)
{
  data_transfer_set_state(dti, DATA_TRANSFER_STATE_ACTIVE_RESOLVE_SRC_IP);
  con_start_state_machine(&dti->data_con, CON_STATE_RESOLVE_SRC_IP, SD_OPTION_ON);
}

void data_transfer_set_passive(struct sd_data_transfer_info *dti,
    char local_address, struct sd_serv_info *si, char any_port)
{
  dti->con_meth = CON_METH_PASSIVE;

  if (local_address) {
    dti->data_server = si;
    dti->using_local_address = SD_OPTION_ON;
    if (any_port) {
      dti->allow_any_port = SD_OPTION_ON;
    }
    else {
      dti->allow_any_port = SD_OPTION_OFF;
    }
  }
  else {
    dti->using_local_address = SD_OPTION_OFF;
  }
  
  ui_data_transfer_change(dti, SD_DATA_TRANSFER_CHANGE_TYPE_UPDATE);
}

void data_transfer_setup_passive_remote_address(
    struct sd_data_transfer_info *dti, const char *a, const char *s)
{
  resolve_addr_set_info(&dti->data_con.resolve_dst_addr, a, s, SD_OPTION_OFF);

}


/* on receiving file-verdict or before sending 
 * file-verdict accept for passive */
struct sd_serv_accept_info *data_transfer_setup_passive_accept(
    struct sd_data_transfer_info *dti)
{
  struct sd_serv_accept_info *ret;

  ret = NULL;

  if (dti->using_local_address)
  {
    data_transfer_set_state(dti, DATA_TRANSFER_STATE_PASSIVE_RESOLVE_ACCEPT_IP);
    /* start resolving peers connecting address */
    ret = server_accept_init(dti->data_server,
        dti->data_con.resolve_dst_addr.lookup_address,
        dti->data_con.resolve_dst_addr.lookup_service);
    ret->con = &dti->data_con;
    ret->allow_any_port = dti->allow_any_port;
    dti->accept_info = ret;

  }
  else
  {
    switch (dti->direction)
    {
      case DATA_TRANSFER_DIRECTION_INCOMING:
        data_transfer_set_state(dti, DATA_TRANSFER_STATE_PREPARATION_PENDING);
        file_verdict_command_pack_and_send(dti, DATA_TRANSFER_VERDICT_ACCEPTED);
        break;
      case DATA_TRANSFER_DIRECTION_OUTGOING:
        data_transfer_set_state(dti, DATA_TRANSFER_STATE_PREPARATION_PENDING);
        break;
    }
  }

  return ret;
}

void data_transfer_set_id(struct sd_data_transfer_info *dti, char direction,
    uint64_t *id)
{
  uint64_t *n_id;
  /* get next id */
  if (direction == DATA_TRANSFER_DIRECTION_OUTGOING)
  {
    n_id = get_next_transfer_id(dti->parent_peer, direction);
    dti->id = *n_id;
    SAFE_FREE(n_id);
  }
  else
  {
    dti->id = *id;
  }

  ui_data_transfer_change(dti, SD_DATA_TRANSFER_CHANGE_TYPE_UPDATE);
}

void data_transfer_set_verdict(struct sd_data_transfer_info *dti, char v)
{
  dti->verdict = v;

  switch (dti->verdict)
  {
    /* move to other list */
    case DATA_TRANSFER_VERDICT_ACCEPTED:
      ui_data_transfer_accepted(dti);
      break;
    case DATA_TRANSFER_VERDICT_DECLINDED:
      break;
  }

  ui_data_transfer_change(dti, SD_DATA_TRANSFER_CHANGE_TYPE_UPDATE);
}

void data_transfer_set_transfer_state(struct sd_data_transfer_info *dti, char v)
{
  dti->transfer_state = v;
  ui_data_transfer_change(dti, SD_DATA_TRANSFER_CHANGE_TYPE_UPDATE);
}

void data_transfer_set_state(struct sd_data_transfer_info *dti, char nstate)
{
  dti->state = nstate;

  switch (dti->state)
  {
    case DATA_TRANSFER_STATE_PREPARATION_PENDING:
      file_prepared_command_pack_and_send(dti);
      break;
    case DATA_TRANSFER_STATE_TRANSFERING:
      /* io */
      data_transfer_init_io(dti);
      break;
    case DATA_TRANSFER_STATE_COMPLETED:
      data_transfer_reset_io(dti);
      break;
  }

  ui_data_transfer_change(dti, SD_DATA_TRANSFER_CHANGE_TYPE_UPDATE);
}

void data_transfer_set_wan(struct sd_data_transfer_info *dti,
    const char *send_a, const char *send_s)  /* used for protocol message only */
{

  /* let the peer know how to get to here with WAN info */
  if (send_a)
    snprintf(dti->wan_address, sizeof dti->wan_address, "%s", send_a);
  if (send_s)
    snprintf(dti->wan_service, sizeof dti->wan_service, "%s", send_s);

  ui_data_transfer_change(dti, SD_DATA_TRANSFER_CHANGE_TYPE_UPDATE);
}


void data_transfer_set_io(struct sd_data_transfer_info *dti)
{
  struct timeval time_now;
  uint64_t bd;
  int td;
  
  gettimeofday(&time_now, NULL);

  td = (time_diff(&time_now, &dti->io_time_last));
  bd = dti->io_total_bytes_current - dti->io_total_bytes_last;

  if (td > UPDATE_PROGRESS_INTERVAL)
  {
    memcpy(&dti->io_time_last, &dti->io_time_current, sizeof (struct timeval));
    memcpy(&dti->io_time_current, &time_now, sizeof (struct timeval));

    ui_data_transfer_progress_change(dti);
    
    if (!bd)
      dti->io_byte_diff_zero = SD_OPTION_ON;
    else
      dti->io_byte_diff_zero = SD_OPTION_OFF;

    dti->io_total_bytes_last = dti->io_total_bytes_current;
  }

}


// vim:ts=2:expandtab
