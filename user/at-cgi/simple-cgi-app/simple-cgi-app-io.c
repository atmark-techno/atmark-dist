/*
 * simple-cgi-app/simple-cgi-app-io.c - file and command functions
 *
 * Copyright (C) 2006 Atmark Techno, Inc.
 * Author: Chris McHarg <chris (at) atmark-techno.com>
 */

#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>

#include <simple-cgi-app-io.h>

#define READ_BUF_LEN		(2048)

int cgi_file_exists_l(const char *path)
{
	int ret;
	struct stat buf;
	ret = lstat(path, &buf);
	if (ret != 0) {
		return 0;
	}
	return 1;
}

int cgi_file_exists(const char *path)
{
	int ret;
	struct stat buf;
	ret = stat(path, &buf);
	if (ret != 0) {
		return 0;
	}
	return 1;
}

int cgi_read_file(cgiStr **str_from_file, const char *file_path)
{
	int in_file, read_in;
	char buffer[READ_BUF_LEN];

	in_file = open(file_path, O_RDONLY);
	if (in_file < 0) {
		return -1;
	}

	while ((read_in = read(in_file, buffer, sizeof(buffer))) > 0) {
		cgi_strn_add(str_from_file, buffer, read_in);
	}
	close(in_file);
	if (read_in < 0) {
		return -1;
	} else {
		return 0;
	}
}

void cgi_print_file(const char *file_path)
{
	cgiStr *str_from_file = cgi_str_new(NULL);

	cgi_read_file(&str_from_file, file_path);

	cgi_str_print(str_from_file);

	cgi_str_free(str_from_file);
}

int cgi_dump_to_file(char *path, const char *string)
{
	int ret;
	FILE *file_out;

	file_out = fopen(path, "w");
	if (file_out == NULL) {
		return -1;
	}

	ret = fprintf(file_out, string);
	fclose(file_out);
	if (ret < 0) {
		return -1;
	}

	return 0;
}

int cgi_read_command(cgiStr **cgi_str, const char *path, char *args[])
{
	pid_t pid;
	int pipe_fds_stdout[2];
	int status, ret, read_in;
	char buffer[READ_BUF_LEN];

	if (pipe(pipe_fds_stdout) != 0) {
		return -1;
	}

	pid = fork();

	if (pid == 0) {

		close(1);
		dup2(pipe_fds_stdout[1], 1);
		close(pipe_fds_stdout[0]);
		close(pipe_fds_stdout[1]);

		ret = execvp(path, args);

		if (ret < 0) {
			exit(-1);
		}

	} else if (pid < 0) {

		return -1;

	}

	close(pipe_fds_stdout[1]);

	while ((read_in = read(pipe_fds_stdout[0], buffer, 1)) > 0) {
		cgi_strn_add(cgi_str, buffer, read_in);
	}

	close(pipe_fds_stdout[0]);

	ret = waitpid(pid, &status, 0);
	if (ret < 0) {
		return -1;
	}

	if (!WIFEXITED(status)) {
		return -1;
	} else if (WEXITSTATUS(status)) {
		return -1;
	}

	return 0;
}

void cgi_print_command(const char *path, char *args[])
{
	cgiStr *str_from_file = cgi_str_new(NULL);

	cgi_read_command(&str_from_file, path, args);

	cgi_str_print(str_from_file);

	cgi_str_free(str_from_file);
}

int cgi_exec(const char *path, char *args[])
{
	int ret, status;
	pid_t pid;

	pid = fork();

	if (pid == 0) {

		close(1);
		close(2);

		ret = execv(path, args);

		if (ret < 0) {
			exit(-1);
		}

	} else if (pid < 0) {

		return -1;

	}

	waitpid(pid, &status, 0);
	if (!WIFEXITED(status) || WEXITSTATUS(status)) {
		return -1;
	}

	return 0;
}

int cgi_exec_return_exit_status(const char *path, char *args[])
{
	int ret, status;
	pid_t pid;

	pid = fork();

	if (pid == 0) {

		close(1);
		close(2);

		ret = execv(path, args);

		if (ret < 0) {
			exit(-1);
		}

	} else if (pid < 0) {

		return -1;

	}

	waitpid(pid, &status, 0);
	if (!WIFEXITED(status))
		return -1;
	
	return WEXITSTATUS(status);
}

int cgi_exec_no_wait(const char *path, char *args[])
{
	pid_t pid = fork();

	if (pid == 0) {

		close(1);
		close(2);
		execv(path, args);

	} else if (pid < 0) {

		return -1;

	}

	return 0;
}
