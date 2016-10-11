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

START_TEST(test_module_list)
{
	struct module mod1, mod2;

	fail_if(module_list_iter_first() != NULL,
		"module list is not empty on startup");

	module_list_append(&mod1);
	fail_if(module_list_iter_first() != &mod1,
		"append a module to empty list");

	module_list_append(&mod2);
	fail_if(module_list_iter_first() != &mod1,
		"first module has changed when 2nd module is appended");
	fail_if(module_list_iter_next(&mod1) != &mod2,
		"module_list_iter_next does not return 2nd module");
	fail_if(module_list_iter_next(&mod2) != NULL,
		"module_list_iter_next does not return NULL");

	module_list_remove(&mod2);
	fail_if(module_list_iter_first() != &mod1,
		"first module has changed when 2nd module is removed");
	fail_if(module_list_iter_next(&mod1) != NULL,
		"2nd module was not removed properly");

	module_list_append(&mod2);
	module_list_remove(&mod1);
	fail_if(module_list_iter_first() != &mod2,
		"first module is not removed properly");

	module_list_remove(&mod2);
	fail_if(module_list_iter_first() != NULL,
		"module list is not empty even if all modules removed");
}
END_TEST

static TCase *
tc_module_list(void)
{
	TCase *tc = tcase_create("module_list");
	tcase_add_test(tc, test_module_list);
	return tc;
}

Suite *
ts_module(void)
{
	Suite *s = suite_create("module");

	suite_add_tcase(s, tc_module_list());
	return s;
}
