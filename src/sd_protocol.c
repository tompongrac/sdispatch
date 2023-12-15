/*
   Text-based protocol module
 
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
#include <signal.h>
#include <fcntl.h>

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
#include "sd_protocol_commands.h"
#include "sd_version.h"


int string_url_encode(char *dst, const char *str, int nbytes)
{
  int x, i, stringlen, trn;

  stringlen = strlen(str);
  i = 0;
  trn = 0;

  for (x = 0; x < stringlen; x++)
  {
    if (isgraph(str[x]) && (str[x] != '%'))
    {
      goto copy_char;
    }
    else
    {
      /* check other valid chars */
      goto encode_char;
    }

copy_char:
    if (i < nbytes - 1)
    {
      dst[i] = str[x];
      i += 1;
      continue;
    }
    else {
      goto truncate;
    }

encode_char:
    if (i < nbytes - 3)
    {
      snprintf((dst + i), 4, "%%%x", str[x]);
      i += 3;
      continue;
    }
    else {
      goto truncate;
    }

truncate:
    trn = 1;
    break;

  }

  dst[i] = '\0';

  return trn;
}

#if 0
int string_url_decode(char *dst, const char *str, int nbytes)
{
  int x, i, stringlen, trn;
  char hexstr[3];

  stringlen = strlen(str);
  i = 0;
  trn = 0;

  for (x = 0; x < stringlen; x++)
  {
    if (i < nbytes - 1)
    {
      if (str[i] == '%')
      {
        if (stringlen - (i + 1) < 2)
        {
          goto truncate;
        }
        else
        {
          memcpy(hexstr, &str[i + 1], 2);
          hexstr[2] = '\0';
          dst[x] = (char) strtol(hexstr, NULL, 16);
          i += 3;
        }
      }
      else
      {
        dst[x] = str[i];
        i += 1;
      }
    }
    else
    {
truncate:
      trn = 1;
      break;
    }
  }

  dst[x] = '\0';

  return trn;
}
#else
int string_url_decode(char *dst, const char *str, int nbytes)
{
  int x, i, stringlen, trn;
  char hexstr[3];

  stringlen = strlen(str);
  i = 0;
  trn = 0;

  for (x = 0; x < nbytes && i < stringlen; x++)
  {
    if (str[i] == '%')
    {
      if (stringlen - (i + 1) < 2)
      {
        break;
      }
      else
      {
        memcpy(hexstr, &str[i + 1], 2);
        hexstr[2] = '\0';
        dst[x] = (char) strtol(hexstr, NULL, 16);
        i += 3;
      }
    }
    else
    {
      dst[x] = str[i];
      i += 1;
    }
  }

  dst[x] = '\0';

  return trn;
}
#endif

/* process single command entry then remove and dalloc its args */
int ctl_process_cmd_entry_iter_cb(void *v, int index)
{
  int i, ncmds;

  ncmds = get_control_command_qty();

  for (i = 0; i < ncmds; i++)
  {
    if (!strncasecmp(((struct sd_protocol_command_entry *) v)->name,
          control_command[i].name, SD_MAX_PROTOCOL_COMMAND_NAME_LEN))
    {
      (*control_command[i].process)(((struct sd_protocol_command_entry *)v)->peer,
          &((struct sd_protocol_command_entry *)v)->args);
      break;
    }
  }

  /* remove all entries */
  int n;
  n = linked_list_rem_all_entries(&((struct sd_protocol_command_entry *)v)->args, 1);

  return 0;
}

int ctl_process_cmd_iter_cb(void *v, int index)
{
  linked_list_iterate(&((struct sd_peer_info *) v)->ctl_cmd_backlog,
      ctl_process_cmd_entry_iter_cb);
  
  /* remove all entries */
  int n;
  n = linked_list_rem_all_entries(&((struct sd_peer_info *)v)->ctl_cmd_backlog, 1);

  return 0;
}

int command_pack(const char *name, char *b, int nbytes, va_list args)
{
  int i, ncmds;
  char fmt[512];

  ncmds = get_control_command_qty();

  for (i = 0; i < ncmds; i++)
  {
    if (!strncasecmp(name, control_command[i].name, SD_MAX_PROTOCOL_COMMAND_NAME_LEN))
      break;
  }

  if (i >= ncmds)
  {
    ui_notify("Attempted to pack an unrecognised command.");
    return -1;
  }

  snprintf(fmt, sizeof fmt, "%s %s\r\n",
      control_command[i].name, control_command[i].pack_arg_fmt);

  vsnprintf(b, nbytes, fmt, args);

  return 0;
}

int send_protocol_command(struct sd_peer_info *pi, const char *name, ...)
{
  va_list args;
  char sbuffer[SEND_BUFFER_LEN];
  int ret;

  va_start(args, name);
  ret = command_pack(name, sbuffer, sizeof sbuffer, args);
  va_end(args);

  /* don't send if invalid */
  if (ret == -1)
    return -1;

  char *saddr;
  char dmsg[SEND_BUFFER_LEN];
  int mlen;
  saddr = get_sockaddr_storage_string(&pi->ctl_con.dst_sa);
  mlen = strlen(sbuffer);
  strncpy(dmsg, sbuffer, mlen);
  dmsg[mlen - 2] = '\0';
  ui_notify_printf("Protocol --> %s \"%s\"", saddr, dmsg);
  SAFE_FREE(saddr);

  if (handle_ctl_send(pi, sbuffer, strlen(sbuffer)) == -1)
  {
    ctl_con_close(pi);
    return -1;
  }

  return 0;
}

void init_protocol_command_entry(struct sd_peer_info *pi,
    struct sd_protocol_command_info *pci, ...)
{
  va_list args;
  list_item *new_command_entry_item;
  struct sd_protocol_command_entry *pce;
  int i;
  void *nextarg;
  

  SAFE_CALLOC(pce, 1, sizeof(struct sd_protocol_command_entry));
  new_command_entry_item = linked_list_add(&pi->ctl_cmd_backlog, (void *)pce);

  /* set up command details */
  strncpy(pce->name, pci->name, sizeof(pce->name));
  /* set associated peer */
  pce->peer = pi;
  
  va_start(args, pci);
  for (i = 0; i < pci->nargs; i++)
  {
    nextarg = va_arg(args, void *);
    linked_list_add(&pce->args, nextarg);
  }
  va_end(args);
}

/* unpack recieved command */
int process_protocol_command(struct sd_peer_info *pi, const char *msg)
{
  int i, ncmds;
  char command[strlen(msg) + 1];
  char *tok;
  
  /* for debuging */
  char *saddr;
  saddr = get_sockaddr_storage_string(&pi->ctl_con.dst_sa);
  ui_notify_printf("Protocol <-- %s \"%s\"", saddr, msg);
  SAFE_FREE(saddr);

  memcpy(command, msg, sizeof(command));
  ncmds = get_control_command_qty();

  tok = strtok(command, SD_PROTOCOL_ARGUMENT_DELIM);
  if (tok == NULL) {
    ui_notify("Recieved an empty command.");
    return -1;
  }

  for (i = 0; i < ncmds; i++)
  {
    if (!strncasecmp(tok, control_command[i].name, SD_MAX_PROTOCOL_COMMAND_NAME_LEN))
      break;
  }

  if (i >= ncmds)
  {
    ui_notify("Recieved an unrecognised command.");
    return -1;
  }
  
  /* get all arguments */
  tok = strtok(NULL, "\0");

  if (tok == NULL) {
    if (control_command[i].nargs != 0)
      goto invalid_args;
  }

  /* make sure we have correct arguments */
  if ((*control_command[i].unpack)(pi, &control_command[i], tok) == -1)
  {
invalid_args:
    ui_sd_err("Recieved command with invalid arguments.");
    return -1;
  }

  return 0;
}

void ctl_con_close(struct sd_peer_info *pi)
{
  char *addrs;
  char msg[256];

  addrs = get_sockaddr_storage_string(&pi->ctl_con.dst_sa);
	socket_close(&pi->ctl_con.sock_fd, SD_OPTION_ON, pi->ctl_con.enable_ssl, 
      pi->ctl_con.ssl);
  snprintf(msg, sizeof(msg), "Closed control connection with %s.", addrs);
  ui_notify(msg);
  SAFE_FREE(addrs);

  peer_set_closed(pi);
}

int handle_ctl_recv(struct sd_peer_info *pi, fd_set *m, int mfd, int len)
{
  int recvb, startn;

  /* select method not working well on windows */

#if 0
  fd_set read_fd_set;

  read_fd_set = *m;
  if (select(mfd + 1, &read_fd_set, NULL, NULL, &pi->ctl_con.read_timeout) == -1)
  {
    ui_sock_err("select");
    if (errno == EINTR) {
      perror("select()");
    }
    else {
      DEBUG_SYS_ERROR_EXIT(errno, "select");
    }
    return -1;
  }
  if (FD_ISSET(pi->ctl_con.sock_fd, &read_fd_set))
#endif



#ifdef WIN32
      // If iMode != 0, non-blocking mode is enabled.
  u_long iMode = 1;
  ioctlsocket(pi->ctl_con.sock_fd, FIONBIO, &iMode);
#else
  fcntl(pi->ctl_con.sock_fd, F_SETFL, O_NONBLOCK);
#endif

  if (pi->ctl_con.enable_ssl == SD_OPTION_ON)
  {
    recvb = SSL_read(pi->ctl_con.ssl,
          (pi->ctl_buffer + pi->ctl_buffer_offset),
          len - 1 - pi->ctl_buffer_offset);
  }
  else
  {
    recvb = recv(pi->ctl_con.sock_fd,
          (pi->ctl_buffer + pi->ctl_buffer_offset),
          len - 1 - pi->ctl_buffer_offset,
          0);
  }

#ifndef WIN32
  fcntl(pi->ctl_con.sock_fd, F_SETFL, 0);
#else
  int recv_ret;
  recv_ret = WSAGetLastError();
  // If iMode != 0, non-blocking mode is enabled.
  iMode = 0;
  ioctlsocket(pi->ctl_con.sock_fd, FIONBIO, &iMode);
#endif

  if (recvb <= 0)
  {
    /* peer closed connection */
    if (recvb == 0)
    {
      ui_notify("Peer closed the connection.");
      socket_close(&pi->ctl_con.sock_fd, SD_OPTION_ON, pi->ctl_con.enable_ssl,
          pi->ctl_con.ssl);
      peer_set_closed(pi);
      return 0;
    }
    else
    {
      /* call again with same values */
      if (pi->ctl_con.enable_ssl)
      {
        int ret;
        ret = SSL_get_error(pi->ctl_con.ssl, recvb);

        if (ret == SSL_ERROR_WANT_READ || ret == SSL_ERROR_WANT_WRITE)
        {
          /* retry */
          return 0;
        }
      }
      else
      {
#ifdef WIN32
        if (recv_ret == WSAEWOULDBLOCK)
#else
        if (errno == EAGAIN)
#endif
        /* resource unavaliable */
        {
          /* retry */
          return 0;
        }
      }

      ui_sock_err("recv");
      socket_close(&pi->ctl_con.sock_fd, SD_OPTION_ON, pi->ctl_con.enable_ssl,
          pi->ctl_con.ssl);
      peer_set_closed(pi);
      return -1;
    }
  }

  /* valid data ready for processing */
  else
  {
    /* prepare for next call and processing */
    pi->ctl_buffer_offset += recvb;
    pi->ctl_buffer[pi->ctl_buffer_offset] = '\0';

    /* extract all messages */
    for (;;)
    {
      /* is there a record */
      startn = get_end_of_message(pi->ctl_buffer);
      if (startn == -1)
      {
        /* no message */
        break;
      }
      else
      {
        process_protocol_command(pi, pi->ctl_buffer);
        //ui_notify(pi->ctl_buffer);

        /* move memory to front of buffer */
        pi->ctl_buffer_offset -= startn;
        memmove(pi->ctl_buffer, pi->ctl_buffer + startn,
           pi->ctl_buffer_offset + 1);
      }
    }

    /* check if there is room for more data */
    if (len - 1 <= pi->ctl_buffer_offset)
    {
      ui_sd_err("Control command character limit exceeded");
      pi->ctl_buffer_offset = 0;
      return -1;
    }
  }

  return 0; /* still not at end */
}

int get_end_of_message(char *b)
{
  int i;
  char *end;

  if ((end = strstr(b, "\r")) != NULL)
  {
    goto found_term;
  }
  if ((end = strstr(b, "\n")) != NULL)
  {
    goto found_term;
  }
  
  return -1;

found_term:
  /* remember \0 will be after last byte recv */
  for (i = end - b; b[i] == '\r' || b[i] == '\n'; i++)
  {
    b[i] = '\0';
  }

  return i;
} 

int ctl_recv_iter_cb(void *v, int index)
{
  if (((struct sd_peer_info *)v)->ctl_con.state == CON_STATE_ESTABLISHED)
    handle_ctl_recv((struct sd_peer_info *) v, &gbls->net->master_fd_set,
        gbls->net->highest_fd, sizeof(((struct sd_peer_info *)(v))->ctl_buffer));

  return 0;
}

int handle_ctl_send(struct sd_peer_info *pi, char *b, int len)
{
  int t, n, brem;

  brem = len;
  t = 0;

  while (t < len)
  {
    if (pi->ctl_con.enable_ssl)
    {
      n = SSL_write(pi->ctl_con.ssl, b + t, brem);
    }
    else
    {
      n = send(pi->ctl_con.sock_fd, b + t, brem, 0);
    }

    if (n == -1)
    {
      ui_sock_err("send");
      return -1;
    }
    t += n;
    brem -= n;
  }

  return 0;
}


/* -[ data connection specific ] -------------------------------------- */

void data_con_close(struct sd_data_transfer_info *dti)
{
  char *addrs;
  char msg[256];

  addrs = get_sockaddr_storage_string(&dti->data_con.dst_sa);
  snprintf(msg, sizeof(msg), "Data connection was closed with %s.", addrs);
  ui_notify(msg);
  SAFE_FREE(addrs);

	socket_close(&dti->data_con.sock_fd, SD_OPTION_ON, dti->data_con.enable_ssl, 
      dti->data_con.ssl);
  sd_set_state(&dti->data_con.state, CON_STATE_CLOSED);

  if (dti->file.state == FILE_STATE_OPENED)
    file_close(&dti->file);

  data_transfer_abort(dti);
}

void data_transfer_set_completed(struct sd_data_transfer_info *dti)
{
  char *addrs;
  char msg[256];

  addrs = get_sockaddr_storage_string(&dti->data_con.dst_sa);
  snprintf(msg, sizeof(msg), "Data transfer %"PRIu64" with %s successfully completed!",
      dti->id, addrs);
  ui_notify(msg);
  SAFE_FREE(addrs);

	socket_close(&dti->data_con.sock_fd, SD_OPTION_ON, dti->data_con.enable_ssl, 
      dti->data_con.ssl);
  sd_set_state(&dti->data_con.state, CON_STATE_CLOSED);
  
  if (dti->file.state == FILE_STATE_OPENED)
    file_close(&dti->file);

  data_transfer_set_state(dti, DATA_TRANSFER_STATE_COMPLETED);
  
  data_transfer_reset_io(dti);
}

int handle_data_recv(struct sd_data_transfer_info *dti, fd_set *m, int mfd, int len)
{
  int recvb;
  //fd_set read_fd_set;

  /* return if were paused */
  if (dti->transfer_state == DATA_TRANSFER_TRANSFER_STATE_PAUSED)
  {
    data_transfer_set_io(dti);
    return 0;
  }

  /* select multiplexing */

#if 0
  read_fd_set = *m;
  if (select(mfd + 1, &read_fd_set, NULL, NULL, &dti->data_con.read_timeout) == -1)
  {
    if (errno == EINTR) {
      perror("select()");
      return 0;
    }
    else {
      DEBUG_SYS_ERROR_EXIT(errno, "select");
    }
    return -1;
  }
#endif

  /* non-blocking */
#ifdef WIN32
      // If iMode != 0, non-blocking mode is enabled.
      u_long iMode = 1;
      ioctlsocket(dti->data_con.sock_fd, FIONBIO, &iMode);
#else
      fcntl(dti->data_con.sock_fd, F_SETFL, O_NONBLOCK);
#endif

#if 0 
  /* we have something */
  if (FD_ISSET(dti->data_con.sock_fd, &read_fd_set))
  {
#endif
    if (dti->data_con.enable_ssl == SD_OPTION_ON)
    {
      recvb = SSL_read(dti->data_con.ssl,
            dti->data_buffer,
            len);
    }
    else
    {
      recvb = recv(dti->data_con.sock_fd,
            dti->data_buffer,
            len,
            0);
    }

#ifndef WIN32
      fcntl(dti->data_con.sock_fd, F_SETFL, 0);
#else
      int recv_ret;
      recv_ret = WSAGetLastError();
      // If iMode != 0, non-blocking mode is enabled.
      iMode = 0;
      ioctlsocket(dti->data_con.sock_fd, FIONBIO, &iMode);
#endif

    if (recvb <= 0)
    {
      /* peer closed connection */
      if (recvb == 0)
      {
        data_con_close(dti);
        return 0;
      }
      else
      {
        /* call again with same values */
        if (dti->data_con.enable_ssl)
        {
          int ret;
          ret = SSL_get_error(dti->data_con.ssl, recvb);

          if (ret == SSL_ERROR_WANT_READ || ret == SSL_ERROR_WANT_WRITE)
          {
            /* we must retry with same send values */
            data_transfer_set_io(dti);
            return 0;
          }
        }
        else
        {
#ifdef WIN32
          if (recv_ret == WSAEWOULDBLOCK)
#else
          if (errno == EAGAIN)
#endif
          /* resource unavaliable */
          {
            /* we must retry with same send values */
            data_transfer_set_io(dti);
            return 0;
          }
        }

        ui_sock_err("recv");
        data_con_close(dti);
	      return -1;
      }
    }
    else
    {
      /* write to file */
      int bwrite, brem;

      brem = recvb;
      dti->data_buffer_lower_offset = 0;
  
      while (brem > 0)
      {
        /* write */
        bwrite = fwrite(dti->data_buffer + dti->data_buffer_lower_offset,
                      1,
                      brem,
                      dti->file.file);

        /* error occured, abort */
        if (ferror(dti->file.file)) {
          ui_sys_err(errno, "fwrite");
          data_con_close(dti);
          return -1;
        }

        /* update file position */
        file_tell(dti->file.file, &dti->file.position);

        dti->io_total_bytes_current +=bwrite;
        dti->data_buffer_lower_offset += bwrite;
        brem -= bwrite;
    
      }

      /* update progress */
      data_transfer_set_io(dti);


      /* finished writing recieved bytes */
      if (dti->io_total_bytes_current >= dti->file.size)
      {
        data_transfer_set_completed(dti);
      }

      /* should we close up if they have passed the limit? */

    }

  return 0;
}


int handle_data_send(struct sd_data_transfer_info *dti, char *b, int len)
{
  int bsent;

  if (dti->data_buffer_window_size)
  {
    if (dti->transfer_state == DATA_TRANSFER_TRANSFER_STATE_RESUMED)
    {

      /* non-blocking */

#ifdef WIN32
      // If iMode != 0, non-blocking mode is enabled.
      u_long iMode = 1;
      ioctlsocket(dti->data_con.sock_fd, FIONBIO, &iMode);
#else
      void *prev;
      prev = signal(SIGPIPE, SIG_IGN);

      fcntl(dti->data_con.sock_fd, F_SETFL, O_NONBLOCK);
#endif
      
      /* send the data */
      if (dti->data_con.enable_ssl)
      {
        bsent = SSL_write(dti->data_con.ssl,
                          b + dti->data_buffer_lower_offset,
                          dti->data_buffer_window_size);
      }
      else
      {
        bsent = send(dti->data_con.sock_fd,
                     b + dti->data_buffer_lower_offset,
                     dti->data_buffer_window_size,
                     0);
      }
#ifndef WIN32
      signal(SIGPIPE, prev);
      
      fcntl(dti->data_con.sock_fd, F_SETFL, 0);
#else
      int send_ret;

      send_ret = WSAGetLastError();
      // If iMode != 0, non-blocking mode is enabled.
      iMode = 0;
      ioctlsocket(dti->data_con.sock_fd, FIONBIO, &iMode);
#endif

      /* on error */
#if WIN32
      if (bsent <= 0)
#else
      if (bsent < (dti->data_con.enable_ssl ? 1 : 0))
#endif
      {
        if (dti->data_con.enable_ssl)
        {
          int ret;
          ret = SSL_get_error(dti->data_con.ssl, bsent);

          if (ret == SSL_ERROR_WANT_READ || ret == SSL_ERROR_WANT_WRITE)
          {
            /* we must retry with same send values */
            data_transfer_set_io(dti);
            return 0;
          }
        }
        else
        {
#ifdef WIN32
          if (send_ret == WSAEWOULDBLOCK)
#else
          if (errno == EAGAIN)
#endif
          /* resource unavaliable */
          {
            /* we must retry with same send values */
            data_transfer_set_io(dti);
            return 0;
          }
        }

        ui_sock_err("send");
        data_con_close(dti);
        return -1;
      }

      /* remove sent bytes */
      dti->data_buffer_window_size -= bsent;
      dti->data_buffer_lower_offset += bsent;
      dti->io_total_bytes_current += bsent;
      
    }
    else {
      /* paused */
    }
    data_transfer_set_io(dti);
  }
  else
  {
    /* reset lower offset */
    dti->data_buffer_lower_offset = 0;

    /* check if read all the bytes */
    if (dti->io_total_bytes_current >= dti->file.size) {
      data_transfer_set_completed(dti);
      return 0;
    }

    /* fill buffer as much as possible */
    int bsize, bread, brem;
    uint64_t fbrem;

    bsize = sizeof dti->data_buffer;
    brem = bsize;

    fbrem = dti->file.size - dti->file.position;
    

    /* check if there is enough space to fill full buffer */
    if (fbrem < (uint64_t) brem)
      brem = (int) fbrem;

    while (brem > 0)
    {
      /* read */
      bread = fread(dti->data_buffer + dti->data_buffer_window_size,
                    1,
                    brem,
                    dti->file.file);

      /* update file position */
      file_tell(dti->file.file, &dti->file.position);


      dti->data_buffer_window_size += bread;
      brem -= bread;


      /* error occured, abort */
      if (ferror(dti->file.file)) {
        ui_sys_err(errno, "fread");
        data_con_close(dti);
        return -1;
      }

      /* premature EOF, abort */
      if (feof(dti->file.file) && (brem > 0)) {
        ui_sys_err(errno, "fread");
        data_con_close(dti);
        return -1;
      }

    }

  }

  
  //ui_data_transfer_progress_change(dti);

  return 0;
}



// vim:ts=2:expandtab
