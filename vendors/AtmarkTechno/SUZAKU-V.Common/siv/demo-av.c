/**
 * demo-av.c - SUZAKU I/O AV Board Sample Application
 *
 * Copyright (C) 2007 Atmark Techno, Inc.
 * Author: Daisuke Mizobuchi <mizo (at) atmark-techno.com>
 *
 * History:
 *    2008-01-18  created by mizo
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <asm/suzaku_siv.h>
#include <linux/fb.h>
#include <linux/byteorder/generic.h>

/* Needed for BYTE_ORDER and BIG/LITTLE_ENDIAN macros. */
#ifndef _BSD_SOURCE
# define _BSD_SOURCE
# include <endian.h>
# undef  _BSD_SOURCE
#else
# include <endian.h>
#endif
#include <byteswap.h>

/* Adapted from the byteorder macros in the Linux kernel. */
#if BYTE_ORDER == LITTLE_ENDIAN
#define cpu_to_le32(x) (x)
#define cpu_to_le16(x) (x)
#else
#define cpu_to_le32(x) bswap_32((x))
#define cpu_to_le16(x) bswap_16((x))
#endif

#define le32_to_cpu(x) cpu_to_le32((x))
#define le16_to_cpu(x) cpu_to_le16((x))

#define PROGRAM_NAME    "demo-av"

#define TEMPLATE_NAME	"demoXXXXXX"
#define TMP_PATH	"/var/tmp/"

#define DEV_FB		"/dev/fb0"

#define FORM_PERFORM_BUTTON	"perform_button"
#define FORM_DISPLAY_BUTTON	"display_button"
#define FORM_COLORBAR_VALUE	(-1)
#define FORM_EDIT_SEL		"edit_sel"
#define FORM_EFFECT_SEL		"effect_sel"
#define FORM_PICTURE_NAME	"picture_name"

#define SCALE (2)
#define BMP_BYTE_PER_PIXEL (3)
#define STRLEN_MAX (50)

struct bmpfileheader {
	unsigned short type;
	unsigned short size_lo;
	unsigned short size_hi;
	unsigned short reserved1;
	unsigned short reserved2;
	unsigned short offbits_lo;
	unsigned short offbits_hi;
};

struct bmpinfoheader {
	unsigned long	size;
	long		width;
	long		height;
	unsigned short	planes;
	unsigned short	bitcount;
	unsigned long	compression;
	unsigned long	image_size;
	long		xpels_per_meter;
	long		ypels_per_meter;
	unsigned long	clr_used;
	unsigned long	clr_important;
};

static void convert_bmp(int fd, int file){
	int i, j;
	int pad;
	unsigned char *buf, *buf_top;
	struct bmpfileheader bf;
	struct bmpinfoheader bi;
	int tmp;
	unsigned long fb_x, fb_y;
	unsigned long bmp_x, bmp_y;
	unsigned long fb_byte_per_pixel;
	unsigned long fb_size, bmp_size;
	struct fb_var_screeninfo var;
	int ret;
	unsigned char *fbmem;

	ret = ioctl(fd, FBIOGET_VSCREENINFO, &var);
	if (ret < 0)
		return;

	fb_x = var.xres_virtual;
	fb_y = var.yres_virtual;
	bmp_x = fb_x / SCALE;
	bmp_y = fb_y / SCALE;
	fb_byte_per_pixel = var.bits_per_pixel / 8;
	fb_size = fb_x * fb_y * fb_byte_per_pixel;
	/* padding of 32bit Boundary condition  */
	pad = (bmp_x + 1) * BMP_BYTE_PER_PIXEL / fb_byte_per_pixel *
	  fb_byte_per_pixel - bmp_x * BMP_BYTE_PER_PIXEL;
	/* bitmap image size */
	bmp_size = (bmp_x * BMP_BYTE_PER_PIXEL + pad) * bmp_y;

	fbmem = mmap(NULL, fb_size, PROT_READ, MAP_SHARED, fd, 0);
	if (fbmem == MAP_FAILED)
		return;

	/* create bitmap file header */
	bf.type            = *(unsigned short*)"BM";
	tmp                = bmp_size + sizeof(struct bmpfileheader) +
			       sizeof(struct bmpinfoheader);
	bf.size_lo         = cpu_to_le16(tmp & 0xffff);
	bf.size_hi         = cpu_to_le16(tmp >> 16);
	bf.reserved1       = 0;
	bf.reserved2       = 0;
	tmp                = sizeof(struct bmpfileheader) +
			       sizeof(struct bmpinfoheader);
	bf.offbits_lo      = cpu_to_le16(tmp & 0xffff);
	bf.offbits_hi      = cpu_to_le16(tmp >> 16);
	bi.size            = cpu_to_le32(sizeof(struct bmpinfoheader));
	bi.width           = cpu_to_le32(bmp_x);
	bi.height          = cpu_to_le32(bmp_y);
	bi.planes          = cpu_to_le16(1);
	bi.bitcount        = cpu_to_le16(24);
	bi.compression     = 0;
	bi.image_size      = cpu_to_le32(bmp_size);
	bi.xpels_per_meter = 0;
	bi.ypels_per_meter = 0;
	bi.clr_used        = 0;
	bi.clr_important   = 0;

	buf_top = buf = (unsigned char *)malloc(bmp_size);
	if(!buf_top)
		return;

	for(i = fb_y - 1 ; i >= 0 ; i -= SCALE){
		for(j = 0 ; j < fb_x ; j += SCALE){
			*(buf++) = (fbmem + (i * fb_x + j) *
				    fb_byte_per_pixel)[3];
			*(buf++) = (fbmem + (i * fb_x + j) *
				    fb_byte_per_pixel)[2];
			*(buf++) = (fbmem + (i * fb_x + j) *
				    fb_byte_per_pixel)[1];
		}
		for(j = 0; j < pad; j++)
			*(buf++) = 0;
	}
  
	ret = write(file, (void *)&bf, sizeof(struct bmpfileheader));
	if (ret < 0)
		return ;
	ret = write(file, (void *)&bi, sizeof(struct bmpinfoheader));
	if (ret < 0)
		return;
	ret = write(file, (void *)buf_top, bmp_size);
	if (ret < 0)
		return;

	free(buf_top);
}

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
        printf("color: #1e90ff;\n");
        printf("font-weight: normal;\n");
        printf("font-size: 26px;\n");
        printf("}\n\n");

        printf("h2 {\n");
        printf("margin: 0 0 0 0;\n");
        printf("padding: 0 0 0 0;\n");
        printf("font-size: 14px;\n");
        printf("}\n\n");

        printf("hr {\n");
        printf("height: 2px;\n");
        printf("color: #1e90ff;\n");
        printf("border-style:dotted;\n");
        
        printf("margin: 5px 0 30px 0;\n");
        printf("}\n\n");

        printf(".leds {\n");
        printf("font-size: 12px;\n");
        printf("font-weight: bold;\n");
        printf("line-height: 20px;\n");
        printf("}\n\n");

        printf("</style>\n\n");
}

static void print_java_script(void)
{
        printf("<SCRIPT language=\"JavaScript\">\n\n"); 

        printf("function set_value(s_val){\n");
        printf("document.frmGet.onbtn.value = s_val;\n");
        printf("}\n\n");

        printf("</SCRIPT>\n\n"); 
}

static void print_html_head(void)
{
        printf("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 "
	       "Transitional//EN\" "
	       "\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\""
	       ">\n");
        printf("<html xmlns=\"http://www.w3.org/1999/xhtml\" lang=\"ja\" "
	       "xml:lang=\"ja\">\n\n");

        printf("<head>\n\n");

        printf("<meta http-equiv=\"Expires\" content=\"-1\"/>\n\n");
        printf("<meta http-equiv=\"Pragma\" content=\"no-cache\"/>\n\n");
        printf("<meta http-equiv=\"cache-control\" "
	       "content=\"no-cache\"/>\n\n");
        printf("<meta http-equiv=\"content-type\" content=\"text/html; "
	       "charset=utf-8\"/>\n\n");
        printf("<title>%s</title>\n\n", PROGRAM_NAME);

        print_style_sheet();
        print_java_script();
                
        printf("</head>\n\n");

        printf("<body>\n\n");
}

static void print_html_tail(void)
{
        printf("</body>\n\n");
        printf("</html>\n");
}

static void display_page(const char *filename, int effect_sel)
{
        print_content_type();

        print_html_head();
        
        printf("<br>\n\n");
        printf("<center><h1>%s</h1></center>\n", PROGRAM_NAME);
        printf("<br>\n\n");
        printf("<hr />\n\n");

        printf("<form name=\"frmGet\" method=\"get\">\n\n");
        printf("<table border=\"0\" cellpadding=\"10\" cellspacing=\"0\" "
	       "align=\"center\">\n");
        printf("<tr>\n");
        printf("<table border=\"0\" cellpadding=\"10\" cellspacing=\"0\" "
	       "width=\"360px\" align=\"center\" class=\"leds\">\n");

	if (filename) {
		printf("<tr>\n");
		printf("<td colspan=\"3\" align=\"center\">\n");
		printf("<img src=\"%s\" alt=\"photo\" border=\"0\">\n",
		       filename);
		printf("</td>\n");
		printf("</tr>\n");
	}
        printf("</tr>\n");



        printf("<tr>\n");
        printf("<tr bgcolor=\"#ddeeff\">\n");
        printf("<td width=\"180px\">\n");
        printf("<fieldset style=\"width:300;height:50;border-color:#000000;\">"
	       "\n");
        printf("<legend align=\"left\" style=\"color:#7a6045;\">"
	       "Effect</legend>\n");
        printf("<input type=\"radio\" value=\"%d\" name=\"%s\" %s/>None\n",
	       EFFECT_COLOR, FORM_EFFECT_SEL,
	       (effect_sel == EFFECT_COLOR) ? "Checked " : "");
        printf("<br>\n\n");
        printf("<input type=\"radio\" value=\"%d\" name=\"%s\" %s/>Black&White"
	       "\n",
	       EFFECT_MONO, FORM_EFFECT_SEL,
	       (effect_sel == EFFECT_MONO) ? "Checked " : "");
        printf("<br>\n\n");
        printf("<input type=\"radio\" value=\"%d\" name=\"%s\" %s/>Gray\n",
	       EFFECT_GRAY, FORM_EFFECT_SEL,
	       (effect_sel == EFFECT_GRAY) ? "Checked " : "");
        printf("<br>\n\n");
        printf("<input type=\"radio\" value=\"%d\" name=\"%s\" %s/>Edge\n",
	       EFFECT_EDGE, FORM_EFFECT_SEL,
	       (effect_sel == EFFECT_EDGE) ? "Checked " : "");
        printf("</fieldset>");
        printf("</td>\n");
        printf("</tr>\n");
        printf("<tr bgcolor=\"#ddeeff\">\n");
        printf("<td>\n");
        printf("<fieldset style=\"width:300;height:50;border-color:#000000\">"
	       "\n");
        printf("<legend align=\"left\" style=\"color:#7a6045;\">Codec Setting"
               "</legend>\n");
        printf("<input type=\"radio\" value=\"%d\" name=\"%s\" %s/>Colorbar\n",
	       FORM_COLORBAR_VALUE, FORM_EFFECT_SEL,
	       (effect_sel == FORM_COLORBAR_VALUE) ? "Checked " : "");
        printf("</fieldset>\n");
        printf("</td>\n");
        printf("</tr>\n");
        printf("<tr>\n");
        printf("<td bgcolor=\"#ddeeff\" align=\"center\">\n");
        printf("<input type=\"submit\" value=\"Set\" name=\"%s\" "
	       "style=\"font-size:16pt; width:100px; height:50px; "
	       "background-color:#ffffff; color:#1e90ff; border-style:solid; "
	       "border-color:#1e90ff; border-width:thin;\"/>\n",
	       FORM_PERFORM_BUTTON);
        printf("</td>\n");
	printf("<td align=\"center\">\n");
	if (effect_sel != FORM_COLORBAR_VALUE)
		printf("<input type=\"submit\" value=\"Display\" name=\"%s\" "
		       "style=\" font-size:16pt; width:100px; height:50px; "
		       "background-color:#1e90ff; color:#ffffff; "
		       "border-style:solid; border-color:#1e90ff; "
		       "border-width:thin;\" />\n",
		       FORM_DISPLAY_BUTTON);
	printf("</td>\n");
        printf("</tr>\n");
        printf("</table>\n");
        printf("</tr>\n");
        printf("</table>\n");

	if (filename)
		printf("<input type=\"hidden\" name=\"%s\" value=\"%s\">",
		       FORM_PICTURE_NAME, filename);

        printf("</form>\n\n");

        print_html_tail();
}

extern int errn;

int main(int argc, char *argv[])
{
        int fd;
	char *bmpfile_name = NULL;
	char template_name[] = TEMPLATE_NAME;
	char *pos;
	char *picture_name = NULL;
	int perform_set = 0;
	int effect_sel = EFFECT_COLOR;
	int display = 0;

        fd = open(DEV_FB, O_RDONLY);

	if (fd >= 0) {
		ioctl(fd, FBIO_DISPLAY_SELECT);
		ioctl(fd, FBIO_CAPTURE_SELECT);
		ioctl(fd, FBIO_DISPLAY_ENABLE);
		ioctl(fd, FBIO_CAPTURE_ENABLE);
		ioctl(fd, FBIO_CAPTURE_MODE_SELECT, CAPTURE_CONTINUOUS_MODE);
	}

        pos = getenv("QUERY_STRING");
	if (pos) {
		pos = strtok(pos, "=");
	}
	while (pos) {
		if (!strcasecmp(pos, FORM_PICTURE_NAME)) {
			picture_name = strtok(NULL, "&");
			if (picture_name && strlen(picture_name) > STRLEN_MAX)
				picture_name = NULL;
		}
		else if (!strcasecmp(pos, FORM_PERFORM_BUTTON)) {
			pos = strtok(NULL, "&");
			if (pos)
				if (!strcasecmp(pos, "Set"))
					perform_set = 1;
		}
		else if (!strcasecmp(pos, FORM_EFFECT_SEL)) {
			pos = strtok(NULL, "&");
			if (pos)
				sscanf(pos, "%x", &effect_sel);
		}
		else if (!strcasecmp(pos, FORM_DISPLAY_BUTTON)) {
			pos = strtok(NULL, "&");
			if (pos)
				if (!strcasecmp(pos, "Display"))
					display = 1;
		}
		pos = strtok(NULL, "=");
	}

	if (picture_name) {
		char tmp[sizeof(TMP_PATH) + strlen(picture_name) + 1];
		sprintf(tmp, TMP_PATH "%s", picture_name);
		unlink(tmp);
	}

	if (perform_set) {
		if (fd >= 0) {
			struct codec_param param;

			param.subaddr = 1;
			if (effect_sel >= 0) {
				ioctl(fd, FBIO_EFFECT_SELECT, effect_sel);
				param.data = 0x58;
			} else
				param.data = 0xD8; /* display colorbar */
			ioctl(fd, FBIO_WRITE_ENCODER_REG, &param);
		}
	}

	if (display) {
		int fd_bmp;

		fd_bmp = mkstemp(template_name);
		if (fd_bmp >= 0) {
			fchmod(fd_bmp,
			       S_IRUSR | S_IWUSR | S_IRGRP |
			       S_IWGRP |S_IROTH | S_IWOTH);
			fd_bmp = open(template_name,
				      O_RDWR | O_CREAT | O_TRUNC, 0666);
			if (fd_bmp >= 0) {
				ioctl(fd, FBIO_CAPTURE_DISABLE);
				convert_bmp(fd, fd_bmp);
				ioctl(fd, FBIO_CAPTURE_ENABLE);
				close(fd_bmp);
				bmpfile_name = template_name;
			}
		}
	}

	display_page(bmpfile_name, effect_sel);

	if (fd >= 0)
		close(fd);

	return EXIT_SUCCESS;
}
