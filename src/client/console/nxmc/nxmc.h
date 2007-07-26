#ifndef _nxmc_h_
#define _nxmc_h_

#include <nms_common.h>
#include <nms_util.h>
#include <nxclapi.h>
#include <nxsnmp.h>

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
    DECLARE_LOCAL_EVENT_TYPE(nxEVT_SET_STATUS_TEXT, 0)
END_DECLARE_EVENT_TYPES()

#define EVT_NXMC_EVENT(evt, fn) \
    DECLARE_EVENT_TABLE_ENTRY( \
        evt, wxID_ANY, wxID_ANY, \
		  (wxObjectEventFunction)(wxEventFunction) wxStaticCastEvent( wxCommandEventFunction, &fn ), \
        (wxObject *) NULL \
    ),
#define EVT_NX_SET_STATUS_TEXT(fn)     EVT_NXMC_EVENT(nxEVT_SET_STATUS_TEXT, fn)


//
// Command codes
//

#define wxID_PANE_DETACH				(wxID_HIGHEST + 1)
#define wxID_PANE_CLOSE					(wxID_HIGHEST + 2)
#define wxID_PANE_FLOAT					(wxID_HIGHEST + 3)
#define wxID_PANE_MOVE_TO_NOTEBOOK	(wxID_HIGHEST + 4)
#define wxID_TAB_DETACH             (wxID_HIGHEST + 5)
#define wxID_TAB_CLOSE              (wxID_HIGHEST + 6)
#define wxID_TAB_FLOAT              (wxID_HIGHEST + 7)
#define wxID_TAB_DOCK               (wxID_HIGHEST + 8)

#define wxID_PLUGIN_RANGE_START		(wxID_HIGHEST + 1000)
#define wxID_PLUGIN_RANGE_END			(wxID_HIGHEST + NXMC_PLUGIN_ID_LIMIT * NXMC_MAX_PLUGINS - 1)


//
// Plugin class
//

class nxmcPlugin
{
private:
	HMODULE m_moduleHandle;
	NXMC_PLUGIN_INFO *m_info;
	int m_baseId;
	void (*m_fpCommandHandler)(int);

public:
	nxmcPlugin(HMODULE hmod, NXMC_PLUGIN_INFO *info, int baseId, void (*fpCmdHandler)(int));
	~nxmcPlugin();

	const TCHAR *GetName() { return m_info->name; }
	const TCHAR *GetVersion() { return m_info->version; }
	int GetBaseId() { return m_baseId; }

	void CallCommandHandler(int cmd) { if (m_fpCommandHandler != NULL) m_fpCommandHandler(cmd); }
};

WX_DECLARE_OBJARRAY(nxmcPlugin, nxmcArrayOfPlugins);


//
// Application class
//

class nxApp : public wxApp
{
private:
	nxMainFrame *m_mainFrame;

	bool Connect();

public:
	nxApp();

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
void CallPluginCommandHandler(int cmd);


//
// Global variables
//

extern NXC_SESSION g_hSession;
extern DWORD g_appFlags;
extern TCHAR g_userName[];
extern SNMP_MIBObject *g_mibRoot;

#endif

