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

char test_log_logfilename[100];

START_TEST(test_log_open)
{
	const char *nopath = "/nonexistent-d1dfae7c07d22c3065547fc0f7306d34/x";

	fail_unless(log_open_file(test_log_logfilename) == 0,
		"open logfile");
	log_close();

	fail_unless(log_open_file(nopath) == -1,
		"cannot open logfile");

}
END_TEST

START_TEST(test_logit)
{
	int fd;
	size_t n;
	char buf[1000];

	log_open_file(test_log_logfilename);

	logit(LOG_EMERG, "emergency log");

	fd = open(test_log_logfilename, O_RDONLY);
	n = read(fd, buf, sizeof(buf));
	close(fd);

	fail_unless(strstr(buf, "EMERG emergency log") != NULL,
		"EMERG log message:\n%s", buf);

	logit(LOG_ERR, "error log %d", 1234);
	logit(LOG_WARNING, "warning log %s", "hogeri");
	logit(LOG_INFO, "info log");
	logit(LOG_DEBUG, "debug log");

	fd = open(test_log_logfilename, O_RDONLY);
	n = read(fd, buf, sizeof(buf) - 1);
	close(fd);
	buf[n] = '\0';

	fail_unless(strstr(buf, "ERROR error log 1234") != NULL,
		"ERROR log message with a number:\n%s", buf);
	fail_unless(strstr(buf, "WARN  warning log hogeri") != NULL,
		"WARN log message:\n%s", buf);
	fail_unless(strstr(buf, "INFO  info log") != NULL,
		"INFO log message:\n%s", buf);
	fail_unless(strstr(buf, "DEBUG debug log") != NULL,
		"DEBUG log message:\n%s", buf);

	log_close();
}
END_TEST

START_TEST(test_log_close)
{
	log_open_file(test_log_logfilename);
	log_close();
	log_close();	/* we can call log_close() twice */
}
END_TEST

void
test_log_setup(void)
{
	snprintf(test_log_logfilename, sizeof(test_log_logfilename),
		 "/tmp/armsd-test-%u-%u",
		 (unsigned)getpid(),
		 (unsigned)rand());
}

void
test_log_teardown(void)
{
	unlink(test_log_logfilename);
}

static TCase *
tc_log(void)
{
	TCase *tc = tcase_create("log");
	tcase_add_test(tc, test_log_open);
	tcase_add_test(tc, test_logit);
	tcase_add_test(tc, test_log_close);
	tcase_add_unchecked_fixture(tc, test_log_setup, test_log_teardown);
	return tc;
}

Suite *
ts_log(void)
{
	Suite *s = suite_create("log");

	suite_add_tcase(s, tc_log());
	return s;
}
