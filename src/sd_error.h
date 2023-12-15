#ifndef SD_ERROR_H
#define SD_ERROR_H

#include <stdio.h>
#include <openssl/err.h>

#ifdef WIN32

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#endif

#define SD_ERROR_STRING "Error: %s\n"

/*! \brief Print error to stderr */
#define ON_ERROR(msg) do { \
  fprintf(stderr, SD_ERROR_STRING, msg); } while(0)

/*! \brief Print error to stderr and exit */
#define ON_ERROR_EXIT(msg) do { \
  fprintf(stderr, SD_ERROR_STRING, msg); exit(EXIT_FAILURE); } while(0)

/*! \brief Print error to stderr with errno string and exit */
#define ON_SYS_ERROR_EXIT(n, f) do { \
  fprintf(stderr, "Error: %s - %s [%i]\n", f, strerror(errno), errno); \
  exit(EXIT_FAILURE); } while(0)

#endif


// vim:ts=2:expandtab
