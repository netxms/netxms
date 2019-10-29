/* 
** NetXMS - Network Management System
** Copyright (C) 2019 Raden Solutions
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
** File: ncdrv.h
**
**/

#ifndef _ncdrv_h_
#define _ncdrv_h_

#include <nms_common.h>
#include <nxconfig.h>

/**
 * API version
 */
#define NCDRV_API_VERSION           2

/**
 * Notification channel configuration template
 */
struct NCConfigurationTemplate
{
   NCConfigurationTemplate(bool needSubject, bool needRecipient)
   {
      this->needSubject = needSubject;
      this->needRecipient = needRecipient;
   }

   bool needSubject;
   bool needRecipient;
};

/**
 * Storage interface for notification channel drivers
 */
class NCDriverStorageManager
{
protected:
   NCDriverStorageManager() { }
   virtual ~NCDriverStorageManager() { }

public:
   virtual TCHAR *get(const TCHAR *key) = 0;
   virtual StringMap *getAll() = 0;
   virtual void set(const TCHAR *key, const TCHAR *value) = 0;
   virtual void clear(const TCHAR *key) = 0;
};

/**
 * Notification Channel Driver base class
 */
class NCDriver
{
protected:
   NCDriver() { }

public:
   virtual ~NCDriver() { }

   virtual bool send(const TCHAR *recipient, const TCHAR *subject, const TCHAR *body) = 0;
};

/**
 * NCD module entry point
 */
#define DECLARE_NCD_ENTRY_POINT(name, configTemplate) \
extern "C" __EXPORT_VAR(int NcdAPIVersion); \
__EXPORT_VAR(int NcdAPIVersion) = NCDRV_API_VERSION; \
extern "C" __EXPORT_VAR(const char *NcdName); \
__EXPORT_VAR(const char *NcdName) = #name; \
extern "C" __EXPORT const NCConfigurationTemplate *NcdGetConfigurationTemplate() { return (configTemplate); } \
extern "C" __EXPORT NCDriver *NcdCreateInstance(Config *config, NCDriverStorageManager *storageManager)

#endif   /* _ncdrv_h_ */
