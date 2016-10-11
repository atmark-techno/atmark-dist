

#ifndef SIMPLECGIAPPIO_H_
#define SIMPLECGIAPPIO_H_

#include <simple-cgi-app-str.h>

extern int cgi_file_exists(const char *path);
extern int cgi_file_exists_l(const char *path);

extern int cgi_read_file(cgiStr **cgi_str, const char *file_path);
extern void cgi_print_file(const char *file_path);

extern int cgi_dump_to_file(char *path, const char *string);

extern void cgi_print_command(const char *path, char *args[]);
extern int cgi_read_command(cgiStr **cgi_str, const char *path, char *args[]);

extern int cgi_exec(const char *path, char *args[]);
extern int cgi_exec_no_wait(const char *path, char *args[]);

#endif /*SIMPLECGIAPPIO_H_*/
