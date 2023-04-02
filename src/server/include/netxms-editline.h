/*
** NetXMS - Network Management System
** Copyright (C) 2003-2023 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: netxms-editline.h
**
**/

#ifndef _netxms_editline_h_
#define _netxms_editline_h_

#if HAVE_LIBEDIT

#include <histedit.h>

#ifdef UNICODE

#define el_tset el_wset
#define el_tgets el_wgets

#define history_tinit history_winit
#define history_tend history_wend
#define history_t history_w

#define HistoryT HistoryW
#define HistEventT HistEventW

#else

#define el_tset el_set
#define el_tgets el_gets

#define history_tinit history_init
#define history_tend history_end
#define history_t history

#define HistoryT History
#define HistEventT HistEvent

#endif

#endif /* HAVE_LIBEDIT */
#endif /* _netxms_editline_h_ */
