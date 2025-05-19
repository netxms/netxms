/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2023 Victor Kirhenshtein
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
** File: nms_threads.h
**
**/

#ifndef _nms_threads_h_
#define _nms_threads_h_

#ifdef __cplusplus

#include <functional>

#define NMS_THREADS_H_INCLUDED

extern LIBNETXMS_EXPORTABLE_VAR(int g_defaultThreadStackSize);

#if defined(_WIN32)

#include "winmutex.h"
#include "winrwlock.h"

//
// Related datatypes and constants
//

#define INVALID_MUTEX_HANDLE        (nullptr)
#define INVALID_CONDITION_HANDLE    (nullptr)
#define INVALID_THREAD_HANDLE       (nullptr)

typedef unsigned int THREAD_RESULT;
typedef unsigned int THREAD_ID;

#define THREAD_OK       0
#define THREAD_CALL     __stdcall

extern "C" typedef THREAD_RESULT (THREAD_CALL *ThreadFunction)(void *);

struct netxms_thread_t
{
	HANDLE handle;
	THREAD_ID id;
};
typedef struct netxms_thread_t *THREAD;

//
// Inline functions
//

static inline void InitThreadLibrary()
{
}

static inline void ThreadSleep(int seconds)
{
   Sleep(static_cast<uint32_t>(seconds) * 1000);   // Convert to milliseconds
}

static inline void ThreadSleepMs(uint32_t milliseconds)
{
   Sleep(milliseconds);
}

static inline bool ThreadCreate(ThreadFunction startAddress, int stackSize, void *args)
{
   THREAD_ID dwThreadId;
   HANDLE hThread = (HANDLE)_beginthreadex(nullptr, (stackSize != 0) ? stackSize : g_defaultThreadStackSize, startAddress, args, 0, &dwThreadId);
   if (hThread != nullptr)
      CloseHandle(hThread);
   return (hThread != nullptr);
}

static inline THREAD ThreadCreateEx(ThreadFunction startAddress, int stackSize, void *args)
{
   THREAD thread = MemAllocStruct<netxms_thread_t>();
   thread->handle = (HANDLE)_beginthreadex(nullptr, (stackSize != 0) ? stackSize : g_defaultThreadStackSize, startAddress, args, 0, &thread->id);
	if ((thread->handle == (HANDLE)-1) || (thread->handle == nullptr))
	{
		MemFree(thread);
		thread = INVALID_THREAD_HANDLE;
	}
	return thread;
}

#pragma pack(push,8)
typedef struct tagTHREADNAME_INFO
{
   DWORD dwType; // Must be 0x1000.
   LPCSTR szName; // Pointer to name (in user addr space).
   DWORD dwThreadID; // Thread ID (-1=caller thread).
   DWORD dwFlags; // Reserved for future use, must be zero.
 } THREADNAME_INFO;
#pragma pack(pop)

/**
 * Set thread name. Thread can be set to INVALID_THREAD_HANDLE to change name of current thread.
 */
void LIBNETXMS_EXPORTABLE ThreadSetName(THREAD thread, const char *name);

/**
 * Set name for current thread
 */
static inline void ThreadSetName(const char *name)
{
   ThreadSetName(INVALID_THREAD_HANDLE, name);
}

/**
 * Exit thread
 */
static inline void ThreadExit()
{
   _endthreadex(0);
}

static inline bool ThreadJoin(THREAD thread)
{
   if (thread == INVALID_THREAD_HANDLE)
      return false;

   bool success = (WaitForSingleObject(thread->handle, INFINITE) == WAIT_OBJECT_0);
   CloseHandle(thread->handle);
   MemFree(thread);
   return success;
}

static inline void ThreadDetach(THREAD thread)
{
   if (thread != INVALID_THREAD_HANDLE)
   {
      CloseHandle(thread->handle);
      MemFree(thread);
   }
}

static inline THREAD_ID ThreadId(THREAD thread)
{
   return (thread != INVALID_THREAD_HANDLE) ? thread->id : 0;
}

#elif defined(_USE_GNU_PTH)

/****************************************************************************/
/* GNU Pth                                                                  */
/****************************************************************************/

//
// Related datatypes and constants
//

typedef pth_t THREAD;

#define INVALID_THREAD_HANDLE       (NULL)

#ifndef INFINITE
#define INFINITE 0xFFFFFFFF
#endif

typedef void *THREAD_RESULT;

#define THREAD_OK       ((void *)0)
#define THREAD_CALL

extern "C" typedef THREAD_RESULT (THREAD_CALL *ThreadFunction)(void *);


//
// Inline functions
//

static inline void InitThreadLibrary()
{
   if (!pth_init())
   {
      perror("pth_init() failed");
      exit(200);
   }
}

static inline void ThreadSleep(int nSeconds)
{
   pth_sleep(nSeconds);
}

static inline void ThreadSleepMs(uint32_t milliseconds)
{
	pth_usleep(milliseconds * 1000);
}

static inline bool ThreadCreate(ThreadFunction start_address, int stack_size, void *args)
{
	THREAD id;

	if ((id = pth_spawn(PTH_ATTR_DEFAULT, start_address, args)) != NULL) 
   {
      pth_attr_set(pth_attr_of(id), PTH_ATTR_JOINABLE, 0);
		return true;
	} 
   else 
   {
		return false;
	}
}

static inline THREAD ThreadCreateEx(ThreadFunction start_address, int stack_size, void *args)
{
	THREAD id;

	if ((id = pth_spawn(PTH_ATTR_DEFAULT, start_address, args)) != NULL) 
   {
		return id;
	} 
   else 
   {
		return INVALID_THREAD_HANDLE;
	}
}

/**
 * Set thread name
 */
static inline void ThreadSetName(THREAD thread, const char *name)
{
}

/**
 * Set name for current thread
 */
static inline void ThreadSetName(const char *name)
{
   ThreadSetName(INVALID_THREAD_HANDLE, name);
}

/**
 * Exit thread
 */
static inline void ThreadExit(void)
{
   pth_exit(nullptr);
}

static inline bool ThreadJoin(THREAD hThread)
{
   if (hThread == INVALID_THREAD_HANDLE)
      return false;
   return pth_join(hThread, NULL) != -1;
}

static inline void ThreadDetach(THREAD hThread)
{
   if (hThread == INVALID_THREAD_HANDLE)
      return;
   pth_attr_t a = pth_attr_of(hThread);
   pth_attr_set(a, PTH_ATTR_JOINABLE, FALSE);
}

static inline uint32_t GetCurrentProcessId()
{
   return getpid();
}

static inline uint32_t GetCurrentThreadId()
{
   return (uint32_t)pth_self();
}

#else    /* not _WIN32 && not _USE_GNU_PTH */

/****************************************************************************/
/* pthreads                                                                 */
/****************************************************************************/

#include <pthread.h>
#include <errno.h>
#include <sys/time.h>

#if HAVE_SYS_SYSCALL_H
#include <sys/syscall.h>
#endif

#if HAVE_PTHREAD_NP_H && !defined(_IPSO)
#include <pthread_np.h>
#endif

#if (HAVE_PTHREAD_MUTEXATTR_SETTYPE || HAVE___PTHREAD_MUTEXATTR_SETTYPE || HAVE_PTHREAD_MUTEXATTR_SETKIND_NP) && \
    HAVE_DECL_PTHREAD_MUTEXATTR_SETTYPE && \
    (HAVE_DECL_PTHREAD_MUTEX_RECURSIVE || \
	  HAVE_DECL_PTHREAD_MUTEX_RECURSIVE_NP || \
          HAVE_DECL_MUTEX_TYPE_COUNTING_FAST)

#define HAVE_RECURSIVE_MUTEXES 1

#if HAVE_DECL_PTHREAD_MUTEX_RECURSIVE
#define MUTEX_RECURSIVE_FLAG PTHREAD_MUTEX_RECURSIVE
#elif HAVE_DECL_PTHREAD_MUTEX_RECURSIVE_NP
#define MUTEX_RECURSIVE_FLAG PTHREAD_MUTEX_RECURSIVE_NP
#elif HAVE_DECL_MUTEX_TYPE_COUNTING_FAST
#define MUTEX_RECURSIVE_FLAG MUTEX_TYPE_COUNTING_FAST
#else
#error Constant used to declare recursive mutex is not known 
#endif

#if HAVE_PTHREAD_MUTEXATTR_SETTYPE || HAVE_DECL_PTHREAD_MUTEXATTR_SETTYPE
#define MUTEXATTR_SETTYPE pthread_mutexattr_settype
#elif HAVE___PTHREAD_MUTEXATTR_SETTYPE
#define MUTEXATTR_SETTYPE __pthread_mutexattr_settype
#else
#define MUTEXATTR_SETTYPE pthread_mutexattr_setkind_np
#endif

#endif

#if !HAVE_PTHREAD_RWLOCK
#include "rwlock.h"
#endif

//
// Related datatypes and constants
//

typedef pthread_t THREAD;

#define INVALID_THREAD_HANDLE       0

#ifndef INFINITE
#define INFINITE 0xFFFFFFFF
#endif

typedef void *THREAD_RESULT;

#define THREAD_OK       ((void *)0)
#define THREAD_CALL

extern "C" typedef THREAD_RESULT (THREAD_CALL *ThreadFunction)(void *);


//
// Inline functions
//

static inline void InitThreadLibrary()
{
}

static inline void ThreadSleep(int nSeconds)
{
	struct timeval tv;
	tv.tv_sec = nSeconds;
	tv.tv_usec = 0;
	select(1, NULL, NULL, NULL, &tv);
}

static inline void ThreadSleepMs(uint32_t milliseconds)
{
   int rc;
   do
   {
#if HAVE_NANOSLEEP && HAVE_DECL_NANOSLEEP
      struct timespec interval, remainder;
      interval.tv_sec = milliseconds / 1000;
      interval.tv_nsec = (milliseconds % 1000) * 1000000; // milli -> nano
      rc = nanosleep(&interval, &remainder);
#else
      rc = usleep(milliseconds * 1000);   // Convert to microseconds
#endif
   } while((rc != 0) && (errno == EINTR));
}

static inline THREAD ThreadCreateEx(ThreadFunction startAddress, int stackSize, void *args)
{
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, (stackSize != 0) ? stackSize : g_defaultThreadStackSize);

   THREAD id;
   if (pthread_create(&id, &attr, startAddress, args) != 0)
   {
		id = INVALID_THREAD_HANDLE;
   } 

	pthread_attr_destroy(&attr);

	return id;
}

static inline bool ThreadCreate(ThreadFunction startAddress, int stackSize, void *args)
{
	THREAD id = ThreadCreateEx(startAddress, stackSize, args);
	if (id != INVALID_THREAD_HANDLE)
	{
		pthread_detach(id);
		return true;
	}
	return false;
}

/**
 * Set thread name
 */
static inline void ThreadSetName(THREAD thread, const char *name)
{
#if HAVE_PTHREAD_SETNAME_NP
#if PTHREAD_SETNAME_NP_2ARGS
   pthread_setname_np((thread != INVALID_THREAD_HANDLE) ? thread : pthread_self(), name);
#else
   if ((thread == INVALID_THREAD_HANDLE) || (thread == pthread_self()))
      pthread_setname_np(name);
#endif
#endif
}

/**
 * Set name for current thread
 */
static inline void ThreadSetName(const char *name)
{
   ThreadSetName(INVALID_THREAD_HANDLE, name);
}

/**
 * Exit thread
 */
static inline void ThreadExit()
{
   pthread_exit(nullptr);
}

static inline bool ThreadJoin(THREAD hThread)
{
   if (hThread == INVALID_THREAD_HANDLE)
      return false;
   return pthread_join(hThread, nullptr) == 0;
}

static inline void ThreadDetach(THREAD hThread)
{
   if (hThread != INVALID_THREAD_HANDLE)
      pthread_detach(hThread);
}

/**
 * Get ID of current process
 */
static inline uint32_t GetCurrentProcessId()
{
   return getpid();
}

/**
 *  Get ID of current thread
 */
static inline uint32_t GetCurrentThreadId()
{
#if HAVE_GETTID_SYSCALL
   return (uint32_t)syscall(SYS_gettid);
#elif HAVE_PTHREAD_GETTHREADID_NP
   return (uint32_t)pthread_getthreadid_np();
#elif HAVE_PTHREAD_THREADID_NP
   uint64_t id;
   if (pthread_threadid_np(nullptr, &id) != 0)
      return 0;
   return (uint32_t)id;
#elif HAVE_PTHREAD_GETLWPID_NP
   lwpid_t id;
   if (pthread_getlwpid_np(pthread_self(), &id) != 0)
      return 0;
   return (uint32_t)id;
#elif defined(_AIX) || defined(__sun)
   return (uint32_t)pthread_self();
#elif defined(__NetBSD__)
   return _lwp_self();
#elif defined(__OpenBSD__)
   return getthrid();
#else
#error GetCurrentThreadId not implemented for this platform
#endif
}

#endif   /* _WIN32 */

/**
 * Template wrapper for ThreadCreate/ThreadCreateEx thread functions (no arguments)
 */
static inline THREAD_RESULT THREAD_CALL ThreadCreate_Wrapper_0(void *function)
{
   ((void (*)())function)();
   return THREAD_OK;
}

/**
 * Template wrapper for ThreadCreate (no arguments)
 */
static inline bool ThreadCreate(void (*function)(), int stackSize = 0)
{
   return ThreadCreate(ThreadCreate_Wrapper_0, stackSize, (void*)function);
}

/**
 * Template wrapper for ThreadCreateEx (no arguments)
 */
static inline THREAD ThreadCreateEx(void (*function)(), int stackSize = 0)
{
   return ThreadCreateEx(ThreadCreate_Wrapper_0, stackSize, (void*)function);
}

/**
 * Template wrapper data for ThreadCreate/ThreadCreateEx thread functions (1 argument)
 */
template<typename R> class ThreadCreate_WrapperData_1
{
public:
   void (*function)(R);
   R arg;

   ThreadCreate_WrapperData_1(void (*_function)(R), R _arg) : arg(_arg)
   {
      function = _function;
   }
};

/**
 * Template wrapper for ThreadCreate/ThreadCreateEx thread functions (1 argument)
 */
template<typename R> THREAD_RESULT THREAD_CALL ThreadCreate_Wrapper_1(void *context)
{
   auto wd = static_cast<ThreadCreate_WrapperData_1<R>*>(context);
   wd->function(wd->arg);
   delete wd;
   return THREAD_OK;
}

/**
 * Template wrapper for ThreadCreate (1 argument)
 */
template<typename R> bool ThreadCreate(void (*function)(R), R arg, int stackSize = 0)
{
   auto wd = new ThreadCreate_WrapperData_1<R>(function, arg);
   bool success = ThreadCreate(ThreadCreate_Wrapper_1<R>, stackSize, wd);
   if (!success)
      delete wd;
   return success;
}

/**
 * Template wrapper for ThreadCreateEx (1 argument)
 */
template<typename R> THREAD ThreadCreateEx(void (*function)(R), R arg, int stackSize = 0)
{
   auto wd = new ThreadCreate_WrapperData_1<R>(function, arg);
   THREAD thread = ThreadCreateEx(ThreadCreate_Wrapper_1<R>, stackSize, wd);
   if (thread == INVALID_THREAD_HANDLE)
      delete wd;
   return thread;
}

/**
 * Template wrapper data for ThreadCreate/ThreadCreateEx thread functions (2 arguments)
 */
template<typename R1, typename R2> class ThreadCreate_WrapperData_2
{
public:
   void (*function)(R1, R2);
   R1 arg1;
   R2 arg2;

   ThreadCreate_WrapperData_2(void (*_function)(R1, R2), R1 _arg1, R2 _arg2) : arg1(_arg1), arg2(_arg2)
   {
      function = _function;
   }
};

/**
 * Template wrapper for ThreadCreate/ThreadCreateEx thread functions (2 arguments)
 */
template<typename R1, typename R2> THREAD_RESULT THREAD_CALL ThreadCreate_Wrapper_2(void *context)
{
   auto wd = static_cast<ThreadCreate_WrapperData_2<R1, R2>*>(context);
   wd->function(wd->arg1, wd->arg2);
   delete wd;
   return THREAD_OK;
}

/**
 * Template wrapper for ThreadCreate (2 arguments)
 */
template<typename R1, typename R2> bool ThreadCreate(void (*function)(R1, R2), R1 arg1, R2 arg2, int stackSize = 0)
{
   auto wd = new ThreadCreate_WrapperData_2<R1, R2>(function, arg1, arg2);
   bool success = ThreadCreate(ThreadCreate_Wrapper_2<R1, R2>, stackSize, wd);
   if (!success)
      delete wd;
   return success;
}

/**
 * Template wrapper for ThreadCreateEx (2 arguments)
 */
template<typename R1, typename R2> THREAD ThreadCreateEx(void (*function)(R1, R2), R1 arg1, R2 arg2, int stackSize = 0)
{
   auto wd = new ThreadCreate_WrapperData_2<R1, R2>(function, arg1, arg2);
   THREAD thread = ThreadCreateEx(ThreadCreate_Wrapper_2<R1, R2>, stackSize, wd);
   if (thread == INVALID_THREAD_HANDLE)
      delete wd;
   return thread;
}

/**
 * Template wrapper data for ThreadCreate/ThreadCreateEx thread functions (3 arguments)
 */
template<typename R1, typename R2, typename R3> class ThreadCreate_WrapperData_3
{
public:
   void (*function)(R1, R2, R3);
   R1 arg1;
   R2 arg2;
   R3 arg3;

   ThreadCreate_WrapperData_3(void (*_function)(R1, R2, R3), R1 _arg1, R2 _arg2, R3 _arg3) : arg1(_arg1), arg2(_arg2), arg3(_arg3)
   {
      function = _function;
   }
};

/**
 * Template wrapper for ThreadCreate/ThreadCreateEx thread functions (3 arguments)
 */
template<typename R1, typename R2, typename R3> THREAD_RESULT THREAD_CALL ThreadCreate_Wrapper_3(void *context)
{
   auto wd = static_cast<ThreadCreate_WrapperData_3<R1, R2, R3>*>(context);
   wd->function(wd->arg1, wd->arg2, wd->arg3);
   delete wd;
   return THREAD_OK;
}

/**
 * Template wrapper for ThreadCreate (3 arguments)
 */
template<typename R1, typename R2, typename R3> bool ThreadCreate(void (*function)(R1, R2, R3), R1 arg1, R2 arg2, R3 arg3, int stackSize = 0)
{
   auto wd = new ThreadCreate_WrapperData_3<R1, R2, R3>(function, arg1, arg2, arg3);
   bool success = ThreadCreate(ThreadCreate_Wrapper_3<R1, R2, R3>, stackSize, wd);
   if (!success)
      delete wd;
   return success;
}

/**
 * Template wrapper for ThreadCreateEx (3 arguments)
 */
template<typename R1, typename R2, typename R3> THREAD ThreadCreateEx(void (*function)(R1, R2, R3), R1 arg1, R2 arg2, R3 arg3, int stackSize = 0)
{
   auto wd = new ThreadCreate_WrapperData_3<R1, R2, R3>(function, arg1, arg2, arg3);
   THREAD thread = ThreadCreateEx(ThreadCreate_Wrapper_3<R1, R2, R3>, stackSize, wd);
   if (thread == INVALID_THREAD_HANDLE)
      delete wd;
   return thread;
}

/**
 * Wrapper data for ThreadCreate/ThreadCreateEx on object method using pointer to object (no arguments)
 */
template <typename T> class ThreadCreate_ObjectPtr_WrapperData_0
{
public:
   T *m_object;
   void (T::*m_func)();

   ThreadCreate_ObjectPtr_WrapperData_0(T *object, void (T::*func)())
   {
      m_object = object;
      m_func = func;
   }
};

/**
 * Wrapper for ThreadCreate/ThreadCreateEx on object method using pointer to object (no arguments)
 */
template <typename T> THREAD_RESULT THREAD_CALL ThreadCreate_ObjectPtr_Wrapper_0(void *arg)
{
   auto wd = static_cast<ThreadCreate_ObjectPtr_WrapperData_0<T>*>(arg);
   ((*wd->m_object).*(wd->m_func))();
   delete wd;
   return THREAD_OK;
}

/**
 * Template wrapper for ThreadCreate on object method using pointer to object (no arguments)
 */
template<typename T, typename B> bool ThreadCreate(T *object, void (B::*method)(), int stackSize = 0)
{
   auto wd = new ThreadCreate_ObjectPtr_WrapperData_0<B>(object, method);
   bool success = ThreadCreate(ThreadCreate_ObjectPtr_Wrapper_0<B>, stackSize, wd);
   if (!success)
      delete wd;
   return success;
}

/**
 * Template wrapper for ThreadCreateEx on object method using pointer to object (no arguments)
 */
template<typename T, typename B> THREAD ThreadCreateEx(T *object, void (B::*method)(), int stackSize = 0)
{
   auto wd = new ThreadCreate_ObjectPtr_WrapperData_0<B>(object, method);
   THREAD thread = ThreadCreateEx(ThreadCreate_ObjectPtr_Wrapper_0<B>, stackSize, wd);
   if (thread == INVALID_THREAD_HANDLE)
      delete wd;
   return thread;
}

/**
 * Wrapper data for ThreadCreate/ThreadCreateEx on object method using pointer to object (no arguments)
 */
template <typename T, typename R> class ThreadCreate_ObjectPtr_WrapperData_1
{
public:
   T *m_object;
   void (T::*m_func)(R);
   R m_arg;

   ThreadCreate_ObjectPtr_WrapperData_1(T *object, void (T::*func)(R), const R& arg) : m_arg(arg)
   {
      m_object = object;
      m_func = func;
   }
};

/**
 * Wrapper for ThreadCreate/ThreadCreateEx on object method using pointer to object (one argument)
 */
template <typename T, typename R> THREAD_RESULT THREAD_CALL ThreadCreate_ObjectPtr_Wrapper_1(void *arg)
{
   auto wd = static_cast<ThreadCreate_ObjectPtr_WrapperData_1<T, R>*>(arg);
   ((*wd->m_object).*(wd->m_func))(wd->m_arg);
   delete wd;
   return THREAD_OK;
}

/**
 * Template wrapper for ThreadCreate on object method using pointer to object (one argument)
 */
template<typename T, typename B, typename R> bool ThreadCreate(T *object, void (B::*method)(R), const R& arg, int stackSize = 0)
{
   auto wd = new ThreadCreate_ObjectPtr_WrapperData_1<B, R>(object, method, arg);
   bool success = ThreadCreate(ThreadCreate_ObjectPtr_Wrapper_1<B, R>, stackSize, wd);
   if (!success)
      delete wd;
   return success;
}

/**
 * Template wrapper for ThreadCreateEx on object method using pointer to object (one argument)
 */
template<typename T, typename B, typename R> THREAD ThreadCreateEx(T *object, void (B::*method)(R), const R& arg, int stackSize = 0)
{
   auto wd = new ThreadCreate_ObjectPtr_WrapperData_1<B, R>(object, method, arg);
   THREAD thread = ThreadCreateEx(ThreadCreate_ObjectPtr_Wrapper_1<B, R>, stackSize, wd);
   if (thread == INVALID_THREAD_HANDLE)
      delete wd;
   return thread;
}

/**
 * Wrapper data for ThreadCreate/ThreadCreateEx on object method using smart pointer to object (no arguments)
 */
template <typename T> class ThreadCreate_SharedPtr_WrapperData_0
{
public:
   shared_ptr<T> m_object;
   void (T::*m_func)();

   ThreadCreate_SharedPtr_WrapperData_0(const shared_ptr<T>& object, void (T::*func)()) : m_object(object)
   {
      m_func = func;
   }
};

/**
 * Wrapper for ThreadCreate/ThreadCreateEx on object method using smart pointer to object (no arguments)
 */
template <typename T> THREAD_RESULT THREAD_CALL ThreadCreate_SharedPtr_Wrapper_0(void *arg)
{
   auto wd = static_cast<ThreadCreate_SharedPtr_WrapperData_0<T>*>(arg);
   ((*wd->m_object.get()).*(wd->m_func))();
   delete wd;
   return THREAD_OK;
}

/**
 * Template wrapper for ThreadCreate on object method using smart pointer to object (no arguments)
 */
template<typename T, typename B> bool ThreadCreate(const shared_ptr<T>& object, void (B::*method)(), int stackSize = 0)
{
   auto wd = new ThreadCreate_SharedPtr_WrapperData_0<B>(object, method);
   bool success = ThreadCreate(ThreadCreate_SharedPtr_Wrapper_0<B>, stackSize, wd);
   if (!success)
      delete wd;
   return success;
}

/**
 * Template wrapper for ThreadCreateEx on object method using smart pointer to object (no arguments)
 */
template<typename T, typename B> THREAD ThreadCreateEx(const shared_ptr<T>& object, void (B::*method)(), int stackSize = 0)
{
   auto wd = new ThreadCreate_SharedPtr_WrapperData_0<B>(object, method);
   THREAD thread = ThreadCreateEx(ThreadCreate_SharedPtr_Wrapper_0<B>, stackSize, wd);
   if (thread == INVALID_THREAD_HANDLE)
      delete wd;
   return thread;
}

/**
 * Wrapper for ThreadCreate/ThreadCreateEx with std::function argument
 */
static inline THREAD_RESULT THREAD_CALL __ThreadCreate_Callable_Wrapper(void *arg)
{
   auto f = static_cast<std::function<void ()>*>(arg);
   (*f)();
   delete f;
   return THREAD_OK;
}

/**
 * Wrapper for ThreadCreate to use std::function
 */
static inline bool ThreadCreate(const std::function<void ()>& f, int stackSize = 0)
{
   auto wd = new std::function<void ()>(f);
   bool success = ThreadCreate(__ThreadCreate_Callable_Wrapper, stackSize, wd);
   if (!success)
      delete wd;
   return success;
}

/**
 * Wrapper for ThreadCreateEx to use std::function
 */
static inline THREAD ThreadCreateEx(const std::function<void ()>& f, int stackSize = 0)
{
   auto wd = new std::function<void ()>(f);
   THREAD thread = ThreadCreateEx(__ThreadCreate_Callable_Wrapper, stackSize, wd);
   if (thread == INVALID_THREAD_HANDLE)
      delete wd;
   return thread;
}

/**
 * String list
 */
class StringList;

/**
 * Thread pool
 */
struct ThreadPool;

/**
 * Thread pool information
 */
struct ThreadPoolInfo
{
   const TCHAR *name;          // pool name
   int32_t minThreads;         // min threads
   int32_t maxThreads;         // max threads
   int32_t curThreads;         // current threads
   int32_t activeRequests;     // number of active requests
   int32_t scheduledRequests;  // number of requests scheduled for execution
   int32_t serializedRequests; // number of requests waiting in serialized execution queues
   uint64_t totalRequests;     // Total requests executed (cumulative count)
   uint64_t threadStarts;      // Number of thread starts
   uint64_t threadStops;       // Number of thread stops
   int32_t usage;              // Pool usage in %
   int32_t load;               // Pool current load in % (can be more than 100% if there are more requests then threads available)
   double loadAvg[3];          // Pool load average
   uint32_t waitTimeEMA;       // Task wait time exponential moving average
   uint32_t waitTimeSMA;       // Task wait time simple moving average
   uint32_t waitTimeSD;        // Task wait time standard deviation
   uint32_t queueSizeEMA;      // Task queue size exponential moving average
   uint32_t queueSizeSMA;      // Task queue size simple moving average
   uint32_t queueSizeSD;       // Task queue size standard deviation
};

/**
 * Worker function for thread pool
 */
typedef void (*ThreadPoolWorkerFunction)(void *);

/* Thread pool functions */
ThreadPool LIBNETXMS_EXPORTABLE *ThreadPoolCreate(const TCHAR *name, int minThreads, int maxThreads, int stackSize = 0);
void LIBNETXMS_EXPORTABLE ThreadPoolDestroy(ThreadPool *p);
void LIBNETXMS_EXPORTABLE ThreadPoolExecute(ThreadPool *p, ThreadPoolWorkerFunction f, void *arg);
void LIBNETXMS_EXPORTABLE ThreadPoolExecuteSerialized(ThreadPool *p, const TCHAR *key, ThreadPoolWorkerFunction f, void *arg);
void LIBNETXMS_EXPORTABLE ThreadPoolScheduleAbsolute(ThreadPool *p, time_t runTime, ThreadPoolWorkerFunction f, void *arg);
void LIBNETXMS_EXPORTABLE ThreadPoolScheduleAbsoluteMs(ThreadPool *p, int64_t runTime, ThreadPoolWorkerFunction f, void *arg);
void LIBNETXMS_EXPORTABLE ThreadPoolScheduleRelative(ThreadPool *p, uint32_t delay, ThreadPoolWorkerFunction f, void *arg);
void LIBNETXMS_EXPORTABLE ThreadPoolGetInfo(ThreadPool *p, ThreadPoolInfo *info);
bool LIBNETXMS_EXPORTABLE ThreadPoolGetInfo(const TCHAR *name, ThreadPoolInfo *info);
ThreadPool LIBNETXMS_EXPORTABLE *ThreadPoolGetByName(const TCHAR *name);
int LIBNETXMS_EXPORTABLE ThreadPoolGetSerializedRequestCount(ThreadPool *p, const TCHAR *key);
uint32_t LIBNETXMS_EXPORTABLE ThreadPoolGetSerializedRequestMaxWaitTime(ThreadPool *p, const TCHAR *key);
StringList LIBNETXMS_EXPORTABLE ThreadPoolGetAllPools();
void LIBNETXMS_EXPORTABLE ThreadPoolSetResizeParameters(int responsiveness, uint32_t waitTimeHWM, uint32_t waitTimeLWM);

/**
 * Wrapper for ThreadPoolExecute for function without arguments
 */
static inline void ThreadPoolExecute_NoArg_Wrapper(void *arg)
{
   ((void (*)())arg)();
}

/**
 * Wrapper for ThreadPoolExecute for function without arguments
 */
static inline void ThreadPoolExecute(ThreadPool *p, void (*f)())
{
   ThreadPoolExecute(p, ThreadPoolExecute_NoArg_Wrapper, (void *)f);
}

/**
 * Wrapper for ThreadPoolExecuteSerialized for function without arguments
 */
static inline void ThreadPoolExecuteSerialized(ThreadPool *p, const TCHAR *key, void (*f)())
{
   ThreadPoolExecuteSerialized(p, key, ThreadPoolExecute_NoArg_Wrapper, (void *)f);
}

/**
 * Wrapper for ThreadPoolScheduleAbsolute for function without arguments
 */
static inline void ThreadPoolScheduleAbsolute(ThreadPool *p, time_t runTime, void (*f)())
{
   ThreadPoolScheduleAbsolute(p, runTime, ThreadPoolExecute_NoArg_Wrapper, (void *)f);
}

/**
 * Wrapper for ThreadPoolScheduleAbsoluteMs for function without arguments
 */
static inline void ThreadPoolScheduleAbsoluteMs(ThreadPool *p, int64_t runTime, void (*f)())
{
   ThreadPoolScheduleAbsoluteMs(p, runTime, ThreadPoolExecute_NoArg_Wrapper, (void *)f);
}

/**
 * Wrapper for ThreadPoolScheduleRelative for function without arguments
 */
static inline void ThreadPoolScheduleRelative(ThreadPool *p, uint32_t delay, void (*f)())
{
   ThreadPoolScheduleRelative(p, delay, ThreadPoolExecute_NoArg_Wrapper, (void *)f);
}

/**
 * Wrapper for ThreadPoolExecute to use pointer to given type as argument
 */
template <typename T> static inline void ThreadPoolExecute(ThreadPool *p, void (*f)(T *), T *arg)
{
   ThreadPoolExecute(p, (ThreadPoolWorkerFunction)f, (void *)arg);
}

/**
 * Wrapper data for ThreadPoolExecute with shared_ptr argument
 */
template <typename T> class __ThreadPoolExecute_SharedPtr_WrapperData
{
public:
   shared_ptr<T> m_object;
   void (*m_func)(const shared_ptr<T>&);

   __ThreadPoolExecute_SharedPtr_WrapperData(const shared_ptr<T>& object, void (*func)(const shared_ptr<T>&)) : m_object(object)
   {
      m_func = func;
   }
};

/**
 * Wrapper for ThreadPoolExecute with shared_ptr argument
 */
template <typename T> void __ThreadPoolExecute_SharedPtr_Wrapper(void *arg)
{
   auto wd = static_cast<__ThreadPoolExecute_SharedPtr_WrapperData<T> *>(arg);
   wd->m_func(wd->m_object);
   delete wd;
}

/**
 * Wrapper for ThreadPoolExecute to use smart pointer to given type as argument
 */
template <typename T> static inline void ThreadPoolExecute(ThreadPool *p, void (*f)(const shared_ptr<T>&), const shared_ptr<T>& arg)
{
   ThreadPoolExecute(p, __ThreadPoolExecute_SharedPtr_Wrapper<T>, new __ThreadPoolExecute_SharedPtr_WrapperData<T>(arg, f));
}

/**
 * Wrapper for ThreadPoolExecuteSerialized to use pointer to given type as argument
 */
template <typename T> static inline void ThreadPoolExecuteSerialized(ThreadPool *p, const TCHAR *key, void (*f)(T *), T *arg)
{
   ThreadPoolExecuteSerialized(p, key, (ThreadPoolWorkerFunction)f, (void *)arg);
}

/**
 * Wrapper for ThreadPoolExecuteSerialized to use smart pointer to given type as argument
 */
template <typename T> static inline void ThreadPoolExecuteSerialized(ThreadPool *p, const TCHAR *key, void (*f)(const shared_ptr<T>&), const shared_ptr<T>& arg)
{
   ThreadPoolExecuteSerialized(p, key, __ThreadPoolExecute_SharedPtr_Wrapper<T>, new __ThreadPoolExecute_SharedPtr_WrapperData<T>(arg, f));
}

/**
 * Wrapper for ThreadPoolScheduleAbsolute to use pointer to given type as argument
 */
template <typename T> static inline void ThreadPoolScheduleAbsolute(ThreadPool *p, time_t runTime, void (*f)(T *), T *arg)
{
   ThreadPoolScheduleAbsolute(p, runTime, (ThreadPoolWorkerFunction)f, (void *)arg);
}

/**
 * Wrapper for ThreadPoolScheduleAbsolute to use pointer to given type as argument
 */
template <typename T> static inline void ThreadPoolScheduleAbsolute(ThreadPool *p, time_t runTime, void (*f)(const shared_ptr<T>&), const shared_ptr<T>& arg)
{
   ThreadPoolScheduleAbsolute(p, runTime, __ThreadPoolExecute_SharedPtr_Wrapper<T>, new __ThreadPoolExecute_SharedPtr_WrapperData<T>(arg, f));
}

/**
 * Wrapper for ThreadPoolScheduleAbsoluteMs to use pointer to given type as argument
 */
template <typename T> static inline void ThreadPoolScheduleAbsoluteMs(ThreadPool *p, int64_t runTime, void (*f)(T *), T *arg)
{
   ThreadPoolScheduleAbsoluteMs(p, runTime, (ThreadPoolWorkerFunction)f, (void *)arg);
}

/**
 * Wrapper for ThreadPoolScheduleAbsoluteMs to use pointer to given type as argument
 */
template <typename T> static inline void ThreadPoolScheduleAbsoluteMs(ThreadPool *p, int64_t runTime, void (*f)(const shared_ptr<T>&), const shared_ptr<T>& arg)
{
   ThreadPoolScheduleAbsoluteMs(p, runTime, __ThreadPoolExecute_SharedPtr_Wrapper<T>, new __ThreadPoolExecute_SharedPtr_WrapperData<T>(arg, f));
}

/**
 * Wrapper for ThreadPoolScheduleRelative to use pointer to given type as argument
 */
template <typename T> static inline void ThreadPoolScheduleRelative(ThreadPool *p, uint32_t delay, void (*f)(T *), T *arg)
{
   ThreadPoolScheduleRelative(p, delay, (ThreadPoolWorkerFunction)f, (void *)arg);
}

/**
 * Wrapper for ThreadPoolScheduleRelative to use pointer to given type as argument
 */
template <typename T> static inline void ThreadPoolScheduleRelative(ThreadPool *p, uint32_t delay, void (*f)(const shared_ptr<T>&), const shared_ptr<T>& arg)
{
   ThreadPoolScheduleRelative(p, delay, __ThreadPoolExecute_SharedPtr_Wrapper<T>, new __ThreadPoolExecute_SharedPtr_WrapperData<T>(arg, f));
}

/**
 * Wrapper data for ThreadPoolExecute (class method without arguments)
 */
template <typename T> class __ThreadPoolExecute_WrapperData_0
{
public:
   T *m_object;
   void (T::*m_func)();

   __ThreadPoolExecute_WrapperData_0(T *object, void (T::*func)())
   {
      m_object = object;
      m_func = func;
   }
};

/**
 * Wrapper for ThreadPoolExecute (no arguments)
 */
template <typename T> static void __ThreadPoolExecute_Wrapper_0(void *arg)
{
   auto wd = static_cast<__ThreadPoolExecute_WrapperData_0<T> *>(arg);
   ((*wd->m_object).*(wd->m_func))();
   delete wd;
}

/**
 * Execute task as soon as possible (use class member without arguments)
 */
template <typename T, typename B> static inline void ThreadPoolExecute(ThreadPool *p, T *object, void (B::*f)())
{
   ThreadPoolExecute(p, __ThreadPoolExecute_Wrapper_0<B>, new __ThreadPoolExecute_WrapperData_0<B>(object, f));
}

/**
 * Execute serialized task as soon as possible (use class member without arguments)
 */
template <typename T, typename B> static inline void ThreadPoolExecuteSerialized(ThreadPool *p, const TCHAR *key, T *object, void (B::*f)())
{
   ThreadPoolExecuteSerialized(p, key, __ThreadPoolExecute_Wrapper_0<B>, new __ThreadPoolExecute_WrapperData_0<B>(object, f));
}

/**
 * Execute task with delay (use class member without arguments)
 */
template <typename T, typename B> static inline void ThreadPoolScheduleRelative(ThreadPool *p, uint32_t delay, T *object, void (B::*f)())
{
   ThreadPoolScheduleRelative(p, delay, __ThreadPoolExecute_Wrapper_0<B>, new __ThreadPoolExecute_WrapperData_0<B>(object, f));
}

/**
 * Wrapper data for ThreadPoolExecute (no arguments) using smart pointer to object
 */
template <typename T> class __ThreadPoolExecute_SharedPtr_WrapperData_0
{
public:
   shared_ptr<T> m_object;
   void (T::*m_func)();

   __ThreadPoolExecute_SharedPtr_WrapperData_0(const shared_ptr<T>& object, void (T::*func)()) : m_object(object)
   {
      m_func = func;
   }
};

/**
 * Wrapper for ThreadPoolExecute (no arguments) using smart pointer to object
 */
template <typename T> static void __ThreadPoolExecute_SharedPtr_Wrapper_0(void *arg)
{
   auto wd = static_cast<__ThreadPoolExecute_SharedPtr_WrapperData_0<T> *>(arg);
   ((*wd->m_object.get()).*(wd->m_func))();
   delete wd;
}

/**
 * Execute task as soon as possible (use class member without arguments) using smart pointer to object
 */
template <typename T, typename B> static inline void ThreadPoolExecute(ThreadPool *p, const shared_ptr<T>& object, void (B::*f)())
{
   ThreadPoolExecute(p, __ThreadPoolExecute_SharedPtr_Wrapper_0<B>, new __ThreadPoolExecute_SharedPtr_WrapperData_0<B>(object, f));
}

/**
 * Execute serialized task as soon as possible (use class member without arguments) using smart pointer to object
 */
template <typename T, typename B> static inline void ThreadPoolExecuteSerialized(ThreadPool *p, const TCHAR *key, const shared_ptr<T>& object, void (B::*f)())
{
   ThreadPoolExecuteSerialized(p, key, __ThreadPoolExecute_SharedPtr_Wrapper_0<B>, new __ThreadPoolExecute_SharedPtr_WrapperData_0<B>(object, f));
}

/**
 * Execute task with delay (use class member without arguments) using smart pointer to object
 */
template <typename T, typename B> static inline void ThreadPoolScheduleRelative(ThreadPool *p, uint32_t delay, const shared_ptr<T>& object, void (B::*f)())
{
   ThreadPoolScheduleRelative(p, delay, __ThreadPoolExecute_SharedPtr_Wrapper_0<B>, new __ThreadPoolExecute_SharedPtr_WrapperData_0<B>(object, f));
}

/**
 * Wrapper data for ThreadPoolExecute (one argument)
 */
template <typename T, typename R> class __ThreadPoolExecute_WrapperData_1
{
public:
   T *m_object;
   void (T::*m_func)(R);
   R m_arg;

   __ThreadPoolExecute_WrapperData_1(T *object, void (T::*func)(R), R arg) : m_arg(arg)
   {
      m_object = object;
      m_func = func;
   }
};

/**
 * Wrapper for ThreadPoolExecute (one argument)
 */
template <typename T, typename R> static void __ThreadPoolExecute_Wrapper_1(void *arg)
{
   auto wd = static_cast<__ThreadPoolExecute_WrapperData_1<T, R> *>(arg);
   ((*wd->m_object).*(wd->m_func))(wd->m_arg);
   delete wd;
}

/**
 * Execute task as soon as possible (use class member with one argument)
 */
template <typename T, typename B, typename R> static inline void ThreadPoolExecute(ThreadPool *p, T *object, void (B::*f)(R), R arg)
{
   ThreadPoolExecute(p, __ThreadPoolExecute_Wrapper_1<B, R>, new __ThreadPoolExecute_WrapperData_1<B, R>(object, f, arg));
}

/**
 * Execute serialized task as soon as possible (use class member with one argument)
 */
template <typename T, typename B, typename R> static inline void ThreadPoolExecuteSerialized(ThreadPool *p, const TCHAR *key, T *object, void (B::*f)(R), R arg)
{
   ThreadPoolExecuteSerialized(p, key, __ThreadPoolExecute_Wrapper_1<B, R>, new __ThreadPoolExecute_WrapperData_1<B, R>(object, f, arg));
}

/**
 * Execute task with delay (use class member with one argument)
 */
template <typename T, typename B, typename R> static inline void ThreadPoolScheduleRelative(ThreadPool *p, uint32_t delay, T *object, void (B::*f)(R), R arg)
{
   ThreadPoolScheduleRelative(p, delay, __ThreadPoolExecute_Wrapper_1<B, R>, new __ThreadPoolExecute_WrapperData_1<B, R>(object, f, arg));
}

/**
 * Wrapper data for ThreadPoolExecute (one argument) using smart pointer to object
 */
template <typename T, typename R> class __ThreadPoolExecute_SharedPtr_WrapperData_1
{
public:
   shared_ptr<T> m_object;
   void (T::*m_func)(R);
   R m_arg;

   __ThreadPoolExecute_SharedPtr_WrapperData_1(const shared_ptr<T>& object, void (T::*func)(R), R arg) : m_object(object), m_arg(arg)
   {
      m_func = func;
   }
};

/**
 * Wrapper for ThreadPoolExecute (one argument) using smart pointer to object
 */
template <typename T, typename R> static void __ThreadPoolExecute_SharedPtr_Wrapper_1(void *arg)
{
   auto wd = static_cast<__ThreadPoolExecute_SharedPtr_WrapperData_1<T, R> *>(arg);
   ((*wd->m_object.get()).*(wd->m_func))(wd->m_arg);
   delete wd;
}

/**
 * Execute task as soon as possible (use class member with one argument) using smart pointer to object
 */
template <typename T, typename B, typename R> static inline void ThreadPoolExecute(ThreadPool *p, const shared_ptr<T>& object, void (B::*f)(R), R arg)
{
   ThreadPoolExecute(p, __ThreadPoolExecute_SharedPtr_Wrapper_1<B, R>, new __ThreadPoolExecute_SharedPtr_WrapperData_1<B, R>(object, f, arg));
}

/**
 * Execute serialized task as soon as possible (use class member with one argument) using smart pointer to object
 */
template <typename T, typename B, typename R> static inline void ThreadPoolExecuteSerialized(ThreadPool *p, const TCHAR *key, const shared_ptr<T>& object, void (B::*f)(R), R arg)
{
   ThreadPoolExecuteSerialized(p, key, __ThreadPoolExecute_SharedPtr_Wrapper_1<B, R>, new __ThreadPoolExecute_SharedPtr_WrapperData_1<B, R>(object, f, arg));
}

/**
 * Execute task with delay (use class member with one argument) using smart pointer to object
 */
template <typename T, typename B, typename R> static inline void ThreadPoolScheduleRelative(ThreadPool *p, uint32_t delay, const shared_ptr<T>& object, void (B::*f)(R), R arg)
{
   ThreadPoolScheduleRelative(p, delay, __ThreadPoolExecute_SharedPtr_Wrapper_1<B, R>, new __ThreadPoolExecute_SharedPtr_WrapperData_1<B, R>(object, f, arg));
}

/**
 * Wrapper data for ThreadPoolExecute (function with two arguments)
 */
template <typename R1, typename R2> class __ThreadPoolExecute_WrapperData_2F
{
public:
   void (*m_func)(R1, R2);
   R1 m_arg1;
   R2 m_arg2;

   __ThreadPoolExecute_WrapperData_2F(void (*func)(R1, R2), R1 arg1, R2 arg2) : m_arg1(arg1), m_arg2(arg2)
   {
      m_func = func;
   }
};

/**
 * Wrapper for ThreadPoolExecute (function with two arguments)
 */
template <typename R1, typename R2> static void __ThreadPoolExecute_Wrapper_2F(void *arg)
{
   auto wd = static_cast<__ThreadPoolExecute_WrapperData_2F<R1, R2>*>(arg);
   wd->m_func(wd->m_arg1, wd->m_arg2);
   delete wd;
}

/**
 * Execute task as soon as possible (function with two arguments)
 */
template <typename R1, typename R2> static inline void ThreadPoolExecute(ThreadPool *p, void (*f)(R1, R2), R1 arg1, R2 arg2)
{
   ThreadPoolExecute(p, __ThreadPoolExecute_Wrapper_2F<R1, R2>, new __ThreadPoolExecute_WrapperData_2F<R1, R2>(f, arg1, arg2));
}

/**
 * Execute serialized task as soon as possible (function with two arguments)
 */
template <typename R1, typename R2> static inline void ThreadPoolExecuteSerialized(ThreadPool *p, const TCHAR *key, void (*f)(R1, R2), R1 arg1, R2 arg2)
{
   ThreadPoolExecuteSerialized(p, key, __ThreadPoolExecute_Wrapper_2F<R1, R2>, new __ThreadPoolExecute_WrapperData_2F<R1, R2>(f, arg1, arg2));
}

/**
 * Wrapper data for ThreadPoolExecute (class method with two arguments)
 */
template <typename T, typename R1, typename R2> class __ThreadPoolExecute_WrapperData_2
{
public:
   T *m_object;
   void (T::*m_func)(R1, R2);
   R1 m_arg1;
   R2 m_arg2;

   __ThreadPoolExecute_WrapperData_2(T *object, void (T::*func)(R1, R2), R1 arg1, R2 arg2) : m_arg1(arg1), m_arg2(arg2)
   {
      m_object = object;
      m_func = func;
   }
};

/**
 * Wrapper for ThreadPoolExecute (class method with two arguments)
 */
template <typename T, typename R1, typename R2> static void __ThreadPoolExecute_Wrapper_2(void *arg)
{
   auto wd = static_cast<__ThreadPoolExecute_WrapperData_2<T, R1, R2>*>(arg);
   ((*wd->m_object).*(wd->m_func))(wd->m_arg1, wd->m_arg2);
   delete wd;
}

/**
 * Execute task as soon as possible (class method with two arguments)
 */
template <typename T, typename B, typename R1, typename R2> static inline void ThreadPoolExecute(ThreadPool *p, T *object, void (B::*f)(R1, R2), R1 arg1, R2 arg2)
{
   ThreadPoolExecute(p, __ThreadPoolExecute_Wrapper_2<B, R1, R2>, new __ThreadPoolExecute_WrapperData_2<B, R1, R2>(object, f, arg1, arg2));
}

/**
 * Execute serialized task as soon as possible (use class member with two argumenta)
 */
template <typename T, typename B, typename R1, typename R2> static inline void ThreadPoolExecuteSerialized(ThreadPool *p, const TCHAR *key, T *object, void (B::*f)(R1, R2), R1 arg1, R2 arg2)
{
   ThreadPoolExecuteSerialized(p, key, __ThreadPoolExecute_Wrapper_2<B, R1, R2>, new __ThreadPoolExecute_WrapperData_2<B, R1, R2>(object, f, arg1, arg2));
}

/**
 * Execute task with delay (use class member with two arguments)
 */
template <typename T, typename B, typename R1, typename R2> static inline void ThreadPoolScheduleRelative(ThreadPool *p, uint32_t delay, T *object, void (B::*f)(R1, R2), R1 arg1, R2 arg2)
{
   ThreadPoolScheduleRelative(p, delay, __ThreadPoolExecute_Wrapper_2<B, R1, R2>, new __ThreadPoolExecute_WrapperData_2<B, R1, R2>(object, f, arg1, arg2));
}

/**
 * Wrapper data for ThreadPoolExecute (two arguments) using smart pointer to object
 */
template <typename T, typename R1, typename R2> class __ThreadPoolExecute_SharedPtr_WrapperData_2
{
public:
   shared_ptr<T> m_object;
   void (T::*m_func)(R1, R2);
   R1 m_arg1;
   R2 m_arg2;

   __ThreadPoolExecute_SharedPtr_WrapperData_2(const shared_ptr<T>& object, void (T::*func)(R1, R2), R1 arg1, R2 arg2) : m_object(object), m_arg1(arg1), m_arg2(arg2)
   {
      m_func = func;
   }
};

/**
 * Wrapper for ThreadPoolExecute (two arguments) using smart pointer to object
 */
template <typename T, typename R1, typename R2> static void __ThreadPoolExecute_SharedPtr_Wrapper_2(void *arg)
{
   auto wd = static_cast<__ThreadPoolExecute_SharedPtr_WrapperData_2<T, R1, R2> *>(arg);
   ((*wd->m_object.get()).*(wd->m_func))(wd->m_arg1, wd->m_arg2);
   delete wd;
}

/**
 * Execute task as soon as possible (use class member with two arguments) using smart pointer to object
 */
template <typename T, typename B, typename R1, typename R2> static inline void ThreadPoolExecute(ThreadPool *p, const shared_ptr<T>& object, void (B::*f)(R1, R2), R1 arg1, R2 arg2)
{
   ThreadPoolExecute(p, __ThreadPoolExecute_SharedPtr_Wrapper_2<B, R1, R2>, new __ThreadPoolExecute_SharedPtr_WrapperData_2<B, R1, R2>(object, f, arg1, arg2));
}

/**
 * Execute serialized task as soon as possible (use class member with two arguments) using smart pointer to object
 */
template <typename T, typename B, typename R1, typename R2> static inline void ThreadPoolExecuteSerialized(ThreadPool *p, const TCHAR *key, const shared_ptr<T>& object, void (B::*f)(R1, R2), R1 arg1, R2 arg2)
{
   ThreadPoolExecuteSerialized(p, key, __ThreadPoolExecute_SharedPtr_Wrapper_2<B, R1, R2>, new __ThreadPoolExecute_SharedPtr_WrapperData_2<B, R1, R2>(object, f, arg1, arg2));
}

/**
 * Execute task with delay (use class member with two arguments) using smart pointer to object
 */
template <typename T, typename B, typename R1, typename R2> static inline void ThreadPoolScheduleRelative(ThreadPool *p, uint32_t delay, const shared_ptr<T>& object, void (B::*f)(R1, R2), R1 arg1, R2 arg2)
{
   ThreadPoolScheduleRelative(p, delay, __ThreadPoolExecute_SharedPtr_Wrapper_2<B, R1, R2>, new __ThreadPoolExecute_SharedPtr_WrapperData_2<B, R1, R2>(object, f, arg1, arg2));
}

/**
 * Wrapper data for ThreadPoolExecute (three arguments)
 */
template <typename T, typename R1, typename R2, typename R3> class __ThreadPoolExecute_WrapperData_3
{
public:
   T *m_object;
   void (T::*m_func)(R1, R2, R3);
   R1 m_arg1;
   R2 m_arg2;
   R3 m_arg3;

   __ThreadPoolExecute_WrapperData_3(T *object, void (T::*func)(R1, R2, R3), R1 arg1, R2 arg2, R3 arg3) : m_arg1(arg1), m_arg2(arg2), m_arg3(arg3)
   {
      m_object = object;
      m_func = func;
   }
};

/**
 * Wrapper for ThreadPoolExecute (three arguments)
 */
template <typename T, typename R1, typename R2, typename R3> static void __ThreadPoolExecute_Wrapper_3(void *arg)
{
   auto wd = static_cast<__ThreadPoolExecute_WrapperData_3<T, R1, R2, R3>*>(arg);
   ((*wd->m_object).*(wd->m_func))(wd->m_arg1, wd->m_arg2, wd->m_arg3);
   delete wd;
}

/**
 * Execute task as soon as possible (use class member with three arguments)
 */
template <typename T, typename B, typename R1, typename R2, typename R3> static inline void ThreadPoolExecute(ThreadPool *p, T *object, void (B::*f)(R1, R2, R3), R1 arg1, R2 arg2, R3 arg3)
{
   ThreadPoolExecute(p, __ThreadPoolExecute_Wrapper_3<B, R1, R2, R3>, new __ThreadPoolExecute_WrapperData_3<B, R1, R2, R3>(object, f, arg1, arg2, arg3));
}

/**
 * Wrapper data for ThreadPoolExecute (three arguments)
 */
template <typename R1, typename R2, typename R3> class __ThreadPoolExecute_WrapperData_3F
{
public:
   void (*m_func)(R1, R2, R3);
   R1 m_arg1;
   R2 m_arg2;
   R3 m_arg3;

   __ThreadPoolExecute_WrapperData_3F(void (*func)(R1, R2, R3), R1 arg1, R2 arg2, R3 arg3) : m_arg1(arg1), m_arg2(arg2), m_arg3(arg3)
   {
      m_func = func;
   }
};

/**
 * Wrapper for ThreadPoolExecute (three arguments)
 */
template <typename R1, typename R2, typename R3> static void __ThreadPoolExecute_Wrapper_3F(void *arg)
{
   auto wd = static_cast<__ThreadPoolExecute_WrapperData_3F<R1, R2, R3>*>(arg);
   wd->m_func(wd->m_arg1, wd->m_arg2, wd->m_arg3);
   delete wd;
}

/**
 * Execute task as soon as possible (use function with three arguments)
 */
template <typename R1, typename R2, typename R3> static inline void ThreadPoolExecute(ThreadPool *p, void (*f)(R1, R2, R3), R1 arg1, R2 arg2, R3 arg3)
{
   ThreadPoolExecute(p, __ThreadPoolExecute_Wrapper_3F<R1, R2, R3>, new __ThreadPoolExecute_WrapperData_3F<R1, R2, R3>(f, arg1, arg2, arg3));
}

/**
 * Execute serialized task as soon as possible (use function with three arguments)
 */
template <typename R1, typename R2, typename R3> static inline void ThreadPoolExecuteSerialized(ThreadPool *p, const TCHAR *key, void (*f)(R1, R2, R3), R1 arg1, R2 arg2, R3 arg3)
{
   ThreadPoolExecuteSerialized(p, key, __ThreadPoolExecute_Wrapper_3F<R1, R2, R3>, new __ThreadPoolExecute_WrapperData_3F<R1, R2, R3>(f, arg1, arg2, arg3));
}

/**
 * Execute task with delay (use function with three arguments)
 */
template <typename R1, typename R2, typename R3> static inline void ThreadPoolScheduleRelative(ThreadPool *p, uint32_t delay, void (*f)(R1, R2, R3), R1 arg1, R2 arg2, R3 arg3)
{
   ThreadPoolScheduleRelative(p, delay, __ThreadPoolExecute_Wrapper_3F<R1, R2, R3>, new __ThreadPoolExecute_WrapperData_3F<R1, R2, R3>(f, arg1, arg2, arg3));
}

/**
 * Wrapper data for ThreadPoolExecute (four arguments)
 */
template <typename T, typename R1, typename R2, typename R3, typename R4> class __ThreadPoolExecute_WrapperData_4
{
public:
   T *m_object;
   void (T::*m_func)(R1, R2, R3, R4);
   R1 m_arg1;
   R2 m_arg2;
   R3 m_arg3;
   R4 m_arg4;

   __ThreadPoolExecute_WrapperData_4(T *object, void (T::*func)(R1, R2, R3, R4), R1 arg1, R2 arg2, R3 arg3, R4 arg4) : m_arg1(arg1), m_arg2(arg2), m_arg3(arg3), m_arg4(arg4)
   {
      m_object = object;
      m_func = func;
   }
};

/**
 * Wrapper for ThreadPoolExecute (four arguments)
 */
template <typename T, typename R1, typename R2, typename R3, typename R4> static void __ThreadPoolExecute_Wrapper_4(void *arg)
{
   auto wd = static_cast<__ThreadPoolExecute_WrapperData_4<T, R1, R2, R3, R4>*>(arg);
   ((*wd->m_object).*(wd->m_func))(wd->m_arg1, wd->m_arg2, wd->m_arg3, wd->m_arg4);
   delete wd;
}

/**
 * Execute task as soon as possible (use class member with four arguments)
 */
template <typename T, typename B, typename R1, typename R2, typename R3, typename R4> static inline void ThreadPoolExecute(ThreadPool *p, T *object, void (B::*f)(R1, R2, R3, R4), R1 arg1, R2 arg2, R3 arg3, R4 arg4)
{
   ThreadPoolExecute(p, __ThreadPoolExecute_Wrapper_4<B, R1, R2, R3, R4>, new __ThreadPoolExecute_WrapperData_4<B, R1, R2, R3, R4>(object, f, arg1, arg2, arg3, arg4));
}

/**
 * Wrapper data for ThreadPoolExecute (four arguments)
 */
template <typename R1, typename R2, typename R3, typename R4> class __ThreadPoolExecute_WrapperData_4F
{
public:
   void (*m_func)(R1, R2, R3, R4);
   R1 m_arg1;
   R2 m_arg2;
   R3 m_arg3;
   R4 m_arg4;

   __ThreadPoolExecute_WrapperData_4F(void (*func)(R1, R2, R3, R4), R1 arg1, R2 arg2, R3 arg3, R4 arg4) : m_arg1(arg1), m_arg2(arg2), m_arg3(arg3), m_arg4(arg4)
   {
      m_func = func;
   }
};

/**
 * Wrapper for ThreadPoolExecute (three arguments)
 */
template <typename R1, typename R2, typename R3, typename R4> static void __ThreadPoolExecute_Wrapper_4F(void *arg)
{
   auto wd = static_cast<__ThreadPoolExecute_WrapperData_4F<R1, R2, R3, R4>*>(arg);
   wd->m_func(wd->m_arg1, wd->m_arg2, wd->m_arg3, wd->m_arg4);
   delete wd;
}

/**
 * Execute task as soon as possible (use function with four arguments)
 */
template <typename R1, typename R2, typename R3, typename R4> static inline void ThreadPoolExecute(ThreadPool *p, void (*f)(R1, R2, R3, R4), R1 arg1, R2 arg2, R3 arg3, R4 arg4)
{
   ThreadPoolExecute(p, __ThreadPoolExecute_Wrapper_4F<R1, R2, R3, R4>, new __ThreadPoolExecute_WrapperData_4F<R1, R2, R3, R4>(f, arg1, arg2, arg3, arg4));
}

/**
 * Execute serialized task as soon as possible (use function with four arguments)
 */
template <typename R1, typename R2, typename R3, typename R4> static inline void ThreadPoolExecuteSerialized(ThreadPool *p, const TCHAR *key, void (*f)(R1, R2, R3, R4), R1 arg1, R2 arg2, R3 arg3, R4 arg4)
{
   ThreadPoolExecuteSerialized(p, key, __ThreadPoolExecute_Wrapper_4F<R1, R2, R3, R4>, new __ThreadPoolExecute_WrapperData_4F<R1, R2, R3, R4>(f, arg1, arg2, arg3, arg4));
}

/**
 * Execute task with delay (use function with four arguments)
 */
template <typename R1, typename R2, typename R3, typename R4> static inline void ThreadPoolScheduleRelative(ThreadPool *p, uint32_t delay, void (*f)(R1, R2, R3, R4), R1 arg1, R2 arg2, R3 arg3, R4 arg4)
{
   ThreadPoolScheduleRelative(p, delay, __ThreadPoolExecute_Wrapper_4F<R1, R2, R3, R4>, new __ThreadPoolExecute_WrapperData_4F<R1, R2, R3, R4>(f, arg1, arg2, arg3, arg4));
}

/**
 * Wrapper for ThreadPoolExecute with shared_ptr argument
 */
static inline void __ThreadPoolExecute_Callable_Wrapper(void *arg)
{
   auto f = static_cast<std::function<void ()>*>(arg);
   (*f)();
   delete f;
}

/**
 * Wrapper for ThreadPoolExecute to use std::function
 */
static inline void ThreadPoolExecute(ThreadPool *p, const std::function<void ()>& f)
{
   ThreadPoolExecute(p, __ThreadPoolExecute_Callable_Wrapper, new std::function<void ()>(f));
}

/**
 * Wrapper for ThreadPoolExecuteSerialized to use std::function
 */
static inline void ThreadPoolExecuteSerialized(ThreadPool *p, const TCHAR *key, const std::function<void ()>& f)
{
   ThreadPoolExecuteSerialized(p, key, __ThreadPoolExecute_Callable_Wrapper, new std::function<void ()>(f));
}

/**
 * Wrapper for ThreadPoolScheduleAbsolute to use std::function
 */
static inline void ThreadPoolScheduleAbsolute(ThreadPool *p, time_t runTime, const std::function<void ()>& f)
{
   ThreadPoolScheduleAbsolute(p, runTime, __ThreadPoolExecute_Callable_Wrapper, new std::function<void ()>(f));
}

/**
 * Wrapper for ThreadPoolScheduleAbsoluteMs to use std::function
 */
static inline void ThreadPoolScheduleAbsoluteMs(ThreadPool *p, int64_t runTime, const std::function<void ()>& f)
{
   ThreadPoolScheduleAbsoluteMs(p, runTime, __ThreadPoolExecute_Callable_Wrapper, new std::function<void ()>(f));
}

/**
 * Wrapper for ThreadPoolScheduleRelative to use std::function
 */
static inline void ThreadPoolScheduleRelative(ThreadPool *p, uint32_t delay, const std::function<void ()>& f)
{
   ThreadPoolScheduleRelative(p, delay, __ThreadPoolExecute_Callable_Wrapper, new std::function<void ()>(f));
}

/**
 * Set default thread stack size
 */
void LIBNETXMS_EXPORTABLE ThreadSetDefaultStackSize(int size);

/**
 * Mutex type
 */
enum class MutexType
{
   NORMAL,
   FAST,
   RECURSIVE
};

/**
 * Wrappers for mutex
 */
class LIBNETXMS_EXPORTABLE Mutex
{
   Mutex(const Mutex& src) = delete;
   Mutex& operator =(const Mutex& src) = delete;

private:
#if defined(_WIN32)
   win_mutex_t m_mutex;
#elif defined(_USE_GNU_PTH)
   pth_mutex_t m_mutex;
#else
   pthread_mutex_t m_mutex;
#endif

public:
   Mutex(MutexType type = MutexType::NORMAL)
   {
#if defined(_WIN32)
      InitializeMutex(&m_mutex, 4000); // Under Windows we always use mutex with spin
#elif defined(_USE_GNU_PTH)
      pth_mutex_init(&m_mutex);  // In libpth, recursive locking is explicitly supported, and no separate "fast" mutexes
#else
      if (type == MutexType::FAST)
      {
         // Create fast mutex if supported, otherwise normal mutex
#if HAVE_DECL_PTHREAD_MUTEX_ADAPTIVE_NP
         pthread_mutexattr_t a;
         pthread_mutexattr_init(&a);
         MUTEXATTR_SETTYPE(&a, PTHREAD_MUTEX_ADAPTIVE_NP);
         pthread_mutex_init(&m_mutex, &a);
         pthread_mutexattr_destroy(&a);
#else
         pthread_mutex_init(&m_mutex, nullptr);
#endif
      }
      else if (type == MutexType::RECURSIVE)
      {
#ifdef HAVE_RECURSIVE_MUTEXES
         pthread_mutexattr_t a;
         pthread_mutexattr_init(&a);
         MUTEXATTR_SETTYPE(&a, MUTEX_RECURSIVE_FLAG);
         pthread_mutex_init(&m_mutex, &a);
         pthread_mutexattr_destroy(&a);
#else
#error FIXME: implement recursive mutexes
#endif
      }
      else
      {
         pthread_mutex_init(&m_mutex, nullptr);
      }
#endif
   }
   ~Mutex()
   {
#if defined(_WIN32)
      DestroyMutex(&m_mutex);
#elif defined(_USE_GNU_PTH)
      // No cleanup for mutex
#else
      pthread_mutex_destroy(&m_mutex);
#endif
   }

   /**
    * Lock mutex. Calling thread will block if necessary.
    */
   void lock() const
   {
#if defined(_WIN32)
      LockMutex((win_mutex_t*)&m_mutex, INFINITE);
#elif defined(_USE_GNU_PTH)
      pth_mutex_acquire((pth_mutex_t*)&m_mutex, FALSE, nullptr);
#else
      pthread_mutex_lock((pthread_mutex_t*)&m_mutex);
#endif
   }

   /**
    * Try lock mutex without locking calling thread. Returns true if lock was successful.
    */
   bool tryLock() const
   {
#if defined(_WIN32)
      return TryLockMutex((win_mutex_t*)&m_mutex);
#elif defined(_USE_GNU_PTH)
      return pth_mutex_acquire((pth_mutex_t*)&m_mutex, TRUE, nullptr) != 0;
#else
      return pthread_mutex_trylock((pthread_mutex_t*)&m_mutex) == 0;
#endif
   }

   /**
    * Try mutex lock with timeout. Returns true if lock was successful.
    */
   bool timedLock(uint32_t timeout) const
   {
      if (timeout == 0)
         return tryLock();

      if (timeout == INFINITE)
      {
         lock();
         return true;
      }

#if defined(_WIN32)
      return LockMutex((win_mutex_t*)&m_mutex, timeout);
#elif defined(_USE_GNU_PTH)
      pth_event_t ev = pth_event(PTH_EVENT_TIME, pth_timeout(timeout / 1000, (timeout % 1000) * 1000));
      int retcode = pth_mutex_acquire((pth_mutex_t*)&m_mutex, FALSE, ev);
      if ((retcode > 0) && (pth_event_status(ev) == PTH_STATUS_OCCURRED))
         retcode = 0;
      pth_event_free(ev, PTH_FREE_ALL);
      return retcode > 0;
#else
#if HAVE_PTHREAD_MUTEX_TIMEDLOCK
   struct timeval now;
   gettimeofday(&now, nullptr);
   now.tv_usec += (timeout % 1000) * 1000;

   struct timespec absTimeout;
   absTimeout.tv_sec = now.tv_sec + (timeout / 1000) + now.tv_usec / 1000000;
   absTimeout.tv_nsec = (now.tv_usec % 1000000) * 1000;

   return pthread_mutex_timedlock((pthread_mutex_t*)&m_mutex, &absTimeout) == 0;
#else
   if (pthread_mutex_trylock((pthread_mutex_t*)&m_mutex) == 0)
      return true;

   while(timeout > 0)
   {
      if (timeout >= 10)
      {
         ThreadSleepMs(10);
         timeout -= 10;
      }
      else
      {
         ThreadSleepMs(timeout);
         timeout = 0;
      }

      if (pthread_mutex_trylock((pthread_mutex_t*)&m_mutex) == 0)
         return true;
   };

   return false;
#endif
#endif
   }

   /**
    * Unlock mutex
    */
   void unlock() const
   {
#if defined(_WIN32)
      UnlockMutex((win_mutex_t*)&m_mutex);
#elif defined(_USE_GNU_PTH)
      pth_mutex_release((pth_mutex_t*)&m_mutex);
#else
      pthread_mutex_unlock((pthread_mutex_t*)&m_mutex);
#endif
   }
};

/**
 * Helper class that locks mutex on creation and unlocks on destruction
 */
class LockGuard
{
   LockGuard(const LockGuard& src) = delete;
   LockGuard& operator =(const LockGuard& src) = delete;

private:
   const Mutex& m_mutex;

public:
   explicit LockGuard(const Mutex& mutex) : m_mutex(mutex)
   {
      mutex.lock();
   }
   ~LockGuard()
   {
      m_mutex.unlock();
   }
};

/**
 * Wrappers for condition
 */
class LIBNETXMS_EXPORTABLE Condition
{
   Condition(const Condition& src) = delete;
   Condition& operator =(const Condition& src) = delete;

private:
#if defined(_WIN32)
   CRITICAL_SECTION m_mutex;
   CONDITION_VARIABLE m_condition;
#elif defined(_USE_GNU_PTH)
   pth_mutex_t m_mutex;
   pth_cond_t m_condition;
#else
   pthread_mutex_t m_mutex;
   pthread_cond_t m_condition;
#endif
   bool m_broadcast;
   bool m_isSet;

public:
   Condition(bool broadcast)
   {
#if defined(_WIN32)
      InitializeCriticalSectionAndSpinCount(&m_mutex, 4000);
      InitializeConditionVariable(&m_condition);
#elif defined(_USE_GNU_PTH)
      pth_mutex_init(&m_mutex);
      pth_cond_init(&m_condition);
#else
#if HAVE_DECL_PTHREAD_MUTEX_ADAPTIVE_NP
      pthread_mutexattr_t a;
      pthread_mutexattr_init(&a);
      MUTEXATTR_SETTYPE(&a, PTHREAD_MUTEX_ADAPTIVE_NP);
      pthread_mutex_init(&m_mutex, &a);
      pthread_mutexattr_destroy(&a);
#else
      pthread_mutex_init(&m_mutex, nullptr);
#endif
      pthread_cond_init(&m_condition, nullptr);
#endif
      m_broadcast = broadcast;
      m_isSet = false;
   }

   ~Condition()
   {
#if defined(_WIN32)
      DeleteCriticalSection(&m_mutex);
#elif defined(_USE_GNU_PTH)
      // No cleanup for mutex or condition
#else
      pthread_cond_destroy(&m_condition);
      pthread_mutex_destroy(&m_mutex);
#endif
   }

   /**
    * Set condition
    */
   void set()
   {
#if defined(_WIN32)
      EnterCriticalSection(&m_mutex);
      m_isSet = true;
      if (m_broadcast)
         WakeAllConditionVariable(&m_condition);
      else
         WakeConditionVariable(&m_condition);
      LeaveCriticalSection(&m_mutex);
#elif defined(_USE_GNU_PTH)
      pth_mutex_acquire(&m_mutex, FALSE, nullptr);
      m_isSet = true;
      pth_cond_notify(&m_condition, m_broadcast);
      pth_mutex_release(&m_mutex);
#else
      pthread_mutex_lock(&m_mutex);
      m_isSet = true;
      if (m_broadcast)
         pthread_cond_broadcast(&m_condition);
      else
         pthread_cond_signal(&m_condition);
      pthread_mutex_unlock(&m_mutex);
#endif
   }

   /**
    * Pulse condition - will wake up thread or all threads (for broadcast condition) that is currently waiting on condition
    * but wil not mark conditionas set if no threads are currently waiting.
    */
   void pulse()
   {
#if defined(_WIN32)
      EnterCriticalSection(&m_mutex);
      if (m_broadcast)
         WakeAllConditionVariable(&m_condition);
      else
         WakeConditionVariable(&m_condition);
      m_isSet = false;
      LeaveCriticalSection(&m_mutex);
#elif defined(_USE_GNU_PTH)
      pth_mutex_acquire(&m_mutex, FALSE, nullptr);
      pth_cond_notify(&m_condition, m_broadcast);
      m_isSet = false;
      pth_mutex_release(&m_mutex);
#else
      pthread_mutex_lock(&m_mutex);
      if (m_broadcast)
         pthread_cond_broadcast(&m_condition);
      else
         pthread_cond_signal(&m_condition);
      m_isSet = false;
      pthread_mutex_unlock(&m_mutex);
#endif
   }

   /**
    * Reset condition to unset state
    */
   void reset()
   {
#if defined(_WIN32)
      EnterCriticalSection(&m_mutex);
      m_isSet = false;
      LeaveCriticalSection(&m_mutex);
#elif defined(_USE_GNU_PTH)
      pth_mutex_acquire(&m_mutex, FALSE, nullptr);
      m_isSet = false;
      pth_mutex_release(&m_mutex);
#else
      pthread_mutex_lock(&m_mutex);
      m_isSet = false;
      pthread_mutex_unlock(&m_mutex);
#endif
   }

   /**
    * Wait for condition to set. Timeout is in milliseconds.
    */
   bool wait(uint32_t timeout = INFINITE)
   {
      bool success;

#if defined(_WIN32)
      EnterCriticalSection(&m_mutex);

      if (m_isSet)
      {
         success = true;
         if (!m_broadcast)
            m_isSet = false;
      }
      else if (timeout == INFINITE)
      {
         // SleepConditionVariableCS may have spurious wakeups, so wait should be done in a cycle
         do
         {
            SleepConditionVariableCS(&m_condition, &m_mutex, INFINITE);
         } while (!m_isSet);
         success = true;
         if (!m_broadcast)
            m_isSet = false;
      }
      else
      {
         do
         {
            uint64_t start = GetTickCount64();
            SleepConditionVariableCS(&m_condition, &m_mutex, timeout);
            uint32_t elapsed = static_cast<uint32_t>(GetTickCount64() - start);
            timeout -= std::min(elapsed, timeout);
         } while (!m_isSet && (timeout > 0));
         success = m_isSet;
         if (!m_broadcast)
            m_isSet = false;
      }

      LeaveCriticalSection(&m_mutex);
#elif defined(_USE_GNU_PTH)
      pth_mutex_acquire(&m_mutex, FALSE, nullptr);
      if (m_isSet)
      {
         success = true;
         if (!m_broadcast)
            m_isSet = false;
      }
      else
      {
         if (timeout != INFINITE)
         {
            pth_event_t ev = pth_event(PTH_EVENT_TIME, pth_timeout(timeout / 1000, (timeout % 1000) * 1000));
            int retcode = pth_cond_await(&m_condition, &m_mutex, ev);
            if (retcode > 0)
               success = (pth_event_status(ev) != PTH_STATUS_OCCURRED);
            else
               success = false;
            pth_event_free(ev, PTH_FREE_ALL);
         }
         else
         {
            success = (pth_cond_await(&m_condition, &m_mutex, nullptr) > 0);
         }

         if (success && !m_broadcast)
            m_isSet = false;
      }
      pth_mutex_release(&m_mutex);
#else
      pthread_mutex_lock(&m_mutex);
      if (m_isSet)
      {
         success = true;
         if (!m_broadcast)
            m_isSet = false;
      }
      else
      {
         int retcode;

         if (timeout != INFINITE)
         {
#if HAVE_PTHREAD_COND_RELTIMEDWAIT_NP
            struct timespec ts;
            ts.tv_sec = timeout / 1000;
            ts.tv_nsec = (timeout % 1000) * 1000000;
            retcode = pthread_cond_reltimedwait_np(&m_condition, &m_mutex, &ts);
#else
            struct timeval now;
            gettimeofday(&now, nullptr);
            now.tv_usec += (timeout % 1000) * 1000;

            struct timespec ts;
            ts.tv_sec = now.tv_sec + (timeout / 1000) + now.tv_usec / 1000000;
            ts.tv_nsec = (now.tv_usec % 1000000) * 1000;

            retcode = pthread_cond_timedwait(&m_condition, &m_mutex, &ts);
#endif
         }
         else
         {
            retcode = pthread_cond_wait(&m_condition, &m_mutex);
         }

         if (retcode == 0)
         {
            if (!m_broadcast)
               m_isSet = false;
            success = true;
         }
         else
         {
            success = false;
         }
      }
      pthread_mutex_unlock(&m_mutex);
#endif

      return success;
   }
};

/**
 * Wrappers for read/write lock
 */
class LIBNETXMS_EXPORTABLE RWLock
{
   RWLock(const RWLock& src) = delete;
   RWLock& operator =(const RWLock& src) = delete;

private:
#if HAVE_PTHREAD_RWLOCK
   pthread_rwlock_t m_rwlock;
#elif defined(_USE_GNU_PTH)
   pth_rwlock_t m_rwlock;
#elif defined(_WIN32)
   win_rwlock_t m_rwlock;
#else
   rwlock_t m_rwlock;
#endif

public:
   RWLock()
   {
#if HAVE_PTHREAD_RWLOCK
      pthread_rwlock_init(&m_rwlock, nullptr);
#elif defined(_USE_GNU_PTH)
      pth_rwlock_init(&m_rwlock);
#else
      InitializeRWLock(&m_rwlock);
#endif
   }

   ~RWLock()
   {
#if HAVE_PTHREAD_RWLOCK
      pthread_rwlock_destroy(&m_rwlock);
#elif defined(_USE_GNU_PTH)
      // No cleanup for rwlock
#else
      DestroyRWLock(&m_rwlock);
#endif
   }

   /**
    * Lock for reading (non-exclusive lock)
    */
   void readLock() const
   {
#if HAVE_PTHREAD_RWLOCK
      pthread_rwlock_rdlock(const_cast<pthread_rwlock_t*>(&m_rwlock));
#elif defined(_USE_GNU_PTH)
      pth_rwlock_acquire(const_cast<pth_rwlock_t*>(&m_rwlock), PTH_RWLOCK_RD, FALSE, nullptr);
#elif defined(_WIN32)
      ReadLockRWLock(const_cast<win_rwlock_t*>(&m_rwlock));
#else
      ReadLockRWLock(const_cast<rwlock_t*>(&m_rwlock));
#endif
   }

   /**
    * Lock for writing (exclusive lock)
    */
   void writeLock()
   {
#if HAVE_PTHREAD_RWLOCK
      pthread_rwlock_wrlock(&m_rwlock);
#elif defined(_USE_GNU_PTH)
      pth_rwlock_acquire(&m_rwlock, PTH_RWLOCK_RW, FALSE, nullptr);
#else
      WriteLockRWLock(&m_rwlock);
#endif
   }

   /**
    * Unlock
    */
   void unlock() const
   {
#if HAVE_PTHREAD_RWLOCK
      pthread_rwlock_unlock(const_cast<pthread_rwlock_t*>(&m_rwlock));
#elif defined(_USE_GNU_PTH)
      pth_rwlock_release(const_cast<pth_rwlock_t*>(&m_rwlock));
#elif defined(_WIN32)
      UnlockRWLock(const_cast<win_rwlock_t*>(&m_rwlock));
#else
      UnlockRWLock(const_cast<rwlock_t*>(&m_rwlock));
#endif
   }
};

#endif   /* __cplusplus */

#endif   /* _nms_threads_h_ */
