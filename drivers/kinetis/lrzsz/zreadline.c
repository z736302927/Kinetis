/*
  zreadline.c - line reading stuff for lrzsz
  Copyright (C) until 1998 Chuck Forsberg (OMEN Technology Inc)
  Copyright (C) 1994 Matt Porter
  Copyright (C) 1996, 1997 Uwe Ohse
  Copyright (C) 2018 Michael L. Gran

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2, or (at your option)
  any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
  02111-1307, USA.

  originally written by Chuck Forsberg
*/
/* once part of lrz.c, taken out to be useful to lsz.c too */

#include <linux/printk.h>
#include <linux/slab.h>

#include "kinetis/serial-port.h"
#include "kinetis/basic-timer.h"

#include "zglobal.h"

/* Ward Christensen / CP/M parameters - Don't change these! */
#define TIMEOUT (-2)

static int
readline_internal(zreadline_t *zr, unsigned int timeout);

zreadline_t *
zreadline_init(struct serial_port *serial, size_t readnum, size_t bufsize, int no_timeout)
{
	zreadline_t *zr = (zreadline_t *) kmalloc(sizeof(zreadline_t), GFP_KERNEL);
	memset(zr, 0, sizeof(zreadline_t));
	zr->serial = serial;
	zr->readline_fd = 0;
	zr->readline_readnum = readnum;
	zr->readline_buffer = kmalloc(bufsize > readnum ? bufsize : readnum, GFP_KERNEL);
	if (!zr->readline_buffer) {
		pr_crit(_("out of memory"));
		kfree(zr);
		return NULL;
	}
	zr->no_timeout = no_timeout;
	return zr;
}

void
zreadline_free(zreadline_t *zr)
{
	if (!zr) {
		return;
	}
	kfree(zr->readline_buffer);
	kfree(zr);
}

int
zreadline_getc(zreadline_t *zr, int timeout)
{
	zr->readline_left --;
	if (zr->readline_left >= 0) {
		char c = *(zr->readline_ptr);
		zr->readline_ptr ++;
		return (unsigned char) c;
	} else {
		return readline_internal(zr, timeout);
	}
}

static void
zreadline_alarm_handler(int dummy LRZSZ_ATTRIB_UNUSED)
{
	/* doesn't need to do anything */
}

/*
 * This version of readline is reasonably well suited for
 * reading many characters.
 *
 * timeout is in tenths of seconds
 */
static int
readline_internal(zreadline_t *zr, unsigned int timeout)
{
	unsigned int timeout_ms = 10000;
	if (!zr->no_timeout) {
		/* Convert timeout (tenths of seconds) to milliseconds */
		unsigned int n = timeout / 10;
		if (n < 2 && timeout != 1) {
			n = 3;
		} else if (n == 0) {
			n = 1;
		}
		timeout_ms = n * 1000;  /* Convert to milliseconds */
		pr_notice("Calling read: timeout=%d ms  Readnum=%d ",
			timeout_ms, zr->readline_readnum);
	} else {
		pr_notice("Calling read: Readnum=%d ", zr->readline_readnum);
	}
	zr->readline_ptr = zr->readline_buffer;
	zr->readline_left = serial_port_receive_bytes(zr->serial,
			zr->readline_ptr,
			zr->readline_readnum,
			timeout_ms);

	if (zr->readline_left < 1) {
		pr_notice("Read failure\n");
		return TIMEOUT;
	} else {
		pr_notice("Read returned %d bytes\n", zr->readline_left);
	}
	zr->readline_left -- ;
	char c = *zr->readline_ptr;
	zr->readline_ptr++;
	return (unsigned char) c;
}

void
zreadline_flush(zreadline_t *zr)
{
	zr->readline_left = 0;
	return;
}

void
zreadline_flushline(zreadline_t *zr)
{
	zr->readline_left = 0;

	/* Clear the serial port buffer - read and discard any pending data */
	serial_port_clear_rx(zr->serial);
}

/* send cancel string to get the other end to shut up */
void
zreadline_canit(zreadline_t *zr)
{
	static char canistr[] = {
		24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 0
	};
	int canistr_len = strlen(canistr);

	zreadline_flushline(zr);

	serial_port_transmit_bytes(zr->serial,
				 (const u8 *)canistr,
				 canistr_len);
}
