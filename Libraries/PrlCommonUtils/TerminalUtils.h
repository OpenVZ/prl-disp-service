///////////////////////////////////////////////////////////////////////////////
///
/// @file TerminalUtils.h
///
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

#ifndef PRL_COMMON_UTILS_TERMINAL_UTILS_INCLUDED
#define PRL_COMMON_UTILS_TERMINAL_UTILS_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _MAC_
#include <termios.h>
#endif

#define PARALLELS_NCCS 32

struct parallels_termios {
	unsigned int	c_iflag;
	unsigned int	c_oflag;
	unsigned int	c_cflag;
	unsigned int	c_lflag;
	char		c_line;
	char		c_cc[PARALLELS_NCCS];
	unsigned int	c_ispeed;
	unsigned int	c_ospeed;
};

#ifdef _MAC_
void PackTermios(struct termios *t, struct parallels_termios *p);
void UnpackTermios(struct parallels_termios *p, struct termios *t);
#endif

#ifdef __cplusplus
}
#endif

#endif	// PRL_COMMON_UTILS_TERMINAL_UTILS_INCLUDED
