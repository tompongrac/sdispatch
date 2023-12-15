#ifndef SD_IDLE_H
#define SD_IDLE_H

#define FRAME_TIME_MIN_S  0.03 /* s */

#define FRAME_SLEEP_MS                        1 /* ms */
#define FRAME_SLEEP_US    FRAME_SLEEP_MS * 1000 /* us */

/*! \brief Contains the main sd processing loop */
extern void ui_idle(void);

/*! \brief For efficiency */
extern void relieve_cpu(void);

#endif


// vim:ts=2:expandtab
