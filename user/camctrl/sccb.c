/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2013 Atmark Techno, Inc.
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.  
 */

#include <glib.h>
#include <glib/gstdio.h>
#include <inttypes.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#include "sccb.h"

static int sccb_fd = -1;
static int sccb_addr = -1;

static int smbus_raw_writeb(uint8_t val)
{
	struct i2c_smbus_ioctl_data idata;
	int ret;

	idata.read_write = I2C_SMBUS_WRITE;
	idata.command = val;
	idata.size = 1;
	ret = ioctl(sccb_fd, I2C_SMBUS, &idata);
	if (ret == -1)
		return -1;

	return 0;
}

static int smbus_raw_readb(void)
{
	struct i2c_smbus_ioctl_data idata;
	union i2c_smbus_data data;
	int ret;

	idata.read_write = I2C_SMBUS_READ;
	idata.command = 0;
	idata.size = 1;
	idata.data = &data;

	ret = ioctl(sccb_fd, I2C_SMBUS, &idata);
	if (ret == -1)
		return -1;

	return (data.byte & 0xff);
}


static int smbus_writeb(uint8_t daddr, uint8_t val)
{
	struct i2c_smbus_ioctl_data idata;
	union i2c_smbus_data data;
	int ret;

	data.byte = val;

	idata.read_write = I2C_SMBUS_WRITE;
	idata.command = daddr;
	idata.size = 2;
	idata.data = &data;
	ret = ioctl(sccb_fd, I2C_SMBUS, &idata);
	if (ret == -1)
		return -1;

	return 0;
}

static inline int smbus_set_slave(void)
{
	return ioctl(sccb_fd, I2C_SLAVE_FORCE, sccb_addr);
}

static int sccb_readb(int daddr)
{
	int res;

	res = smbus_set_slave();
	if (res == -1) {
		g_printerr("Error: Could not set address to 0x%02x: %s\n",
			   sccb_addr, g_strerror(errno));
		return res;
	}

	res = smbus_raw_writeb(daddr);
	if (res == -1)
		g_printerr("Error: Write address failed: %s\n",
			   g_strerror(errno));

	res = smbus_raw_readb();
	if (res == -1)
		g_printerr("Error: Read data failed: %s\n",
			   g_strerror(errno));

	return res;
}

static int sccb_writeb(int daddr, uint8_t val)
{
	int res;

	res = smbus_set_slave();
	if (res == -1) {
		g_printerr("Error: Could not set address to 0x%02x: %s\n",
			   sccb_addr, g_strerror(errno));
		return res;
	}

	res = smbus_writeb(daddr, val);
	if (res < 0)
		g_printerr("Error: Write data failed: %s\n",
			   g_strerror(errno));

	return res;
}

#define SCCB_DEVFILE_LEN (20)
gboolean sccb_open(int bus, guint8 addr)
{
	gchar dev[SCCB_DEVFILE_LEN];

	g_sprintf(dev, "/dev/i2c-%d", bus);
	sccb_fd = open(dev, O_RDWR, 0);
	if (sccb_fd == -1) {
		if (errno == ENOENT) {
			g_printerr("Error: Could not open file "
				   "`/dev/i2c-%d': %s\n",
				   bus, g_strerror(errno));
		} else {
			g_printerr("Error: Could not open file "
				   "`%s': %s\n", dev, g_strerror(errno));
			if (errno == EACCES)
				g_printerr("Run as root?\n");
		}
		return FALSE;
	}
	sccb_addr = addr;

	return TRUE;
}

gboolean sccb_close(void)
{
	int ret;

	ret = close(sccb_fd);
	sccb_fd = -1;
	sccb_addr = -1;

	if (ret == -1)
		return FALSE;

	return TRUE;
}

gboolean sccb_read(struct sccb_data *data)
{
	int ret;
	int i;

	data->val = 0;
	for (i = 0; data->daddr[i] >= 0; i++) {
		if (data->daddr[i] < 0 || data->daddr[i] > 0xff) {
			g_printerr("Error: Data address invalid!\n");
			return FALSE;
		}

		ret = sccb_readb(data->daddr[i]);
		if (ret < 0)
			return FALSE;
		data->val |= ((ret & 0xff) << (i * 8));
	}

	return TRUE;
}

gboolean sccb_write(struct sccb_data *data)
{
	guint32 val = data->val;
	int ret;
	int i;

	for (i = 0; data->daddr[i] >= 0; i++) {
		if (data->daddr[i] < 0 || data->daddr[i] > 0xff) {
			g_printerr("Error: Data address invalid!\n");
			return FALSE;
		}

		ret = sccb_writeb(data->daddr[i], val & 0xff);
		if (ret < 0)
			return FALSE;
		val >>= ((i + 1) * 8);
	}

	return TRUE;
}
