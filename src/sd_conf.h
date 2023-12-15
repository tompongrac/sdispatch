#ifndef SD_CONF_H
#define SD_CONF_H

#define SD_CONFIG_VALUE_TYPE_STRING          0
#define SD_CONFIG_VALUE_TYPE_INT             1
#define SD_CONFIG_VALUE_TYPE_STRING_BOOLEAN  2

/*! \brief Struct that hold configuration item information */
struct sd_conf_item
{
  char *name;
  char type;
  int length;
  void *value;
};

/*! \brief Get number of configuration items */
extern int conf_get_num_items(void);

/*! \brief Get the configuration file path */
extern void conf_set_conf_default_path(void);

/*! \brief Set configuration item with name to point to specified value */
extern int conf_set_pointer(const char *n, void *p);

/*! \brief Set all the configuration items to point to global objects */
extern void conf_set_all_pointers(void);

/*! \brief Setup configuration */
extern void conf_init(void);

/*! \brief Parse the configuration file */
extern int conf_load_file(void);

#endif


// vim:ts=2:expandtab
