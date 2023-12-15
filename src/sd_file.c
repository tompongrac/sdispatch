/*
   File extraction and handling
 
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

#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <inttypes.h>

#ifdef WIN32
#include <windows.h>
#include <shlwapi.h>
#else
#include <libgen.h>
#endif

#include "sd_file.h"
#include "sd_ui.h"
#include "sd_dynamic_memory.h"
#include "sd_error.h"
#include "sd_logging.h"

void file_set_state(struct file_info *fi, char state)
{
  fi->state = state;
}

int file_set_info(const char *filepath, struct file_info *fi)
{
#ifdef WIN32
  struct _stat fstat;

  if (_stat(filepath, &fstat) == -1) {
#else
  struct stat fstat;

  if (stat(filepath, &fstat) == -1) {
#endif
    ui_sys_err(errno, "stat()");
    return -1;
  }

  fi->size = (uint64_t) fstat.st_size;
  fi->position = 0;

  file_set_path_from_fullpath(fi, filepath);
  file_set_state(fi, FILE_STATE_CLOSED);
  
  /* set mod time */
  struct tm *tm_file;
  char *mt, *cr;

  /* not thread safe for thread safe (_r) */
  tm_file = localtime(&fstat.st_mtime); 
  mt = asctime(tm_file);
  snprintf(fi->modtime, sizeof fi->modtime, "%s", mt);

  /* asctime appends \n, so remove it */
  cr = strrchr(fi->modtime, '\n');
  if (cr)
    *cr = '\0';

  if (!S_ISREG(fstat.st_mode)) {
    ui_sd_err("Only regular files are supported.");
    return -1;
  }

  return 0;
}

void file_set_path_from_fullpath(struct file_info *fi, const char *filepath)
{
  /*  set path */
  char t_dir[sizeof fi->directory];
  char t_base[sizeof fi->name];

  /* make copies as dirname & basename could modify original */
  snprintf(t_dir, sizeof t_dir, "%s", filepath);
  snprintf(t_base, sizeof t_base, "%s", filepath);

#ifdef WIN32
  PathRemoveFileSpec(t_dir);
  PathStripPath(t_base);

  snprintf(fi->directory, sizeof fi->directory, "%s", t_dir);
  snprintf(fi->name, sizeof fi->name, "%s", t_base);
#else 
  snprintf(fi->directory, sizeof fi->directory, "%s", dirname(t_dir));
  snprintf(fi->name, sizeof fi->name, "%s", basename(t_base));
#endif
}

void file_manual_set_info(struct file_info *fi,
    const char *f_name, const char *f_dir, const char *m_time,
    uint64_t *of, uint64_t *size)
{
  if (f_name) snprintf(fi->name, sizeof fi->name, "%s", f_name);
  if (f_dir) snprintf(fi->directory, sizeof fi->directory, "%s", f_dir);
  if (m_time) snprintf(fi->modtime, sizeof fi->modtime, "%s", m_time);
  if (size) fi->size = *size;

  if (of) fi->position = *of;
  else fi->position = 0;

  file_set_state(fi, FILE_STATE_CLOSED);
}

char *file_make_full_path(const char *name, const char *dir)
{
  char *fullpath;
  
  /* handle seperation character */
  int dirlen, fullpathlen;
  char *sep;
  char *sepch = "\\/";

  fullpathlen = SD_MAX_FILENAME_LEN + SD_MAX_FILENAME_LEN + 1;
  SAFE_CALLOC(fullpath, 1, fullpathlen);
  if (dir)
    dirlen = strlen(dir);
  else
    dirlen = 0;
  
  if (dirlen)
  {
    if ( (!strrchr(dir + dirlen - 1, sepch[0])) ||
         (!strrchr(dir + dirlen - 1, sepch[1])) )
    {
#ifdef WIN32
  sep = "\\";
#else
  sep = "/";
#endif
    }
    else {
      sep = "";
    }
  }
  
  snprintf(fullpath, fullpathlen, "%s%s%s", dir, sep, name);

  return fullpath;
}

char file_exists(const char *filepath)
{
#ifdef WIN32
  struct _stat fstat;
  if (_stat(filepath, &fstat) == -1) {
#else
  struct stat fstat;
  if (stat(filepath, &fstat) == -1) {
#endif
    return SD_OPTION_OFF;
  }
  
  return SD_OPTION_ON;
}

int file_open(struct file_info *fi, char direction)
{
  char *fullpath, *mode;

  fullpath = file_make_full_path(fi->name, fi->directory);

  /* set the mode */
  switch (direction)
  {
    case DATA_TRANSFER_DIRECTION_OUTGOING:
      mode = "rb";
      break;
    case DATA_TRANSFER_DIRECTION_INCOMING:
      if (file_exists(fullpath) == SD_OPTION_ON) {
        mode = "r+b";
      }
      else {
        mode = "w+b";
      }
      break;
    case SD_TO_LOG_FILE:
      mode = "a";
      break;
  }

  fi->file = fopen(fullpath, mode);

  if (!fi->file) {
    ui_sys_err(errno, "fopen");
    goto file_open_error;
  }

  /* no need to set position for logging */
  if (direction == SD_TO_LOG_FILE)
    goto file_open_success;


  /* set position */
  uint64_t size;
  if (file_get_size(fullpath, &size) == -1)
    goto file_open_error;

  if (fi->position > size) {
    ui_sd_err("Required an invalid file position.");
    goto file_open_error;
  }
  /* useable */
  else if(fi->position == size) {
    /* at the end */
    if (file_seek(fi->file, 0, SEEK_END) == -1) {
      ui_sys_err(errno, "fseeko");
      goto file_open_error;
    }
  }
  else
  {
    /* normal seek */
    if (file_seek(fi->file, (off_t) fi->position, SEEK_SET) == -1) {
      ui_sys_err(errno, "fseeko");
      goto file_open_error;
    }
  }

file_open_success:
  file_set_state(fi, FILE_STATE_OPENED);
  SAFE_FREE(fullpath);
  return 0;

file_open_error:
  SAFE_FREE(fullpath);
  return -1;
}

int file_seek(FILE *fptr, uint64_t pos, int whence)
{
#ifdef WIN32
  if (fseeko64(fptr, (off_t) pos, whence) == -1)
    return -1;
#else
  if (fseeko(fptr, (off_t) pos, whence) == -1)
    return -1;
#endif

  /* success */
  return 0;
}

int file_tell(FILE *fptr, uint64_t *pos)
{
  off_t p;

#ifdef WIN32
  p = ftello64(fptr);
#else
  p = ftello(fptr);
#endif

  if (p == -1)
    return -1;

  /* success */
  *pos = (uint64_t) p;
  return 0;
}

int file_close(struct file_info *fi)
{
  if (fi->state != FILE_STATE_CLOSED)
  {
    file_set_state(fi, FILE_STATE_CLOSED);

    if (fclose(fi->file))
    {
      ui_sys_err(errno, "fopen");
      return -1;
    }
  }
  else {
    return -1;
  }

  return 0;
}

int file_get_size(const char *filepath, uint64_t *size)
{
#ifdef WIN32
  struct _stat fstat;
  if (_stat(filepath, &fstat) == -1) {
#else
  struct stat fstat;
  if (stat(filepath, &fstat) == -1) {
#endif
    ui_sys_err(errno, "stat()");
    return -1;
  }

  *size = (uint64_t) fstat.st_size;
  
  return 0;
}


// vim:ts=2:expandtab
