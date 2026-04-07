/*
  rbsb.c - terminal handling stuff for lrzsz
  Copyright (C) until 1988 Chuck Forsberg (Omen Technology INC)
  Copyright (C) 1994 Matt Porter, Michael D. Black
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

/*
 *  Rev 05-05-1988
 *  ============== (not quite, but originated there :-). -- uwe
 */
#include <linux/printk.h>

#include "kinetis/serial-port.h"

#include "zglobal.h"

#ifdef USE_SGTTY
#  ifdef LLITOUT
long Locmode;		/* Saved "local mode" for 4.x BSD "new driver" */
long Locbit = LLITOUT;	/* Bit SUPPOSED to disable output translations */
#  endif
#endif

/*
 * return 1 if stdout and stderr are different devices
 *  indicating this program operating with a modem on a
 *  different line
 */
int Fromcu;		/* Were called from cu or yam */
int
from_cu(void)
{
	Fromcu = 0;

	return Fromcu;
}

/*
 *  Return non 0 if something to read from io descriptor f
 */
int
rdchk(struct serial_port *serial)
{
	return serial_port_data_available(serial);
}

/*
 * mode(n)
 *  3: save old tty stat, set raw mode with flow control
 *  2: set XON/XOFF for sb/sz with ZMODEM
 *  1: save old tty stat, set raw mode
 *  0: restore original tty mode
 * Returns the output baudrate, or zero on failure
 */
int
io_mode(struct serial_port *serial, int n)
{
	static int did0 = FALSE;
	u8 parity, data_bits, flow_control;
	static u8 old_parity, old_data_bits, old_flow_control;

	pr_debug("io_mode: mode=%d", n);

	switch (n) {

	case 2:		/* Un-raw mode used by sz, sb when -g detected */
		/* Original: BRKINT | IXON */
		/* BRKINT: generate interrupt on break
		 * IXON: enable XON/XOFF output flow control
		 * Transparent output (c_oflag = 0)
		 * 8 data bits, no parity
		 * No local processing (c_lflag = 0)
		 * VMIN=1, VTIME=1: block until 1 char or 0.1s timeout
		 */
		if (!did0) {
			did0 = TRUE;
			/* Save current configuration */
			old_parity = serial->parity;
			old_data_bits = serial->data_bits;
			old_flow_control = serial->flow_control;
		}

		/* Configure: 8 data bits, no parity, XON/XOFF flow control */
		serial_port_config(serial, serial->baud_rate, 0, 8, 1);

		/* Return baud rate */
		return serial->baud_rate;

	case 1:
	case 3:
		/* Original: IGNBRK | (IXOFF if n==3)
		 * IGNBRK: ignore break conditions
		 * IXOFF: enable input flow control (if n==3)
		 * No echo, no canonical mode, no signals (c_lflag & ~(ECHO|ICANON|ISIG))
		 * Transparent output (c_oflag = 0)
		 * 8 data bits, no parity
		 * VMIN=1, VTIME=1: block until 1 char or 0.1s timeout
		 */
		if (!did0) {
			did0 = TRUE;
			/* Save current configuration */
			old_parity = serial->parity;
			old_data_bits = serial->data_bits;
			old_flow_control = serial->flow_control;
		}

		if (n == 3) { /* with flow control */
			flow_control = 1;
		} else {
			flow_control = 0;
		}

		serial_port_config(serial, serial->baud_rate, 0, 8, flow_control);

		/* Return baud rate */
		return serial->baud_rate;

	case 0:
		/* Original: tcdrain, tcflush, restore oldtty, tcflow(TCOON)
		 * tcdrain: wait until everything is sent
		 * tcflush(TCIOFLUSH): flush input/output queues
		 * tcsetattr: restore original terminal settings
		 * tcflow(TCOON): restart output
		 */
		if (!did0) {
			return 0;
		}

		serial_port_clear_rx(serial);

		serial_port_config(serial, serial->baud_rate, old_parity, old_data_bits, old_flow_control);

		return serial->baud_rate;

	default:
		return 0;
	}
}

void
sendbrk(struct serial_port *serial)
{
	serial_port_send_break(serial);
}

/* End of rbsb.c */
