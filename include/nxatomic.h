/*
** NetXMS - Network Management System
** Copyright (C) 2003-2022 Victor Kirhenshtein
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
** File: nxatomic.h
**
**/

#ifndef _nxatomic_h_
#define _nxatomic_h_

#include <nms_common.h>

#ifdef __cplusplus

#include <atomic>
using std::atomic;

#ifdef __sun
#include <sys/atomic.h>
#endif

#ifdef _WIN32

// DLL implementations of atomic<> for standard types (int32_t, double, etc.) should be placed here
template struct LIBNETXMS_EXPORTABLE atomic<double>;

typedef volatile LONG VolatileCounter;
typedef volatile LONGLONG VolatileCounter64;

// 64 bit atomics not available on Windows XP, implement them using _InterlockedCompareExchange64
#if !defined(_WIN64) && (_WIN32_WINNT < 0x0502)

#pragma intrinsic(_InterlockedCompareExchange64)

FORCEINLINE LONGLONG InterlockedIncrement64(LONGLONG volatile *v)
{
   LONGLONG old;
   do 
   {
      old = *v;
   } while(_InterlockedCompareExchange64(v, old + 1, old) != old);
   return old + 1;
}

FORCEINLINE LONGLONG InterlockedDecrement64(LONGLONG volatile *v)
{
   LONGLONG old;
   do 
   {
      old = *v;
   } while(_InterlockedCompareExchange64(v, old - 1, old) != old);
   return old - 1;
}

FORCEINLINE LONGLONG InterlockedAdd64(LONGLONG volatile *v, LONGLONG a)
{
   LONGLONG old;
   do
   {
      old = *v;
   } while(_InterlockedCompareExchange64(v, old + a, old) != old);
   return old + a;
}

#endif

#else

#if defined(__sun)

typedef volatile int32_t VolatileCounter;
typedef volatile int64_t VolatileCounter64;

/**
 * Atomically increment 32-bit value by 1
 */
static inline int32_t InterlockedIncrement(VolatileCounter *v)
{
   return (int32_t)atomic_inc_32_nv((volatile uint32_t *)v);
}

/**
 * Atomically decrement 32-bit value by 1
 */
static inline int32_t InterlockedDecrement(VolatileCounter *v)
{
   return atomic_dec_32_nv((volatile uint32_t *)v);
}

/**
 * Atomically increment 32-bit values
 */
static inline int32_t InterlockedAdd(VolatileCounter *v, int32_t a)
{
   return (int32_t)atomic_add_32_nv((volatile uint32_t *)v, a);
}

/**
 * Atomically exchange 32-bit values
 */
static inline int32_t InterlockedCompareExchange(VolatileCounter *target, int32_t exchange, int32_t comparand)
{
   return atomic_cas_32((volatile uint32_t *)target, (uint32_t)comparand, (uint32_t)exchange);
}

/**
 * Atomically exchange 64-bit values
 */
static inline int64_t InterlockedCompareExchange(VolatileCounter64 *target, int64_t exchange, int64_t comparand)
{
   return atomic_cas_64((volatile uint64_t *)target, (uint64_t)comparand, (uint64_t)exchange);
}

/**
 * Atomically increment 64-bit value by 1
 */
static inline int64_t InterlockedIncrement64(VolatileCounter64 *v)
{
   return (int64_t)atomic_inc_64_nv((volatile uint64_t *)v);
}

/**
 * Atomically decrement 64-bit value by 1
 */
static inline int64_t InterlockedDecrement64(VolatileCounter64 *v)
{
   return (int64_t)atomic_dec_64_nv((volatile uint64_t *)v);
}

/**
 * Atomically add 64-bit values
 */
static inline int64_t InterlockedAdd64(VolatileCounter64 *v, int64_t a)
{
   return (int64_t)atomic_add_64_nv((volatile uint64_t *)v, a);
}

/**
 * Atomically set pointer
 */
static inline void *InterlockedExchangePointer(void *volatile *target, void *value)
{
   return atomic_swap_ptr(target, value);
}

/**
 * Atomic bitwise OR
 */
static inline void InterlockedOr(VolatileCounter *target, uint32_t bits)
{
   atomic_or_32((volatile uint32_t *)target, bits);
}

/**
 * Atomic bitwise AND
 */
static inline void InterlockedAnd(VolatileCounter *target, uint32_t bits)
{
   atomic_and_32((volatile uint32_t *)target, bits);
}

/**
 * Atomic bitwise OR 64 bit
 */
static inline void InterlockedOr64(VolatileCounter64 *target, uint64_t bits)
{
   atomic_or_64((volatile uint64_t *)target, bits);
}

/**
 * Atomic bitwise AND 64 bit
 */
static inline void InterlockedAnd64(VolatileCounter64 *target, uint64_t bits)
{
   atomic_and_64((volatile uint64_t *)target, bits);
}

#else /* not Solaris */

typedef volatile int32_t VolatileCounter;
typedef volatile int64_t VolatileCounter64;

/**
 * Atomically increment 32-bit value by 1
 */
static inline int32_t InterlockedIncrement(VolatileCounter *v)
{
#if defined(__GNUC__) && ((__GNUC__ < 4) || (__GNUC_MINOR__ < 1)) && (defined(__i386__) || defined(__x86_64__))
   VolatileCounter temp = 1;
   __asm__ __volatile__("lock; xaddl %0,%1" : "+r" (temp), "+m" (*v) : : "memory");
   return temp + 1;
#elif HAVE_ATOMIC_BUILTINS
   return __atomic_add_fetch(v, 1, __ATOMIC_SEQ_CST);
#else
   return __sync_add_and_fetch(v, 1);
#endif
}

/**
 * Atomically decrement 32-bit value by 1
 */
static inline int32_t InterlockedDecrement(VolatileCounter *v)
{
#if defined(__GNUC__) && ((__GNUC__ < 4) || (__GNUC_MINOR__ < 1)) && (defined(__i386__) || defined(__x86_64__))
   VolatileCounter temp = -1;
   __asm__ __volatile__("lock; xaddl %0,%1" : "+r" (temp), "+m" (*v) : : "memory");
   return temp - 1;
#elif HAVE_ATOMIC_BUILTINS
   return __atomic_sub_fetch(v, 1, __ATOMIC_SEQ_CST);
#else
   return __sync_sub_and_fetch(v, 1);
#endif
}

/**
 * Atomically add 32-bit values
 */
static inline int32_t InterlockedAdd(VolatileCounter *v, int32_t a)
{
#if defined(__GNUC__) && ((__GNUC__ < 4) || (__GNUC_MINOR__ < 1)) && (defined(__i386__) || defined(__x86_64__))
   VolatileCounter temp = a;
   __asm__ __volatile__("lock; xaddl %0,%1" : "+r" (temp), "+m" (*v) : : "memory");
   return temp + a;
#elif HAVE_ATOMIC_BUILTINS
   return __atomic_add_fetch(v, a, __ATOMIC_SEQ_CST);
#else
   return __sync_add_and_fetch(v, a);
#endif
}

/**
 * Atomically exchange 32-bit values
 */
static inline int32_t InterlockedCompareExchange(VolatileCounter *target, int32_t exchange, int32_t comparand)
{
#if defined(__GNUC__) && ((__GNUC__ < 4) || (__GNUC_MINOR__ < 1)) && (defined(__i386__) || defined(__x86_64__))
   __asm__ __volatile__("xchgl %2, %1" : "=a" (comparand), "+m" (*target) : "0" (exchange));
#elif HAVE_ATOMIC_BUILTINS
   int32_t expected = comparand;
   return __atomic_compare_exchange_n(target, &expected, exchange, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST) ? comparand : expected;
#else
   return __sync_val_compare_and_swap(target, comparand, exchange);
#endif
}

/**
 * Atomically increment 64-bit value by 1
 */
static inline int64_t InterlockedIncrement64(VolatileCounter64 *v)
{
#if defined(__GNUC__) && ((__GNUC__ < 4) || (__GNUC_MINOR__ < 1)) && (defined(__i386__) || defined(__x86_64__))
   VolatileCounter64 temp = 1;
   __asm__ __volatile__("lock; xaddq %0,%1" : "+r" (temp), "+m" (*v) : : "memory");
   return temp + 1;
#elif HAVE_ATOMIC_BUILTINS && !defined(__minix)
   return __atomic_add_fetch(v, 1, __ATOMIC_SEQ_CST);
#else
   return __sync_add_and_fetch(v, 1);
#endif
}

/**
 * Atomically decrement 64-bit value by 1
 */
static inline int64_t InterlockedDecrement64(VolatileCounter64 *v)
{
#if defined(__GNUC__) && ((__GNUC__ < 4) || (__GNUC_MINOR__ < 1)) && (defined(__i386__) || defined(__x86_64__))
   VolatileCounter64 temp = -1;
   __asm__ __volatile__("lock; xaddq %0,%1" : "+r" (temp), "+m" (*v) : : "memory");
   return temp - 1;
#elif HAVE_ATOMIC_BUILTINS
   return __atomic_sub_fetch(v, 1, __ATOMIC_SEQ_CST);
#else
   return __sync_sub_and_fetch(v, 1);
#endif
}

/**
 * Atomically add 64-bit values
 */
static inline int64_t InterlockedAdd64(VolatileCounter64 *v, int64_t a)
{
#if defined(__GNUC__) && ((__GNUC__ < 4) || (__GNUC_MINOR__ < 1)) && (defined(__i386__) || defined(__x86_64__))
   VolatileCounter64 temp = a;
   __asm__ __volatile__("lock; xaddq %0,%1" : "+r" (temp), "+m" (*v) : : "memory");
   return temp + a;
#elif HAVE_ATOMIC_BUILTINS && !defined(__minix)
   return __atomic_add_fetch(v, a, __ATOMIC_SEQ_CST);
#else
   return __sync_add_and_fetch(v, a);
#endif
}

/**
 * Atomically set pointer
 */
static inline void *InterlockedExchangePointer(void* volatile *target, void *value)
{
#if defined(__GNUC__) && ((__GNUC__ < 4) || (__GNUC_MINOR__ < 1)) && (defined(__i386__) || defined(__x86_64__))
   void *oldval;
#ifdef __64BIT__
   __asm__ __volatile__("xchgq %q2, %1" : "=a" (oldval), "+m" (*target) : "0" (value));
#else
   __asm__ __volatile__("xchgl %2, %1" : "=a" (oldval), "+m" (*target) : "0" (value));
#endif
   return oldval;
#elif HAVE_ATOMIC_BUILTINS
   return __atomic_exchange_n(target, value, __ATOMIC_SEQ_CST);
#else
   __sync_synchronize();
   return __sync_lock_test_and_set(target, value);
#endif
}

/**
 * Atomic bitwise OR
 */
static inline void InterlockedOr(VolatileCounter *target, uint32_t bits)
{
#if defined(__GNUC__) && ((__GNUC__ < 4) || (__GNUC_MINOR__ < 1)) && (defined(__i386__) || defined(__x86_64__))
   int32_t c;
   do
   {
      c = *target;
   } while(InterlockedCompareExchange(target, (int32_t)((uint32_t)c | bits), c) != c)
#elif HAVE_ATOMIC_BUILTINS
   __atomic_or_fetch(target, bits, __ATOMIC_SEQ_CST);
#else
   __sync_or_and_fetch(target, bits);
#endif
}

/**
 * Atomic bitwise AND
 */
static inline void InterlockedAnd(VolatileCounter *target, uint32_t bits)
{
#if defined(__GNUC__) && ((__GNUC__ < 4) || (__GNUC_MINOR__ < 1)) && (defined(__i386__) || defined(__x86_64__))
   int32_t c;
   do
   {
      c = *target;
   } while(InterlockedCompareExchange(target, (int32_t)((uint32_t)c & bits), c) != c)
#elif HAVE_ATOMIC_BUILTINS
   __atomic_and_fetch(target, bits, __ATOMIC_SEQ_CST);
#else
   __sync_and_and_fetch(target, bits);
#endif
}

/**
 * Atomic bitwise OR 64 bit
 */
static inline void InterlockedOr64(VolatileCounter64 *target, uint64_t bits)
{
#if defined(__GNUC__) && ((__GNUC__ < 4) || (__GNUC_MINOR__ < 1)) && (defined(__i386__) || defined(__x86_64__))
   int64_t c;
   do
   {
      c = *target;
   } while(InterlockedCompareExchange64(target, (int64_t)((uint64_t)c | bits), c) != c)
#elif HAVE_ATOMIC_BUILTINS
   __atomic_or_fetch(target, bits, __ATOMIC_SEQ_CST);
#else
   __sync_or_and_fetch(target, bits);
#endif
}

/**
 * Atomic bitwise AND 64 bit
 */
static inline void InterlockedAnd64(VolatileCounter64 *target, uint64_t bits)
{
#if defined(__GNUC__) && ((__GNUC__ < 4) || (__GNUC_MINOR__ < 1)) && (defined(__i386__) || defined(__x86_64__))
   int64_t c;
   do
   {
      c = *target;
   } while(InterlockedCompareExchange(target, (int64_t)((uint64_t)c & bits), c) != c)
#elif HAVE_ATOMIC_BUILTINS
   __atomic_and_fetch(target, bits, __ATOMIC_SEQ_CST);
#else
   __sync_and_and_fetch(target, bits);
#endif
}

#endif   /* __sun */

#endif   /* _WIN32 */

/**
 * Atomically set pointer - helper template
 */
template<typename T> static inline T *InterlockedExchangeObjectPointer(T* volatile *target, T *value)
{
   return static_cast<T*>(InterlockedExchangePointer(reinterpret_cast<void* volatile *>(target), value));
}

#endif   /* __cplusplus */

#endif
