/*
   SSL support
 
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

#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "sd_ssl.h"
#include "sd_globals.h"
#include "sd_ui.h"
#include "sd_error.h"
#include "sd_net.h"

void ssl_init()
{
  SSL_library_init(); /* load encryption & hash algorithms for SSL */
  SSL_load_error_strings(); /* load error strings for reporting */
  
  if (ssl_ssl_ctx_init(&gbls->net->ssl_ctx) == -1)
  {
    ON_ERROR_EXIT("SSL context initialisation failed.");
  }
  
  ui_notify("SSL context successfully initialised.");
  
  /* path to look for trusted CA certs */
  if (!SSL_CTX_load_verify_locations(gbls->net->ssl_ctx, NULL,
        gbls->conf->ssl_verify.ssl_ca_cert_dir_path))
  {
    ui_ssl_err("SSL_load_verify_locations");
  }
}

void ssl_deinit()
{
  /* free ssl context */
  ssl_ssl_ctx_deinit(gbls->net->ssl_ctx);
}

int ssl_ssl_ctx_init(SSL_CTX **ctx)
{
  /* setup ssl for combined server & client */
  if ((*ctx = SSL_CTX_new(SSLv23_method())) == NULL)
  {
    ui_ssl_err("SSL_CTX_new");
    return -1;
  }

  return 0;
}

void ssl_ssl_ctx_deinit(SSL_CTX *ctx)
{
  SSL_CTX_free(ctx);
}

int ssl_ssl_init(SSL_CTX *ctx, SSL **ssl, struct sd_ssl_verify_info *vi, int fd)
{
  /* allocate new structure for connection */
  if ((*ssl = SSL_new(ctx)) == NULL)
  {
    ui_ssl_err("SSL_new");
    return -1;
  }

  /* set up keys */
  if (vi->ssl_use_cert == SD_OPTION_ON)
  {
    /* loads first certificate in file to ctx */
    if (SSL_use_certificate_file(*ssl, vi->ssl_cert_path, SSL_FILETYPE_PEM) <= 0)
    {
      ui_ssl_err("SSL_use_certificate_file");
      SSL_free(*ssl);
      return -1;
    }

    /* load first private key in file to ctx */
    if (SSL_use_PrivateKey_file(*ssl, vi->ssl_pr_key_path, SSL_FILETYPE_PEM) <= 0)
    {
      ui_ssl_err("SSL_use_PrivateKey_file");
      SSL_free(*ssl);
      return -1;
    }

    /* make sure pub and pri keys match */
    if (!SSL_check_private_key(*ssl))
    {
      ui_sd_err("Private key does not match certificate public key.\n");
      SSL_free(*ssl);
      return -1;
    }
  }

  /* ---------- verification ---------- */

  /* set peer cert verification mode */
  SSL_set_verify(*ssl, vi->ssl_verify_mode, NULL);

  if (vi->ssl_verify_peer == SD_OPTION_ON)
  {
    /* set depth of certificate chain verification */
    SSL_set_verify_depth(*ssl, vi->ssl_verify_depth);
  }


  /* connect SSL to fd */
  if (SSL_set_fd(*ssl, fd) == 0)
  {
    ui_ssl_err("SSL_set_fd");
    SSL_free(*ssl);
    return -1;
  }

  return 0;
}

int ssl_handshake_connect(SSL *ssl)
{
  /* preform the handshake as client */
  if (SSL_connect(ssl) <= 0)
  {
    ui_ssl_err("SSL_connect");
    SSL_free(ssl);
    return -1;
  }

  return 0;
}

int ssl_handshake_accept(SSL *ssl)
{
  /* preform the handshake as server */
  if (SSL_accept(ssl) <= 0)
  {
    ui_ssl_err("SSL_accept");
    SSL_free(ssl);
    return -1;
  }

  return 0;
}

void ssl_ssl_deinit(SSL *ssl)
{
  SSL_free(ssl);
}

void ssl_set_verify_info(struct sd_ssl_verify_info *vi, char ucert,
    char *cert, char *prkey, char vpeer, int vdepth, char *cacert, char hsa)
{
  if (ucert == SD_OPTION_ON)
  {
    vi->ssl_use_cert = SD_OPTION_ON;
    if (cert) {
      strncpy(vi->ssl_cert_path, cert, sizeof(vi->ssl_cert_path));
      vi->ssl_cert_path[sizeof(vi->ssl_cert_path) - 1] = '\0';
    }
    if (prkey) {
      strncpy(vi->ssl_pr_key_path, prkey, sizeof(vi->ssl_pr_key_path));
      vi->ssl_pr_key_path[sizeof(vi->ssl_pr_key_path) - 1] = '\0';
    }
  }
  else
  {
    vi->ssl_use_cert = SD_OPTION_OFF;
  }

  if (vpeer == SD_OPTION_ON)
  {
    vi->ssl_verify_peer = SD_OPTION_ON;
    //vi->ssl_verify_mode = SSL_VERIFY_PEER;
    vi->ssl_verify_mode = SSL_VERIFY_FAIL_IF_NO_PEER_CERT;
    vi->ssl_verify_depth = vdepth;
    if (cacert) {
      strncpy(vi->ssl_ca_cert_dir_path, cacert, sizeof(vi->ssl_ca_cert_dir_path));
      vi->ssl_ca_cert_dir_path[sizeof(vi->ssl_ca_cert_dir_path) - 1] = '\0';
    }
  }
  else
  {
    vi->ssl_verify_peer = SD_OPTION_OFF;
    vi->ssl_verify_mode = SSL_VERIFY_NONE;
  }
    
  vi->ssl_hs_action = hsa;
}

int con_ssl_init(struct sd_con_info *ci)
{
  int ret;

  if (ssl_ssl_init(gbls->net->ssl_ctx, &ci->ssl, &ci->ssl_verify,
        ci->sock_fd) == -1)
  {
    ui_sd_err("SSL initialisation failed.");
    /* close & no need shutdown ssl */
    socket_close(&ci->sock_fd, SD_OPTION_ON, SD_OPTION_OFF, NULL);
    return -1;
  }
  else
  {
    switch (ci->ssl_verify.ssl_hs_action)
    {
      case SSL_HANDSHAKE_ACTION_ACCEPT:
        ret = ssl_handshake_accept(ci->ssl);
        break;
      case SSL_HANDSHAKE_ACTION_CONNECT:
        ret = ssl_handshake_connect(ci->ssl);
        break;
    }

    if (ret == -1)
    {
      ui_sd_err("SSL handshake failed.");
      /* close & no need shutdown ssl */
      socket_close(&ci->sock_fd, SD_OPTION_ON, SD_OPTION_OFF, NULL);
      return -1;
    }
  }
  return 0;
}

void ssl_verify_set_defaults(struct sd_ssl_verify_info *vi)
{
  memcpy(vi, &gbls->conf->ssl_verify, sizeof (struct sd_ssl_verify_info));
}



// vim:ts=2:expandtab
