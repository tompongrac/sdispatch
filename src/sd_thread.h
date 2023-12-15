#ifndef SD_THREAD_H
#define SD_THREAD_H

/*! \brief Mutural exclusion and volatile state information */
struct sd_mutex_state_info
{
#ifdef WIN32
  CRITICAL_SECTION cs_mutex;
#else
  pthread_mutex_t cs_mutex;
#endif
  volatile char proc_state;
#define PROC_STATE_INCOMPLETE            0
#define PROC_STATE_COMPLETE              1
#define PROC_STATE_COMPLETE_WITH_ERROR   2
};

typedef void *(*thread_pos_cb)(void *);
typedef void (*thread_win_cb)(void *);

/*! \brief Set the mutex state */
extern void sd_set_mutex_state(volatile char *s, char ns);

/*! \brief Initialise a thread (mutex) */
extern void sd_thread_init(void *);

/*! \brief Deinitialise a thread (mutex) */
extern void sd_thread_deinit(void *);

/*! \brief Create a thread */
extern int sd_create_thread(thread_pos_cb f, void *args);

/*! \brief Server thread processing */
extern void *sd_mutex_server_func(void *v);

/*! \brief Accept server thread processing */
extern void *sd_mutex_server_accept_func(void *v);

/*! \brief Connection thread processing */
extern void *sd_mutex_con_func(void *v);

/*! \brief Mutex wrapper for control connection idle handling */
extern int sd_mutex_con_idle(void *value, int index);

/*! \brief Mutex wrapper for server idle handling */
extern int sd_mutex_serv_idle(void *value, int index);

/*! \brief Mutex wrapper for data connection idle handling */
extern int sd_mutex_data_con_idle_iter(void *value, int index);

/*! \brief Mutex wrapper for server accept idle handling */
extern int sd_mutex_serv_accept_idle(void *value, int index);

/*! \brief Start a critical section */
extern void sd_cs_lock(
#ifdef WIN32
  CRITICAL_SECTION *cs
#else
  pthread_mutex_t *cs
#endif
);

/*! \brief End a critical section */
extern void sd_cs_unlock(
#ifdef WIN32
  CRITICAL_SECTION *cs
#else
  pthread_mutex_t *cs
#endif
);

#endif

// vim:ts=2:expandtab
