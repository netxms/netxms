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
** File: nxcall.h
**
**/

#ifndef _nxcall_h_
#define _nxcall_h_

/**
 * Call handler type
 */
typedef int (*CallHandler)(const void*, void*);

void LIBNETXMS_EXPORTABLE RegisterCallHandler(const char *name, CallHandler handler);
void LIBNETXMS_EXPORTABLE UnregisterCallHandler(const char *name);
int LIBNETXMS_EXPORTABLE CallNamedFunction(const char *name, const void *input = nullptr, void *output = nullptr);

template<typename _Ti, typename _To> static inline void RegisterCallHandler(const char *name, int (*handler)(const _Ti*, _To*))
{
   RegisterCallHandler(name, (CallHandler)handler);
}

uint32_t LIBNETXMS_EXPORTABLE RegisterHook(const char *name, std::function<void (void*)> hook);
void LIBNETXMS_EXPORTABLE UnregisterHook(uint32_t hookId);
void LIBNETXMS_EXPORTABLE CallHook(const char *name, void *data);

template<typename R1> struct _HookData1
{
   R1 arg1;
   
   _HookData1(R1 _arg1) : arg1(std::move(_arg1)) { }
};

template<typename R1, typename R2> struct _HookData2
{
   R1 arg1;
   R2 arg2;

   _HookData2(R1 _arg1, R2 _arg2) : arg1(std::move(_arg1)), arg2(std::move(_arg2)) { }
};

template<typename R1> static inline uint32_t RegisterHook(const char *name, void (*handler)(R1))
{
   return RegisterHook(name, [handler] (void *data) {
      handler(std::move(static_cast<_HookData1<R1>*>(data)->arg1));
   });
}

template<typename R1, typename R2> static inline uint32_t RegisterHook(const char *name, void (*handler)(R1, R2))
{
   return RegisterHook(name, [handler] (void *data) {
      handler(std::move(static_cast<_HookData2<R1, R2>*>(data)->arg1), std::move(static_cast<_HookData2<R1, R2>*>(data)->arg2));
   });
}

template<typename R1> static inline void CallHook(const char *name, R1 arg1)
{
   _HookData1<R1> data(std::move(arg1));
   CallHook(name, (void*)&data);
}

template<typename R1, typename R2> static inline void CallHook(const char *name, R1 arg1, R2 arg2)
{
   _HookData2<R1, R2> data(std::move(arg1), std::move(arg2));
   CallHook(name, (void*)&data);
}

#endif
