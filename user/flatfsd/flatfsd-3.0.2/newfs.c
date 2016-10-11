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
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>
#include <config/autoconf.h>
#include "flatfs.h"

/*****************************************************************************/

/*
 * Maximum path name size that we will support. This is completely arbitary.
 */
#define	MAXNAME	256

/*
 * General work buffer size (these are often allocated on the stack).
 */
#define BUF_SIZE 1024

/*****************************************************************************/

/*
 * If the file .init exists, it indicates that the filesystem should be
 * reinitialised, so return 1.
 */

int flat_needinit(void)
{
#ifndef CONFIG_USER_FLATFSD_EXTERNAL_INIT
	if (access(SRCDIR "/.init", R_OK) == 0)
		return 1;
#endif
	return 0;
}

/*****************************************************************************/

/*
 * Writes the .init file which indicates that the filesystem should be
 * cleaned out after the next reboot (during flatfsd -r)
 */
int flat_requestinit(void)
{
	FILE *fh = fopen(SRCDIR "/.init", "w");
	if (fh) {
		fclose(fh);
		return 1;
	}
	return ERROR_CODE();
}

/*****************************************************************************/

static int flat_dirfilecount(char *dir, int *numfiles, int *numbytes)
{
	DIR *dirp;
	struct dirent *dp;
	struct stat sb;
	char cwdname[MAXNAME], dirname[MAXNAME];
	int rc;

	if (getcwd(cwdname, sizeof(cwdname)) == NULL)
		return ERROR_CODE();
	if (chdir(dir) < 0)
		return ERROR_CODE();

	/* Scan directory */
	if ((dirp = opendir(".")) == NULL)
		return ERROR_CODE();

	while ((dp = readdir(dirp)) != NULL) {

		if ((strcmp(dp->d_name, ".") == 0) ||
		    (strcmp(dp->d_name, "..") == 0) ||
		    (strcmp(dp->d_name, FLATFSD_CONFIG) == 0))
			continue;

		if (lstat(dp->d_name, &sb) == 0) {
			if (S_ISLNK(sb.st_mode)) {
				/* Count symbolic links */
			} else if (S_ISDIR(sb.st_mode)) {
				/* Count directories */
				if (strlen(dir) + 1 + strlen(dp->d_name) + 1 >
				    sizeof(dirname)) {
					syslog(LOG_ERR,
					       "%s/%s: Directory name too long",
					       dir, dp->d_name);
					return ERROR_CODE();
				}
				sprintf(dirname, "%s/%s", dir, dp->d_name);
				rc = flat_dirfilecount(dirname,
						       numfiles, numbytes);
				if (rc < 0)
					return rc;

				sb.st_size = 0;
			} else if (S_ISREG(sb.st_mode)) {
				/* Count normal files */
			} else {
				/* Unsupported files */
				continue;
			}

			(*numfiles)++;
			(*numbytes) += sb.st_size;
		}
	}

	closedir(dirp);

	chdir(cwdname);
	return 0;
}

/*
 * Count the number of files in the config area.
 * Updates numfiles and numbytes and returns numfiles or < 0 on error.
 */

int flat_filecount(char *configdir)
{
	int rc;
	int numfiles = 0, numbytes = 0;

	rc = flat_dirfilecount(configdir, &numfiles, &numbytes);
	if (rc < 0)
		return rc;
	return numfiles;
}

/*****************************************************************************/

#ifndef CONFIG_USER_FLATFSD_EXTERNAL_INIT
/*
 * Remove all files from the config file-system.
 */

static int flat_dirclean(const char *dir)
{
	DIR *dirp;
	struct dirent *dp;
	struct stat sb;
	char cwdname[MAXNAME], dirname[MAXNAME];
	int rc;

	if (getcwd(cwdname, sizeof(cwdname)) == NULL)
		return ERROR_CODE();
	if (chdir(dir) < 0)
		return ERROR_CODE();

	/* Scan directory */
	if ((dirp = opendir(".")) == NULL)
		return ERROR_CODE();

	while ((dp = readdir(dirp)) != NULL) {

		if ((strcmp(dp->d_name, ".") == 0) ||
		    (strcmp(dp->d_name, "..") == 0))
			continue;

		if (lstat(dp->d_name, &sb) == 0) {
			if (S_ISLNK(sb.st_mode)) {
				/* Delete symbolic links */
				unlink(dp->d_name);
			} else if (S_ISDIR(sb.st_mode)) {
				/* Delete directories */
				if (strlen(dir) + 1 + strlen(dp->d_name) + 1 >
				    sizeof(dirname)) {
					syslog(LOG_ERR,
					       "%s/%s; File name too long",
					       dir, dp->d_name);
					return ERROR_CODE();
				}
				sprintf(dirname, "%s/%s", dir, dp->d_name);
				rc = flat_dirclean(dirname);
				if (rc < 0)
					return rc;
				rmdir(dp->d_name);
			} else if (S_ISREG(sb.st_mode)) {
				/* Delete normal files */
				unlink(dp->d_name);
			} else {
				/* Ignore files */
				continue;
			}
		}
	}

	closedir(dirp);

	chdir(cwdname);
	return 0;
}

int flat_clean(void)
{
	return flat_dirclean(SRCDIR);
}
#endif

/*****************************************************************************/

#ifndef CONFIG_USER_FLATFSD_EXTERNAL_INIT
static int flat_filecopy(const char *src, const char *dst,
			 const struct stat *st,
			 int *numfiles, int *numbytes, int *numdropped)
{
	unsigned int size, n;
	int fddefault, fdconfig;
	unsigned char buf[BUF_SIZE];

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
		(*numdropped)++;
	} else {
		(*numfiles)++;
		(*numbytes) += st->st_size;
	}

	return 0;
}

static int flat_dircopy(const char *dir,
			int *numfiles, int *numbytes, int *numdropped)
{
	DIR *dirp;
	struct stat st;
	struct dirent *dp;
	char cwdname[MAXNAME], filename[MAXNAME];
	int rc;

	if (getcwd(cwdname, sizeof(cwdname)) == NULL)
		return ERROR_CODE();
	if (chdir(dir) < 0)
		return ERROR_CODE();

	/* Scan directory */
	if ((dirp = opendir(".")) == NULL)
		return ERROR_CODE();

	while ((dp = readdir(dirp)) != NULL) {

		if ((strcmp(dp->d_name, ".") == 0) ||
		    (strcmp(dp->d_name, "..") == 0))
			continue;

		if (lstat(dp->d_name, &st) < 0)
			return ERROR_CODE();

		if (strlen(SRCDIR) + strlen(&dir[strlen(DEFAULTDIR)]) + 1 +
		    strlen(dp->d_name) + 1 > sizeof(filename)) {
			syslog(LOG_ERR, "%s%s/%s: File name too long",
			       SRCDIR, &dir[strlen(DEFAULTDIR)], dp->d_name);
			return ERROR_CODE();
		}
		sprintf(filename, "%s%s/%s",
			SRCDIR, &dir[strlen(DEFAULTDIR)], dp->d_name);
		if (S_ISLNK(st.st_mode)) {
			/* Copy symbolic links */
			char oldname[st.st_size + 1];

			rc = readlink(dp->d_name, oldname, st.st_size);
			if (rc < 0)
				return ERROR_CODE();
			oldname[rc] = '\0';

			if (symlink(oldname, filename) < 0)
				return ERROR_CODE();
			lchown(filename, st.st_uid, st.st_gid);

			(*numfiles)++;
			(*numbytes) += strlen(oldname);
		} else if (S_ISDIR(st.st_mode)) {
			/* Copy directories */
			if (mkdir(filename, st.st_mode) < 0)
				return ERROR_CODE();
			chown(filename, st.st_uid, st.st_gid);

			if (strlen(dir) + 1 + strlen(dp->d_name) + 1 >
			    sizeof(filename)) {
				syslog(LOG_ERR, "%s/%s: File name too long",
				       dir, dp->d_name);
				return ERROR_CODE();
			}
			sprintf(filename, "%s/%s", dir, dp->d_name);
			rc = flat_dircopy(filename,
					  numfiles, numbytes, numdropped);
			if (rc < 0)
				return rc;

			(*numfiles)++;
		} else if (S_ISREG(st.st_mode)) {
			/* Copy normal files */
			rc = flat_filecopy(dp->d_name, filename, &st,
					   numfiles, numbytes, numdropped);
			if (rc < 0)
				return rc;
		} else {
			/* Ignore files */
			continue;
		}
	}

	closedir(dirp);

	chdir(cwdname);
	return 0;
}

/*
 * This is basically just a directory copy. Copy all files from the
 * given directory to the config directory.
 */

int flat_new(const char *dir)
{
	int rc;
	int numfiles = 0, numbytes = 0, numdropped = 0;

	rc = flat_dircopy(dir, &numfiles, &numbytes, &numdropped);
	if (rc < 0)
		return rc;
	syslog(LOG_INFO, "Created %d configuration files (%d bytes)",
	       numfiles, numbytes);
	return 0;
}
#endif

/*****************************************************************************/
