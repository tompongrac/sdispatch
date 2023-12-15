#ifndef SD_FILE_H
#define SD_FILE_H

#include <stdint.h>

#include "sd.h"

#define SD_MAX_MODIFICATION_TIME_LEN       128

/*! \brief File information */
struct file_info
{
  char directory[SD_MAX_PATH_LEN];
  char name[SD_MAX_FILENAME_LEN];
  uint64_t size;
  uint64_t position;
  //time_t modtime;
  char modtime[SD_MAX_MODIFICATION_TIME_LEN];

  FILE *file;
  char state;
#define FILE_STATE_CLOSED         0
#define FILE_STATE_OPENED         1
};

/*! \brief Set file state */
extern void file_set_state(struct file_info *fi, char state);

/*! \brief Set file information automatically with filepath */
extern int file_set_info(const char *filepath, struct file_info *fi);

/*! \brief Set file information manually */
extern void file_manual_set_info(struct file_info *fi,
    const char *f_name, const char *f_dir, const char *m_time,
    uint64_t *of, uint64_t *size);

/*! \brief Set the file directory and file by splitting filepath */
extern void file_set_path_from_fullpath(struct file_info *fi, const char *filepath);

/*! \brief Return a full path from a name and directory */
extern char *file_make_full_path(const char *name, const char *dir);

/*! \brief Efficiently check if file exists */
extern char file_exists(const char *filepath);

/*! \brief Open file with state specific mode */
extern int file_open(struct file_info *fi, char direction);

/*! \brief Set the file pointer */
extern int file_seek(FILE *fi, uint64_t pos, int whence);

/*! \brief Get the currect file pointer */
extern int file_tell(FILE *fi, uint64_t *pos);

/*! \brief Close the file */
extern int file_close(struct file_info *fi);

/*! \brief Efficiently get file length */
extern int file_get_size(const char *filepath, uint64_t *size);


#endif


// vim:ts=2:expandtab
