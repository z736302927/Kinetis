/*
  timing.c - Timing routines for computing elapsed wall time
  Copyright (C) 1994 Michael D. Black
  Copyright (C) 1996, 1997 Uwe Ohse

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

  originally written by Michael D. Black, mblack@csihq.com
*/

#include "kinetis/basic-timer.h"

#include "zglobal.h"

#include "timing.h"

double
timing(int reset, u64 *nowp)
{
	static double elaptime, starttime, stoptime;
	double yet;

	yet = (double) basic_timer_get_us() / 1000000.0;

	if (nowp) {
		*nowp = (u64) yet;
	}
	if (reset) {
		starttime = yet;
		return starttime;
	} else {
		stoptime = yet;
		elaptime = stoptime - starttime;
		return elaptime;
	}
}

/*#define TEST*/
#ifdef TEST
main()
{
	int i;
	display("timing %g", timing(1));
	display("timing %g", timing(0));
	for (i = 0; i < 20; i++) {
		sleep(1);
		display("timing %g", timing(0));
	}
}
#endif
