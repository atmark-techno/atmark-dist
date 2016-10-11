/**
 * 7seg-led-control.c - Demo application (cgi) for SUZAKU LED/SW Board
 *
 * Copyright (C) 2006 Atmark Techno, Inc.
 * Author: Chris McHarg <chris (at) atmark-techno.com>
 *         Tetsuya OHKAWA <tetsuya@atmark-techno.com>
 *
 * History:
 *    2006-07-14  created by chris
 *    2006-08-11  changed interaction with LED hardware to use
 *                the device driver instead of direct register access.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define PROGRAM_NAME		"7seg-led-control"
#define CGI_PATH		PROGRAM_NAME".cgi"

#define DEV_NAME                "/dev/sil7segc"

#define FORM_OK_BUTTON		"ok_button"
#define FORM_LED1_TEXT_BOX	"led1"
#define FORM_LED2_TEXT_BOX	"led2"
#define FORM_LED3_TEXT_BOX	"led3"

static void print_content_type(void)
{
	printf("Content-Type:text/html\n\n");
}

static void print_style_sheet(void)
{
	printf("<style type=\"text/css\">\n\n");

	printf("body {\n");
	printf("margin: 0 0 0 0;\n");
	printf("padding: 10px 10px 10px 10px;\n");
	printf("font-family: Arial, sans-serif;\n");
	printf("background: #ffffff;\n");
	printf("}\n\n");

	printf("h1 {\n");
	printf("margin: 0 0 0 0;\n");
	printf("padding: 0 0 0 0;\n");
	printf("color: #cc0000;\n");
	printf("font-weight: normal;\n");
	printf("}\n\n");

	printf("h2 {\n");
	printf("margin: 0 0 0 0;\n");
	printf("padding: 0 0 0 0;\n");
	printf("font-size: 14px;\n");
	printf("}\n\n");

	printf("hr {\n");
	printf("height: 1px;\n");
	printf("background-color: #999999;\n");
	printf("border: none;\n");
	printf("margin: 5px 0 70px 0;\n");
	printf("}\n\n");

	printf(".leds {\n");
	printf("font-size: 12px;\n");
	printf("font-weight: bold;\n");
	printf("line-height: 20px;\n");
	printf("}\n\n");

	printf("</style>\n\n");
}

static void print_html_head(void)
{
	printf("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n");
	printf("<html xmlns=\"http://www.w3.org/1999/xhtml\" lang=\"ja\" xml:lang=\"ja\">\n\n");

	printf("<head>\n\n");

	printf("<meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\"/>\n\n");
	printf("<title>%s</title>\n\n", PROGRAM_NAME);

	print_style_sheet();

	printf("</head>\n\n");

	printf("<body>\n\n");
}

static void print_html_tail(void)
{
	printf("</body>\n\n");
	printf("</html>\n");
}

static void display_page(int fd)
{
	unsigned char leds[3];

	read(fd, leds, 3);

	print_content_type();

	print_html_head();

	printf("<h1>7SEG LED CONTROL</h1>\n");
	printf("<h2>ATMARK TECHNO</h2>\n\n");

	printf("<hr />\n\n");

	printf("<form action=\"%s\" method=\"get\">\n\n", CGI_PATH);

	printf("<table border=\"0\" cellpadding=\"10\" cellspacing=\"0\" width=\"200px\" align=\"center\" class=\"leds\">\n");

	printf("<tr>\n");

	printf("<td align=\"center\">");

	printf("LED3<br />\n");
	printf("<input type=\"text\" name=\"%s\" value=\"%x\" size=\"1\" maxlength=\"1\" />\n", FORM_LED3_TEXT_BOX, leds[2]);

	printf("</td>\n<td align=\"center\">");

	printf("LED2<br />\n");
	printf("<input type=\"text\" name=\"%s\" value=\"%x\" size=\"1\" maxlength=\"1\" />\n", FORM_LED2_TEXT_BOX, leds[1]);

	printf("</td>\n<td align=\"center\">");

	printf("LED1<br />\n");
	printf("<input type=\"text\" name=\"%s\" value=\"%x\" size=\"1\" maxlength=\"1\" />\n", FORM_LED1_TEXT_BOX, leds[0]);

	printf("</td>\n");

	printf("</tr><tr>\n");

	printf("<td colspan=\"3\" align=\"center\">\n");

	printf("<input type=\"submit\" value=\"OK\" name=\"%s\" />\n", FORM_OK_BUTTON);

	printf("</td>\n");

	printf("</tr>\n");

	printf("</table>\n\n");

	printf("</form>\n\n");

	print_html_tail();
}

static unsigned int get_query_pair_hex_value(char *query, char *query_pair_name)
{
	char *pair_start, *pair_value;
	unsigned int hex_value = 0;

	pair_start = strstr(query, query_pair_name);
	if (pair_start) {
		pair_value = strchr(pair_start, '=') + 1;
		if (pair_value) {
			sscanf(pair_value, "%x", &hex_value);
		}
	}

	return hex_value;
}

static void handle_query(int fd)
{
	char *query;
	unsigned char leds[3];

	query = getenv("QUERY_STRING");
	if (!query) {
		return;
	}

	if (!strstr(query, FORM_OK_BUTTON)) {
		return;
	}

	leds[0] = (unsigned char) get_query_pair_hex_value(query, FORM_LED1_TEXT_BOX);
	leds[1] = (unsigned char) get_query_pair_hex_value(query, FORM_LED2_TEXT_BOX);
	leds[2] = (unsigned char) get_query_pair_hex_value(query, FORM_LED3_TEXT_BOX);
	write(fd, leds, 3);
}

int main(int argc, char *argv[])
{
	int fd;

	fd = open(DEV_NAME, O_RDWR);
	handle_query(fd);
	display_page(fd);
	close(fd);

	exit(EXIT_SUCCESS);
}
