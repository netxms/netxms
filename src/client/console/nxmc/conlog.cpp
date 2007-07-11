#include "nxmc.h"
#include "conlog.h"


//
// Event table
//

BEGIN_EVENT_TABLE(nxConsoleLogger, nxView)
	EVT_SIZE(nxConsoleLogger::OnSize)
END_EVENT_TABLE()


//
// Constructor
//

nxConsoleLogger::nxConsoleLogger(wxWindow *parent)
                : nxView(parent)
{
	m_wndTextCtrl = new wxTextCtrl(this, wxID_ANY, wxEmptyString,
	                               wxDefaultPosition, wxDefaultSize,
	                               wxTE_MULTILINE | wxSUNKEN_BORDER);
	m_logOld = wxLog::SetActiveTarget(new wxLogTextCtrl(m_wndTextCtrl));
	wxLogMessage(_T("Message Logger Console started"));
	RegisterUniqueView(_T("conlog"), this);
}


//
// Destructor
//

nxConsoleLogger::~nxConsoleLogger()
{
	delete wxLog::SetActiveTarget(m_logOld);
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
