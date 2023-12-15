/*
   Safe thread handling
 
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
#include <process.h>
#else
#include <pthread.h>
#endif

#include <stdio.h>

#include "sd_net.h"
#include "sd_thread.h"
#include "sd_error.h"
#include "sd_peers.h"


void sd_thread_init(void *mutex)
{
#ifdef WIN32
  InitializeCriticalSection((CRITICAL_SECTION *)mutex);
#else
  pthread_mutex_init(mutex, NULL);
#endif
}

void sd_thread_deinit(void *mutex)
{
#ifdef WIN32
  DeleteCriticalSection((CRITICAL_SECTION *)mutex);
#else
  pthread_mutex_destroy(mutex);
#endif
}

void sd_cs_lock(
#ifdef WIN32
  CRITICAL_SECTION *cs
#else
  pthread_mutex_t *cs
#endif
)
{
#ifdef WIN32
  EnterCriticalSection(cs);
#else
  pthread_mutex_lock(cs);
#endif
}

void sd_cs_unlock(
#ifdef WIN32
  CRITICAL_SECTION *cs
#else
  pthread_mutex_t *cs
#endif
)
{
#ifdef WIN32
  LeaveCriticalSection(cs);
#else
  pthread_mutex_unlock(cs);
#endif
}

#if 1
void sd_set_mutex_state(volatile char *s, char ns)
{
  *s = ns;
}
#endif

int sd_create_thread(thread_pos_cb f, void *args)
{
#ifdef WIN32
  if (_beginthread((thread_win_cb)f, 0, args) == -1)
  {
    ON_ERROR_EXIT("_beginthread() failed.");
  }
#else
  pthread_t id;

  if (pthread_create(&id, NULL, f, args))
  {
    ON_ERROR_EXIT("pthread_create() failed.");
  }
#endif
  return 0;
}

void *sd_mutex_con_func(void *v)
{
  int ret;
  //sd_cs_lock(&((struct sd_con_info *)v)->mutex_state.cs_mutex);

  /* thread safe processing */
  sd_set_mutex_state(&((struct sd_con_info *)v)->mutex_state.proc_state,
      PROC_STATE_INCOMPLETE);
  ret = handle_con_thread((struct sd_con_info *)v);
  sd_set_mutex_state(&((struct sd_con_info *)v)->mutex_state.proc_state, ret);

  //sd_cs_unlock(&((struct sd_con_info *)v)->mutex_state.cs_mutex);

#ifdef WIN32
  _endthread();
#else
#endif
  return NULL;
}

int sd_mutex_con_idle(void *value, int index)
{
  handle_con_state((struct sd_con_info *)value);
  return 0;
}

int sd_mutex_data_con_idle(void *value, int index)
{
  handle_con_state(&((struct sd_data_transfer_info *)value)->data_con);
  return 0;
}
int sd_mutex_data_con_idle_iter(void *value, int index)
{
  linked_list_iterate(&((struct sd_peer_info *)value)->data_transfers,
      &sd_mutex_data_con_idle);
  return 0;
}



void *sd_mutex_server_func(void *v)
{
  int ret;
  //sd_cs_lock(&((struct sd_serv_info *)v)->mutex_state.cs_mutex);

  /* thread safe processing */

  sd_set_mutex_state(&((struct sd_serv_info *)v)->mutex_state.proc_state,
      PROC_STATE_INCOMPLETE);
  ret = handle_server_thread((struct sd_serv_info *)v);
  sd_set_mutex_state(&((struct sd_serv_info *)v)->mutex_state.proc_state, ret);


  //sd_cs_unlock(&((struct sd_serv_info *)v)->mutex_state.cs_mutex);

#ifdef WIN32
  _endthread();
#else
#endif
  return NULL;
}

int sd_mutex_serv_idle(void *value, int index)
{
  handle_server_state((struct sd_serv_info *)value);
  return 0;
}



void *sd_mutex_server_accept_func(void *v)
{
  int ret;
  //sd_cs_lock(&((struct sd_serv_info *)v)->mutex_state.cs_mutex);

  /* thread safe processing */

  sd_set_mutex_state(&((struct sd_serv_accept_info *)v)->mutex_state.proc_state,
      PROC_STATE_INCOMPLETE);
  ret = handle_server_accept_thread((struct sd_serv_accept_info *)v);
  sd_set_mutex_state(&((struct sd_serv_accept_info *)v)->mutex_state.proc_state, ret);


  //sd_cs_unlock(&((struct sd_serv_info *)v)->mutex_state.cs_mutex);

#ifdef WIN32
  _endthread();
#else
#endif
  return NULL;
}

int sd_mutex_serv_accept_idle_iter(void *value, int index)
{
  handle_server_accept_state((struct sd_serv_accept_info *)value);
  return 0;
}

int sd_mutex_serv_accept_idle(void *value, int index)
{
  linked_list_iterate(&((struct sd_serv_info *)value)->accept_addresses,
      &sd_mutex_serv_accept_idle_iter);
  return 0;
}


// vim:ts=2:expandtab
