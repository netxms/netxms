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
#include "tbicon.h"


//
// Application flags
//

#define AF_SAVE_OBJECT_CACHE		0x00000001
#define AF_HIDE_MAIN_MENU        0x00000002
#define AF_FULLSCREEN            0x00000004
#define AF_START_MAXIMIZED       0x00000008
#define AF_AUTOCONNECT           0x00000010
#define AF_OVERRIDE_SERVER       0x00000020
#define AF_OVERRIDE_USERNAME     0x00000040
#define AF_OVERRIDE_PASSWORD     0x00000080
#define AF_HIDE_TABS             0x00000100
#define AF_OPEN_VIEW_ON_START    0x00000200
#define AF_HIDE_STATUS_BAR       0x00000400
#define AF_EMPTY_WORKAREA        0x00000800
#define AF_HIDDEN                0x00001000
#define AF_TASKBAR_ICON          0x00002000


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
#define wxID_TOOL_ATTACH            (wxID_HIGHEST + 9)
#define wxID_TOOL_CLOSE             (wxID_HIGHEST + 10)
#define wxID_CTRLPANEL_SERVERCFG    (wxID_HIGHEST + 11)
#define wxID_TBICON_SHOW_CONSOLE    (wxID_HIGHEST + 12)
#define wxID_TBICON_EXIT            (wxID_HIGHEST + 13)

#define wxID_PERSPECTIVE_START      (wxID_HIGHEST + 100)
#define wxID_PERSPECTIVE_END        (wxID_HIGHEST + 199)

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
	nxTaskBarIcon *m_tbIcon;
	wxString m_acServer;
	wxString m_acUsername;
	wxString m_acPassword;
	wxString m_autoView;

	bool Connect();

public:
	nxApp();

   virtual bool OnInit();
	virtual void OnInitCmdLine(wxCmdLineParser& parser);
	virtual bool OnCmdLineParsed(wxCmdLineParser& parser);
	virtual int OnExit();

	nxMainFrame *GetMainFrame() { return m_mainFrame; }

	nxTaskBarIcon& GetTaskBarIcon() { return *m_tbIcon; }
	void DestroyTaskBarIcon() { delete_and_null(m_tbIcon); }
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

