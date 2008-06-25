#ifndef _nxmc_api_h_
#define _nxmc_api_h_

#pragma warning(disable: 4284)

#define WXUSINGDLL

#ifdef _WIN32
#include <wx/msw/winundef.h>
#endif

#include <wx/wx.h>

#ifndef WX_PRECOMP
#include <wx/app.h>
#include <wx/frame.h>
#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/filesys.h>
#include <wx/fs_arc.h>
#include <wx/fs_mem.h>
#include <wx/aui/aui.h>
#include <wx/dir.h>
#include <wx/config.h>
#include <wx/stdpaths.h>
#include <wx/dynarray.h>
#include <wx/splitter.h>
#include <wx/treectrl.h>
#include <wx/listctrl.h>
#include <wx/notebook.h>
#include <wx/imaglist.h>
#include <wx/aboutdlg.h>
#include <wx/cmdline.h>
#include <wx/taskbar.h>
#include <wx/xml/xml.h>
#endif


#ifdef _WIN32
#ifdef LIBNXMC_EXPORTS
#define LIBNXMC_EXPORTABLE __declspec(dllexport)
#else
#define LIBNXMC_EXPORTABLE __declspec(dllimport)
#endif
#else
#define LIBNXMC_EXPORTABLE
#endif


//
// Constants
//

#define MAX_PLUGIN_NAME_LEN      256
#define MAX_PLUGIN_VERSION_LEN   64
#define NXMC_PLUGIN_ID_LIMIT     100
#define NXMC_MAX_PLUGINS         100


//
// Integration points
//

#define NXMC_IP_CONTROL_PANEL           0x0001
#define NXMC_IP_MAIN_MENU               0x0002
#define NXMC_IP_PLUGIN_CONTEXT_MENU     0x0004


//
// Image lists
//

#define IMAGE_LIST_OBJECTS_SMALL    1
#define IMAGE_LIST_OBJECTS_NORMAL   2
#define IMAGE_LIST_STATUS_SMALL     3


//
// Timestamp formats
//

#define TS_LONG_DATE_TIME  0
#define TS_LONG_TIME       1
#define TS_DAY_AND_MONTH   2
#define TS_MONTH           3


//
// Export definition
//

#ifdef _WIN32
#define NXMC_PLUGIN_EXPORT __declspec(dllexport)
#else
#define NXMC_PLUGIN_EXPORT
#endif


//
// Windows/UNIX compatibility defines
//

#ifdef _WIN32
#define NXMC_LIB_INSTANCE          HINSTANCE
#define NXMC_LIB_INSTANCE_ARG(x)   (x)
#define wxMAKEINTRESOURCE(x)       MAKEINTRESOURCE(x)
#else
#define NXMC_LIB_INSTANCE          int
#define NXMC_LIB_INSTANCE_ARG(x)   0
#define wxMAKEINTRESOURCE(x)       NULL
#endif


//
// Plugin handle
//

typedef void * NXMC_PLUGIN_HANDLE;


//
// Macros for declaring plugin entry point
//

#define NXMC_IMPLEMENT_PLUGIN_REGISTRATION(n,v,f) \
	extern "C" void NXMC_PLUGIN_EXPORT nxmcRegisterPlugin(NXMC_PLUGIN_INFO *p) \
	{ \
		nx_strncpy(p->name, n, MAX_PLUGIN_NAME_LEN); \
		nx_strncpy(p->version, v, MAX_PLUGIN_VERSION_LEN); \
		p->flags = f; \
	}


//
// Plugin registration structure
//

typedef struct
{
	TCHAR name[MAX_PLUGIN_NAME_LEN];
	TCHAR version[MAX_PLUGIN_VERSION_LEN];
	DWORD flags;
} NXMC_PLUGIN_INFO;


//
// Data exchange macros
//

#define DX_GET_TEXTCTRL(id,var) \
	{ \
		var = XRCCTRL(*this,id,wxTextCtrl)->GetValue(); \
	}
#define DX_SET_TEXTCTRL(id,var) \
	{ \
		XRCCTRL(*this,id,wxTextCtrl)->SetValue(var); \
	}

#define DX_GET_CHECKBOX(id,var) \
	{ \
		var = XRCCTRL(*this,id,wxCheckBox)->IsChecked(); \
	}
#define DX_SET_CHECKBOX(id,var) \
	{ \
		XRCCTRL(*this,id,wxCheckBox)->SetValue(var); \
	}

#define DX_GET_STATIC(id,var) \
	{ \
		var = XRCCTRL(*this,id,wxStaticText)->GetLabel(); \
	}
#define DX_SET_STATIC(id,var) \
	{ \
		XRCCTRL(*this,id,wxStaticText)->SetLabel(var); \
	}

#define DX_GET_COMBOBOX(id,var) \
	{ \
		var = XRCCTRL(*this,id,wxComboBox)->GetValue(); \
	}
#define DX_SET_COMBOBOX(id,var) \
	{ \
		XRCCTRL(*this,id,wxComboBox)->SetValue(var); \
	}


//
// Additional event table macros
//

#define EVT_NC_RIGHT_DOWN(func) wx__DECLARE_EVT0(wxEVT_NC_RIGHT_DOWN, wxMouseEventHandler(func))
#define EVT_NC_RIGHT_UP(func) wx__DECLARE_EVT0(wxEVT_NC_RIGHT_UP, wxMouseEventHandler(func))


//
// Predefined IDs
//

#define wxID_TREE_CTRL		(wxID_HIGHEST + 500)
#define wxID_NOTEBOOK_CTRL	(wxID_HIGHEST + 501)
#define wxID_LIST_CTRL		(wxID_HIGHEST + 502)
#define wxID_TEXT_CTRL		(wxID_HIGHEST + 503)


//
// Custom events
//

BEGIN_DECLARE_EVENT_TYPES()
    DECLARE_EXPORTED_EVENT_TYPE(LIBNXMC_EXPORTABLE, nxEVT_REQUEST_COMPLETED, 0)
    DECLARE_EXPORTED_EVENT_TYPE(LIBNXMC_EXPORTABLE, nxEVT_REFRESH_VIEW, 0)
    DECLARE_EXPORTED_EVENT_TYPE(LIBNXMC_EXPORTABLE, nxEVT_NXC_ALARM_CHANGE, 0)
    DECLARE_EXPORTED_EVENT_TYPE(LIBNXMC_EXPORTABLE, nxEVT_NXC_OBJECT_CHANGE, 0)
END_DECLARE_EVENT_TYPES()

#define EVT_LIBNXMC_EVENT(evt, fn) \
    DECLARE_EVENT_TABLE_ENTRY( \
        evt, wxID_ANY, wxID_ANY, \
                  (wxObjectEventFunction)(wxEventFunction) wxStaticCastEvent(wxCommandEventFunction, &fn), \
        (wxObject *) NULL \
    ),
#define EVT_NX_REQUEST_COMPLETED(fn)	EVT_LIBNXMC_EVENT(nxEVT_REQUEST_COMPLETED, fn)
#define EVT_NX_REFRESH_VIEW(fn)			EVT_LIBNXMC_EVENT(nxEVT_REFRESH_VIEW, fn)
#define EVT_NXC_ALARM_CHANGE(fn)			EVT_LIBNXMC_EVENT(nxEVT_NXC_ALARM_CHANGE, fn)
#define EVT_NXC_OBJECT_CHANGE(fn)		EVT_LIBNXMC_EVENT(nxEVT_NXC_OBJECT_CHANGE, fn)


//
// Menu/Control Panel item registration
//

class nxmcItemRegistration
{
private:
	NXMC_PLUGIN_HANDLE m_plugin;
	TCHAR *m_name;
	int m_id;
	int m_type;
	void (*m_fpHandler)(int);
	const wxIcon *m_icon;

public:
	nxmcItemRegistration(NXMC_PLUGIN_HANDLE plugin, const TCHAR *name, int id,
	                     int type, const wxIcon &icon, void (*fpHandler)(int));
	~nxmcItemRegistration();

	const TCHAR *GetName() { return m_name; }
	int GetType() { return m_type; }
	int GetId() { return m_id; }
	const wxIcon GetIcon() { return *m_icon; }
	NXMC_PLUGIN_HANDLE GetPlugin() { return m_plugin; }

	void CallHandler(int param) { m_fpHandler(param); }
};

WX_DECLARE_OBJARRAY(nxmcItemRegistration, nxmcArrayOfRegItems);


//
// Registration item types
//

#define REGITEM_VIEW_MENU        0
#define REGITEM_CONTROL_PANEL    1


//
// View creation areas
//

#define VIEWAREA_MAIN            0
#define VIEWAREA_DOCKED          1
#define VIEWAREA_FLOATING        2
#define VIEWAREA_DETACHED        3
#define VIEWAREA_DOCKED_LEFT     4
#define VIEWAREA_DOCKED_RIGHT    5
#define VIEWAREA_DOCKED_TOP      6
#define VIEWAREA_DOCKED_BOTTOM   7


//
// libnxcl object index structure
//

struct NXC_OBJECT_INDEX
{
   DWORD key;
   NXC_OBJECT *object;
};


//
// Additional array and hash types
//

WX_DECLARE_HASH_MAP(DWORD, NXC_ALARM*, wxIntegerHash, wxIntegerEqual, nxAlarmList);
WX_DECLARE_HASH_MAP_WITH_DECL(int, TCHAR*, wxIntegerHash, wxIntegerEqual, nxIntToStringHash, class LIBNXMC_EXPORTABLE);


//
// Sorted array of DWORDs
//

int LIBNXMC_EXPORTABLE CompareDwords(DWORD, DWORD);

#ifdef _WIN32
WX_DEFINE_SORTED_ARRAY_LONG(DWORD, DWORD_Array);
#else
#if (SIZEOF_LONG == 4)
WX_DEFINE_SORTED_ARRAY_LONG(DWORD, DWORD_Array);
#else
WX_DEFINE_SORTED_ARRAY_INT(DWORD, DWORD_Array);
#endif
#endif


//
// Classes
//

#include "../libnxmc/nxview.h"
#include "../libnxmc/heading.h"
#include "../libnxmc/graph.h"
#include "../libnxmc/objseldlg.h"


//
// View creator callback type
//

typedef nxView* (*nxViewCreator)(wxWindow *, const TCHAR *, NXC_OBJECT *, void *);


//
// Functions
//

void LIBNXMC_EXPORTABLE NXMCInitAUI(wxAuiManager *mgr, wxAuiNotebook *nb, wxWindow *defParent);
void LIBNXMC_EXPORTABLE NXMCSetMainFrame(wxFrame *frame);
void LIBNXMC_EXPORTABLE NXMCInitializationComplete();

nxmcArrayOfRegItems LIBNXMC_EXPORTABLE &NXMCGetRegistrations();

void LIBNXMC_EXPORTABLE NXMCShowClientError(DWORD rcc, const TCHAR *msgTemplate);

bool LIBNXMC_EXPORTABLE NXMCLoadResources(const TCHAR *name, NXMC_LIB_INSTANCE instance, TCHAR *resName);

bool LIBNXMC_EXPORTABLE NXMCAddControlPanelItem(NXMC_PLUGIN_HANDLE handle, const TCHAR *name, int id, const wxIcon &icon = wxNullIcon);
bool LIBNXMC_EXPORTABLE NXMCAddViewMenuItem(NXMC_PLUGIN_HANDLE handle, const TCHAR *name, int id);

wxWindow LIBNXMC_EXPORTABLE *NXMCGetDefaultParent();
bool LIBNXMC_EXPORTABLE NXMCCreateView(nxView *view, int area);

void LIBNXMC_EXPORTABLE NXMCSetSession(NXC_SESSION session);
NXC_SESSION LIBNXMC_EXPORTABLE NXMCGetSession();

void LIBNXMC_EXPORTABLE InitViewTracker(wxAuiManager *mgr, wxAuiNotebook *nb);
void LIBNXMC_EXPORTABLE RegisterUniqueView(const TCHAR *name, nxView *view);
void LIBNXMC_EXPORTABLE UnregisterUniqueView(const TCHAR *name);
nxView LIBNXMC_EXPORTABLE *FindUniqueView(const TCHAR *name);
void LIBNXMC_EXPORTABLE ActivateView(nxView *view);

void LIBNXMC_EXPORTABLE NXMCRegisterViewCreator(const TCHAR *viewClass, nxViewCreator func);
nxView LIBNXMC_EXPORTABLE *NXMCCreateViewByClass(const TCHAR *viewClass, wxWindow *parent,
                                                 const TCHAR *context, NXC_OBJECT *object,
                                                 void *userData);

const TCHAR LIBNXMC_EXPORTABLE *NXMCCodeToText(int code, CODE_TO_TEXT *translator, const TCHAR *defaultText);
const TCHAR LIBNXMC_EXPORTABLE *NXMCGetStatusText(int status);
const TCHAR LIBNXMC_EXPORTABLE *NXMCGetStatusTextSmall(int status);
const TCHAR LIBNXMC_EXPORTABLE *NXMCGetAlarmStateName(int state);
const TCHAR LIBNXMC_EXPORTABLE *NXMCGetClassName(int objClass);
const TCHAR LIBNXMC_EXPORTABLE *NXMCGetIfTypeName(int type);
const TCHAR LIBNXMC_EXPORTABLE *NXMCGetNodeTypeName(int type);
const wxColour LIBNXMC_EXPORTABLE &NXMCGetStatusColor(int status);

void LIBNXMC_EXPORTABLE NXMCInitImageLists();
wxImageList LIBNXMC_EXPORTABLE *NXMCGetImageList(int list);
wxImageList LIBNXMC_EXPORTABLE *NXMCGetImageListCopy(int list);

void LIBNXMC_EXPORTABLE NXMCInitAlarms(DWORD count, NXC_ALARM *list);
void LIBNXMC_EXPORTABLE NXMCUpdateAlarms(DWORD code, NXC_ALARM *data);
nxAlarmList LIBNXMC_EXPORTABLE *NXMCGetAlarmList();
void LIBNXMC_EXPORTABLE NXMCUnlockAlarmList();

TCHAR LIBNXMC_EXPORTABLE *NXMCFormatTimeStamp(time_t timeStamp, TCHAR *buffer, int type);

void LIBNXMC_EXPORTABLE NXMCSaveListCtrlColumns(wxConfigBase *cfg, wxListCtrl &wndListCtrl, const TCHAR *prefix);
void LIBNXMC_EXPORTABLE NXMCLoadListCtrlColumns(wxConfigBase *cfg, wxListCtrl &wndListCtrl, const TCHAR *prefix);
void LIBNXMC_EXPORTABLE NXMCAdjustListColumn(wxListCtrl *listCtrl, int col);

void LIBNXMC_EXPORTABLE NXMCSetMainEventHandler(wxEvtHandler *evtHandler);
void LIBNXMC_EXPORTABLE NXMCEvtConnect(wxEventType eventType, wxObjectEventFunction func, wxEvtHandler *sink);
void LIBNXMC_EXPORTABLE NXMCEvtDisconnect(wxEventType eventType, wxObjectEventFunction func, wxEvtHandler *sink);

#endif
