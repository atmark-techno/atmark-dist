/*****************************************************************************/

/*
 *	newfs.c -- create new flat FLASH file-system.
 *
 *	(C) Copyright 1999, Greg Ungerer (gerg@snapgear.com).
 *	(C) Copyright 2000, Lineo Inc. (www.lineo.com)
 *	(C) Copyright 2002, SnapGear (www.snapgear.com)
 */

/*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>
#include "flatfs.h"
#include "flatfs1.h"

/*****************************************************************************/

/*
 * If the file .init exists, it indicates that the filesystem should be
 * reinitialised, so return 1.
 */

int flat_needinit(void)
{
	if (access(SRCDIR "/.init", R_OK) == 0)
		return 1;
	return 0;
}

/*****************************************************************************/

static int flat_dirfilecount(char *dir)
{
	DIR *dirp;
	struct dirent *dp;
	struct stat st;
	char *curdir, *nextdir;

	curdir = (char *)malloc(FLAT_MAX_PATH_SIZE);
	if (curdir == NULL)
		return ERROR_CODE();

	if (getcwd(curdir, FLAT_MAX_PATH_SIZE) == 0) {
		free(curdir);
		return ERROR_CODE();
	}
	if (chdir(dir) < 0) {
		free(curdir);
		return ERROR_CODE();
	}

	/* Scan directory */
	if ((dirp = opendir(".")) == NULL) {
		free(curdir);
		return ERROR_CODE();
	}

	while ((dp = readdir(dirp)) != NULL) {

		if ((strcmp(dp->d_name, ".") == 0) ||
		    (strcmp(dp->d_name, "..") == 0) ||
		    (strcmp(dp->d_name, FLATFSD_CONFIG) == 0))
			continue;

		if (lstat(dp->d_name, &st) < 0) {
			free(curdir);
			return ERROR_CODE();
		}

		if (S_ISDIR(st.st_mode)) {
			if ((strlen(dir) + strlen(dp->d_name) + 1) >
			    (FLAT_MAX_PATH_SIZE - 1)) {
				syslog(LOG_ERR,
				       "%s/%s: File name too long",
				       dir, dp->d_name);
				free(curdir);
				return ERROR_CODE();
			}
			nextdir = (char *)malloc(FLAT_MAX_PATH_SIZE);
			if (nextdir == NULL) {
				free(curdir);
				return ERROR_CODE();
			}
			sprintf(nextdir, "%s/%s", dir, dp->d_name);
			flat_dirfilecount(nextdir);
			free(nextdir);
			numfiles++;
		} else if (S_ISLNK(st.st_mode)) {
			numfiles++;
		} else if (S_ISREG(st.st_mode)) {
			numfiles++;
			numbytes += st.st_size;
		} else {
			/* Unsupport file type. */
			continue;
		}
	}
	closedir(dirp);

	chdir(curdir);
	free(curdir);

	return 0;
}

/*
 * Count the number of files in the config area.
 * Updates numfiles and numbytes and returns numfiles or < 0 on error.
 */

int flat_filecount(void)
{
	numfiles = 0;
	numbytes = 0;

	flat_dirfilecount(SRCDIR);

	return numfiles;
}

/*****************************************************************************/

static int flat_dirclean(const char *dir)
{
	DIR *dirp;
	struct dirent *dp;
	struct stat st;
	char *curdir, *nextdir;
	int ret;

	curdir = (char *)malloc(FLAT_MAX_PATH_SIZE);
	if (curdir == NULL)
		return ERROR_CODE();

	if (getcwd(curdir, FLAT_MAX_PATH_SIZE) == 0) {
		free(curdir);
		return ERROR_CODE();
	}

	if (chdir(dir) < 0) {
		free(curdir);
		return ERROR_CODE();
	}

	/* Scan directory */
	if ((dirp = opendir(".")) == NULL) {
		free(curdir);
		return ERROR_CODE();
	}

	while ((dp = readdir(dirp)) != NULL) {

		if ((strcmp(dp->d_name, ".") == 0) ||
		    (strcmp(dp->d_name, "..") == 0))
			continue;

		if (lstat(dp->d_name, &st) < 0) {
			free(curdir);
			return ERROR_CODE();
		}

		if (S_ISDIR(st.st_mode)) {
			if ((strlen(dir) + strlen(dp->d_name) + 1) >
			    (FLAT_MAX_PATH_SIZE - 1)) {
				syslog(LOG_ERR, "%s/%s/; File name too long",
				       dir, dp->d_name);
				free(curdir);
				return(ERROR_CODE());
			}

			nextdir = (char *)malloc(FLAT_MAX_PATH_SIZE);
			if (nextdir == NULL) {
				free(curdir);
				return(ERROR_CODE());
			}

			sprintf(nextdir, "%s/%s", dir, dp->d_name);
			ret = flat_dirclean(nextdir);
			free(nextdir);
			if (ret != 0) {
				free(curdir);
				return(ret);
			}
			if (rmdir(dp->d_name) < 0) {
				free(curdir);
				return(ERROR_CODE());
			}

		} else if (S_ISLNK(st.st_mode)) {
			unlink(dp->d_name);

		} else if (S_ISREG(st.st_mode)) {
			unlink(dp->d_name);

		} else {
			/* ignore file */
			continue;
		}
	}

	closedir(dirp);

	chdir(curdir);
	free(curdir);

	return 0;
}

/*
 * Remove all files from the config file-system.
 * If 'realclean' is 0, actually just writes the .init file
 * which indicates that the filesystem should be cleaned out
 * after the next reboot (during flatfsd -r)
 */

int flat_clean(int realclean)
{
	if (realclean) {
		if (chdir(SRCDIR) < 0)
			return ERROR_CODE();
		return flat_dirclean(SRCDIR);
	} else {
		FILE *fh = fopen(SRCDIR "/.init", "w");
		if (fh) {
			fclose(fh);
			return 1;
		}
	}
	return ERROR_CODE();
}

/*****************************************************************************/

static int flat_filecopy(const char *src, const char *dst,
			 const struct stat *st)
{
	unsigned int size, n;
	int fddefault, fdconfig;
	unsigned char buf[1024];

	/* Write the contents of the file. */
	if ((fddefault = open(src, O_RDONLY)) < 0)
		return ERROR_CODE();
	fdconfig = open(dst, O_WRONLY | O_TRUNC | O_CREAT, st->st_mode);
	if (fdconfig < 0)
		return ERROR_CODE();

	for (size = st->st_size; (size > 0); size -= n) {
		n = (size > sizeof(buf)) ? sizeof(buf) : size;
		if (read(fddefault, &buf[0], n) != n)
			break;
		if (write(fdconfig, (void *) &buf[0], n) != n)
			break;
	}
	fchown(fdconfig, st->st_uid, st->st_gid);
	close(fdconfig);
	close(fddefault);

	if (size > 0) {
		numdropped++;
	} else {
		numfiles++;
		numbytes += st->st_size;
	}

	return 0;
}

static int flat_dircopy(const char *dir)
{
	DIR *dirp;
	struct stat st;
	struct dirent *dp;
	char *curdir, *filename;
	int ret;

	curdir = (char *)malloc(FLAT_MAX_PATH_SIZE);
	if (curdir == NULL)
		return(ERROR_CODE());

	if (getcwd(curdir, FLAT_MAX_PATH_SIZE) == NULL) {
		free(curdir);
		return(ERROR_CODE());
	}

	if (chdir(dir) < 0) {
		free(curdir);
		return(ERROR_CODE());
	}

	/* Scan directory */
	if ((dirp = opendir(".")) == NULL) {
		free(curdir);
		return ERROR_CODE();
	}

	while ((dp = readdir(dirp)) != NULL) {

		if ((strcmp(dp->d_name, ".") == 0) ||
		    (strcmp(dp->d_name, "..") == 0))
			continue;

		if (lstat(dp->d_name, &st) < 0) {
			free(curdir);
			return ERROR_CODE();
		}

		if (strlen(SRCDIR) + strlen(&dir[strlen(DEFAULTDIR)]) +
		    strlen(dp->d_name) + 1 > FLAT_MAX_PATH_SIZE - 1) {
			syslog(LOG_ERR, "%s%s/%s: File name too long",
			       SRCDIR, &dir[strlen(DEFAULTDIR)], dp->d_name);
			free(curdir);
			return(ERROR_CODE());
		}

		filename = (char *)malloc(FLAT_MAX_PATH_SIZE);
		if (filename == NULL) {
			free(curdir);
			return(ERROR_CODE());
		}

		sprintf(filename, "%s%s/%s",
			SRCDIR, &dir[strlen(DEFAULTDIR)], dp->d_name);

		if (S_ISDIR(st.st_mode)) {
			if (mkdir(filename, st.st_mode) < 0) {
				free(curdir);
				free(filename);
				return(ERROR_CODE());
			}
			chown(filename, st.st_uid, st.st_gid);

			if (strlen(dir) + strlen(dp->d_name) > 511) {
				syslog(LOG_ERR, "%s/%s: File name too long",
				       dir, dp->d_name);
				free(curdir);
				free(filename);
				return(ERROR_CODE());
			}
			sprintf(filename, "%s/%s", dir, dp->d_name);
			ret = flat_dircopy(filename);
			free(filename);
			if (ret != 0) {
				free(curdir);
				return(ret);
			}

		} else if (S_ISLNK(st.st_mode)) {
			char *slink;

			slink = (char *)malloc(st.st_size + 1);
			if (slink == NULL) {
				free(curdir);
				free(filename);
				return(ERROR_CODE());
			}

			ret = readlink(dp->d_name, slink, st.st_size);
			if (ret < 0) {
				free(curdir);
				free(filename);
				free(slink);
				return(ERROR_CODE());
			}
			slink[ret] = 0;
					
			ret = symlink(slink, filename);
			lchown(filename, st.st_uid, st.st_gid);
			free(slink);
			free(filename);
			if(ret < 0) {
				free(curdir);
				return(ERROR_CODE());
			}

		} else if (S_ISREG(st.st_mode)) {
			ret = flat_filecopy(dp->d_name, filename, &st);
			free(filename);
			if(ret != 0) {
				free(curdir);
				return(ret);
			}
		} else {
			/* ignore file */
			free(filename);
			continue;
		}
		numfiles++;
	}

	closedir(dirp);

	chdir(curdir);
	free(curdir);

	return 0;
}

/*
 * This is basically just a directory copy. Copy all files from the
 * given directory to the config directory.
 */

int flat_new(const char *dir)
{
	if (chdir(dir) < 0)
		return ERROR_CODE();

	numfiles = 0;
	numbytes = 0;
	numdropped = 0;

	return flat_dircopy(dir);
}

/*****************************************************************************/
