/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2024 Victor Kirhenshtein
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
#define HDLINK_API_VERSION           3

/**
 * Driver header
 */
#define DECLARE_HDLINK_ENTRY_POINT(name, implClass) \
extern "C" int __EXPORT hdlinkAPIVersion; \
extern "C" const TCHAR __EXPORT *hdlinkName; \
int __EXPORT hdlinkAPIVersion = HDLINK_API_VERSION; \
const TCHAR __EXPORT *hdlinkName = name; \
extern "C" HelpDeskLink __EXPORT *hdlinkCreateInstance() { return new implClass; }

/**
 * Base class for device drivers
 */
class LIBNXSRV_EXPORTABLE HelpDeskLink
{
protected:
   void onResolveIssue(const TCHAR *hdref);
   void onCloseIssue(const TCHAR *hdref);
   void onNewComment(const TCHAR *hdref, const TCHAR *comment);

public:
   HelpDeskLink();
   virtual ~HelpDeskLink();

   virtual const TCHAR *getName();
   virtual const TCHAR *getVersion();

   virtual bool init();
   virtual bool checkConnection();
   virtual uint32_t openIssue(const TCHAR *description, TCHAR *hdref);
   virtual uint32_t addComment(const TCHAR *hdref, const TCHAR *comment);
   virtual uint32_t getIssueState(const TCHAR *hdref, bool *open);
   virtual bool getIssueUrl(const TCHAR *hdref, TCHAR *url, size_t size);
};

/**
 * Init call for server
 */
void LIBNXSRV_EXPORTABLE SetHDLinkEntryPoints(uint32_t (*__resolve)(const TCHAR*), uint32_t (*__close)(const TCHAR*), uint32_t (*__newComment)(const TCHAR*, const TCHAR*));

#endif   /* _nddrv_h_ */
