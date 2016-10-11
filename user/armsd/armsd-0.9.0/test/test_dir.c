/*
 * Copyright (c) 2012, Internet Initiative Japan, Inc.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>
#include <dirent.h>

char tmpdirtmpl[256];
char *tmpdir;

static void
tmpdir_setup(void)
{
	snprintf(tmpdirtmpl, sizeof(tmpdirtmpl), "/tmp/armsd-test-XXXXXX");
	tmpdir = mkdtemp(tmpdirtmpl);
	if (tmpdir != NULL)
		rmdir(tmpdir);
}

static void
tmpdir_teardown(void)
{
	char buf[256];

	if (tmpdir != NULL) {
		snprintf(buf, sizeof(buf), "rm -rf %s", tmpdir);
		system(buf);
	}
	tmpdir = NULL;
}

START_TEST(test_armsdir_set_base)
{
	opt_basedir = "/hoge";
	fail_if(armsdir_set_base() != 0,
		"armsdir_set_base fails with /hoge");
	fail_if(strcmp(armsdir.basedir, "/hoge") != 0,
		"armsdir_set_base failed to set /hoge");

	fail_if(chdir("/tmp") != 0,
		"chdir(/tmp) failed...???");
	opt_basedir = "fuga";
	fail_if(armsdir_set_base() != 0,
		"armsdir_set_base fails with fuga");
	fail_if(strcmp(armsdir.basedir, "/tmp/fuga") != 0,
		"armsdir_set_base failed to set fuga");

	opt_basedir = NULL;
	fail_if(armsdir_set_base() != 0,
		"armsdir_set_base fails with NULL");

	opt_basedir = "/";
	fail_if(armsdir_set_base() != -1,
		"base directory existence check");
}
END_TEST

/* armsdir_create() and armsdir_create_sub() */
START_TEST(test_armsdir_creates)
{
	struct stat st;

	opt_basedir = tmpdir;
	armsdir_set_base();

	fail_if(armsdir_create() != 0,
		"armsdir_create failed");
	fail_if(stat(tmpdir, &st) != 0,
		"armsdir_create failed to create");
	fail_if(!(st.st_mode & S_IFDIR),
		"tmpdir is not a directory!?");
}
END_TEST

/* armsdir_create_tmpfile() and armsdir_remove_tmpfiles() */
START_TEST(test_armsdir_tmpfiles)
{
	FILE *fp;
	char *filename;

	opt_basedir = tmpdir;
	fail_if(armsdir_set_base() != 0, "armsdir_set_base failed");
	fail_if(armsdir_create() != 0, "armsdir_create failed");

	fp = armsdir_create_tmpfile(NULL);
	fail_if(fp == NULL,
		"armsdir_create_tmpfile failed");
	fclose(fp);

	fp = armsdir_create_tmpfile(&filename);
	fail_if(fp == NULL,
		"returns NULL");
	fail_if(filename == NULL,
		"filename is NULL");
	fail_if(access(filename, R_OK|W_OK) != 0,
		"tmpfile created but cannot read or write");
	fclose(fp);

	armsdir_remove_tmpfiles();
	fail_if(access(filename, F_OK) == 0,
		"failed to remove tmpfiles");
}
END_TEST

START_TEST(test_armsdir_removal)
{
	/* Note: armsdir_remove() depends on module_all_remove_files() */

	opt_basedir = tmpdir;
	armsdir_set_base();
	armsdir_create();
	fclose(armsdir_create_tmpfile(NULL));

	armsdir_remove();
	fail_if(access(tmpdir, F_OK) == 0,
		"failed to remove tmpdir");
}
END_TEST

Suite *
ts_dir(void)
{
	Suite *s = suite_create("dir");
	TCase *tc;

	tc = tcase_create("armsdir_set_base");
	tcase_add_test(tc, test_armsdir_set_base);
	tcase_add_unchecked_fixture(tc, tmpdir_setup, tmpdir_teardown);
	suite_add_tcase(s, tc);

	tc = tcase_create("armsdir_creates");
	tcase_add_test(tc, test_armsdir_creates);
	tcase_add_unchecked_fixture(tc, tmpdir_setup, tmpdir_teardown);
	suite_add_tcase(s, tc);

	tc = tcase_create("armsdir_tmpfiles");
	tcase_add_test(tc, test_armsdir_tmpfiles);
	tcase_add_unchecked_fixture(tc, tmpdir_setup, tmpdir_teardown);
	suite_add_tcase(s, tc);

	tc = tcase_create("armsdir_removal");
	tcase_add_test(tc, test_armsdir_removal);
	tcase_add_unchecked_fixture(tc, tmpdir_setup, tmpdir_teardown);
	suite_add_tcase(s, tc);
	return s;
}
