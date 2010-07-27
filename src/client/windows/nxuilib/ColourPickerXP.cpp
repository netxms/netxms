// CColourPickerXP & CColourPopupXP version 1.4
//
// Copyright © 2002-2003 Zorglab
//
//   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//   !! You are not allowed to use these classes in a commercial project !!
//   !!              without the permission of the author !              !!
//   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
// Feel free to remove or otherwise mangle any part.
// Please report any bug, comment, suggestion, etc. to the following address :
//   mailto:zorglab@wanadoo.be
//
// These classes are based on work by Chris Maunder, Alexander Bischofberger,
// James White and Descartes Systems Sciences, Inc.
//   http://www.codeproject.com/miscctrl/colour_picker.asp
//   http://www.codeproject.com/miscctrl/colorbutton.asp
//   http://www.codeproject.com/wtl/wtlcolorbutton.asp
//
// Thanks to Keith Rule for his CMemDC class (see MemDC.h).
// Thanks to Pål Kristian Tønder for his CXPTheme class, which is based on
// the CVisualStyleXP class of David Yuheng Zhao (see XPTheme.cpp).
//
// Other people who have contributed to the improvement of the ColourPickerXP,
// by sending a bug report, by solving a bug, by submitting a suggestion, etc.
// are mentioned in the history-section (see below).
//
// Many thanks to them all.
//
//                             === HISTORY ===
//
//	version 1.4		- fixed : "A required resource was"-dialog due to not
//					  restoring the DC after drawing pop-up (thanks to
//					  Kris Wojtas, KRI Software)
//					- using old style selection rectangle in pop-up when
//					  flat menus are disabled
//					- pop-up will now raise when user hit F4-key or down-arrow
//					- modified : moving around in the pop-up with arrow keys
//					  when no colour is selected
//
//	version 1.3		- parent window stays active when popup is up on screen
//					  (thanks to Damir Valiulin)
//					- when using track-selection, the initial colour is shown
//					  for an invalid selection instead of black
//					- added bTranslateDefault parameter in GetColor
//
//	version 1.2		- fixed : in release configuration, with neither
//					  'Automatic' nor 'Custom' labels, the pop-up won't work
//					- diasbled combo-box is drawn correctly
//					- combo-box height depends on text size
//					- font support : use SetFont() and GetFont(), for combo-
//					  box style call SetStyle() after changing font
//
//	version 1.1		- fixed some compile errors in VC6
//					- no need anymore to change the defines in stdafx.h
//					  except for multi-monitor support
//
//	version 1.0		first release
//
//                  === ORIGINAL COPYRIGHT STATEMENTS ===
//
// ------------------- Descartes Systems Sciences, Inc. --------------------
//
// Copyright (c) 2000-2002 - Descartes Systems Sciences, Inc.
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without 
// modification, are permitted provided that the following conditions are 
// met:
// 
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer. 
// 2. Neither the name of Descartes Systems Sciences, Inc nor the names of 
//    its contributors may be used to endorse or promote products derived 
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED 
// TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// ---------------- Chris Maunder & Alexander Bischofberger ----------------
//
// Written by Chris Maunder (chrismaunder@codeguru.com)
// Extended by Alexander Bischofberger (bischofb@informatik.tu-muenchen.de)
// Copyright (c) 1998.
//
// Updated 30 May 1998 to allow any number of colours, and to
//                     make the appearance closer to Office 97. 
//                     Also added "Default" text area.         (CJM)
//
//         13 June 1998 Fixed change of focus bug (CJM)
//         30 June 1998 Fixed bug caused by focus bug fix (D'oh!!)
//                      Solution suggested by Paul Wilkerson.
//
// ColourPopup is a helper class for the colour picker control
// CColourPicker. Check out the header file or the accompanying 
// HTML doc file for details.
//
// This code may be used in compiled form in any way you desire. This
// file may be redistributed unmodified by any means PROVIDING it is 
// not sold for profit without the authors written consent, and 
// providing that this notice and the authors name is included. 
//
// This file is provided "as is" with no expressed or implied warranty.
// The author accepts no liability if it causes any damage to you or your
// computer whatsoever. It's free, so don't hassle me about it.
//
// Expect bugs.
// 
// Please use and enjoy. Please let me know of any bugs/mods/improvements 
// that you have found/implemented and I will fix/incorporate them into this
// file. 
//
// -------------------------------------------------------------------------
//
//                   === END OF COPYRIGHT STATEMENTS ===
//

#include "stdafx.h"
#include "nxuilib.h"
#include "ColourPickerXP.h"

#include "MemDC.h"

#include <math.h>


//***********************************************************************
//**                         MFC Debug Symbols                         **
//***********************************************************************
#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//***********************************************************************
//**               Defines and init for CColourPickerXP                **
//***********************************************************************

#define COMBOBOX_DISABLED_COLOUR_OUT RGB(201,199,186)
#define COMBOBOX_DISABLED_COLOUR_IN  RGB(245,244,234)

//***********************************************************************
//**               Defines and init for CColourPopupXP                 **
//***********************************************************************

#define DEFAULT_BOX_VALUE -3
#define CUSTOM_BOX_VALUE  -2
#define INVALID_COLOUR    -1

#define MAX_COLOURS      100

#ifndef SPI_GETFLATMENU
#define SPI_GETFLATMENU                     0x1022
#endif

#ifndef SPI_GETDROPSHADOW
#define SPI_GETDROPSHADOW                   0x1024
#endif

#ifndef CS_DROPSHADOW
#define CS_DROPSHADOW       0x00020000
#endif

#ifndef ODS_HOTLIGHT
#define ODS_HOTLIGHT        0x0040
#endif

#ifndef COLOR_MENUHILIGHT
#define COLOR_MENUHILIGHT       29
#endif


TCHAR CColourPopupXP::m_strInitNames[][256] = 
{
    { _T("Black")             },
    { _T("Brown")             },
    { _T("Dark Olive Green")  },
    { _T("Dark Green")        },
    { _T("Dark Teal")         },
    { _T("Dark blue")         },
    { _T("Indigo")            },
    { _T("Dark grey")         },

    { _T("Dark red")          },
    { _T("Orange")            },
    { _T("Dark yellow")       },
    { _T("Green")             },
    { _T("Teal")              },
    { _T("Blue")              },
    { _T("Blue-grey")         },
    { _T("Grey - 40")         },

    { _T("Red")               },
    { _T("Light orange")      },
    { _T("Lime")              }, 
    { _T("Sea green")         },
    { _T("Aqua")              },
    { _T("Light blue")        },
    { _T("Violet")            },
    { _T("Grey - 50")         },

    { _T("Pink")              },
    { _T("Gold")              },
    { _T("Yellow")            },    
    { _T("Bright green")      },
    { _T("Turquoise")         },
    { _T("Skyblue")           },
    { _T("Plum")              },
    { _T("Light grey")        },

    { _T("Rose")              },
    { _T("Tan")               },
    { _T("Light yellow")      },
    { _T("Pale green ")       },
    { _T("Pale turquoise")    },
    { _T("Pale blue")         },
    { _T("Lavender")          },
    { _T("White")             }
};

ColourTableEntry CColourPopupXP::m_crColours[] = 
{
    { RGB(0x00, 0x00, 0x00),    _T("Black")             },
    { RGB(0xA5, 0x2A, 0x00),    _T("Brown")             },
    { RGB(0x00, 0x40, 0x40),    _T("Dark Olive Green")  },
    { RGB(0x00, 0x55, 0x00),    _T("Dark Green")        },
    { RGB(0x00, 0x00, 0x5E),    _T("Dark Teal")         },
    { RGB(0x00, 0x00, 0x8B),    _T("Dark blue")         },
    { RGB(0x4B, 0x00, 0x82),    _T("Indigo")            },
    { RGB(0x28, 0x28, 0x28),    _T("Dark grey")         },

    { RGB(0x8B, 0x00, 0x00),    _T("Dark red")          },
    { RGB(0xFF, 0x68, 0x20),    _T("Orange")            },
    { RGB(0x8B, 0x8B, 0x00),    _T("Dark yellow")       },
    { RGB(0x00, 0x93, 0x00),    _T("Green")             },
    { RGB(0x38, 0x8E, 0x8E),    _T("Teal")              },
    { RGB(0x00, 0x00, 0xFF),    _T("Blue")              },
    { RGB(0x7B, 0x7B, 0xC0),    _T("Blue-grey")         },
    { RGB(0x66, 0x66, 0x66),    _T("Grey - 40")         },

    { RGB(0xFF, 0x00, 0x00),    _T("Red")               },
    { RGB(0xFF, 0xAD, 0x5B),    _T("Light orange")      },
    { RGB(0x32, 0xCD, 0x32),    _T("Lime")              }, 
    { RGB(0x3C, 0xB3, 0x71),    _T("Sea green")         },
    { RGB(0x7F, 0xFF, 0xD4),    _T("Aqua")              },
    { RGB(0x7D, 0x9E, 0xC0),    _T("Light blue")        },
    { RGB(0x80, 0x00, 0x80),    _T("Violet")            },
    { RGB(0x80, 0x80, 0x80),    _T("Grey - 50")         },

    { RGB(0xFF, 0xC0, 0xCB),    _T("Pink")              },
    { RGB(0xFF, 0xD7, 0x00),    _T("Gold")              },
    { RGB(0xFF, 0xFF, 0x00),    _T("Yellow")            },    
    { RGB(0x00, 0xFF, 0x00),    _T("Bright green")      },
    { RGB(0x40, 0xE0, 0xD0),    _T("Turquoise")         },
    { RGB(0xC0, 0xFF, 0xFF),    _T("Skyblue")           },
    { RGB(0x48, 0x00, 0x48),    _T("Plum")              },
    { RGB(0xC0, 0xC0, 0xC0),    _T("Light grey")        },

    { RGB(0xFF, 0xE4, 0xE1),    _T("Rose")              },
    { RGB(0xD2, 0xB4, 0x8C),    _T("Tan")               },
    { RGB(0xFF, 0xFF, 0xE0),    _T("Light yellow")      },
    { RGB(0x98, 0xFB, 0x98),    _T("Pale green ")       },
    { RGB(0xAF, 0xEE, 0xEE),    _T("Pale turquoise")    },
    { RGB(0x68, 0x83, 0x8B),    _T("Pale blue")         },
    { RGB(0xE6, 0xE6, 0xFA),    _T("Lavender")          },
    { RGB(0xFF, 0xFF, 0xFF),    _T("White")             }
};

int CColourPopupXP::m_nNumColours = sizeof(CColourPopupXP::m_crColours)/sizeof(ColourTableEntry);

CString CColourPickerXP::m_strRegSectionStatic(_T(""));

//***********************************************************************
//**                            DDX Method                             **
//***********************************************************************

void NXUILIB_EXPORTABLE AFXAPI DDX_ColourPickerXP(CDataExchange *pDX, int nIDC, COLORREF& crColour)
{
    HWND hWndCtrl = pDX->PrepareCtrl(nIDC);
    ASSERT (hWndCtrl != NULL);                
    
    CColourPickerXP* pColourButton = (CColourPickerXP*) CWnd::FromHandle(hWndCtrl);
    if (pDX->m_bSaveAndValidate)
    {
		crColour = pColourButton->Color;
    }
    else // initializing
    {
		pColourButton->Color = crColour;
    }
}

//***********************************************************************
//**                             Constants                             **
//***********************************************************************
const int g_ciArrowSizeX = 4 ;
const int g_ciArrowSizeY = 2 ;

//***********************************************************************
//**                            MFC Macros                            **
//***********************************************************************
IMPLEMENT_DYNCREATE(CColourPickerXP, CButton)

//***********************************************************************
// Method:	CColourPickerXP::CColourPickerXP(void)
// Notes:	Default Constructor.
//***********************************************************************
CColourPickerXP::CColourPickerXP(void):
	_Inherited(),
	m_Color(CLR_DEFAULT),
	m_DefaultColor(::GetSysColor(COLOR_APPWORKSPACE)),
	m_strDefaultText(_T("Automatic")),
	m_strCustomText(_T("More Colors...")),
	m_bPopupActive(FALSE),
	m_bTrackSelection(FALSE),
	m_bMouseOver(FALSE),
	m_strRegSection(_T("")),
	m_bComboBoxStyle(FALSE),
	m_strRGBText(_T("RGB")),
	m_bAlwaysRGB(FALSE)
{
	OSVERSIONINFO osvi;
	osvi.dwOSVersionInfoSize = sizeof (osvi);
	::GetVersionEx (&osvi);
	bool bIsXP = osvi.dwPlatformId == VER_PLATFORM_WIN32_NT &&
		(osvi.dwMajorVersion > 5 || (osvi.dwMajorVersion == 5 &&
		osvi.dwMinorVersion >= 1));

	m_bFlatMenus = FALSE;
 	if (bIsXP)
		::SystemParametersInfo (SPI_GETFLATMENU, 0, &m_bFlatMenus, FALSE);
}

//***********************************************************************
// Method:	CColourPickerXP::~CColourPickerXP(void)
// Notes:	Destructor.
//***********************************************************************
CColourPickerXP::~CColourPickerXP(void)
{
}

//***********************************************************************
// Methods:	CColourPickerXP::GetColor()
//			CColourPickerXP::GetColor(BOOL bTranslateDefault)
// Notes:	None.
//***********************************************************************
COLORREF CColourPickerXP::GetColor(void) const
{
	return m_Color;
}

COLORREF CColourPickerXP::GetColor(BOOL bTranslateDefault) const
{
	return ( bTranslateDefault && m_Color == CLR_DEFAULT ? m_DefaultColor : m_Color );
}


//***********************************************************************
// Method:	CColourPickerXP::SetColor()
// Notes:	None.
//***********************************************************************
void CColourPickerXP::SetColor(COLORREF Color)
{
	m_Color = Color;

	if (::IsWindow(m_hWnd)) 
	{
        Invalidate(FALSE);
	}
}


//***********************************************************************
// Method:	CColourPickerXP::GetDefaultColor()
// Notes:	None.
//***********************************************************************
COLORREF CColourPickerXP::GetDefaultColor(void) const
{
	return m_DefaultColor;
}

//***********************************************************************
// Method:	CColourPickerXP::SetDefaultColor()
// Notes:	None.
//***********************************************************************
void CColourPickerXP::SetDefaultColor(COLORREF Color)
{
	m_DefaultColor = Color;
}

//***********************************************************************
// Method:	CColourPickerXP::SetCustomText()
// Notes:	None.
//***********************************************************************
void CColourPickerXP::SetCustomText(LPCTSTR tszText)
{
	m_strCustomText = tszText;
}

//***********************************************************************
// Method:	CColourPickerXP::SetDefaultText()
// Notes:	None.
//***********************************************************************
void CColourPickerXP::SetDefaultText(LPCTSTR tszText)
{
	m_strDefaultText = tszText;
	if(m_bComboBoxStyle && m_Color==CLR_DEFAULT)
		Invalidate(FALSE);
}

//***********************************************************************
// Method:	CColourPickerXP::SetColoursName()
// Notes:	None.
//***********************************************************************
void CColourPickerXP::SetColoursName(UINT nFirstID)
{
	if(nFirstID)
	{
		CString szTemp;
		for(int i=0; i<(sizeof(CColourPopupXP::m_crColours)/sizeof(ColourTableEntry)); i++)
		{
			if(!szTemp.LoadString(nFirstID + i))
				szTemp = CColourPopupXP::m_strInitNames[i];
			_tcscpy(CColourPopupXP::m_crColours[i].szName, szTemp);
		}
	}
	else
	{
		for(int i=0; i<(sizeof(CColourPopupXP::m_crColours)/sizeof(ColourTableEntry)); i++)
			_tcscpy(CColourPopupXP::m_crColours[i].szName, CColourPopupXP::m_strInitNames[i]);
	}
}

//***********************************************************************
// Method:	CColourPickerXP::SetRegSection()
// Notes:	None.
//***********************************************************************
void CColourPickerXP::SetRegSection(LPCTSTR tszRegSection /*= _T("")*/)
{
	m_strRegSection = tszRegSection;
}

//***********************************************************************
// Method:	CColourPickerXP::SetRegSectionStatic()
// Notes:	None.
//***********************************************************************
void CColourPickerXP::SetRegSectionStatic(LPCTSTR tszRegSection /*= _T("")*/)
{
	m_strRegSectionStatic = tszRegSection;
}

//***********************************************************************
// Method:	CColourPickerXP::SetTrackSelection()
// Notes:	None.
//***********************************************************************
void CColourPickerXP::SetTrackSelection(BOOL bTrack)
{
	m_bTrackSelection = bTrack;
}

//***********************************************************************
// Method:	CColourPickerXP::GetTrackSelection()
// Notes:	None.
//***********************************************************************
BOOL CColourPickerXP::GetTrackSelection(void) const
{
	return m_bTrackSelection;
}

//***********************************************************************
// Method:	CColourPickerXP::SetStyle()
// Notes:	None.
//***********************************************************************
void CColourPickerXP::SetStyle(BOOL bComboBoxStyle)
{
	m_bComboBoxStyle = bComboBoxStyle;
	if(bComboBoxStyle)
	{
		CRect rc;
		GetWindowRect(rc);

		CDC *pDC = GetDC();
		CFont *pOldFont = pDC->SelectObject(GetFont());

		TEXTMETRIC tm;
		pDC->GetTextMetrics(&tm);

		pDC->SelectObject(pOldFont);
		ReleaseDC(pDC);

		SetWindowPos(NULL, 0, 0, rc.Width(), tm.tmHeight + 8, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE | SWP_NOREDRAW);
	}
	Invalidate(FALSE);
}

//***********************************************************************
// Method:	CColourPickerXP::GetStyle()
// Notes:	None.
//***********************************************************************
BOOL CColourPickerXP::GetStyle(void) const
{
	return m_bComboBoxStyle;
}

//***********************************************************************
// Method:	CColourPickerXP::SetRGBText()
// Notes:	None.
//***********************************************************************
void CColourPickerXP::SetRGBText(LPCTSTR tszRGB /*= _T("RGB")*/)
{
	m_strRGBText = tszRGB;
	while(m_strRGBText.GetLength() < 3)
		m_strRGBText += _T(" ");
	if(m_bComboBoxStyle && m_Color!=CLR_DEFAULT)
		Invalidate(FALSE);
}

//***********************************************************************
// Method:	CColourPickerXP::SetAlwaysRGB()
// Notes:	None.
//***********************************************************************
void CColourPickerXP::SetAlwaysRGB(BOOL bShow)
{
	m_bAlwaysRGB = bShow;
	if(m_bComboBoxStyle && m_Color!=CLR_DEFAULT)
		Invalidate(FALSE);
}

//***********************************************************************
// Method:	CColourPickerXP::GetAlwaysRGB()
// Notes:	None.
//***********************************************************************
BOOL CColourPickerXP::GetAlwaysRGB(void) const
{
	return m_bAlwaysRGB;
}

//***********************************************************************
//**                         CButton Overrides                         **
//***********************************************************************
void CColourPickerXP::PreSubclassWindow() 
{
    ModifyStyle(0, BS_OWNERDRAW);      

    _Inherited::PreSubclassWindow();
}

//***********************************************************************
//**                         Message Handlers                         **
//***********************************************************************
BEGIN_MESSAGE_MAP(CColourPickerXP, CButton)
    //{{AFX_MSG_MAP(CColourPickerXP)
    ON_WM_CREATE()
	ON_WM_MOUSEMOVE()
    //}}AFX_MSG_MAP
    ON_CONTROL_REFLECT_EX(BN_CLICKED, OnClicked)
    ON_MESSAGE(CPN_SELENDOK,     OnSelEndOK)
    ON_MESSAGE(CPN_SELENDCANCEL, OnSelEndCancel)
    ON_MESSAGE(CPN_SELCHANGE,    OnSelChange)
	ON_MESSAGE(WM_MOUSELEAVE, OnMouseLeave)
END_MESSAGE_MAP()


//***********************************************************************
// Method:	CColourPickerXP::OnSelEndOK()
// Notes:	None.
//***********************************************************************
LRESULT CColourPickerXP::OnSelEndOK(WPARAM wParam, LPARAM lParam)
{
	m_bPopupActive = FALSE;

    COLORREF OldColor = m_Color;
	
	Color = (COLORREF)wParam;

    CWnd *pParent = GetParent();

    if (pParent) 
	{
        pParent->SendMessage(CPN_CLOSEUP, wParam, (LPARAM) GetDlgCtrlID());
        pParent->SendMessage(CPN_SELENDOK, wParam, (LPARAM) GetDlgCtrlID());
    }

    if (OldColor != m_Color)
        if (pParent) pParent->SendMessage(CPN_SELCHANGE, m_Color, (LPARAM) GetDlgCtrlID());

    return TRUE;
}


//***********************************************************************
// Method:	CColourPickerXP::OnSelEndCancel()
// Notes:	None.
//***********************************************************************
LRESULT CColourPickerXP::OnSelEndCancel(WPARAM wParam, LPARAM lParam)
{
	m_bPopupActive = FALSE;
	Color = (COLORREF)wParam;
	CWnd *pParent = GetParent();

	if (pParent) 
	{
		pParent->SendMessage(CPN_CLOSEUP, wParam, (LPARAM) GetDlgCtrlID());
		pParent->SendMessage(CPN_SELENDCANCEL, wParam, (LPARAM) GetDlgCtrlID());
	}

	return TRUE;
}


//***********************************************************************
// Method:	CColourPickerXP::OnSelChange()
// Notes:	None.
//***********************************************************************
LRESULT CColourPickerXP::OnSelChange(WPARAM wParam, LPARAM lParam)
{
    if (m_bTrackSelection) 
		Color = (COLORREF)wParam;

    CWnd *pParent = GetParent();

    if (pParent) 
		 pParent->SendMessage(CPN_SELCHANGE, wParam, (LPARAM) GetDlgCtrlID());

    return TRUE;
}

//***********************************************************************
// Method:	CColourPickerXP::OnCreate()
// Notes:	None.
//***********************************************************************
int CColourPickerXP::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
    if (CButton::OnCreate(lpCreateStruct) == -1)
        return -1;

    return 0;
}

//***********************************************************************
// Method:	CColourPickerXP::OnClicked()
// Notes:	None.
//***********************************************************************
BOOL CColourPickerXP::OnClicked()
{
	m_bPopupActive = TRUE;

    CRect rDraw;
    GetWindowRect(rDraw);

    new CColourPopupXP(CPoint(rDraw.left, rDraw.bottom),		// Point to display popup
                       m_Color,									// Selected colour
                       this,									// parent
                       m_strDefaultText,						// "Default" text area
                       m_strCustomText,							// Custom Text
					   (m_strRegSection.GetLength() ?
						m_strRegSection
						: m_strRegSectionStatic));				// Registry Section for Custom Colours

    CWnd *pParent = GetParent();

    if (pParent)
        pParent->SendMessage(CPN_DROPDOWN, (WPARAM)m_Color, (LPARAM) GetDlgCtrlID());

    return TRUE;
}

//***********************************************************************
// Method:	CColourPickerXP::OnNMThemeChanged()
// Notes:	For Windows XP theme support.
//***********************************************************************
void CColourPickerXP::OnNMThemeChanged(NMHDR *pNMHDR, LRESULT *pResult)
{
	// This feature requires Windows XP or greater.
	// The symbol _WIN32_WINNT must be >= 0x0501.
	// TODO: Add your control notification handler code here
#ifdef _THEME_H_
	m_xpButton.Open(GetSafeHwnd(), L"BUTTON");
	m_xpEdit.Open(GetSafeHwnd(), L"EDIT");
	m_xpCombo.Open(GetSafeHwnd(), L"COMBOBOX");
#endif
	Invalidate(FALSE);
	*pResult = 0;
}

//***********************************************************************
// Method:	CColourPickerXP::OnMouseMove()
// Notes:	Hover support.
//***********************************************************************
void CColourPickerXP::OnMouseMove(UINT nFlags, CPoint point)
{
	if (!m_bMouseOver)
	{
		m_bMouseOver = TRUE;
		TRACKMOUSEEVENT tme;
		tme.cbSize = sizeof(tme);
		tme.dwFlags = TME_LEAVE;
		tme.hwndTrack = m_hWnd;
		_TrackMouseEvent(&tme);
		if(m_bComboBoxStyle)
			DrawHotArrow(TRUE);
		else
			Invalidate(FALSE);
	}

	CButton::OnMouseMove(nFlags, point);
}

//***********************************************************************
// Method:	CColourPickerXP::OnMouseLeave()
// Notes:	Hover support.
//***********************************************************************
LRESULT CColourPickerXP::OnMouseLeave(WPARAM wParam, LPARAM lParam)
{
	if (m_bMouseOver)
	{
		m_bMouseOver = FALSE;
		if(m_bComboBoxStyle)
			DrawHotArrow(FALSE);
		else
			Invalidate(FALSE);
	}
	return 0;
}





//***********************************************************************
// Method:	CColourPickerXP::DrawHotArrow()
// Notes:	None.
//***********************************************************************
void CColourPickerXP::DrawHotArrow(BOOL bHot)
{
#ifdef _THEME_H_
    if (m_xpCombo.IsAppThemed())
    {
		CClientDC dc(this);

		CRect rcArrow;
		GetClientRect(rcArrow);
		m_xpEdit.GetBackgroundContentRect(dc.GetSafeHdc(), EP_EDITTEXT, ETS_NORMAL, &rcArrow, &rcArrow);
		rcArrow.left = rcArrow.right-(rcArrow.Height()-2);

		m_xpCombo.DrawBackground(dc.GetSafeHdc(), CP_DROPDOWNBUTTON, (bHot ? CBXS_HOT : CBXS_NORMAL), &rcArrow, NULL);
    }
#endif
}

//***********************************************************************
// Method:	CColourPickerXP::DrawItem()
// Notes:	None.
//***********************************************************************
void CColourPickerXP::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct) 
{
	ASSERT(lpDrawItemStruct);

	CDC*    pDC      = CDC::FromHandle(lpDrawItemStruct->hDC);
	CMemDC  dcMem(pDC);
	UINT    state    = lpDrawItemStruct->itemState;
    CRect   rDraw    = lpDrawItemStruct->rcItem;
	CRect	rArrow;
	bool	bFocus   = (state & ODS_FOCUS) && ((state & ODS_SELECTED)==0);

	SendMessage(WM_ERASEBKGND, (WPARAM)dcMem.GetSafeHdc());

	if (m_bPopupActive && !m_bComboBoxStyle)
		state |= ODS_SELECTED/*|ODS_FOCUS*/;

	if (state & ODS_FOCUS)
		state |= ODS_DEFAULT;

#ifdef _THEME_H_
	if(!m_xpButton)
		m_xpButton.Open(GetSafeHwnd(), L"BUTTON");
	if(!m_xpEdit)
		m_xpEdit.Open(GetSafeHwnd(), L"EDIT");
	if(!m_xpCombo)
		m_xpCombo.Open(GetSafeHwnd(), L"COMBOBOX");
#endif

	//******************************************************
	//**                  Draw Outer Edge
	//******************************************************
#ifdef _THEME_H_
	if (m_xpButton.IsAppThemed())
	{
		if ((state & ODS_DISABLED) != 0 && m_bComboBoxStyle)
		{
			dcMem.Draw3dRect(rDraw, COMBOBOX_DISABLED_COLOUR_OUT, COMBOBOX_DISABLED_COLOUR_OUT);
			rDraw.DeflateRect(1, 1);
		}
		else
		{
			int nFrameState = 0;
			if ((state & ODS_SELECTED) != 0 || m_bPopupActive)
				nFrameState |= (m_bComboBoxStyle ? ETS_SELECTED : PBS_PRESSED);
			if ((state & ODS_DISABLED) != 0)
				nFrameState |= (m_bComboBoxStyle ? ETS_DISABLED : PBS_DISABLED);
			if ((state & ODS_HOTLIGHT) != 0 || m_bMouseOver)
				nFrameState |= (m_bComboBoxStyle ? ETS_HOT : PBS_HOT);
			else if ((state & ODS_DEFAULT) != 0 && !m_bComboBoxStyle)
				nFrameState |= PBS_DEFAULTED;

			if(m_bComboBoxStyle)
			{
				m_xpEdit.DrawBackground(dcMem.GetSafeHdc(), EP_EDITTEXT, nFrameState, &rDraw, NULL);
				m_xpEdit.GetBackgroundContentRect(dcMem.GetSafeHdc(), EP_EDITTEXT, nFrameState, &rDraw, &rDraw);
			}
			else
			{
				m_xpButton.DrawBackground(dcMem.GetSafeHdc(), BP_PUSHBUTTON, nFrameState, &rDraw, NULL);
				m_xpButton.GetBackgroundContentRect(dcMem.GetSafeHdc(), BP_PUSHBUTTON, nFrameState, &rDraw, &rDraw);
				rDraw.InflateRect(0, 0, 1, 0);
			}
		}
	}
	else
#endif
	{
		if(m_bComboBoxStyle)
		{
			if ((state & ODS_DISABLED) == 0)
				dcMem.FillSolidRect(&rDraw, ::GetSysColor(COLOR_WINDOW));
			dcMem.DrawEdge(&rDraw, EDGE_SUNKEN, BF_RECT);

			rDraw.DeflateRect(::GetSystemMetrics(SM_CXEDGE),
							::GetSystemMetrics(SM_CYEDGE));
		}
		else
		{
			UINT uFrameState = DFCS_BUTTONPUSH|DFCS_ADJUSTRECT;

			if (state & ODS_SELECTED)
				uFrameState |= DFCS_PUSHED;

			if (state & ODS_DISABLED)
				uFrameState |= DFCS_INACTIVE;
			
			dcMem.DrawFrameControl(&rDraw,
								DFC_BUTTON,
								uFrameState);

			if (state & ODS_SELECTED)
				rDraw.OffsetRect(1,1);
		}
	}

	//******************************************************
	//**                     Draw Arrow (ComboBox style)
	//******************************************************
	if(m_bComboBoxStyle)
	{
#ifdef _THEME_H_
		if (m_xpCombo.IsAppThemed())
		{
			int nFrameState = 0;
			if ((state & ODS_SELECTED) != 0 /*|| m_bPopupActive*/)
				nFrameState |= CBXS_PRESSED;
			if ((state & ODS_DISABLED) != 0)
				nFrameState |= CBXS_DISABLED;
			if ((state & ODS_HOTLIGHT) != 0 || m_bMouseOver)
				nFrameState |= CBXS_HOT;

			rArrow.SetRect(rDraw.right-(rDraw.Height()-2), rDraw.top, rDraw.right, rDraw.bottom);

			m_xpCombo.DrawBackground(dcMem.GetSafeHdc(), CP_DROPDOWNBUTTON, nFrameState, &rArrow, NULL);

			rDraw.DeflateRect(0, 0, rArrow.Width(), 0);
		}
		else
#endif
		{
			rArrow.SetRect(rDraw.right-(rDraw.Height()), rDraw.top, rDraw.right, rDraw.bottom);

			dcMem.DrawFrameControl(&rArrow, DFC_SCROLL, DFCS_SCROLLDOWN  | 
								((state & ODS_SELECTED) ? DFCS_FLAT | DFCS_PUSHED : 0) |
								((state & ODS_DISABLED) ? DFCS_INACTIVE : 0));

			rDraw.DeflateRect(1, 1, rArrow.Width(), 1);
		}
	}

	//******************************************************
	//**                  Draw Inner Edge (Disabled Combo-box)
	//******************************************************
#ifdef _THEME_H_
	if (m_xpButton.IsAppThemed() && m_bComboBoxStyle)
	{
		if ((state & ODS_DISABLED) != 0)
		{
			dcMem.Draw3dRect(rDraw, COMBOBOX_DISABLED_COLOUR_IN, COMBOBOX_DISABLED_COLOUR_IN);
		}
		rDraw.DeflateRect(2, 2, 1, 2);
	}
#endif

	//******************************************************
	//**                     Draw Focus
	//******************************************************
	if (bFocus) 
    {
		CRect rFocus(rDraw.left,
					 rDraw.top,
					 rDraw.right - 1,
					 rDraw.bottom);

		if(m_bComboBoxStyle)
		{
			COLORREF clFocus = ::GetSysColor(COLOR_HIGHLIGHT);
			COLORREF clInvert = RGB(255-GetRValue(clFocus), 255-GetGValue(clFocus), 255-GetBValue(clFocus));

			rFocus.DeflateRect(1, 1);
			dcMem.FillSolidRect(rFocus, clFocus);
			rFocus.InflateRect(1, 1);

			dcMem.Draw3dRect(rFocus, clInvert, clInvert);
		}
  
        dcMem.DrawFocusRect(&rFocus);
    }

	rDraw.DeflateRect(::GetSystemMetrics(SM_CXEDGE),
					  ::GetSystemMetrics(SM_CYEDGE));
	rDraw.DeflateRect(0, 0, 1, 0);

	if(!m_bComboBoxStyle)
	{
		//******************************************************
		//**                     Draw Arrow (Button style)
		//******************************************************
		rArrow.left		= rDraw.right - g_ciArrowSizeX - ::GetSystemMetrics(SM_CXEDGE) /2;
		rArrow.right	= rArrow.left + g_ciArrowSizeX;
		rArrow.top		= (rDraw.bottom + rDraw.top)/2 - g_ciArrowSizeY / 2;
		rArrow.bottom	= (rDraw.bottom + rDraw.top)/2 + g_ciArrowSizeY / 2;

		DrawArrow(&dcMem,
				&rArrow,
				0,
				(state & ODS_DISABLED) 
				? ::GetSysColor(COLOR_GRAYTEXT) 
				: RGB(0,0,0));

		rDraw.right = rArrow.left - ::GetSystemMetrics(SM_CXEDGE)/2 - 2;

		//******************************************************
		//**                   Draw Separator (Button Style)
		//******************************************************
		dcMem.DrawEdge(&rDraw,
					EDGE_ETCHED,
					BF_RIGHT | (m_bFlatMenus ? BF_FLAT : 0));

		rDraw.right -= (::GetSystemMetrics(SM_CXEDGE) * 2) + 1 ;
	}
				  
	//******************************************************
	//**                     Draw Color
	//******************************************************
	if ((state & ODS_DISABLED) == 0)
	{
		CRect rcColor(rDraw);
		if(m_bComboBoxStyle)
			rcColor.right = rDraw.left+rDraw.Height();

		dcMem.FillSolidRect(&rcColor,
						   (m_Color == CLR_DEFAULT)
						   ? m_DefaultColor
						   : m_Color);

		::FrameRect(dcMem.m_hDC,
					&rcColor,
					::GetSysColorBrush(bFocus && m_bComboBoxStyle ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT));

		if(m_bComboBoxStyle)
		{
			rDraw.DeflateRect(rcColor.Width()+2, -1, 0, -1);

			CString szTemp;
			if(m_Color == CLR_DEFAULT)
				szTemp = m_strDefaultText;
			else
			{
				szTemp.Format(_T("%s:%d %s:%d %s:%d"), m_strRGBText.Left(1), GetRValue(m_Color), m_strRGBText.Mid(1, 1), GetGValue(m_Color), m_strRGBText.Mid(2, 1), GetBValue(m_Color));
				for (int i = 0; i < CColourPopupXP::m_nNumColours; i++)
				{
					if (CColourPopupXP::GetColour(i) == m_Color)
					{
						if(m_bAlwaysRGB)
						{
							CString szRGB = szTemp;
							szTemp.Format(_T("%s (%s)"), CColourPopupXP::GetColourName(i), szRGB);
						}
						else
							szTemp = CColourPopupXP::GetColourName(i);
						break;
					}
				}
			}

			CFont *pOldFont = dcMem.SelectObject(GetFont());

			dcMem.SetBkMode(TRANSPARENT);
			
			if (bFocus) 
				dcMem.SetTextColor(::GetSysColor(COLOR_HIGHLIGHTTEXT));

			dcMem.DrawText(szTemp, rDraw, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);

			dcMem.SelectObject(pOldFont);
		}
	}
}


//***********************************************************************
//**                          Static Methods                          **
//***********************************************************************

//***********************************************************************
// Method:	CColourPickerXP::DrawArrow()
// Notes:	None.
//***********************************************************************
void CColourPickerXP::DrawArrow(CDC* pDC, 
							 RECT* pRect, 
							 int iDirection,
							 COLORREF clrArrow /*= RGB(0,0,0)*/)
{
	POINT ptsArrow[3];

	switch (iDirection)
	{
		case 0 : // Down
		{
			ptsArrow[0].x = pRect->left;
			ptsArrow[0].y = pRect->top;
			ptsArrow[1].x = pRect->right;
			ptsArrow[1].y = pRect->top;
			ptsArrow[2].x = (pRect->left + pRect->right)/2;
			ptsArrow[2].y = pRect->bottom;
			break;
		}

		case 1 : // Up
		{
			ptsArrow[0].x = pRect->left;
			ptsArrow[0].y = pRect->bottom;
			ptsArrow[1].x = pRect->right;
			ptsArrow[1].y = pRect->bottom;
			ptsArrow[2].x = (pRect->left + pRect->right)/2;
			ptsArrow[2].y = pRect->top;
			break;
		}

		case 2 : // Left
		{
			ptsArrow[0].x = pRect->right;
			ptsArrow[0].y = pRect->top;
			ptsArrow[1].x = pRect->right;
			ptsArrow[1].y = pRect->bottom;
			ptsArrow[2].x = pRect->left;
			ptsArrow[2].y = (pRect->top + pRect->bottom)/2;
			break;
		}

		case 3 : // Right
		{
			ptsArrow[0].x = pRect->left;
			ptsArrow[0].y = pRect->top;
			ptsArrow[1].x = pRect->left;
			ptsArrow[1].y = pRect->bottom;
			ptsArrow[2].x = pRect->right;
			ptsArrow[2].y = (pRect->top + pRect->bottom)/2;
			break;
		}
	}
	
	CBrush brsArrow(clrArrow);
	CPen penArrow(PS_SOLID, 1 , clrArrow);

	CBrush* pOldBrush = pDC->SelectObject(&brsArrow);
	CPen*   pOldPen   = pDC->SelectObject(&penArrow);
	
	pDC->SetPolyFillMode(WINDING);
	pDC->Polygon(ptsArrow, 3);

	pDC->SelectObject(pOldBrush);
	pDC->SelectObject(pOldPen);
}

BOOL CColourPickerXP::PreTranslateMessage(MSG* pMsg)
{
	if(pMsg->message == WM_KEYDOWN)
	{
		switch(pMsg->wParam)
		{
		case VK_DOWN:
		case VK_F4:
			OnClicked();		// Activate pop-up when F4-key or down-arrow is hit
			return TRUE;
		case VK_UP:
			return TRUE;		// Disable up-arrow
		}
	}

	return _Inherited::PreTranslateMessage(pMsg);
}

/////////////////////////////////////////////////////////////////////////////
// ColourPopupXP : implementation                                          //
/////////////////////////////////////////////////////////////////////////////

//
// Sizing definitions
//

static const CSize s_sizeTextHiBorder (3, 3);
static const CSize s_sizeTextMargin (2, 2);
static const CSize s_sizeBoxHiBorder (2, 2);
static const CSize s_sizeBoxMargin (0, 0);
static const CSize s_sizeBoxCore (14, 14);

/////////////////////////////////////////////////////////////////////////////
// CColourPopupXP

CColourPopupXP::CColourPopupXP()
{
    Initialise();
}

CColourPopupXP::CColourPopupXP(CPoint p, COLORREF crColour, CWnd* pParentWnd,
                           LPCTSTR szDefaultText /* = NULL */,
                           LPCTSTR szCustomText  /* = NULL */,
						   LPCTSTR szRegSection  /* = NULL */)
{
    Initialise();

    m_crColour       = m_crInitialColour = crColour;
    m_pParent        = pParentWnd;
    m_strDefaultText = (szDefaultText)? szDefaultText : _T("");
    m_strCustomText  = (szCustomText)?  szCustomText  : _T("");

	m_strRegSection  = (szRegSection)?  szRegSection  : _T("");

    CColourPopupXP::Create(p, crColour, pParentWnd, szDefaultText, szCustomText);
}

void CColourPopupXP::Initialise()
{
	ASSERT(m_nNumColours <= MAX_COLOURS);
	if (m_nNumColours > MAX_COLOURS)
		m_nNumColours = MAX_COLOURS;

	m_nNumColumns       = 0;
    m_nNumRows          = 0;
    m_nBoxSize          = 18;
    m_nMargin           = ::GetSystemMetrics(SM_CXEDGE);
    m_nCurrentSel       = INVALID_COLOUR;
    m_nChosenColourSel  = INVALID_COLOUR;
    m_pParent           = NULL;
    m_crColour          = m_crInitialColour = RGB(0,0,0);

    m_bChildWindowVisible = FALSE;

	OSVERSIONINFO osvi;
	osvi.dwOSVersionInfoSize = sizeof (osvi);
	::GetVersionEx (&osvi);
	m_bIsXP = osvi.dwPlatformId == VER_PLATFORM_WIN32_NT &&
		(osvi.dwMajorVersion > 5 || (osvi.dwMajorVersion == 5 &&
		osvi.dwMinorVersion >= 1));

	m_bFlatmenus = FALSE;
 	if (m_bIsXP)
		::SystemParametersInfo (SPI_GETFLATMENU, 0, &m_bFlatmenus, FALSE);

	//
	// Get all the colors I need
	//

	int nAlpha = 48;
	m_clrBackground = ::GetSysColor (COLOR_MENU);
	m_clrHiLightBorder = ::GetSysColor (COLOR_HIGHLIGHT);
	m_clrHiLight = m_clrHiLightBorder;
	if (m_bIsXP)
		m_clrHiLight = ::GetSysColor (COLOR_MENUHILIGHT);
	m_clrHiLightText = ::GetSysColor (COLOR_HIGHLIGHTTEXT);
	m_clrText = ::GetSysColor (COLOR_MENUTEXT);
	m_clrLoLight = RGB (
		(GetRValue (m_clrBackground) * (255 - nAlpha) + 
			GetRValue (m_clrHiLightBorder) * nAlpha) >> 8,
		(GetGValue (m_clrBackground) * (255 - nAlpha) + 
			GetGValue (m_clrHiLightBorder) * nAlpha) >> 8,
		(GetBValue (m_clrBackground) * (255 - nAlpha) + 
			GetBValue (m_clrHiLightBorder) * nAlpha) >> 8);

    // Idiot check: Make sure the colour square is at least 5 x 5;
    if (m_nBoxSize - 2*m_nMargin - 2 < 5) m_nBoxSize = 5 + 2*m_nMargin + 2;

    // Create the font
    NONCLIENTMETRICS ncm;
    ncm.cbSize = sizeof(NONCLIENTMETRICS);
    VERIFY(SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, 0));
    m_Font.CreateFontIndirect(&(ncm.lfMessageFont));

    // Create the palette
    struct {
        LOGPALETTE    LogPalette;
        PALETTEENTRY  PalEntry[MAX_COLOURS];
    } pal;

    LOGPALETTE* pLogPalette = (LOGPALETTE*) &pal;
    pLogPalette->palVersion    = 0x300;
    pLogPalette->palNumEntries = (WORD) m_nNumColours; 

    for (int i = 0; i < m_nNumColours; i++)
    {
        pLogPalette->palPalEntry[i].peRed   = GetRValue(m_crColours[i].crColour);
        pLogPalette->palPalEntry[i].peGreen = GetGValue(m_crColours[i].crColour);
        pLogPalette->palPalEntry[i].peBlue  = GetBValue(m_crColours[i].crColour);
        pLogPalette->palPalEntry[i].peFlags = 0;
    }

    m_Palette.CreatePalette(pLogPalette);
}

CColourPopupXP::~CColourPopupXP()
{
    m_Font.DeleteObject();
    m_Palette.DeleteObject();
}

BOOL CColourPopupXP::Create(CPoint p, COLORREF crColour, CWnd* pParentWnd,
                          LPCTSTR szDefaultText /* = NULL */,
                          LPCTSTR szCustomText  /* = NULL */)
{
    ASSERT(pParentWnd && ::IsWindow(pParentWnd->GetSafeHwnd()));
  
    m_pParent  = pParentWnd;
    m_crColour = m_crInitialColour = crColour;

    // Get the class name and create the window
	UINT nClassStyle = CS_CLASSDC|CS_SAVEBITS|CS_HREDRAW|CS_VREDRAW;
	if(m_bIsXP)
	{
		BOOL bDropShadow;
		::SystemParametersInfo(SPI_GETDROPSHADOW, 0, &bDropShadow, FALSE);
		if (bDropShadow)
			nClassStyle |= CS_DROPSHADOW;
	}

    CString szClassName = AfxRegisterWndClass(nClassStyle,
                                              0,
                                              (HBRUSH) (COLOR_MENU+1), 
                                              0);

	// Finding the window's topmost parent
	CWnd* pTopParent = m_pParent->GetParentOwner();
    
	if ( pTopParent != NULL )
		pTopParent->SetRedraw( FALSE );

	BOOL bCreate = CWnd::CreateEx(0, szClassName, _T(""), WS_VISIBLE|WS_POPUP, 
                        p.x, p.y, 100, 100, // size updated soon
                        pParentWnd->GetSafeHwnd(), 0, NULL);
    
	if ( pTopParent != NULL )
	{
		pTopParent->SendMessage( WM_NCACTIVATE, TRUE );
		pTopParent->SetRedraw( TRUE );
	}
    
	if ( !bCreate )
		return FALSE;

    // Store the Custom text
    if (szCustomText != NULL) 
        m_strCustomText = szCustomText;

    // Store the Default Area text
    if (szDefaultText != NULL) 
        m_strDefaultText = szDefaultText;
        
    // Set the window size
    SetWindowSize();

    // Create the tooltips
    CreateToolTips();

    // Find which cell (if any) corresponds to the initial colour
    FindCellFromColour(crColour);

    // Capture all mouse events for the life of this window
    SetCapture();

    return TRUE;
}

BEGIN_MESSAGE_MAP(CColourPopupXP, CWnd)
    //{{AFX_MSG_MAP(CColourPopupXP)
    ON_WM_NCDESTROY()
    ON_WM_LBUTTONUP()
    ON_WM_PAINT()
    ON_WM_MOUSEMOVE()
    ON_WM_KEYDOWN()
    ON_WM_QUERYNEWPALETTE()
    ON_WM_PALETTECHANGED()
	ON_WM_KILLFOCUS()
	ON_WM_ACTIVATEAPP()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CColourPopupXP message handlers

// For tooltips
BOOL CColourPopupXP::PreTranslateMessage(MSG* pMsg) 
{
    m_ToolTip.RelayEvent(pMsg);

    // Fix (Adrian Roman): Sometimes if the picker loses focus it is never destroyed
    if (GetCapture()->GetSafeHwnd() != m_hWnd)
        SetCapture(); 

    return CWnd::PreTranslateMessage(pMsg);
}

// If an arrow key is pressed, then move the selection
void CColourPopupXP::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
    int row = GetRow(m_nCurrentSel),
        col = GetColumn(m_nCurrentSel);

    if (nChar == VK_DOWN) 
    {
        if (row == DEFAULT_BOX_VALUE) 
            row = col = 0; 
        else if (row == CUSTOM_BOX_VALUE || row == INVALID_COLOUR)
        {
            if (m_strDefaultText.GetLength())
                row = col = DEFAULT_BOX_VALUE;
            else
                row = col = 0;
        }
        else
        {
            row++;
            if (GetIndex(row,col) < 0)
            {
                if (m_strCustomText.GetLength())
                    row = col = CUSTOM_BOX_VALUE;
                else if (m_strDefaultText.GetLength())
                    row = col = DEFAULT_BOX_VALUE;
                else
                    row = col = 0;
            }
        }
        ChangeSelection(GetIndex(row, col));
    }

    if (nChar == VK_UP) 
    {
        if (row == DEFAULT_BOX_VALUE || row == INVALID_COLOUR)
        {
            if (m_strCustomText.GetLength())
                row = col = CUSTOM_BOX_VALUE;
            else
           { 
                row = GetRow(m_nNumColours-1); 
                col = GetColumn(m_nNumColours-1); 
            }
        }
        else if (row == CUSTOM_BOX_VALUE)
        { 
            row = GetRow(m_nNumColours-1); 
            col = GetColumn(m_nNumColours-1); 
        }
        else if (row > 0) row--;
        else /* row == 0 */
        {
            if (m_strDefaultText.GetLength())
                row = col = DEFAULT_BOX_VALUE;
            else if (m_strCustomText.GetLength())
                row = col = CUSTOM_BOX_VALUE;
            else
            { 
                row = GetRow(m_nNumColours-1); 
                col = GetColumn(m_nNumColours-1); 
            }
        }
        ChangeSelection(GetIndex(row, col));
    }

    if (nChar == VK_RIGHT) 
    {
        if (row == DEFAULT_BOX_VALUE) 
            row = col = 0; 
        else if (row == CUSTOM_BOX_VALUE || row == INVALID_COLOUR)
        {
            if (m_strDefaultText.GetLength())
                row = col = DEFAULT_BOX_VALUE;
            else
                row = col = 0;
        }
        else if (col < m_nNumColumns-1) 
            col++;
        else 
        { 
            col = 0; row++;
        }

        if (GetIndex(row,col) == INVALID_COLOUR)
        {
            if (m_strCustomText.GetLength())
                row = col = CUSTOM_BOX_VALUE;
            else if (m_strDefaultText.GetLength())
                row = col = DEFAULT_BOX_VALUE;
            else
                row = col = 0;
        }

        ChangeSelection(GetIndex(row, col));
    }

    if (nChar == VK_LEFT) 
    {
        if (row == DEFAULT_BOX_VALUE || row == INVALID_COLOUR)
        {
            if (m_strCustomText.GetLength())
                row = col = CUSTOM_BOX_VALUE;
            else
           { 
                row = GetRow(m_nNumColours-1); 
                col = GetColumn(m_nNumColours-1); 
            }
        }
        else if (row == CUSTOM_BOX_VALUE)
        { 
            row = GetRow(m_nNumColours-1); 
            col = GetColumn(m_nNumColours-1); 
        }
        else if (col > 0) col--;
        else /* col == 0 */
        {
            if (row > 0) { row--; col = m_nNumColumns-1; }
            else 
            {
                if (m_strDefaultText.GetLength())
                    row = col = DEFAULT_BOX_VALUE;
                else if (m_strCustomText.GetLength())
                    row = col = CUSTOM_BOX_VALUE;
                else
                { 
                    row = GetRow(m_nNumColours-1); 
                    col = GetColumn(m_nNumColours-1); 
                }
            }
        }
        ChangeSelection(GetIndex(row, col));
    }

    if (nChar == VK_ESCAPE) 
    {
        m_crColour = m_crInitialColour;
        EndSelection(CPN_SELENDCANCEL);
        return;
    }

    if (nChar == VK_RETURN || nChar == VK_SPACE)
    {
        EndSelection(CPN_SELENDOK);
        return;
    }

    CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
}

// auto-deletion
void CColourPopupXP::OnNcDestroy() 
{
    CWnd::OnNcDestroy();
    delete this;
}

void CColourPopupXP::OnPaint() 
{
    CPaintDC dc(this); // device context for painting

	// Draw the Default Area text
    if (m_strDefaultText.GetLength())
        DrawCell(&dc, DEFAULT_BOX_VALUE);
 
    // Draw colour cells
    for (int i = 0; i < m_nNumColours; i++)
        DrawCell(&dc, i);
    
    // Draw custom text
    if (m_strCustomText.GetLength())
        DrawCell(&dc, CUSTOM_BOX_VALUE);

    // Draw raised window edge (ex-window style WS_EX_WINDOWEDGE is sposed to do this,
    // but for some reason isn't
    CRect rect;
    GetClientRect(rect);
	DrawBorder(&dc, rect, EDGE_RAISED, BF_RECT);
}

void CColourPopupXP::OnMouseMove(UINT nFlags, CPoint point) 
{
    int nNewSelection = INVALID_COLOUR;

    // Translate points to be relative raised window edge
    point.x -= m_nMargin;
    point.y -= m_nMargin;

    // First check we aren't in text box
    if (m_strCustomText.GetLength() && m_CustomTextRect.PtInRect(point))
        nNewSelection = CUSTOM_BOX_VALUE;
    else if (m_strDefaultText.GetLength() && m_DefaultTextRect.PtInRect(point))
        nNewSelection = DEFAULT_BOX_VALUE;
	else if (!m_BoxesRect.PtInRect(point))
		nNewSelection = INVALID_COLOUR;
    else
    {
        // Take into account text box
        if (m_strDefaultText.GetLength()) 
            point.y -= m_DefaultTextRect.Height();  

        // Get the row and column
        nNewSelection = GetIndex(point.y / m_nBoxSize, point.x / m_nBoxSize);

        // In range? If not, default and exit
        if (nNewSelection < 0 || nNewSelection >= m_nNumColours)
        {
            //CWnd::OnMouseMove(nFlags, point);
            //return;
			nNewSelection = INVALID_COLOUR;
        }
    }

    // OK - we have the row and column of the current selection (may be CUSTOM_BOX_VALUE)
    // Has the row/col selection changed? If yes, then redraw old and new cells.
    if (nNewSelection != m_nCurrentSel)
        ChangeSelection(nNewSelection);

    CWnd::OnMouseMove(nFlags, point);
}

// End selection on LButtonUp
void CColourPopupXP::OnLButtonUp(UINT nFlags, CPoint point) 
{    
    CWnd::OnLButtonUp(nFlags, point);

    DWORD pos = GetMessagePos();
    point = CPoint(LOWORD(pos), HIWORD(pos));

    if (m_WindowRect.PtInRect(point))
        EndSelection(CPN_SELENDOK);
    else
        EndSelection(CPN_SELENDCANCEL);
}

/////////////////////////////////////////////////////////////////////////////
// CColourPopupXP implementation

int CColourPopupXP::GetIndex(int row, int col) const
{ 
    if ((row == CUSTOM_BOX_VALUE || col == CUSTOM_BOX_VALUE) && m_strCustomText.GetLength())
        return CUSTOM_BOX_VALUE;
    else if ((row == DEFAULT_BOX_VALUE || col == DEFAULT_BOX_VALUE) && m_strDefaultText.GetLength())
        return DEFAULT_BOX_VALUE;
    else if (row < 0 || col < 0 || row >= m_nNumRows || col >= m_nNumColumns)
        return INVALID_COLOUR;
    else
    {
        if (row*m_nNumColumns + col >= m_nNumColours)
            return INVALID_COLOUR;
        else
            return row*m_nNumColumns + col;
    }
}

int CColourPopupXP::GetRow(int nIndex) const               
{ 
    if (nIndex == CUSTOM_BOX_VALUE && m_strCustomText.GetLength())
        return CUSTOM_BOX_VALUE;
    else if (nIndex == DEFAULT_BOX_VALUE && m_strDefaultText.GetLength())
        return DEFAULT_BOX_VALUE;
    else if (nIndex < 0 || nIndex >= m_nNumColours)
        return INVALID_COLOUR;
    else
        return nIndex / m_nNumColumns; 
}

int CColourPopupXP::GetColumn(int nIndex) const            
{ 
    if (nIndex == CUSTOM_BOX_VALUE && m_strCustomText.GetLength())
        return CUSTOM_BOX_VALUE;
    else if (nIndex == DEFAULT_BOX_VALUE && m_strDefaultText.GetLength())
        return DEFAULT_BOX_VALUE;
    else if (nIndex < 0 || nIndex >= m_nNumColours)
        return INVALID_COLOUR;
    else
        return nIndex % m_nNumColumns; 
}

void CColourPopupXP::FindCellFromColour(COLORREF crColour)
{
    if (crColour == CLR_DEFAULT && m_strDefaultText.GetLength())
    {
        m_nChosenColourSel = DEFAULT_BOX_VALUE;
        return;
    }

    for (int i = 0; i < m_nNumColours; i++)
    {
        if (GetColour(i) == crColour)
        {
            m_nChosenColourSel = i;
            return;
        }
    }

    if (m_strCustomText.GetLength())
        m_nChosenColourSel = CUSTOM_BOX_VALUE;
    else
        m_nChosenColourSel = INVALID_COLOUR;
}

// Gets the dimensions of the colour cell given by (row,col)
BOOL CColourPopupXP::GetCellRect(int nIndex, const LPRECT& rect)
{
    if (nIndex == CUSTOM_BOX_VALUE)
    {
        ::SetRect(rect, 
                  m_CustomTextRect.left,  m_CustomTextRect.top,
                  m_CustomTextRect.right, m_CustomTextRect.bottom);
        return TRUE;
    }
    else if (nIndex == DEFAULT_BOX_VALUE)
    {
        ::SetRect(rect, 
                  m_DefaultTextRect.left,  m_DefaultTextRect.top,
                  m_DefaultTextRect.right, m_DefaultTextRect.bottom);
        return TRUE;
    }

    if (nIndex < 0 || nIndex >= m_nNumColours)
        return FALSE;

    rect->left = GetColumn(nIndex) * m_nBoxSize + m_nMargin;
    rect->top  = GetRow(nIndex) * m_nBoxSize + m_nMargin;

    // Move everything down if we are displaying a default text area
    if (m_strDefaultText.GetLength()) 
        rect->top += (m_nMargin + m_DefaultTextRect.Height());

    rect->right = rect->left + m_nBoxSize;
    rect->bottom = rect->top + m_nBoxSize;

    return TRUE;
}

// Works out an appropriate size and position of this window
void CColourPopupXP::SetWindowSize()
{
    CSize TextSize = CSize(0,0);

    // If we are showing a custom or default text area, get the font and text size.
    if (m_strCustomText.GetLength() || m_strDefaultText.GetLength())
    {
        CClientDC dc(this);
        CFont* pOldFont = (CFont*) dc.SelectObject(&m_Font);

        // Get the size of the custom text (if there IS custom text)
        if (m_strCustomText.GetLength())
            TextSize = dc.GetTextExtent(m_strCustomText);

        // Get the size of the default text (if there IS default text)
        if (m_strDefaultText.GetLength())
        {
            CSize DefaultSize = dc.GetTextExtent(m_strDefaultText);
            if (DefaultSize.cx > TextSize.cx) TextSize.cx = DefaultSize.cx;
            if (DefaultSize.cy > TextSize.cy) TextSize.cy = DefaultSize.cy;
        }

        dc.SelectObject(pOldFont);
        TextSize += CSize(2*m_nMargin,2*m_nMargin);

        // Add even more space to draw the horizontal line
        TextSize.cy += 2*m_nMargin + 2;
    }

    // Get the number of columns and rows
    //m_nNumColumns = (int) sqrt((double)m_nNumColours);    // for a square window (yuk)
    m_nNumColumns = 8;
    m_nNumRows = m_nNumColours / m_nNumColumns;
    if (m_nNumColours % m_nNumColumns) m_nNumRows++;

    // Get the current window position, and set the new size
    CRect rect;
    GetWindowRect(rect);

    m_WindowRect.SetRect(rect.left, rect.top, 
                         rect.left + m_nNumColumns*m_nBoxSize + 2*m_nMargin,
                         rect.top  + m_nNumRows*m_nBoxSize + 2*m_nMargin);

    // if custom text, then expand window if necessary, and set text width as
    // window width
    if (m_strDefaultText.GetLength()) 
    {
        if (TextSize.cx > m_WindowRect.Width())
            m_WindowRect.right = m_WindowRect.left + TextSize.cx;
        TextSize.cx = m_WindowRect.Width()-2*m_nMargin;

        // Work out the text area
        m_DefaultTextRect.SetRect(m_nMargin, m_nMargin, 
                                  m_nMargin+TextSize.cx, 2*m_nMargin+TextSize.cy);
        m_WindowRect.bottom += m_DefaultTextRect.Height() + 2*m_nMargin;
    }
	else
		m_DefaultTextRect.SetRectEmpty();

    // if custom text, then expand window if necessary, and set text width as
    // window width
    if (m_strCustomText.GetLength()) 
    {
        if (TextSize.cx > m_WindowRect.Width())
            m_WindowRect.right = m_WindowRect.left + TextSize.cx;
        TextSize.cx = m_WindowRect.Width()-2*m_nMargin;

        // Work out the text area
        m_CustomTextRect.SetRect(m_nMargin, m_WindowRect.Height(), 
                                 m_nMargin+TextSize.cx, 
                                 m_WindowRect.Height()+m_nMargin+TextSize.cy);
        m_WindowRect.bottom += m_CustomTextRect.Height() + m_nMargin;
	}
	else
		m_CustomTextRect.SetRectEmpty();

	//
	// Compute the min width
	//

	int nBoxTotalWidth = m_nNumColumns * m_nBoxSize;
	int nMinWidth = nBoxTotalWidth;
	if (nMinWidth < TextSize.cx)
		nMinWidth = TextSize.cx;

	//
	// Initialize the color box rectangle
	//

	m_BoxesRect = CRect(
		CPoint ((nMinWidth - nBoxTotalWidth) / 2, m_DefaultTextRect.bottom), 
		CSize (nBoxTotalWidth, m_nNumRows * m_nBoxSize)
		);

	//
	// Get the screen size (with multi-monitor support)
	//

    CSize ScreenSize(::GetSystemMetrics(SM_CXSCREEN), ::GetSystemMetrics(SM_CYSCREEN));

#if (WINVER >= 0x0500)
	HMODULE hUser32 = ::GetModuleHandle (_T("USER32.DLL"));
	if (hUser32 != NULL)
	{
		typedef HMONITOR (WINAPI *FN_MonitorFromWindow) (HWND hWnd, DWORD dwFlags);
		typedef BOOL (WINAPI *FN_GetMonitorInfo) (HMONITOR hMonitor, LPMONITORINFO lpmi);
		FN_MonitorFromWindow pfnMonitorFromWindow = (FN_MonitorFromWindow)
			::GetProcAddress (hUser32, "MonitorFromWindow");
		FN_GetMonitorInfo pfnGetMonitorInfo = (FN_GetMonitorInfo)
			::GetProcAddress (hUser32, "GetMonitorInfoA");
		if (pfnMonitorFromWindow != NULL && pfnGetMonitorInfo != NULL)
		{
			MONITORINFO mi;
			HMONITOR hMonitor = pfnMonitorFromWindow (m_hWnd, 
				MONITOR_DEFAULTTONEAREST);
			mi.cbSize = sizeof (mi);
			pfnGetMonitorInfo (hMonitor, &mi);
			ScreenSize.cx = mi.rcWork.right - mi.rcWork.left;
			ScreenSize.cy = mi.rcWork.bottom - mi.rcWork.top;
		}
	}
#endif


	// Need to check it'll fit on screen: Too far right?
    if (m_WindowRect.right > ScreenSize.cx)
        m_WindowRect.OffsetRect(-(m_WindowRect.right - ScreenSize.cx), 0);

    // Too far left?
    if (m_WindowRect.left < 0)
        m_WindowRect.OffsetRect( -m_WindowRect.left, 0);

    // Bottom falling out of screen?
    if (m_WindowRect.bottom > ScreenSize.cy)
    {
        CRect ParentRect;
        m_pParent->GetWindowRect(ParentRect);
        m_WindowRect.OffsetRect(0, -(ParentRect.Height() + m_WindowRect.Height()));
    }

    // Set the window size and position
    MoveWindow(m_WindowRect, TRUE);
}

void CColourPopupXP::CreateToolTips()
{
    // Create the tool tip
    if (!m_ToolTip.Create(this)) return;

    // Add a tool for each cell
    for (int i = 0; i < m_nNumColours; i++)
    {
        CRect rect;
        if (!GetCellRect(i, rect)) continue;
            m_ToolTip.AddTool(this, GetColourName(i), rect, 1);
    }
}

void CColourPopupXP::ChangeSelection(int nIndex)
{
    CClientDC dc(this);        // device context for drawing

    if (nIndex > m_nNumColours)
        nIndex = CUSTOM_BOX_VALUE; 

    if ((m_nCurrentSel >= 0 && m_nCurrentSel < m_nNumColours) ||
        m_nCurrentSel == CUSTOM_BOX_VALUE || m_nCurrentSel == DEFAULT_BOX_VALUE)
    {
        // Set Current selection as invalid and redraw old selection (this way
        // the old selection will be drawn unselected)
        int OldSel = m_nCurrentSel;
        m_nCurrentSel = INVALID_COLOUR;
        DrawCell(&dc, OldSel);
    }

    // Set the current selection as row/col and draw (it will be drawn selected)
    m_nCurrentSel = nIndex;
    DrawCell(&dc, m_nCurrentSel);

    // Store the current colour
    if (m_nCurrentSel == CUSTOM_BOX_VALUE)
        m_pParent->SendMessage(CPN_SELCHANGE, (WPARAM) m_crInitialColour, 0);
    else if (m_nCurrentSel == DEFAULT_BOX_VALUE)
    {
        m_crColour = CLR_DEFAULT;
        m_pParent->SendMessage(CPN_SELCHANGE, (WPARAM) CLR_DEFAULT, 0);
    }
	else if (m_nCurrentSel == INVALID_COLOUR)
	{
		m_crColour = INVALID_COLOUR;
        m_pParent->SendMessage(CPN_SELCHANGE, (WPARAM) m_crInitialColour, 0);
	}
    else
    {
        m_crColour = GetColour(m_nCurrentSel);
        m_pParent->SendMessage(CPN_SELCHANGE, (WPARAM) m_crColour, 0);
    }
}

void CColourPopupXP::EndSelection(int nMessage)
{
    ReleaseCapture();

    // If custom text selected, perform a custom colour selection
    if (nMessage != CPN_SELENDCANCEL && m_nCurrentSel == CUSTOM_BOX_VALUE)
    {
        m_bChildWindowVisible = TRUE;

        CColorDialog dlg(m_crInitialColour, CC_FULLOPEN | CC_ANYCOLOR, this);

		COLORREF clCustomColors[16];

		CString szTemp;
		DWORD dwValue;
		DWORD dwType;
		DWORD dwCount = sizeof(DWORD);
		HKEY hSecKey = NULL;
		if(m_strRegSection.GetLength())
			hSecKey = AfxGetApp()->GetSectionKey(m_strRegSection);
		for(int i=0; i<16; i++) {
			if (hSecKey == NULL) {
				clCustomColors[i] = RGB(255,255,255);
				continue;
			}
			szTemp.Format(_T("CUSTOM_COLOR_%d"), i);
			if (RegQueryValueEx(hSecKey, (LPCTSTR) szTemp, NULL, &dwType,
				(LPBYTE)&dwValue, &dwCount) == ERROR_SUCCESS)
			{
				ASSERT(dwType == REG_DWORD);
				ASSERT(dwCount == sizeof(dwValue));
				clCustomColors[i] = dwValue;
				continue;
			}
			clCustomColors[i] = RGB(255,255,255);
		}

		dlg.m_cc.lpCustColors = clCustomColors;

        if (dlg.DoModal() == IDOK)
            m_crColour = dlg.GetColor();
        else
            nMessage = CPN_SELENDCANCEL;


		if(hSecKey!=NULL)
		{
			for(int i=0; i<16; i++)
			{
				szTemp.Format(_T("CUSTOM_COLOR_%d"), i);
				RegSetValueEx(hSecKey, (LPCTSTR) szTemp, NULL, REG_DWORD, (LPBYTE)&clCustomColors[i], sizeof(clCustomColors[i]));
			}
			RegCloseKey(hSecKey);
		}

        m_bChildWindowVisible = FALSE;
    } 

    if (nMessage == CPN_SELENDCANCEL)
        m_crColour = m_crInitialColour;

    m_pParent->SendMessage(nMessage, (WPARAM) m_crColour, 0);
    
    // Kill focus bug fixed by Martin Wawrusch
    if (!m_bChildWindowVisible)
        DestroyWindow();
}

void CColourPopupXP::DrawCell(CDC* pDC, int nIndex)
{
	//
	// Get the drawing rect
	//

	CRect rect;
	if (!GetCellRect(nIndex, &rect)) 
		return;

	//
	// Get the text pointer and colors
	//

	CString szText;
	COLORREF clrBox;
	CSize sizeMargin;
	CSize sizeHiBorder;
	UINT nBorder = 0;
	switch(nIndex)
	{
	case CUSTOM_BOX_VALUE:
		szText = m_strCustomText;
		sizeMargin = s_sizeTextMargin;
		sizeHiBorder = s_sizeTextHiBorder;
		nBorder = BF_TOP;
		break;

	case DEFAULT_BOX_VALUE:
		szText = m_strDefaultText;
		sizeMargin = s_sizeTextMargin;
		sizeHiBorder = s_sizeTextHiBorder;
		nBorder = BF_BOTTOM;
		break;

	default:
		szText = _T("");
		clrBox = GetColour(nIndex);
		sizeMargin = s_sizeBoxMargin;
		sizeHiBorder = s_sizeBoxHiBorder;
	}

	//
	// Based on the selectons, get our colors
	//

	COLORREF clrHiLight;
	COLORREF clrText;
	bool bSelected;
	COLORREF clr3dTopLeft, clr3dBottomRight;
	if (m_nCurrentSel == nIndex)
	{
		bSelected = true;
		clrHiLight = m_clrHiLight;
		clrText = (m_bFlatmenus ? m_clrHiLightText : m_clrText);
		clr3dTopLeft = ::GetSysColor(COLOR_3DHIGHLIGHT);
		clr3dBottomRight = ::GetSysColor(COLOR_3DSHADOW);
	}
	else if (m_nChosenColourSel == nIndex)
	{
		bSelected = true;
		clrHiLight = m_clrLoLight;
		clrText = m_clrText;
		clr3dTopLeft = ::GetSysColor(COLOR_3DSHADOW);
		clr3dBottomRight = ::GetSysColor(COLOR_3DHIGHLIGHT);
	}
	else
	{
		bSelected = false;
		clrHiLight = m_clrLoLight;
		clrText = m_clrText;
	}

	//
	// Select and realize the palette
	//

	CPalette* pOldPalette = NULL;
	if (szText == _T(""))
	{
		if (pDC->GetDeviceCaps(RASTERCAPS) & RC_PALETTE)
		{
			pOldPalette = pDC->SelectPalette(&m_Palette, FALSE);
			pDC->RealizePalette();
		}
	}

	//
	// If we are currently selected
	//

	if (bSelected)
	{

		//
		// If we have a background margin, then draw that
		//

		if (sizeMargin.cx > 0 || sizeMargin.cy > 0)
		{
			pDC->FillSolidRect(&rect, m_clrBackground);
			rect.DeflateRect(sizeMargin.cx, sizeMargin.cy);
		}

		//
		// Draw the selection rectagle
		//

		if(m_bFlatmenus)
		{
			CPen pen(PS_SOLID, 1,m_clrHiLightBorder);
			CPen *pOldPen = pDC->SelectObject(&pen);

			CBrush brush(clrHiLight);
			CBrush *pOldBrush = pDC->SelectObject(&brush);

			pDC->Rectangle(&rect);

			rect.DeflateRect(sizeHiBorder.cx - 1, sizeHiBorder.cy - 1);

			// restore DC
			pDC->SelectObject(pOldPen);
			pDC->SelectObject(pOldBrush);
		}
		else
		{
			pDC->FillSolidRect(&rect, m_clrBackground);
			pDC->Draw3dRect(&rect, clr3dTopLeft, clr3dBottomRight);
			rect.DeflateRect(sizeHiBorder.cx, sizeHiBorder.cy);
		}
	}

	//
	// Otherwise, we are not selected
	//

	else
	{
		// Draw the background

		pDC->FillSolidRect(&rect, m_clrBackground);
		rect.DeflateRect(sizeMargin.cx + sizeHiBorder.cx, sizeMargin.cy + sizeHiBorder.cy);
	}

	//
	// Draw custom text
	//

	if (szText != _T(""))
	{
        CFont *pOldFont = (CFont*) pDC->SelectObject(&m_Font);
		pDC->SetTextColor(clrText);
        pDC->SetBkMode(TRANSPARENT);
        pDC->DrawText(szText, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        pDC->SelectObject(pOldFont);
    }        

	//
	// Otherwise, draw color
	//

	else
	{

		//
		// Draw color
		//
		CPen pen(PS_SOLID, 1, ::GetSysColor(COLOR_3DSHADOW));
		CPen* pOldPen = pDC->SelectObject(&pen);

		CBrush brush(PALETTERGB(GetRValue(GetColour(nIndex)), 
								GetGValue(GetColour(nIndex)), 
								GetBValue(GetColour(nIndex)) ));
		CBrush* pOldBrush = pDC->SelectObject(&brush);

		// Draw the cell colour
		rect.DeflateRect(1, 1);
		pDC->Rectangle(&rect);

		// Restore DC
		pDC->SelectObject(pOldPen);
		pDC->SelectObject(pOldBrush);
	}

	//
	// Draw border
	//

	if(nBorder)
	{
		CRect r;
		GetCellRect(nIndex, &r);
		r.InflateRect(-2, 1);
		DrawBorder(pDC, r, EDGE_ETCHED, nBorder);
	}

    // cleanup
    if (pOldPalette && pDC->GetDeviceCaps(RASTERCAPS) & RC_PALETTE)
        pDC->SelectPalette(pOldPalette, FALSE);
}

void CColourPopupXP::DrawBorder(CDC* pDC, CRect rect, UINT nEdge, UINT nBorder)
{
	if(m_bFlatmenus)
	{
		CPen pen(PS_SOLID, 1, ::GetSysColor(COLOR_GRAYTEXT));
		CPen *pOldPen = pDC->SelectObject(&pen);

		rect.DeflateRect(0, 0, 1, 1);

		if(nBorder & BF_TOP)
		{
			pDC->MoveTo(rect.left, rect.top);
			pDC->LineTo(rect.right+1, rect.top);
		}

		if(nBorder & BF_BOTTOM)
		{
			pDC->MoveTo(rect.left, rect.bottom);
			pDC->LineTo(rect.right+1, rect.bottom);
		}

		if(nBorder & BF_LEFT)
		{
			pDC->MoveTo(rect.left, rect.top);
			pDC->LineTo(rect.left, rect.bottom+1);
		}

		if(nBorder & BF_RIGHT)
		{
			pDC->MoveTo(rect.right, rect.top);
			pDC->LineTo(rect.right, rect.bottom+1);
		}

		pDC->SelectObject(pOldPen);
	}
	else
		pDC->DrawEdge(rect, nEdge, nBorder);
}

BOOL CColourPopupXP::OnQueryNewPalette() 
{
    Invalidate(FALSE);    
    return CWnd::OnQueryNewPalette();
}

void CColourPopupXP::OnPaletteChanged(CWnd* pFocusWnd) 
{
    CWnd::OnPaletteChanged(pFocusWnd);

    if (pFocusWnd->GetSafeHwnd() != GetSafeHwnd())
        Invalidate(FALSE);
}

void CColourPopupXP::OnKillFocus(CWnd* pNewWnd) 
{
	CWnd::OnKillFocus(pNewWnd);

    ReleaseCapture();
    //DestroyWindow(); //- causes crash when Custom colour dialog appears.
}

// KillFocus problem fix suggested by Paul Wilkerson.
#if _MFC_VER >= 0x0700
void CColourPopupXP::OnActivateApp(BOOL bActive, DWORD dwTask) 
{
	CWnd::OnActivateApp(bActive, dwTask);
#else
void CColourPopupXP::OnActivateApp(BOOL bActive, HTASK hTask) 
{
	CWnd::OnActivateApp(bActive, hTask);
#endif

	// If Deactivating App, cancel this selection
	if (!bActive)
		 EndSelection(CPN_SELENDCANCEL);
}
