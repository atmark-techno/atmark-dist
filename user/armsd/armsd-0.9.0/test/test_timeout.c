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

static struct timespec testclock;

static void
test_timeout_setclock(unsigned long sec, unsigned long nsec)
{
	testclock.tv_sec = sec;
	testclock.tv_nsec = nsec;
}

#ifdef HAVE_CLOCK_GETTIME
int
clock_gettime(clockid_t which_clock, struct timespec *tp)
{
	*tp = testclock;
	return 0;
}
#else
int
gettimeofday(struct timeval *tv, struct timezone *tz)
{
	tv->tv_sec = testclock.tv_sec;
	tv->tv_usec = testclock.tv_nsec / 1000;
	return 0;
}
#endif

START_TEST(test_timeout_new)
{
	struct timeout *t;

	t = timeout_new(1);
	fail_if(t == NULL,
		"cannot make 1 second timeout");
	timeout_free(t);
}
END_TEST

START_TEST(test_timeout_notyet)
{
	struct timeout *t;

	test_timeout_setclock(1000, 500 * 1000 * 1000);	/* 1000.5 */
	t = timeout_new(10);
	test_timeout_setclock(1002, 0);			/* 1002.0 */
	fail_unless(timeout_remaining(t) == 8500,
		"remaining time is not correct");
	timeout_free(t);
}
END_TEST

START_TEST(test_timeout_equal)
{
	struct timeout *t;

	test_timeout_setclock(2000, 700 * 1000 * 1000);	/* 2000.7 */
	t = timeout_new(5);
	test_timeout_setclock(2005, 700 * 1000 * 1000);	/* 2005.7 */
	fail_unless(timeout_remaining(t) == 0,
		"just fired");
	timeout_free(t);
}
END_TEST

START_TEST(test_timeout_fired)
{
	struct timeout *t;

	test_timeout_setclock(3030, 300 * 1000 * 1000);	/* 3030.3 */
	t = timeout_new(10);
	test_timeout_setclock(3050, 700 * 1000 * 1000);	/* 3050.7 */
	fail_unless(timeout_remaining(t) == 0,
		"already fired");
	timeout_free(t);
}
END_TEST

START_TEST(test_timeout_goback)
{
	struct timeout *t;

	test_timeout_setclock(4444, 400 * 1000 * 1000);	/* 4444.4 */
	t = timeout_new(100);
	test_timeout_setclock(4000, 0);			/* 4000.0 */
#ifdef HAVE_CLOCK_GETTIME
	fail_unless(timeout_remaining(t) == -1,
		"timeout_remaining() returns -1 when system clock goes back");
#else
	fail_unless(timeout_remaining(t) == 100,
		"timeout_remaining() does not reset when system clock "
		"goes back");
#endif
	timeout_free(t);
}
END_TEST

static TCase *
tc_timeout(void)
{
	TCase *tc = tcase_create("timeout");
	tcase_add_test(tc, test_timeout_new);
	tcase_add_test(tc, test_timeout_notyet);
	tcase_add_test(tc, test_timeout_equal);
	tcase_add_test(tc, test_timeout_fired);
	tcase_add_test(tc, test_timeout_goback);
	return tc;
}

Suite *
ts_timeout(void)
{
	Suite *s = suite_create("timeout");

	suite_add_tcase(s, tc_timeout());
	return s;
}
