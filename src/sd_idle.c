/*
   Main loop
 
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

#include "sd_peers.h"
#include "sd_idle.h"
#include "sd_globals.h"
#include "sd_ui.h"
#include "sd_net.h"
#include "sd_protocol.h"
#include "sd_version.h"
#include "sd_thread.h"

void ui_idle(void)
{
  relieve_cpu();

  /* main loop */
  
  /* garbage collection */

  /* cleanup server accept addresses */
  linked_list_iterate(&gbls->net->con_servers, &server_accept_rem_all_state_delete_iter);
  /* remove servers required */
  server_rem_all_state_delete();
  /* remove peers required */
  peer_rem_all_state_delete();



  /* handle servers */
  linked_list_iterate(&gbls->net->con_servers, &sd_mutex_serv_idle);
  linked_list_iterate(&gbls->net->con_servers, &sd_mutex_serv_accept_idle);



  /* handle control connection states */
  linked_list_iterate(&gbls->net->peers, &sd_mutex_con_idle);
  
  /* handle transfer connection states */
  linked_list_iterate(&gbls->net->peers, &sd_mutex_data_con_idle_iter);

  /* handle transfer states */
  linked_list_iterate(&gbls->net->peers, &data_transfer_idle_iter);

  /* get control messages */
  linked_list_iterate(&gbls->net->peers, &ctl_recv_iter_cb);

  /* handle recieved commands */
  linked_list_iterate(&gbls->net->peers, &ctl_process_cmd_iter_cb);

  /* print and remove status */
  ui_process_status_backlog();
}

void relieve_cpu(void)
{
#ifndef WIN32
  /* save cpu */
  set_next_frame(&gbls->frame);
  if (gbls->frame.prev_d < FRAME_TIME_MIN_S)
  {
#ifdef WIN32
    Sleep(FRAME_SLEEP_MS);
#else
    usleep(FRAME_SLEEP_US);
#endif
  }
#endif
}


// vim:ts=2:expandtab
