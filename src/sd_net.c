/*
   TCP/IP handling
 
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

#include <stdio.h>
#include <openssl/ssl.h>
#include <inttypes.h>
#include <errno.h>

#ifdef WIN32

#include <winsock2.h>
#include <ws2tcpip.h>

#else

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netdb.h>

#endif

#include "sd_ui.h"
#include "sd.h"
#include "sd_net.h"
#include "sd_globals.h"
#include "sd_error.h"
#include "sd_dynamic_memory.h"
#include "sd_ssl.h"
#include "sd_protocol.h"
#include "sd_protocol_commands.h"
#include "sd_version.h"
#include "sd_thread.h"
#include "sd_peers.h"
#include "sd_error.h"

static struct sockaddr_storage current_peer_to_validate;

void net_init()
{
#ifdef WIN32
  WSADATA wsaData;

  if (WSAStartup(MAKEWORD(1, 1), &wsaData) != 0)
  {
    ON_ERROR_EXIT("WSAStartup failed.\n");
  }
#endif

  FD_ZERO(&gbls->net->master_fd_set);
  gbls->net->highest_fd = 0;
  
  linked_list_init(&gbls->net->peers);
  linked_list_init(&gbls->net->con_servers);
}

void net_deinit()
{
#ifdef WIN32
  WSACleanup();
#endif

  /* cleanup servers */
  linked_list_deinit_rem_all_entries(
      &gbls->net->con_servers,
      SD_OPTION_ON,
      (void (*)(void *)) &server_deinit);

  linked_list_deinit_rem_all_entries(
      &gbls->net->peers,
      SD_OPTION_ON,
      (void (*)(void *)) &peer_deinit);
  
}


/* -[ servers ]-------------------------------------------------------- */


struct sd_serv_info *server_init(const char *a, const char *s, char essl,
    struct sd_ssl_verify_info *vi, char t)
{
  struct sd_serv_info *si;

  si = (struct sd_serv_info *) server_add()->value;

  sd_set_state(&si->state, SERVER_STATE_CLOSED);
  sd_thread_init(&si->mutex_state.cs_mutex);

  resolve_addr_set_info(&si->resolve_addr, a, s, SD_OPTION_OFF);

  if (essl) si->enable_ssl = SD_OPTION_ON;
  else si->enable_ssl = SD_OPTION_OFF;
  memcpy(&si->ssl_verify, vi, sizeof si->ssl_verify);
  
  linked_list_init(&si->accept_addresses);
  
  si->read_timeout.tv_sec = 0;
  si->read_timeout.tv_usec = 0;

  si->type = t;

  return si;
}

struct list_item *server_add()
{
  list_item *new_serv_item;
  struct sd_serv_info *new_serv;

  /* allocate memory for new peer */
  SAFE_CALLOC(new_serv, 1, sizeof(struct sd_serv_info));
  /* add serv to linked list */
  new_serv_item = linked_list_add(&gbls->net->con_servers,
      (void *) new_serv);
  return new_serv_item;
}

void server_deinit(struct sd_serv_info *si)
{
  linked_list_deinit_rem_all_entries(&si->accept_addresses,
      SD_OPTION_ON, (void (*)(void *)) &server_accept_deinit);
}

int server_get_state_delete_iterate(void *value, int index) {
  if (((struct sd_serv_info *)value)->state == SERVER_STATE_DELETE) {
    server_deinit((struct sd_serv_info *)value);
    return 1; /* stop iterating */
  }
  return 0;
}

int server_rem_all_state_delete()
{
  list_item *li;
  int nrem;

  nrem = 0;

  while ((li = linked_list_iterate(&gbls->net->con_servers,
          &server_get_state_delete_iterate)) != NULL) {
    linked_list_rem(&gbls->net->con_servers, li, SD_OPTION_ON);
    nrem++;
  }

  return nrem;
}

void server_start_state_machine(struct sd_serv_info *si, char istate, char t)
{
  if (si->state != SERVER_STATE_CLOSED)
    ON_ERROR_EXIT("Server state machine error");

  sd_set_state(&si->state, istate);
  /* run thread */
  if (t == SD_OPTION_ON)
    sd_create_thread((thread_pos_cb)(&sd_mutex_server_func), (void *)si);
}

void handle_server_state(struct sd_serv_info *si)
{
  switch (si->state)
  {
    case SERVER_STATE_CLOSED:
      break;
    case SERVER_STATE_RESOLVE_IP:
      if (si->mutex_state.proc_state != PROC_STATE_INCOMPLETE)
      {
        if (si->mutex_state.proc_state == PROC_STATE_COMPLETE)
        {
          /* bind and listen */
          if (server_bind(si) == -1) {
            ui_sd_err("Unable to start server.");
            /* clean up */
            switch (si->type)
            {
              case SERVER_TYPE_CONTROL:
                sd_set_state(&si->state, SERVER_STATE_DELETE);
                break;
              case SERVER_TYPE_DATA:
                sd_set_state(&si->state, SERVER_STATE_DELETE);
                break;
            }
          }
          else {
            char *saddr;
            saddr = get_sockaddr_storage_string(
               (struct sockaddr_storage *)si->servinfo.ai_addr
               );
            ui_notify_printf("Listening socket successfully bound to %s.", saddr);
            SAFE_FREE(saddr);

            sd_set_state(&si->state, SERVER_STATE_LISTENING);
          }
        }
        else {
          /* clean up */
          switch (si->type)
          {
            case SERVER_TYPE_CONTROL:
              sd_set_state(&si->state, SERVER_STATE_DELETE);
              break;
            case SERVER_TYPE_DATA:
              sd_set_state(&si->state, SERVER_STATE_DELETE);
              break;
          }
        }

      }
      break;
    case SERVER_STATE_LISTENING:
      server_handle_con(si);
      break;
  }
  
}

int handle_server_thread(struct sd_serv_info *si)
{
  switch (si->state)
  {
    case SERVER_STATE_CLOSED:
      break;
    case SERVER_STATE_RESOLVE_IP:
      ui_notify("Attempting to resolve local addresses...");
      if (address_lookup(&si->resolve_addr) == -1)
        return PROC_STATE_COMPLETE_WITH_ERROR;
      break;
    case SERVER_STATE_LISTENING:
      break;
  }

  /* success */
  return PROC_STATE_COMPLETE;
}

int server_bind(struct sd_serv_info *serv)
{
  struct addrinfo *res_p;
  int y = 1;

  /* loop through all sockets and bind to first sucessfull */
  for (res_p = serv->resolve_addr.res_ai; res_p != NULL; res_p = res_p->ai_next)
  {
    if ((serv->list_sock_fd = socket(res_p->ai_family, res_p->ai_socktype,
            res_p->ai_protocol)) == -1)
    {
      ui_sock_err("socket");
      continue;
    }

    /* to avoid "address already in use" */
    if (setsockopt(serv->list_sock_fd, SOL_SOCKET, SO_REUSEADDR,
            (const char *) &y, sizeof(int)) == -1)
    {
      ON_SYS_ERROR_EXIT(errno, "setsockopt");
    }

    if (bind(serv->list_sock_fd, res_p->ai_addr, res_p->ai_addrlen) == -1)
    {
      ui_sock_err("bind");
      /* close & no need to shutodwn ssl */
      socket_close(&serv->list_sock_fd, SD_OPTION_OFF, SD_OPTION_OFF, NULL);
      continue;
    }

    break;
  }

  if (res_p == NULL)
  {
    ui_sd_err("Failed to bind addresses.");
    return -1;
  }

  /* copy to server struct */
  memcpy(&serv->servinfo, res_p, sizeof(struct addrinfo));
  SAFE_CALLOC(serv->servinfo.ai_addr, 1, serv->servinfo.ai_addrlen);
  memcpy(serv->servinfo.ai_addr, res_p->ai_addr, serv->servinfo.ai_addrlen);

  freeaddrinfo(serv->resolve_addr.res_ai); /* finished with list */

  if (listen(serv->list_sock_fd, BACKLOG) == -1)
  {
    ui_sock_err("listen");
    return -1;
  }

  /* set up for reading */
  FD_SET(serv->list_sock_fd, &gbls->net->master_fd_set);
  if (serv->list_sock_fd > gbls->net->highest_fd)
    gbls->net->highest_fd = serv->list_sock_fd;
  serv->read_timeout.tv_sec = 0;
  serv->read_timeout.tv_usec = 0;

  /* ready to accept() */
  return 0;
}

int server_close(struct sd_serv_info *serv)
{
  sd_set_state(&serv->state, SERVER_STATE_CLOSED);
  return socket_close(&serv->list_sock_fd, SD_OPTION_ON,
      SD_OPTION_OFF, NULL);
}

int server_handle_con(struct sd_serv_info *serv)
{
  struct sd_con_info new_con; /* don't use up resources yet */
#ifdef WIN32
  int addrlen; /* for accept */
#else
  socklen_t addrlen;
#endif
  fd_set read_fd_set;
  
  /* setup fd set */
  read_fd_set = gbls->net->master_fd_set;

  if (select(gbls->net->highest_fd + 1, &read_fd_set, NULL,
          NULL, &serv->read_timeout) == -1)
  {
    if (errno == EINTR) {
      perror("select()");
    }
    else {
      ON_SYS_ERROR_EXIT(errno, "select");
    }
  }

  if (FD_ISSET(serv->list_sock_fd, &read_fd_set))
  {
    addrlen = sizeof(new_con.dst_sa);

    if ((new_con.sock_fd = accept(serv->list_sock_fd,
            (struct sockaddr *) &new_con.dst_sa, &addrlen)) == -1)
    {
      ui_sock_err("accept");
      return -1;
    }
    else
    {
      /* print notification */
      char *peeraddr, *servaddr;
      peeraddr = get_sockaddr_storage_string(&new_con.dst_sa);
      servaddr =
        get_sockaddr_storage_string((struct sockaddr_storage *)serv->servinfo.ai_addr);
      ui_notify_printf("New connection: %s <-- %s.", servaddr, peeraddr);

      /* check if peer is allowed to use this connection */
      list_item *li;

      switch (serv->type)
      {
        case SERVER_TYPE_DATA:
          memcpy(&current_peer_to_validate, &new_con.dst_sa,
              sizeof current_peer_to_validate);
          li = linked_list_iterate(&serv->accept_addresses, &validate_accept_peers);

          /* we cannot accept this peer */
          if (li == NULL)
          {
            ui_notify_printf("%s does not have permission to use this server, "
                "killing...", peeraddr);
            socket_close(&new_con.sock_fd, SD_OPTION_OFF, SD_OPTION_OFF, NULL);
            return 0;
          }
          ui_notify_printf("%s was successfully validated.", peeraddr);
          sd_set_state(&((struct sd_serv_accept_info *)li->value)->state,
              SERVER_ACCEPT_STATE_DELETE);

          break;
        case SERVER_TYPE_CONTROL:
          break;
      }

      SAFE_FREE(servaddr);
      SAFE_FREE(peeraddr);

      struct sd_con_info *ci;

      switch (serv->type)
      {
        case SERVER_TYPE_CONTROL:
          {
          /* add pear to list */
          struct sd_peer_info *npi;

          npi = peer_init(NULL, NULL, serv->enable_ssl, &serv->ssl_verify);

          ci = &npi->ctl_con;
          }
          break;
        case SERVER_TYPE_DATA:
          ci = ((struct sd_serv_accept_info *)li->value)->con;

          /* ssl */
          if (serv->enable_ssl) {
            ci->enable_ssl = SD_OPTION_ON;
            memcpy(&ci->ssl_verify, &serv->ssl_verify, sizeof ci->ssl_verify);
          }
          else {
            ci->enable_ssl = SD_OPTION_OFF;
          }
          break;
      }

      /* copy some info */
      memcpy(&ci->sock_fd, &new_con.sock_fd, sizeof(new_con.sock_fd));
      memcpy(&ci->dst_sa, &new_con.dst_sa, addrlen);

      /* set up for reading */
      FD_SET(ci->sock_fd, &gbls->net->master_fd_set);
      if (ci->sock_fd > gbls->net->highest_fd)
        gbls->net->highest_fd = ci->sock_fd;


      if (ci->enable_ssl) {
        sd_set_state(&ci->state, CON_STATE_SSL_VERIFY);
        sd_create_thread((thread_pos_cb)(&sd_mutex_con_func), (void *)ci);
      }
      else {
        if (serv->type == SERVER_TYPE_CONTROL)
          con_send_protocol_version(ci);

        sd_set_state(&ci->state, CON_STATE_ESTABLISHED);
      }

      return 1; /* we have connection */
    }
  }

  /* no connection */
  return 0;
}



/* -[ server accepts ]------------------------------------------------- */

struct sd_serv_accept_info *server_accept_init(
    struct sd_serv_info *si, const char *a, const char *s)
{
  struct sd_serv_accept_info *sai;

  sai = (struct sd_serv_accept_info *) server_accept_add(si)->value;

  sd_thread_init(&sai->mutex_state.cs_mutex);

  resolve_addr_set_info(&sai->resolve_addr, a, s, SD_OPTION_OFF);

  server_accept_start_state_machine(sai, SERVER_ACCEPT_STATE_RESOLVE_IP,
      SD_OPTION_ON);

  return sai;
}

struct list_item *server_accept_add(struct sd_serv_info *si)
{
  list_item *new_serv_accept_item;
  struct sd_serv_accept_info *new_serv_accept;

  /* allocate memory for new peer */
  SAFE_CALLOC(new_serv_accept, 1, sizeof(struct sd_serv_accept_info));
  /* add serv to linked list */
  new_serv_accept_item = linked_list_add(&si->accept_addresses,
      (void *) new_serv_accept);
  return new_serv_accept_item;
}

void server_accept_start_state_machine(struct sd_serv_accept_info *sai,
    char istate, char t)
{
  sd_set_state(&sai->state, istate);
  /* run thread */
  if (t == SD_OPTION_ON)
    sd_create_thread((thread_pos_cb)(&sd_mutex_server_accept_func), (void *)sai);
}

void handle_server_accept_state(struct sd_serv_accept_info *sai)
{
  switch (sai->state)
  {
    case SERVER_ACCEPT_STATE_RESOLVE_IP:
      if (sai->mutex_state.proc_state != PROC_STATE_INCOMPLETE)
      {
        if (sai->mutex_state.proc_state == PROC_STATE_COMPLETE)
        {
          ui_notify("Successfully resolved remote accept address.");
          /* let data transfer know where done */
          sd_set_state(&sai->state, SERVER_ACCEPT_STATE_RESOLVED);
        }
        else {
          ui_notify("Could not resolve remote accept address.");
          sd_set_state(&sai->state, SERVER_ACCEPT_STATE_ERROR);
        }
      }
      break;
    case SERVER_ACCEPT_STATE_RESOLVED:
      break;
  }
  
}

int handle_server_accept_thread(struct sd_serv_accept_info *sai)
{
  switch (sai->state)
  {
    case SERVER_ACCEPT_STATE_RESOLVE_IP:
      ui_notify("Attempting to resolve remote accept address...");
      if (address_lookup(&sai->resolve_addr) == -1)
      {
        return PROC_STATE_COMPLETE_WITH_ERROR;
      }
      break;
    case SERVER_ACCEPT_STATE_RESOLVED:
      break;
  }

  /* success */
  return PROC_STATE_COMPLETE;
}

int validate_accept_peers(void *value, int index)
{
  if (((struct sd_serv_accept_info *)value)->state !=
      SERVER_ACCEPT_STATE_RESOLVED)
    return 0;

  struct addrinfo *res_p;
  struct addrinfo *res_ai = 
    ((struct sd_serv_accept_info *)value)->resolve_addr.res_ai;

  for (res_p = res_ai; res_p != NULL; res_p = res_p->ai_next)
  {
    char *rp = get_sockaddr_storage_string((struct sockaddr_storage *)res_p->ai_addr);
    char *vp = get_sockaddr_storage_string(&current_peer_to_validate);
    ui_notify_printf("Checking if %s is allowed to use %s.", vp, rp);
    SAFE_FREE(rp);
    SAFE_FREE(vp);

    /* different protocols */
    if (current_peer_to_validate.ss_family != res_p->ai_family) {
      continue;
    }

    /* allowing any port */
    if (((struct sd_serv_accept_info *)value)->allow_any_port) {

      if (current_peer_to_validate.ss_family == AF_INET) {

        if( !memcmp(&((struct sockaddr_in *)&current_peer_to_validate)->sin_addr,
               &((struct sockaddr_in *)res_p->ai_addr)->sin_addr,
               sizeof (struct in_addr)))
          break;

      }
      else if (current_peer_to_validate.ss_family == AF_INET6)
      {
        if( !memcmp(&((struct sockaddr_in6 *)&current_peer_to_validate)->sin6_addr,
               &((struct sockaddr_in6 *)res_p->ai_addr)->sin6_addr,
               sizeof (struct in6_addr)))
          break;
      }
    }
    else
    {
      if( !memcmp(&current_peer_to_validate,
             res_p->ai_addr,
             res_p->ai_addrlen))
        break;
    }
  }

  if (res_p == NULL)
    return 0;

  return 1;
}

void server_accept_deinit(struct sd_serv_accept_info *sai)
{
  /* used for the address comparisons */
  SAFE_FREE(sai->resolve_addr.res_ai);
  sd_thread_deinit(&sai->mutex_state.cs_mutex);
}

int server_accept_get_state_delete_iterate(void *value, int index) {
  if (((struct sd_serv_accept_info *)value)->state ==
      SERVER_ACCEPT_STATE_DELETE)
  {
    server_accept_deinit((struct sd_serv_accept_info *)value);
    return 1; /* stop iterating */
  }
  return 0;
}
int server_accept_rem_all_state_delete_iter(void *v, int index)
{
  list_item *li;
  int nrem;

  nrem = 0;

  while ((li = linked_list_iterate(&((struct sd_serv_info *)v)->accept_addresses,
          &server_accept_get_state_delete_iterate)) != NULL) {
    linked_list_rem(&((struct sd_serv_info *)v)->accept_addresses, li, SD_OPTION_ON);
    nrem++;
  }

  return 0;
}



/* -[ connections ]---------------------------------------------------- */

void con_init(struct sd_con_info *co, const char *a, const char *p, char enable_ssl,
    struct sd_ssl_verify_info *vi, char t)
{
  resolve_addr_set_info(&co->resolve_dst_addr, a, p, SD_OPTION_OFF);

  /* ssl */
  if (enable_ssl) {
    co->enable_ssl = SD_OPTION_ON;
    if (vi)
      memcpy(&co->ssl_verify, vi, sizeof co->ssl_verify);
  }
  else {
    co->enable_ssl = SD_OPTION_OFF;
  }

  
  /* get rid of this */
  co->read_timeout.tv_sec = 0;
  co->read_timeout.tv_usec = 0;

  co->type = t;
  
  sd_thread_init(&co->mutex_state.cs_mutex);
}

void con_start_state_machine(struct sd_con_info *ci, char istate, char t)
{
  sd_set_state(&ci->state, istate);
  /* run thread */
  if (t == SD_OPTION_ON)
    sd_create_thread((thread_pos_cb)(&sd_mutex_con_func), (void *)ci);
}

void handle_con_state(struct sd_con_info *ci)
{
  switch (ci->state)
  {
    case CON_STATE_CLOSED:
      break;
    case CON_STATE_RESOLVE_SRC_IP:
      if (ci->mutex_state.proc_state != PROC_STATE_INCOMPLETE)
      {
        if (ci->mutex_state.proc_state == PROC_STATE_COMPLETE) {
          /* an error */
          if (con_bind_src_address(ci) == -1) {
            goto resolve_src_ip_err;
          }
          /* successfull */
          else {
            /* print notification */
            char *saddr;
            saddr = get_sockaddr_storage_string(&ci->src_sa);
            ui_notify_printf("Successfully bound to %s.", saddr);
            SAFE_FREE(saddr);

            switch (ci->type)
            {
              case CON_TYPE_CONTROL:
                sd_set_state(&ci->state, CON_STATE_RESOLVED_SRC_IP);
                break;
              case CON_TYPE_DATA:
                sd_set_state(&ci->state, CON_STATE_RESOLVED_SRC_IP);
                break;
            }
          }
        }
        else {
resolve_src_ip_err:
          /* clean up */
          switch (ci->type)
          {
            case CON_TYPE_CONTROL:
              break;
            case CON_TYPE_DATA:
              sd_set_state(&ci->state, CON_STATE_CLOSED);
              break;
          }
        }

      }
      break;
    case CON_STATE_RESOLVED_SRC_IP:
      break;
    case CON_STATE_RESOLVE_DST_IP:
      if (ci->mutex_state.proc_state != PROC_STATE_INCOMPLETE)
      {
        if (ci->mutex_state.proc_state == PROC_STATE_COMPLETE) {
          /* print notification */
          ui_notify_printf("Successfully resolved remote addresses.");

          sd_set_state(&ci->state, CON_STATE_CONNECTING);
          sd_create_thread((thread_pos_cb)(&sd_mutex_con_func), (void *)ci);
        }
        else {
          /* clean up */
          switch (ci->type)
          {
            case CON_TYPE_CONTROL:
              sd_set_state(&ci->state, CON_STATE_DELETE);
              break;
            case CON_TYPE_DATA:
              break;
          }
        }

      }
      break;
    case CON_STATE_CONNECTING:
      if (ci->mutex_state.proc_state != PROC_STATE_INCOMPLETE)
      {
        if (ci->mutex_state.proc_state == PROC_STATE_COMPLETE) {
          /* print notification */
          char *saddr;
          saddr = get_sockaddr_storage_string(&ci->dst_sa);
          ui_notify_printf("Connected to: %s.", saddr);
          SAFE_FREE(saddr);

          if (ci->enable_ssl) {
            sd_set_state(&ci->state, CON_STATE_SSL_VERIFY);
            sd_create_thread((thread_pos_cb)(&sd_mutex_con_func), (void *)ci);
          }
          else {
            switch (ci->type)
            {
              case CON_TYPE_CONTROL:
                con_send_protocol_version(ci);
                break;
            }

            sd_set_state(&ci->state, CON_STATE_ESTABLISHED);
          }
        }
        else {
          /* clean up */
          switch (ci->type)
          {
            case CON_TYPE_CONTROL:
              sd_set_state(&ci->state, CON_STATE_DELETE);
              break;
            case CON_TYPE_DATA:
              sd_set_state(&ci->state, CON_STATE_CLOSED);
              break;
          }
        }

      }
      break;
    case CON_STATE_SSL_VERIFY:
      if (ci->mutex_state.proc_state != PROC_STATE_INCOMPLETE)
      {
        if (ci->mutex_state.proc_state == PROC_STATE_COMPLETE) {
          /* print notification */
          char *saddr;
          saddr = get_sockaddr_storage_string(&ci->dst_sa);
          ui_notify_printf("SSL handshake successfull with %s.", saddr);
          SAFE_FREE(saddr);

          switch (ci->type)
          {
            case CON_TYPE_CONTROL:
              con_send_protocol_version(ci);
              break;
          }

          sd_set_state(&ci->state, CON_STATE_ESTABLISHED);
        }
        else {
          /* clean up */
          switch (ci->type)
          {
            case CON_TYPE_CONTROL:
              sd_set_state(&ci->state, CON_STATE_CLOSED);
              break;
            case CON_TYPE_DATA:
              break;
          }
        }

      }
      break;
    case CON_STATE_ESTABLISHED:
      switch (ci->type)
      {
        case CON_TYPE_CONTROL:

          break;
        case CON_TYPE_DATA:
          break;
      }
      break;
  }
}

int handle_con_thread(struct sd_con_info *ci)
{
  switch (ci->state)
  {
    case CON_STATE_CLOSED:
      break;
    case CON_STATE_RESOLVE_SRC_IP:
      ui_notify("Attempting to resolve and bind local address...");
      if (address_lookup(&ci->resolve_src_addr) == -1)
        return PROC_STATE_COMPLETE_WITH_ERROR;
      break;
    case CON_STATE_RESOLVE_DST_IP:
      ui_notify("Attempting to resolve remote addresses...");
      if (address_lookup(&ci->resolve_dst_addr) == -1)
        return PROC_STATE_COMPLETE_WITH_ERROR;
      break;
    case CON_STATE_CONNECTING:
      if (con_connect(ci) == -1)
        return PROC_STATE_COMPLETE_WITH_ERROR;
      break;
    case CON_STATE_SSL_VERIFY:
      ui_notify("Performing SSL handshake...");
      if (con_ssl_init(ci) == -1)
        return PROC_STATE_COMPLETE_WITH_ERROR;
      break;
    case CON_STATE_ESTABLISHED:
      break;
  }

  /* success */
  return PROC_STATE_COMPLETE;
}


int con_bind_src_address(struct sd_con_info *ci)
{
  struct addrinfo *res_p;
  uint32_t i;

  /* loop through all sockets and bind to first sucessfull */
  for (res_p = ci->resolve_src_addr.res_ai; res_p != NULL; res_p = res_p->ai_next)
  {
    if ((ci->sock_fd = socket(res_p->ai_family, res_p->ai_socktype,
            res_p->ai_protocol)) == -1)
    {
      ui_sock_err("socket");
      continue;
    }
#ifdef WIN32
    int y = -1;
    if (setsockopt(ci->sock_fd, SOL_SOCKET, SO_EXCLUSIVEADDRUSE,
        (const char *)&y, sizeof y) == -1)
      ON_SYS_ERROR_EXIT(errno, "setsockopt");
#endif

    for (i = 1500; i < 65535; i++)
    {
      if (res_p->ai_addr->sa_family == AF_INET)
        ((struct sockaddr_in *)res_p->ai_addr)->sin_port = htons(i);
      else
        ((struct sockaddr_in6 *)res_p->ai_addr)->sin6_port = htons(i);


      if (bind(ci->sock_fd, res_p->ai_addr, res_p->ai_addrlen) == -1)
      {
#ifdef WIN32
        if (WSAGetLastError() == WSAEADDRINUSE ||
            WSAGetLastError() == WSAEACCES)
#else
        if (errno == EADDRINUSE)
#endif
        {
          continue;
        }

        ui_sock_err("bind");
        
        socket_close(&ci->sock_fd, SD_OPTION_OFF, SD_OPTION_OFF, NULL);
        res_p = NULL;
        break;
      }
      /* success */
      goto bind_done;
    }

  }
bind_done:

  if (res_p == NULL)
  {
    ui_sd_err("Failed to bind address.");
    return -1;
  }

  /* copy to address struct */
  socklen_t addrlen;
  if (res_p->ai_family == AF_INET)
    addrlen = sizeof(struct sockaddr_in);
  else if (res_p->ai_family == AF_INET6)
    addrlen = sizeof(struct sockaddr_in6);
  else
    ON_ERROR_EXIT("Unsupported ai_family.");

  memcpy(&ci->src_sa, res_p->ai_addr, addrlen);

  freeaddrinfo(ci->resolve_src_addr.res_ai); /* finished with list */
  
  /* we are bound */
  return 0;
}

/* this function blocks to run in thread */
int con_connect(struct sd_con_info *ci)
{
  struct addrinfo *res_p;

  /* loop through all sockets and bind to first sucessfull */
  for (res_p = ci->resolve_dst_addr.res_ai; res_p != NULL; res_p = res_p->ai_next)
  {
    switch (ci->type)
    {
      case CON_TYPE_CONTROL:
        if ((ci->sock_fd = socket(res_p->ai_family, res_p->ai_socktype,
              res_p->ai_protocol)) == -1)
        {
          ui_sock_err("socket");
          continue;
        }
        break;
      case CON_TYPE_DATA:
        break;
    }

    char *saddr;
    saddr = get_sockaddr_storage_string((struct sockaddr_storage *)res_p->ai_addr);
    ui_notify_printf("Trying to connect to %s...", saddr);
    SAFE_FREE(saddr);

    if (connect(ci->sock_fd, res_p->ai_addr, res_p->ai_addrlen) == -1)
    {
      ui_sock_err("connect");

      socket_close(&ci->sock_fd, SD_OPTION_OFF, SD_OPTION_OFF, NULL);
      continue;
    }

    break;
  }

  if (res_p == NULL)
  {
    ui_sd_err("Failed to connect to remote host");
    return -1;
  }

  /* copy to address struct */
  socklen_t addrlen;
  if (res_p->ai_family == AF_INET)
    addrlen = sizeof(struct sockaddr_in);
  else if (res_p->ai_family == AF_INET6)
    addrlen = sizeof(struct sockaddr_in6);
  else
    ON_ERROR_EXIT("Unsupported ai_family.");

  memcpy(&ci->dst_sa, res_p->ai_addr, addrlen);

  freeaddrinfo(ci->resolve_dst_addr.res_ai); /* finished with list */
  
  /* set up for reading */
  FD_SET(ci->sock_fd, &gbls->net->master_fd_set);
  if (ci->sock_fd > gbls->net->highest_fd)
    gbls->net->highest_fd = ci->sock_fd;

  /* we are connected */
  return 0;
}


char *get_con_state_string(struct sd_con_info *ci)
{
  char *state, *nstr;
  int len;

  switch (ci->state)
  {
    case CON_STATE_CLOSED: state = "CLOSED"; break;
    case CON_STATE_RESOLVE_SRC_IP: state = "RESOLVE SOURCE IP"; break;
    case CON_STATE_RESOLVED_SRC_IP: state = "RESOLVED SOURCE IP"; break;
    case CON_STATE_RESOLVE_DST_IP: state = "RESOLVE DESTINATION IP"; break;
    case CON_STATE_CONNECTING: state = "CONNECTING"; break;
    case CON_STATE_SSL_VERIFY: state = "SSL VERIFY"; break;
    case CON_STATE_ESTABLISHED: state = "ESTABLISHED"; break;
    case CON_STATE_DELETE: state = "DELETE"; break;
    default: state = "ERROR";
  }
  
  len = strlen(state) + 1;
  SAFE_CALLOC(nstr, 1, len);
  memcpy(nstr, state, len);

  return nstr;
}

int socket_close(int *fd, char fd_clr, char enable_ssl, SSL *ssl)
{
  int ret;

  if (fd_clr)
    FD_CLR(*fd, &gbls->net->master_fd_set);

  if (enable_ssl)
  {
      ret = SSL_shutdown(ssl);
      if (ret == -1)
      {
        ui_ssl_err("SSL_shutdown");
        return -1;
      }
      if (ret == 0) /* 0 means shutdown not finished */
      {
        ret = SSL_shutdown(ssl);
        if (ret == -1)
        {
          ui_ssl_err("SSL_shutdown");
          return -1;
        }
      }
  }


#ifdef WIN32 
  if (closesocket(*fd) == SOCKET_ERROR)
  {
    
    return -1;
  }
#else
  if (close(*fd) == -1)
  {
    ui_sock_err("close");
    return -1;
  }
#endif

  /* success */
  return 0;
}


/* -[ common ]--------------------------------------------------------- */

char *get_sockaddr_storage_port_string(struct sockaddr_storage *sas)
{
  uint16_t port;
  void *saddr;
  char *pstr;

  saddr = get_in_addr((struct sockaddr *) sas);

  if (sas->ss_family == AF_INET) {
    port = ((struct sockaddr_in *) sas)->sin_port;
  }
  else if (sas->ss_family == AF_INET6) {
    port = ((struct sockaddr_in6 *) sas)->sin6_port;
  }
  else {
    port = 0;
  }

  port = (uint16_t) htons(port);

  SAFE_CALLOC(pstr, 1, 128);

  snprintf(pstr, 128, "%"PRIu16, port);

  return pstr;
}

void *get_in_addr(struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET)
  {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }

  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


/* this function needs to run in thread as it blocks */
int address_lookup(struct sd_resolve_addr_info *rai)
{
  int rt;
  struct addrinfo hints;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  if ((rt = getaddrinfo(rai->lookup_address,
          (rai->any_service ? NULL : rai->lookup_service),
          &hints, &rai->res_ai)) != 0)
  {
    ui_gai_err(rt, "getaddrinfo");
    return -1;
  }

  return 0;
}

void resolve_addr_set_info(struct sd_resolve_addr_info *ri, const char *a,
    const char *s, char as)
{
  if (a)
    snprintf(ri->lookup_address, sizeof ri->lookup_address, "%s", a);
  if (s)
    snprintf(ri->lookup_service, sizeof ri->lookup_service, "%s", s);
  ri->any_service = as;
}

char *get_sockaddr_storage_string(struct sockaddr_storage *sas)
{
  char addrs[128], known;
  char *msgs;
  
  void *saddr;
  char *ipver, *fmt;
  uint16_t port;
  socklen_t addrlen;

  known = 1;
  saddr = get_in_addr((struct sockaddr *) sas);

  if (sas->ss_family == AF_INET) {
    addrlen = sizeof(struct sockaddr_in);
    port = ((struct sockaddr_in *) sas)->sin_port;
    ipver = "IPv4";
    fmt = "%s:%i [%s]";
  }
  else if (sas->ss_family == AF_INET6) {
    addrlen = sizeof(struct sockaddr_in6);
    port = ((struct sockaddr_in6 *) sas)->sin6_port;
    ipver = "IPv6";
    fmt = "[%s]:%i [%s]";
  }
  else {
    known = 0;
  }

  SAFE_CALLOC(msgs, 1, 256);

  if (!known)
  {
    snprintf(msgs, 256, "%s", "<UNKNOWN>");
    return msgs;
  }

#ifdef WIN32
  DWORD addrslen;

  addrslen = sizeof addrs;
  if (WSAAddressToString((void *)sas, addrlen, NULL, addrs, &addrslen))
  {
    ui_wsa_err("WSAAddressToString");
    return NULL;
  }
  snprintf(msgs, 256, "%s [%s]", addrs, ipver);
#if 0
  char service[32];
  int rt;

  if ((rt = getnameinfo((void *)sas, addrlen, addrs, sizeof addrs,
      service, sizeof service, 0)))
  {
    ui_gai_err(rt, "getnameinfo");
    return NULL;
  }
#endif
#else
  /* another way, but linux only */
  inet_ntop(sas->ss_family, saddr, addrs, sizeof addrs);
  snprintf(msgs, 256, fmt, addrs, htons(port), ipver);

#endif
  return msgs;
}


// vim:ts=2:expandtab
