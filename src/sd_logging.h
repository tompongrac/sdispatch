#ifndef SD_LOGGING_H
#define SD_LOGGING_H

#define SD_TO_LOG_FILE        69

#define SD_LOG_FILE_FORMAT    "%A %d/%m/%y"

/*! \brief Prepare file for logging */
extern void logging_init(void);

/*! \brief Close the log file */
extern void logging_deinit(void);

/*! \brief Print message to the log file */
extern int logging_print_line(const char *m);

/*! \brief Iteration function for logging_print_line() */
extern int logging_print_line_iter(void *v, int i);


#endif


// vim:ts=2:expandtab
