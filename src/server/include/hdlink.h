/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2014 Victor Kirhenshtein
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
** File: hdlink.h
**
**/

#ifndef _hdlink_h_
#define _hdlink_h_

#include <nms_common.h>
#include <nxsrvapi.h>

/**
 *API version
 */
#define HDLINK_API_VERSION           1

/**
 * Driver header
 */
#ifdef _WIN32
#define __HDLINK_EXPORT __declspec(dllexport)
#else
#define __HDLINK_EXPORT
#endif

#define DECLARE_HDLINK_ENTRY_POINT(name, implClass) \
extern "C" int __HDLINK_EXPORT hdlinkAPIVersion; \
extern "C" const TCHAR __HDLINK_EXPORT *hdlinkName; \
int __HDLINK_EXPORT hdlinkAPIVersion = HDLINK_API_VERSION; \
const TCHAR __HDLINK_EXPORT *hdlinkName = name; \
extern "C" HelpDeskLink __HDLINK_EXPORT *hdlinkCreateInstance() { return new implClass; }

/**
 * Base class for device drivers
 */
class LIBNXSRV_EXPORTABLE HelpDeskLink
{
protected:
   virtual void onResolveIssue(const TCHAR *hdref);
   virtual void onCloseIssue(const TCHAR *hdref);

public:
   HelpDeskLink();
   virtual ~HelpDeskLink();

   virtual const TCHAR *getName();
   virtual const TCHAR *getVersion();

   virtual bool init();
   virtual bool checkConnection();
   virtual UINT32 openIssue(const TCHAR *description, TCHAR *hdref);
   virtual UINT32 addComment(const TCHAR *hdref, const TCHAR *comment);
};

/**
 * Init call for server
 */
void LIBNXSRV_EXPORTABLE SetHDLinkEntryPoints(void (*__resolve)(const TCHAR *), void (*__close)(const TCHAR *));

#endif   /* _nddrv_h_ */
