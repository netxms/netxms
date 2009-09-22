// ObjectPropsGeolocation.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ObjectPropsGeolocation.h"
#include "ObjectPropSheet.h"
#include <geolocation.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CObjectPropsGeolocation property page

IMPLEMENT_DYNCREATE(CObjectPropsGeolocation, CPropertyPage)

CObjectPropsGeolocation::CObjectPropsGeolocation() : CPropertyPage(CObjectPropsGeolocation::IDD)
{
	//{{AFX_DATA_INIT(CObjectPropsGeolocation)
	m_locationType = -1;
	m_strLatitude = _T("");
	m_strLongitude = _T("");
	//}}AFX_DATA_INIT
}

CObjectPropsGeolocation::~CObjectPropsGeolocation()
{
}

void CObjectPropsGeolocation::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CObjectPropsGeolocation)
	DDX_Radio(pDX, IDC_RADIO_UNDEFINED, m_locationType);
	DDX_Text(pDX, IDC_EDIT_LATITUDE, m_strLatitude);
	DDV_MaxChars(pDX, m_strLatitude, 32);
	DDX_Text(pDX, IDC_EDIT_LONGITUDE, m_strLongitude);
	DDV_MaxChars(pDX, m_strLongitude, 32);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CObjectPropsGeolocation, CPropertyPage)
	//{{AFX_MSG_MAP(CObjectPropsGeolocation)
	ON_BN_CLICKED(IDC_RADIO_GPS, OnRadioGps)
	ON_BN_CLICKED(IDC_RADIO_MANUAL, OnRadioManual)
	ON_BN_CLICKED(IDC_RADIO_UNDEFINED, OnRadioUndefined)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CObjectPropsGeolocation message handlers

BOOL CObjectPropsGeolocation::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	if (m_locationType != GL_MANUAL)
	{
		EnableDlgItem(this, IDC_EDIT_LATITUDE, FALSE);
		EnableDlgItem(this, IDC_EDIT_LONGITUDE, FALSE);
	}

	m_pUpdate = ((CObjectPropSheet *)GetParent())->GetUpdateStruct();

	return TRUE;
}

void CObjectPropsGeolocation::OnRadioGps() 
{
	EnableDlgItem(this, IDC_EDIT_LATITUDE, FALSE);
	EnableDlgItem(this, IDC_EDIT_LONGITUDE, FALSE);
}

void CObjectPropsGeolocation::OnRadioManual() 
{
	EnableDlgItem(this, IDC_EDIT_LATITUDE, TRUE);
	EnableDlgItem(this, IDC_EDIT_LONGITUDE, TRUE);
}

void CObjectPropsGeolocation::OnRadioUndefined() 
{
	EnableDlgItem(this, IDC_EDIT_LATITUDE, FALSE);
	EnableDlgItem(this, IDC_EDIT_LONGITUDE, FALSE);
}


//
// OK button handler
//

void CObjectPropsGeolocation::OnOK() 
{
	GeoLocation loc;

	// Validate latitude and longitude
	if (SendDlgItemMessage(IDC_RADIO_MANUAL, BM_GETCHECK) == BST_CHECKED)
	{
		TCHAR latitude[64], longitude[64];

		GetDlgItemText(IDC_EDIT_LATITUDE, latitude, 64);
		GetDlgItemText(IDC_EDIT_LONGITUDE, longitude, 64);
		loc = GeoLocation(GL_MANUAL, latitude, longitude);
		if (!loc.isValid())
		{
			MessageBox(_T("Invalid coordinates entered"), _T("Warning"), MB_OK | MB_ICONEXCLAMATION);
			return;
		}
	}

	CPropertyPage::OnOK();

	m_pUpdate->qwFlags |= OBJ_UPDATE_GEOLOCATION;
	m_pUpdate->geolocation.type = m_locationType;
	m_pUpdate->geolocation.latitude = loc.getLatitude();
	m_pUpdate->geolocation.longitude = loc.getLongitude();
}
