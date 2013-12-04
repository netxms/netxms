/* 
** NetXMS - Network Management System
** NetXMS Foundation Library
** Copyright (C) 2003-2013 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published
** by the Free Software Foundation; either version 3 of the License, or
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
** File: parisc_atomic.cpp
**
**/

#include "libnetxms.h"

#if defined(__HP_aCC) && defined(__hppa) && !HAVE_ATOMIC_H

extern "C" volatile int __ldcw(volatile int *);

/**
 * Global lock area
 */
static int lockArea[4] = { 1, 1, 1, 1 };

/**
 * Get 16-byte aligned address within lock area
 */
#ifdef __64BIT__
#define aligned_addr(x) ((volatile int *)(((unsigned long)(x) + 15) & ~0x0F))
#else
#define aligned_addr(x) ((volatile int *)(((unsigned int)(x) + 15) & ~0x0F))
#endif

/**
 * Lock spinlock
 */
inline void spinlock_lock(volatile int *lockArea)
{
        while(1)
        {
                if (*aligned_addr(lockArea) == 1)
                        if (__ldcw(lockArea) == 1)
                                return;
        }
}

/**
 * Unlock spinlock
 */
inline void spinlock_unlock(volatile int *lockArea)
{
        *aligned_addr(lockArea) = 1;
}

/**
 * Atomic increment
 */
VolatileCounter parisc_atomic_inc(VolatileCounter *v)
{
        spinlock_lock(lockArea);
        int nv = ++(*v);
        spinlock_unlock(lockArea);
        return nv;
}

/**
 * Atomic decrement
 */
VolatileCounter parisc_atomic_dec(VolatileCounter *v)
{
        spinlock_lock(lockArea);
        int nv = --(*v);
        spinlock_unlock(lockArea);
        return nv;
}

#endif
