#ifndef SD_NET_H
#define SD_NET_H

#ifdef WIN32

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h> /* addrinfo */

#else

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>
#include <netdb.h>


#endif


#include "sd_ssl.h"
#include "sd_thread.h"
#include "sd_linked_list.h"

#define BACKLOG  10


/*! \brief Information for resolving network addresses */
struct sd_resolve_addr_info
{
  /* address lookup info */
#define LOOKUP_ADDRESS_LEN                   512
  char lookup_address[LOOKUP_ADDRESS_LEN];
#define LOOKUP_SERVICE_LEN                    64
  char lookup_service[LOOKUP_SERVICE_LEN];

  char any_service;
  
  struct addrinfo *res_ai; /* for lookup */
};

/*! \brief Holds information about the connection */
struct sd_con_info
{
  char type;
#define CON_TYPE_CONTROL            0
#define CON_TYPE_DATA               1

  /* Mutex */
  struct sd_mutex_state_info mutex_state;

  /* for getaddrinfo */
  struct sd_resolve_addr_info resolve_dst_addr;
  struct sd_resolve_addr_info resolve_src_addr;
  

  /* SSL */

  char enable_ssl; /* use ssl */
  SSL *ssl; /* session info inherit from ctx */
  struct sd_ssl_verify_info ssl_verify; /* verify info */


  /* TCP/IP (IPv4 & IPv6 support) */

  struct sockaddr_storage dst_sa; /* address from accept */
  struct sockaddr_storage src_sa; /* address from bind */

  int sock_fd; /* connection socket */
  fd_set read_fd_set;
  struct timeval read_timeout;

  char state;
#define CON_STATE_CLOSED          0
#define CON_STATE_RESOLVE_SRC_IP  1 /* reserve port */
#define CON_STATE_RESOLVED_SRC_IP 2 /* reserve port */
#define CON_STATE_RESOLVE_DST_IP  3 /* remote getaddrinfo */
#define CON_STATE_CONNECTING      4 /* connect */
#define CON_STATE_SSL_VERIFY      5 /* ssl handshake */
#define CON_STATE_ESTABLISHED     6 /* connected */
#define CON_STATE_DELETE          7 /* needs to be deleted */

};

/*! \brief Information about which hosts the server should allow */
struct sd_serv_accept_info
{
  /* for thread */
  struct sd_mutex_state_info mutex_state;
  struct sd_con_info *con;
  char allow_any_port;

  char state;
#define SERVER_ACCEPT_STATE_RESOLVE_IP   0
#define SERVER_ACCEPT_STATE_RESOLVED     1
#define SERVER_ACCEPT_STATE_ERROR        2
#define SERVER_ACCEPT_STATE_DELETE       3

  struct sd_resolve_addr_info resolve_addr;
  struct addrinfo addr;
};

/*! \brief Server information */
struct sd_serv_info
{
  int list_sock_fd;


  char type;
#define SERVER_TYPE_CONTROL              0
#define SERVER_TYPE_DATA                 1

  /* list of addresses to approve */
  linked_list accept_addresses;

  struct sd_mutex_state_info mutex_state;

  char state;
#define SERVER_STATE_CLOSED              0
#define SERVER_STATE_RESOLVE_IP          1
#define SERVER_STATE_LISTENING           2
#define SERVER_STATE_DELETE              3

  /* for getaddrinfo */
  struct sd_resolve_addr_info resolve_addr;

  struct addrinfo servinfo; /* user selected sock addr to bind */


  //fd_set read_fd_set;
  struct timeval read_timeout;
  
  /* SSL */

  char enable_ssl;
  struct sd_ssl_verify_info ssl_verify;
};



/*! \brief Initialise the network information */
extern void net_init();

/*! \brief Cleanup the network information */
extern void net_deinit();


/* -[ servers ]-------------------------------------------------------- */

/*! \brief Initialise a server */
extern struct sd_serv_info *server_init(const char *a, const char *s, char essl,
    struct sd_ssl_verify_info *vi, char t);

/*! \brief Add server to linked list */
extern struct list_item *server_add(void);

/*! \brief Cleanup a server */
extern void server_deinit(struct sd_serv_info *si);

/*! \brief Iterate function for server_rem_all_state_delete() */
extern int server_get_state_delete_iterate(void *value, int index);

/*! \brief Remove all the server marked for deletion */
extern int server_rem_all_state_delete(void);

/*! \brief Start the state machine for a server */
extern void server_start_state_machine(struct sd_serv_info *si, char istate, char t);

/*! \brief Idle function for server */
extern void handle_server_state(struct sd_serv_info *si);

/*! \brief Determins which thread code to run based on the state */
extern int handle_server_thread(struct sd_serv_info *si);

/*! \brief Create socket, bind and listen for server */
extern int server_bind(struct sd_serv_info *serv);

/*! \brief Close a server */
extern int server_close(struct sd_serv_info *serv);

/*! \brief Run in idle waiting for clients */
extern int server_handle_con(struct sd_serv_info *serv);


/* -[ server accepts ]------------------------------------------------- */

/*! \brief Initialise accept information */
extern struct sd_serv_accept_info *server_accept_init(
    struct sd_serv_info *si, const char *a, const char *s);

/*! \brief Add server accept item to linked list */
extern struct list_item *server_accept_add(struct sd_serv_info *si);

/*! \brief Start the state machine for a server accept */
extern void server_accept_start_state_machine(struct sd_serv_accept_info *sai,
    char istate, char t);

/*! \brief Idle function for server accept */
extern void handle_server_accept_state(struct sd_serv_accept_info *sai);

/*! \brief Determins which thread code to run based on the state */
extern int handle_server_accept_thread(struct sd_serv_accept_info *sai);

/*! \brief Determine if the connecting peer is valid */
extern int validate_accept_peers(void *value, int index);

/*! \brief Clean up server accept */
extern void server_accept_deinit(struct sd_serv_accept_info *sai);

/*! \brief Iteration function for server_accept_deinit() */
extern int server_accept_get_state_delete_iterate(void *value, int index);

/*! \brief Remove all the server accepts marked for deletion */
extern int server_accept_rem_all_state_delete_iter(void *v, int index);


/* -[ connections ]---------------------------------------------------- */

/*! \brief Manually initialise a connection */
extern void con_init(struct sd_con_info *co, const char *a, const char *p,
    char enable_ssl, struct sd_ssl_verify_info *vi, char t);

/*! \brief Start the state machine for a connection */
extern void con_start_state_machine(struct sd_con_info *ci, char istate, char t);

/*! \brief Idle function for connection */
extern int handle_con_thread(struct sd_con_info *ci);

/*! \brief Determins which thread code to run based on the state */
extern void handle_con_state(struct sd_con_info *ci);

/*! \brief Bind the source address to next avaliable port >= 1500 */
extern int con_bind_src_address(struct sd_con_info *ci);

/*! \brief Connect to destination address */
extern int con_connect(struct sd_con_info *ci);

/*! \brief Get an ascii string for the current connection state */
extern char *get_con_state_string(struct sd_con_info *ci);

/*! \brief Close a socket and cleanup */
extern int socket_close(int *fd, char fd_clr, char enable_ssl, SSL *ssl);


/* -[ common ]--------------------------------------------------------- */

/*! \brief lookup an address with getaddrinfo() */
extern int address_lookup(struct sd_resolve_addr_info *rai);

/*! \brief set resolve address information */
extern void resolve_addr_set_info(struct sd_resolve_addr_info *ri, const char *a,
    const char *s, char as);

/*! \brief get a pointer to the sockaddr_X */
extern void *get_in_addr(struct sockaddr *sa);

/*! \brief Get a full ascii string for an endpoint address */
extern char *get_sockaddr_storage_string(struct sockaddr_storage *sas);

/*! \brief Get an ascii string for an endpoint port */
extern char *get_sockaddr_storage_port_string(struct sockaddr_storage *sas);

#endif


// vim:ts=2:expandtab
