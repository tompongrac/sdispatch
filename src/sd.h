#ifndef SD_H
#define SD_H


#define SD_OPTION_OFF           0
#define SD_OPTION_ON            1

#define SD_MAX_PATH_LEN       512
#define SD_MAX_FILENAME_LEN   256

#define SD_STRINGIFY(s)                 #s
#define SD_TOSTRING(s)     SD_STRINGIFY(s)

//#define _FILE_OFFSET_BITS      64

/*! \brief Used to change states */
extern void sd_set_state(char *s, int ns);

/*! \brief Get boolean numeric value from ascii string */
extern int get_boolean_id_from_string(const char *str);

/*! \brief Get string boolean value from numeric */
extern char *get_boolean_string(char b);

#endif


// vim:ts=2:expandtab
