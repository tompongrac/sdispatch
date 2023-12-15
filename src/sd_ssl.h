#ifndef SD_SSL_H
#define SD_SSL_H

#include <openssl/bio.h>
#include <openssl/ssl.h>
#include "sd.h"

/*! \brief SSL verify information */
struct sd_ssl_verify_info
{
  char ssl_use_cert;
  char ssl_cert_path[SD_MAX_PATH_LEN]; /* digitially signed by ca */
  char ssl_pr_key_path[SD_MAX_PATH_LEN]; /* private-key */

  char ssl_verify_peer;
  char ssl_verify_mode;
  int ssl_verify_depth; /* max chains allowed */
  char ssl_ca_cert_dir_path[SD_MAX_PATH_LEN]; /* trust store */

  char ssl_hs_action;
#define SSL_HANDSHAKE_ACTION_ACCEPT     0
#define SSL_HANDSHAKE_ACTION_CONNECT    1
};

/*! \brief Initialise the SSL library */
extern void ssl_init();

/*! \brief Deinitialise the SSl library */
extern void ssl_deinit();

/*! \brief Initialise the SSL context (global settings) */
extern int ssl_ssl_ctx_init(SSL_CTX **ctx);

/*! \brief Deinitialise the SSL context */
extern void ssl_ssl_ctx_deinit(SSL_CTX *ctx);

/*! \brief Manually set the SSL verification settings */
extern void ssl_set_verify_info(struct sd_ssl_verify_info *vi, char ucert,
    char *cert, char *prkey, char vpeer, int vdepth, char *cacert, char hsa);

/*! \brief Initialise a SSL session */
extern int ssl_ssl_init(SSL_CTX *ctx, SSL **ssl, struct sd_ssl_verify_info *vi, int fd);

/*! \brief Deinitialise a SSL session */
extern void ssl_ssl_deinit(SSL *ssl);

/*! \brief Perform SSL connect */
extern int ssl_handshake_connect(SSL *ssl);

/*! \brief Perform SSL accept */
extern int ssl_handshake_accept(SSL *ssl);

struct sd_con_info;
/*! \brief Perform SSL routines for new connection */
extern int con_ssl_init(struct sd_con_info *ci);

#endif


// vim:ts=2:expandtab
