#ifndef _PARSE_H
#define _PARSE_H

extern int parse_ps3_disc(char *path, char *id);
extern int parse_param_sfo(char *file, const char *field, char *title_name);
extern void change_param_sfo_version(char *file);

#endif
