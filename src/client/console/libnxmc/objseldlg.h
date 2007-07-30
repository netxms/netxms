/* $Id: objseldlg.h,v 1.1 2007-07-30 10:00:06 victor Exp $ */
/* 
** NetXMS - Network Management System
** Portable management console
** Copyright (C) 2007 Victor Kirhenshtein
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
** File: objseldlg.h
**
**/

#ifndef _objseldlg_h_
#define _objseldlg_h_


//
// Allowed classes
//

#define SCL_NODE              0x0001
#define SCL_INTERFACE         0x0002
#define SCL_CONTAINER         0x0004
#define SCL_SUBNET            0x0008
#define SCL_NETWORK           0x0010
#define SCL_SERVICEROOT       0x0020
#define SCL_ZONE              0x0040
#define SCL_NETWORKSERVICE    0x0080
#define SCL_TEMPLATE          0x0100
#define SCL_TEMPLATEGROUP     0x0200
#define SCL_TEMPLATEROOT      0x0400
#define SCL_VPNCONNECTOR      0x0800
#define SCL_CONDITION			0x1000
#define SCL_CLUSTER				0x2000


//
// Object selection dialog
//

class LIBNXMC_EXPORTABLE nxObjectSelDlg : public wxDialog
{
private:
	int m_sortMode;
	int m_sortDir;

	void SortObjects();

protected:
	virtual bool TransferDataFromWindow(void);
	virtual bool TransferDataToWindow(void);

public:
	nxObjectSelDlg(wxWindow *parent);
	virtual ~nxObjectSelDlg();

	DWORD m_parentObjectId;
	DWORD m_allowedClasses;
	bool m_isSelectAddress;
	bool m_isShowLoopback;
	bool m_isSingleSelection;
	bool m_isEmptySelectionAllowed;
	wxArrayLong m_objectList;

// Event handlers
protected:
	void OnInitDialog(wxInitDialogEvent &event);
	void OnListColumnClick(wxListEvent &event);

	DECLARE_EVENT_TABLE()
};

#endif
