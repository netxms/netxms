#ifndef _nxmc_h_
#define _nxmc_h_

#include <nms_common.h>
#include <nms_util.h>
#include <nxclapi.h>

#include <nxmc_api.h>

#ifdef _WIN32
#include "resource.h"

extern "C" WXDLLIMPEXP_BASE HINSTANCE wxGetInstance();

#endif

#include "mainfrm.h"
#include "busydlg.h"
#include "frame.h"


//
// Application flags
//

#define AF_SAVE_OBJECT_CACHE		0x0001


//
// Custom events
//

BEGIN_DECLARE_EVENT_TYPES()
    DECLARE_LOCAL_EVENT_TYPE(nxEVT_CONNECT, 0)
    DECLARE_LOCAL_EVENT_TYPE(nxEVT_REQUEST_COMPLETED, 0)
    DECLARE_LOCAL_EVENT_TYPE(nxEVT_SET_STATUS_TEXT, 0)
END_DECLARE_EVENT_TYPES()

#define EVT_NXMC_EVENT(evt, fn) \
    DECLARE_EVENT_TABLE_ENTRY( \
        evt, wxID_ANY, wxID_ANY, \
		  (wxObjectEventFunction)(wxEventFunction) wxStaticCastEvent( wxCommandEventFunction, &fn ), \
        (wxObject *) NULL \
    ),
#define EVT_NX_CONNECT(fn)             EVT_NXMC_EVENT(nxEVT_CONNECT, fn)
#define EVT_NX_REQUEST_COMPLETED(fn)   EVT_NXMC_EVENT(nxEVT_REQUEST_COMPLETED, fn)
#define EVT_NX_SET_STATUS_TEXT(fn)     EVT_NXMC_EVENT(nxEVT_SET_STATUS_TEXT, fn)


//
// Command codes
//

#define wxID_PANE_DETACH				(wxID_HIGHEST + 1)
#define wxID_PANE_CLOSE					(wxID_HIGHEST + 2)
#define wxID_PANE_FLOAT					(wxID_HIGHEST + 3)
#define wxID_PANE_MOVE_TO_NOTEBOOK	(wxID_HIGHEST + 4)


//
// Application class
//

class nxApp : public wxApp
{
private:
	nxMainFrame *m_mainFrame;

public:
    virtual bool OnInit();
	 virtual int OnExit();

	 nxMainFrame *GetMainFrame(void) { return m_mainFrame; }

	 void ShowClientError(DWORD rcc, TCHAR *msgTemplate);
};

DECLARE_APP(nxApp)


//
// Functions
//

void LoadPlugins(void);


//
// Global variables
//

extern NXC_SESSION g_hSession;
extern DWORD g_appFlags;
extern TCHAR g_userName[];


#endif
