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

START_TEST(test_distid2str)
{
	distribution_id_t distid;
	char buf[50];

        distid.version = 0x0001;
        distid.vendor_code  = 0x00020003;
        distid.sa_type = 0x0004;
        distid.sa_code = 0x0005000600070008ULL;
	fail_if(distid2str(&distid, buf, sizeof(buf)) == -1,
		"distid2str returns -1");
	fail_if(strcmp("0001-0002-0003-0004-0005-0006-0007-0008", buf) != 0,
		"distid2str returns wrong string");
}
END_TEST

START_TEST(test_str2distid)
{
	distribution_id_t distid;
	const char *s;

	fail_if(str2distid("xxx", &distid) != -1,
		"invalid distribution-id");

	s = "0001-0020-0300-4000";
	fail_if(str2distid(s, &distid) != -1,
		"short distribution-id");

	s = "0000-0000-0000-0000-0000-0000-0000-0000";
	fail_if(str2distid(s, &distid) == -1,
		"parse all-zero distribution-id");
	fail_if(distid.version != 0,
		"parse all-zero distribution-id");
	fail_if(distid.vendor_code != 0,
		"parse all-zero distribution-id");
	fail_if(distid.sa_type != 0,
		"parse all-zero distribution-id");
	fail_if(distid.sa_code != 0,
		"parse all-zero distribution-id");

	s = "0001-0020-0300-4000-0005-0060-0700-8000";
	fail_if(str2distid(s, &distid) == -1,
		"parse a normal distribution-id");
	fail_if(distid.version != 1,
		"parse a normal distribution-id");
	fail_if(distid.vendor_code != 0x00200300,
		"parse a normal distribution-id");
	fail_if(distid.sa_type != 0x4000,
		"parse a normal distribution-id");
	fail_if(distid.sa_code != 0x0005006007008000ULL,
		"parse a normal distribution-id");
}
END_TEST

START_TEST(test_str2distid2str)
{
	distribution_id_t distid;
	const char *s;
	char buf[50];

	s = "0001-0020-0300-4000-0005-0060-0700-8000";
	fail_if(str2distid(s, &distid) == -1,
		"parse a normal distribution-id");
	fail_if (distid2str(&distid, buf, sizeof(buf)) != 0,
		"convert back to distribution-id");
	fail_if(strcmp(s, buf) != 0,
		"str2distid2str");
}
END_TEST

static TCase *
tc_distid(void)
{
	TCase *tc = tcase_create("distid");
	tcase_add_test(tc, test_distid2str);
	tcase_add_test(tc, test_str2distid);
	tcase_add_test(tc, test_str2distid2str);
	return tc;
}

Suite *
ts_misc(void)
{
	Suite *s = suite_create("misc");

	suite_add_tcase(s, tc_distid());
	return s;
}
