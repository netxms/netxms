/*
** NetXMS subagent for GNU/Linux
** Copyright (C) 2004-2024 Raden Solutions
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
**/

#include "linux_subagent.h"

/**
 * Count items in a list of ranges, like e.g. /sys/devices/system/cpu/online
 *
 * @param buffer contents of the file verbatim. The buffer is destructively modified!
 * @return count of items (e.g. CPU) as described in the buffer
 */
long CountRanges(char *buffer);

/**
 * Count items in a ranges-list file, like e.g. /sys/devices/system/cpu/online
 */
LONG CountRangesFile(const char *filepath, long *pValue);
