/*
 * at-cgi-core/frame-html.c - frame html for at-cgi
 *
 * Copyright (C) 2006 Atmark Techno, Inc.
 * Author: Chris McHarg <chris (at) atmark-techno.com>
 */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

#include "simple-cgi-app-html-parts.h"

#include "frame-html.h"
#include "core.h"
#include "common.h"
#include "menu.h"

void print_javascript_defs(void)
{
	printf("<script type=\"text/javascript\">\n");
	printf("if (top != self) {\n");
	printf("top.location.href = self.location.href;\n");
	printf("}\n");
	printf("function regevent(caller){\n");
	printf("document.%s.req_area.value = caller.name;\n", FORM_NAME);
	printf("}\n");
	printf("function regeventconfirm(caller, message){\n");
	printf("if (confirm(message)) {\n");
	printf("document.%s.req_area.value = caller.name;\n", FORM_NAME);
	printf("document.%s.submit();\n", FORM_NAME);
	printf("}\n");
	printf("}\n");
	printf("function linkregevent(area){\n");
	printf("document.%s.req_area.value = area;\n", FORM_NAME);
	printf("document.%s.submit();\n", FORM_NAME);
	printf("}\n");
	printf("</script>\n");
}

void print_content_type(void)
{
	printf("Content-Type:text/html\n\n");
}

void print_html_head(const char *post_taget)
{
	printf("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n");
	printf("<html xmlns=\"http://www.w3.org/1999/xhtml\" lang=\"ja\" xml:lang=\"ja\">\n\n<head>\n");
	printf("<meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\"/>\n");
	printf("<title>%s</title>\n", HEAD_TITLE);
	printf("<link rel=\"stylesheet\" type=\"text/css\" href=\"%s\" />\n",
		STYLE_SHEET);

	print_javascript_defs();

	printf("</head>\n\n<body>\n\n");

	printf("<form name=\"%s\" method=\"post\" action=\"%s\">\n", 
		FORM_NAME, post_taget);
	printf("<input type=\"hidden\" name=\"req_area\" value=\"0\" />\n\n");
}

void print_html_tail(void)
{
	printf("</form>\n\n");
	printf("</body>\n\n");
	printf("</html>\n");
}

void print_frame_top(const char *post_taget)
{
	printf("<table border=\"0\" cellpadding=\"0\" cellspacing=\"0\" align=\"center\" width=\"700px\" id=\"frame\">\n\n");
	printf("<tr>\n\n<td>");

	printf("<!-- HEADER -->\n");
	printf("<table border=\"0\" cellpadding=\"0\" cellspacing=\"0\" align=\"center\" width=\"100%%\">\n");
	printf("<tr><td width=\"30%%\" valign=\"bottom\" ><img src=\"%s\" alt=\"Admin\" class=\"title_img\"/></td>\n",
		TITLE_IMG);
	printf("<td width=\"70%%\" valign=\"bottom\" align=\"right\">\n");
	printf("<div class=\"top_system_message\">\n");

	print_system_messages();

	printf("</div>\n</td></tr>\n</table>\n");

	print_main_menu(post_taget);

	printf("</td>\n</tr>\n<tr>\n\n");
	printf("<td width=\"96%%\"><!-- MAIN CONTENT -->\n\n");
}

void print_frame_bottom(void)
{
	printf("\n</td>\n</tr>\n\n");
	printf("<tr>\n<td><!-- FOOTER -->\n");
	printf("<p class=\"footer\">%s version %s<br />", SOFTWARE_NAME, ATCGI_VERSION);
	printf("&copy; 2006 <a href=\"http://www.atmark-techno.com\" target=\"_blank\">Atmark Techno, Inc</a>");
	printf("</p>\n</td>\n</tr>\n\n</table>\n\n");
}
