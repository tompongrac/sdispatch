#ifndef SD_DYNAMIC_MEMORY_H
#define SD_DYNAMIC_MEMORY_H

#include <stdlib.h>

/*! \brief Memory allocation macro */
#define SAFE_CALLOC(x, n, b) do { \
  x = calloc(n, b); \
  if (x == NULL) { \
  ON_ERROR_EXIT("Could not allocate memory.\n"); \
  } } while(0)

/*! \brief Memory free macro */
#define SAFE_FREE(x) do { free(x); } while(0)

#endif


// vim:ts=2:expandtab
