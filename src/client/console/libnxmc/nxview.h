/* 
** NetXMS - Network Management System
** Portable management console - plugin API library
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
** File: nxview.h
**
**/

#ifndef _nxview_h_
#define _nxview_h_


//
// Request flags
//

#define DRF_DELETE_ARG1		0x0001
#define DRF_DELETE_ARG2		0x0002
#define DRF_DELETE_ARG3		0x0004
#define DRF_DELETE_ARG4		0x0008
#define DRF_DELETE_ARG5		0x0010
#define DRF_DELETE_ARG6		0x0020
#define DRF_DELETE_ARG7		0x0040


//
// Request data
//

class RqData
{
public:
	RqData(int id, wxEvtHandler *owner, void *func, int numArgs, const TCHAR *errMsg, DWORD flags);
	~RqData();
	
	int m_id;
	wxEvtHandler *m_owner;
	DWORD (* m_func)(...);
	int m_numArgs;
	wxUIntPtr m_arg[9];
	DWORD m_rcc;
	DWORD m_flags;
	TCHAR *m_errMsg;
};


//
// Basic console view class
//

class LIBNXMC_EXPORTABLE nxView : public wxWindow
{
private:
	wxString m_label;
	wxTimer *m_timer;
	wxIcon m_icon;	// Icon associated with this view
	int m_activeRequestCount;
	int m_freeRqId;	// First free request id
	bool m_isBusy;
	nxIntToStringHash m_rqErrorMessages;	// Error messages for active requests
	
	int DoRequest(RqData *data);
	
protected:
	
	int DoRequestArg1(void *func, wxUIntPtr arg1, const TCHAR *errMsg, DWORD flags = 0);
	int DoRequestArg2(void *func, wxUIntPtr arg1, wxUIntPtr arg2, const TCHAR *errMsg, DWORD flags = 0);
	int DoRequestArg3(void *func, wxUIntPtr arg1, wxUIntPtr arg2, wxUIntPtr arg3, const TCHAR *errMsg, DWORD flags = 0);
	int DoRequestArg4(void *func, wxUIntPtr arg1, wxUIntPtr arg2, wxUIntPtr arg3,
                     wxUIntPtr arg4, const TCHAR *errMsg, DWORD flags = 0);
	int DoRequestArg5(void *func, wxUIntPtr arg1, wxUIntPtr arg2, wxUIntPtr arg3,
                     wxUIntPtr arg4, wxUIntPtr arg5, const TCHAR *errMsg, DWORD flags = 0);
	
	virtual void RequestCompletionHandler(int rqId, DWORD rcc, const TCHAR *errMsg);

public:
	nxView(wxWindow *parent, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize);
	virtual ~nxView();

	virtual void SetLabel(const wxString& label);
	virtual wxString GetLabel() const;
	
	virtual void RefreshView();

	void SetIcon(const wxIcon &icon) { m_icon = icon; }
	const wxIcon& GetIcon() { return m_icon; }
	
	bool IsBusy() { return m_isBusy; }
	
	// Event handlers
protected:
	void OnTimer(wxTimerEvent &event);
	void OnRequestCompleted(wxCommandEvent &event);

	DECLARE_EVENT_TABLE()
};

#endif
