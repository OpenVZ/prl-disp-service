///////////////////////////////////////////////////////////////////////////////
///
/// @file TerminalUtils.cpp
///
/// Copyright (c) 2005-2017, Parallels International GmbH
///
/// This file is part of Virtuozzo Core Libraries. Virtuozzo Core
/// Libraries is free software; you can redistribute it and/or modify it
/// under the terms of the GNU Lesser General Public License as published
/// by the Free Software Foundation; either version 2.1 of the License, or
/// (at your option) any later version.
///
/// This library is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
/// Lesser General Public License for more details.
///
/// You should have received a copy of the GNU Lesser General Public
/// License along with this library.  If not, see
/// <http://www.gnu.org/licenses/> or write to Free Software Foundation,
/// 51 Franklin Street, Fifth Floor Boston, MA 02110, USA.
///
/// Our contact details: Parallels International GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
///////////////////////////////////////////////////////////////////////////////

#include "TerminalUtils.h"

#ifdef _MAC_
#include <string.h>

void PackTermios(struct termios *t,
	struct parallels_termios *p)
{
	memset(p, 0, sizeof(struct parallels_termios));
#define PACK_FLAG(field, src_flag, dst_flag) \
	if (t->field & src_flag)	\
		p->field |= dst_flag
	PACK_FLAG(c_iflag, IGNBRK, 0000001);
	PACK_FLAG(c_iflag, BRKINT, 0000002);
	PACK_FLAG(c_iflag, IGNPAR, 0000004);
	PACK_FLAG(c_iflag, PARMRK, 0000010);
	PACK_FLAG(c_iflag, INPCK, 0000020);
	PACK_FLAG(c_iflag, ISTRIP, 0000040);
	PACK_FLAG(c_iflag, INLCR, 0000100);
	PACK_FLAG(c_iflag, IGNCR, 0000200);
	PACK_FLAG(c_iflag, ICRNL, 0000400);
	PACK_FLAG(c_iflag, IXON, 0002000);
	PACK_FLAG(c_iflag, IXANY, 0004000);
	PACK_FLAG(c_iflag, IXOFF, 0010000);
	PACK_FLAG(c_iflag, IMAXBEL, 0020000);

	PACK_FLAG(c_oflag, OPOST, 0000001);
	PACK_FLAG(c_oflag, ONLCR, 0000004);
	PACK_FLAG(c_oflag, OCRNL, 0000010);
	PACK_FLAG(c_oflag, ONOCR, 0000020);
	PACK_FLAG(c_oflag, ONLRET, 0000040);
	PACK_FLAG(c_oflag, OFILL, 0000100);
	PACK_FLAG(c_oflag, OFDEL, 0000200);

	PACK_FLAG(c_cflag, CSIZE, 0000060);
	PACK_FLAG(c_cflag,   CS6, 0000020);
	PACK_FLAG(c_cflag,   CS7, 0000040);
	PACK_FLAG(c_cflag,   CS8, 0000060);
	PACK_FLAG(c_cflag, CSTOPB, 0000100);
	PACK_FLAG(c_cflag, CREAD, 0000200);
	PACK_FLAG(c_cflag, PARENB, 0000400);
	PACK_FLAG(c_cflag, PARODD, 0001000);
	PACK_FLAG(c_cflag, HUPCL, 0002000);
	PACK_FLAG(c_cflag, CLOCAL, 0004000);

	PACK_FLAG(c_lflag, ISIG, 0000001);
	PACK_FLAG(c_lflag, ICANON, 0000002);
	PACK_FLAG(c_lflag, ECHO, 0000010);
	PACK_FLAG(c_lflag, ECHOE, 0000020);
	PACK_FLAG(c_lflag, ECHOK, 0000040);
	PACK_FLAG(c_lflag, ECHONL, 0000100);
	PACK_FLAG(c_lflag, NOFLSH, 0000200);
	PACK_FLAG(c_lflag, TOSTOP, 0000400);
	PACK_FLAG(c_lflag, IEXTEN, 0100000);
#undef PACK_FLAG

#define PACK_CC(src_n, dst_n)	\
	p->c_cc[dst_n] = t->c_cc[src_n]

	PACK_CC(VINTR, 0);
	PACK_CC(VQUIT, 1);
	PACK_CC(VERASE, 2);
	PACK_CC(VKILL, 3);
	PACK_CC(VEOF, 4);
	PACK_CC(VTIME, 5);
	PACK_CC(VMIN, 6);
	PACK_CC(VSTART, 8);
	PACK_CC(VSTOP, 9);
	PACK_CC(VSUSP, 10);
	PACK_CC(VEOL, 11);
	PACK_CC(VREPRINT, 12);
	PACK_CC(VDISCARD, 13);
	PACK_CC(VWERASE, 14);
	PACK_CC(VLNEXT, 15);
	PACK_CC(VEOL2, 16);
#undef PACK_CC
	p->c_ispeed = (unsigned int)t->c_ispeed;
	p->c_ospeed = (unsigned int)t->c_ospeed;
}

void UnpackTermios(struct parallels_termios *p,
	struct termios *t)
{
	memset(t, 0, sizeof(struct termios));
#define UNPACK_FLAG(field, src_flag, dst_flag) \
	if (p->field & src_flag)	\
		t->field |= dst_flag
	UNPACK_FLAG(c_iflag, 0000001, IGNBRK);
	UNPACK_FLAG(c_iflag, 0000002, BRKINT);
	UNPACK_FLAG(c_iflag, 0000004, IGNPAR);
	UNPACK_FLAG(c_iflag, 0000010, PARMRK);
	UNPACK_FLAG(c_iflag, 0000020, INPCK);
	UNPACK_FLAG(c_iflag, 0000040, ISTRIP);
	UNPACK_FLAG(c_iflag, 0000100, INLCR);
	UNPACK_FLAG(c_iflag, 0000200, IGNCR);
	UNPACK_FLAG(c_iflag, 0000400, ICRNL);
	UNPACK_FLAG(c_iflag, 0002000, IXON);
	UNPACK_FLAG(c_iflag, 0004000, IXANY);
	UNPACK_FLAG(c_iflag, 0010000, IXOFF);
	UNPACK_FLAG(c_iflag, 0020000, IMAXBEL);

	UNPACK_FLAG(c_oflag, 0000001, OPOST);
	UNPACK_FLAG(c_oflag, 0000004, ONLCR);
	UNPACK_FLAG(c_oflag, 0000010, OCRNL);
	UNPACK_FLAG(c_oflag, 0000020, ONOCR);
	UNPACK_FLAG(c_oflag, 0000040, ONLRET);
	UNPACK_FLAG(c_oflag, 0000100, OFILL);
	UNPACK_FLAG(c_oflag, 0000200, OFDEL);

	UNPACK_FLAG(c_cflag, 0000060, CSIZE);
	UNPACK_FLAG(c_cflag, 0000020,   CS6);
	UNPACK_FLAG(c_cflag, 0000040,   CS7);
	UNPACK_FLAG(c_cflag, 0000060,   CS8);
	UNPACK_FLAG(c_cflag, 0000100, CSTOPB);
	UNPACK_FLAG(c_cflag, 0000200, CREAD);
	UNPACK_FLAG(c_cflag, 0000400, PARENB);
	UNPACK_FLAG(c_cflag, 0001000, PARODD);
	UNPACK_FLAG(c_cflag, 0002000, HUPCL);
	UNPACK_FLAG(c_cflag, 0004000, CLOCAL);

	UNPACK_FLAG(c_lflag, 0000001, ISIG);
	UNPACK_FLAG(c_lflag, 0000002, ICANON);
	UNPACK_FLAG(c_lflag, 0000010, ECHO);
	UNPACK_FLAG(c_lflag, 0000020, ECHOE);
	UNPACK_FLAG(c_lflag, 0000040, ECHOK);
	UNPACK_FLAG(c_lflag, 0000100, ECHONL);
	UNPACK_FLAG(c_lflag, 0000200, NOFLSH);
	UNPACK_FLAG(c_lflag, 0000400, TOSTOP);
	UNPACK_FLAG(c_lflag, 0100000, IEXTEN);
#undef UNPACK_FLAG

#define UNPACK_CC(src_n, dst_n)	\
	t->c_cc[dst_n] = p->c_cc[src_n]

	UNPACK_CC(0, VINTR);
	UNPACK_CC(1, VQUIT);
	UNPACK_CC(2, VERASE);
	UNPACK_CC(3, VKILL);
	UNPACK_CC(4, VEOF);
	UNPACK_CC(5, VTIME);
	UNPACK_CC(6, VMIN);
	UNPACK_CC(8, VSTART);
	UNPACK_CC(9, VSTOP);
	UNPACK_CC(10, VSUSP);
	UNPACK_CC(11, VEOL);
	UNPACK_CC(12, VREPRINT);
	UNPACK_CC(13, VDISCARD);
	UNPACK_CC(14, VWERASE);
	UNPACK_CC(15, VLNEXT);
	UNPACK_CC(16, VEOL2);
#undef PACK_CC
	p->c_ispeed = (unsigned int)t->c_ispeed;
	p->c_ospeed = (unsigned int)t->c_ospeed;
}
#endif /* _MAC_ */

