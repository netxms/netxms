#ifndef _nxmc_api_h_
#define _nxmc_api_h_

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


//
// Integration points
//

#define NXMC_IP_CONTROL_PANEL			0x0001
#define NXMC_IP_MAIN_MENU				0x0002
#define NXMC_IP_PLUGIN_CONTEXT_MENU	0x0004


//
// Export definition
//

#ifdef _WIN32
#define NXMC_PLUGIN_EXPORT __declspec(dllexport)
#else
#define NXMC_PLUGIN_EXPORT
#endif


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
// Menu/Control Panel item registration
//

class nxmcItemRegistration
{
private:
	TCHAR *m_name;
	int m_id;
	int m_type;
	void (*m_fpHandler)(int);

public:
	nxmcItemRegistration(const TCHAR *name, int id, int type, void (*fpHandler)(int));
	~nxmcItemRegistration();

	const TCHAR *GetName(void) { return m_name; }
	int GetId(void) { return m_id; }

	void CallHandler(int param) { m_fpHandler(param); }
};

WX_DECLARE_OBJARRAY(nxmcItemRegistration, nxmcArrayOfRegItems);


//
// Registration item types
//

#define REGITEM_VIEW_MENU        0
#define REGITEM_CONTROL_PANEL    1


//
// Classes
//

#include "../libnxmc/nxview.h"


//
// Functions
//

void LIBNXMC_EXPORTABLE NXMCInitializationComplete(void);

bool LIBNXMC_EXPORTABLE NXMCAddControlPanelItem(const TCHAR *name, int id, void (*fpHandler)(int));
bool LIBNXMC_EXPORTABLE NXMCAddViewMenuItem(const TCHAR *name, int id, void (*fpHandler)(int));

void LIBNXMC_EXPORTABLE InitViewTracker(wxAuiManager *mgr, wxAuiNotebook *nb);
void LIBNXMC_EXPORTABLE RegisterUniqueView(const TCHAR *name, nxView *view);
void LIBNXMC_EXPORTABLE UnregisterUniqueView(const TCHAR *name);
nxView LIBNXMC_EXPORTABLE *FindUniqueView(const TCHAR *name);
void LIBNXMC_EXPORTABLE ActivateView(nxView *view);


#endif
