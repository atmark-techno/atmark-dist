/*
 * modules/usbdata_display.c - usbdata data display
 *
 * Copyright (C) 2006 Atmark Techno, Inc.
 * Author: Chris McHarg <chris (at) atmark-techno.com>
 */

#include <stdio.h>

#include <simple-cgi-app-io.h>

#include <misc-utils.h>
#include <core.h>
#include <menu.h>
#include <frame-html.h>
#include <system-config.h>
#include <common.h>

#define USB_DATA_STORE_PATH		"/storage/"

static void display_data_jp(void)
{
	printf("<script type=\"text/javascript\">\n");
	printf("function refreshframe() {\n");
	printf("frames['usbdata'].location.reload(true);\n");
	printf("}\n");
	printf("</script>\n");
}

void display_data(void)
{
	print_content_type();
	print_html_head(USBDATA_CGI_PATH);

	print_frame_top(USBDATA_CGI_PATH);

	printf("<div style=\"margin: 0 0 0 10px;\">\n");
	printf("<button name=\"usbdata_refresh\" onclick=\"refreshframe(); return false\">Refresh</button>\n");	
	printf("</div>\n");
	
	printf("<div class=\"h_gap_small\"></div>\n");

	printf("<div align=\"center\">\n");
	printf("<iframe id=\"usbdata\" name=\"usbdata\" src=\"%s\" width=\"675px\" height=\"900px\" frameborder=\"0\">\n",
		USB_DATA_STORE_PATH);
	printf("</iframe>\n");
	printf("</div>\n");

	printf("<div style=\"margin: 10px 0 0 10px;\">\n");
	printf("<button name=\"usbdata_refresh\" onclick=\"refreshframe(); return false\">Refresh</button>\n");	
	printf("</div>\n");
	
	display_data_jp();
	
	print_frame_bottom();
	print_html_tail();
}
