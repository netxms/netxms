/* $Id$ */
/* 
** NetXMS - Network Management System
** Portable management console
** Copyright (C) 2008 Alex Kirhenshtein
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

#ifndef userSelectionDialog_h__
#define userSelectionDialog_h__

class LIBNXMC_EXPORTABLE nxUserSelectionDialog : public wxDialog
{
public:
	nxUserSelectionDialog(wxWindow *parent);
	virtual ~nxUserSelectionDialog();

protected:
	virtual bool TransferDataFromWindow(void);
	virtual bool TransferDataToWindow(void);

private:
	int m_sortMode;
	int m_sortDir;

	void OnInitDialog(wxInitDialogEvent &event);
	void OnListColumnClick(wxListEvent &event);
	void OnListItemActivate(wxListEvent &event);

	DECLARE_EVENT_TABLE()
};

#endif // userSelectionDialog_h__