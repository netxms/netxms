#include "nxmc.h"
#include "conlog.h"


//
// Event table
//

BEGIN_EVENT_TABLE(nxConsoleLogger, nxView)
	EVT_SIZE(nxConsoleLogger::OnSize)
END_EVENT_TABLE()


//
// Static data
//

wxLog *nxConsoleLogger::m_logOld = NULL;
nxDummyFrame *nxConsoleLogger::m_wndDummy = NULL;
wxTextCtrl *nxConsoleLogger::m_wndTextCtrl = NULL;


//
// Globally initialize
//

void nxConsoleLogger::Init()
{
	m_wndDummy = new nxDummyFrame;
	m_wndDummy->Hide();
	m_wndTextCtrl = new wxTextCtrl(m_wndDummy, wxID_ANY, wxEmptyString,
	                               wxDefaultPosition, wxDefaultSize,
	                               wxTE_MULTILINE | wxSUNKEN_BORDER);
	m_logOld = wxLog::SetActiveTarget(new wxLogTextCtrl(m_wndTextCtrl));
	wxLogMessage(_T("Message Logger started"));
}


//
// Global shutdown
//

void nxConsoleLogger::Shutdown()
{
	delete wxLog::SetActiveTarget(m_logOld);
}


//
// Constructor
//

nxConsoleLogger::nxConsoleLogger(wxWindow *parent)
                : nxView(parent)
{
	RegisterUniqueView(_T("conlog"), this);
	m_wndTextCtrl->Reparent(this);
}


//
// Destructor
//

nxConsoleLogger::~nxConsoleLogger()
{
	m_wndTextCtrl->Reparent(m_wndDummy);
	UnregisterUniqueView(_T("conlog"));
}


//
// Resize handler
//

void nxConsoleLogger::OnSize(wxSizeEvent &event)
{
	wxSize size = GetClientSize();
	m_wndTextCtrl->SetSize(0, 0, size.x, size.y);
}
