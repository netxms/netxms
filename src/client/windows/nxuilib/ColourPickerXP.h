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

//-----------------------------------------------------------------------------
//
// Instructions on how to add CColourPickerXP to your application:
//
//   1. Copy ColourPickerXP.h, ColourPickerXP.cpp, XPTheme.h, XPTheme.cpp and
//		MemDC.h into your application directory.
//
//   2. Add the five files into your project.
//
//   3. If you want to have multi-monitor support, set the WINVER definition in
//		stdafx.h to at least 0x0500.
//
//   4. If you don't want XP theme support, comment out the '#include "XPTheme.h"'
//		statement below.
//
//	 5. Add a button to the dialog in question using the resource editor.
//		You don't have to make and style adjustments to the button.
//
//   6.	Add a variable for that button.
//
//   7.	Add '#include "ColourPickerXP.h"' in the dialog's header file and
//		change the definition of the button from 'CButton ...' to
//		'CColourPickerXP ...'.
//
//   8. Inside your OnInitDialog for the dialog, you can change the style into
//		a combobox and/or modifie the properties of the picker.
//
//	 9. Compile and enjoy.
//
//-----------------------------------------------------------------------------


#ifndef COLOURPICKERXP_INCLUDED
#define COLOURPICKERXP_INCLUDED
#pragma once

// Comment this line out if you don't want XP theme support.
#include "XPTheme.h"

// CColourPopupXP messages
#define CPN_SELCHANGE        WM_USER + 1001        // Colour Picker Selection change
#define CPN_DROPDOWN         WM_USER + 1002        // Colour Picker drop down
#define CPN_CLOSEUP          WM_USER + 1003        // Colour Picker close up
#define CPN_SELENDOK         WM_USER + 1004        // Colour Picker end OK
#define CPN_SELENDCANCEL     WM_USER + 1005        // Colour Picker end (cancelled)

void NXUILIB_EXPORTABLE AFXAPI DDX_ColourPickerXP(CDataExchange *pDX, int nIDC, COLORREF& crColour);

class NXUILIB_EXPORTABLE CColourPickerXP : public CButton
{
public:
	DECLARE_DYNCREATE(CColourPickerXP);

	//***********************************************************************
	// Name:		CColourPickerXP
	// Description:	Default constructor.
	// Parameters:	None.
	// Return:		None.	
	// Notes:		None.
	//***********************************************************************
	CColourPickerXP(void);

	//***********************************************************************
	// Name:		CColourPickerXP
	// Description:	Destructor.
	// Parameters:	None.
	// Return:		None.		
	// Notes:		None.
	//***********************************************************************
	virtual ~CColourPickerXP(void);

	//***********************************************************************
	//**                        Property Accessors                         **
	//***********************************************************************	
	__declspec(property(get=GetColor,put=SetColor))						COLORREF	Color;
	__declspec(property(get=GetDefaultColor,put=SetDefaultColor))		COLORREF	DefaultColor;
	__declspec(property(get=GetTrackSelection,put=SetTrackSelection))	BOOL		TrackSelection;
	__declspec(property(put=SetCustomText))								LPCTSTR		CustomText;
	__declspec(property(put=SetDefaultText))							LPCTSTR		DefaultText;
	__declspec(property(put=SetColoursName))							UINT		ColoursName;
	__declspec(property(put=SetRegSection))								LPCTSTR		RegSection;
	__declspec(property(put=SetRegSectionStatic))						LPCTSTR		RegSectionStatic;
	__declspec(property(get=GetStyle,put=SetStyle))						BOOL		Style;
	__declspec(property(put=SetRGBText))								LPCTSTR		RGBText;
	__declspec(property(get=GetAlwaysRGB,put=SetAlwaysRGB))				BOOL		ShowRGBAlways;

	//***********************************************************************
	// Name:		GetColor
	// Description:	Returns the current color selected in the control.
	// Parameters:	void / BOOL bTranslateDefault
	// Return:		COLORREF 
	// Notes:		None.
	//***********************************************************************
	virtual COLORREF GetColor(void) const;
	virtual COLORREF GetColor(BOOL bTranslateDefault) const;

	//***********************************************************************
	// Name:		SetColor
	// Description:	Sets the current color selected in the control.
	// Parameters:	COLORREF Color
	// Return:		None. 
	// Notes:		None.
	//***********************************************************************
	virtual void SetColor(COLORREF Color);


	//***********************************************************************
	// Name:		GetDefaultColor
	// Description:	Returns the color associated with the 'default' selection.
	// Parameters:	void
	// Return:		COLORREF 
	// Notes:		None.
	//***********************************************************************
	virtual COLORREF GetDefaultColor(void) const;

	//***********************************************************************
	// Name:		SetDefaultColor
	// Description:	Sets the color associated with the 'default' selection.
	//				The default value is COLOR_APPWORKSPACE.
	// Parameters:	COLORREF Color
	// Return:		None. 
	// Notes:		None.
	//***********************************************************************
	virtual void SetDefaultColor(COLORREF Color);

	//***********************************************************************
	// Name:		SetCustomText
	// Description:	Sets the text to display in the 'Custom' selection of the
	//				CColourPicker control, the default text is "More Colors...".
	// Parameters:	LPCTSTR tszText
	// Return:		None. 
	// Notes:		None.
	//***********************************************************************
	virtual void SetCustomText(LPCTSTR tszText);

	//***********************************************************************
	// Name:		SetDefaultText
	// Description:	Sets the text to display in the 'Default' selection of the
	//				CColourPicker control, the default text is "Automatic". If
	//				this value is set to "", the 'Default' selection will not
	//				be shown.
	// Parameters:	LPCTSTR tszText
	// Return:		None. 
	// Notes:		None.
	//***********************************************************************
	virtual void SetDefaultText(LPCTSTR tszText);

	//***********************************************************************
	// Name:		SetColoursName
	// Description:	Sets the text from the resources of the tooltips to be
	//				displayed when the pointer is on a colour.
	//				Set this to 0 to use original English names.
	// Parameters:	UINT nFirstID	(ID of Black)
	// Return:		None. 
	// Notes:		None.
	//***********************************************************************
	static void SetColoursName(UINT nFirstID = 0);

	//***********************************************************************
	// Name:		SetRegSection
	// Description:	Sets the registry section where to write custom colours.
	//				Set this to _T("") to disable.
	// Parameters:	LPCTSTR tszRegSection
	// Return:		None. 
	// Notes:		None.
	//***********************************************************************
	virtual void SetRegSection(LPCTSTR tszRegSection = _T(""));

	//***********************************************************************
	// Name:		SetRegSectionStatic
	// Description:	Sets the registry section where to write custom colours.
	//				Set this to _T("") to disable. This will be applied in
	//              all CColourPickerXPs of the application.
	// Parameters:	LPCTSTR tszRegSection
	// Return:		None. 
	// Notes:		None.
	//***********************************************************************
	static void SetRegSectionStatic(LPCTSTR tszRegSection = _T(""));

	//***********************************************************************
	// Name:		SetTrackSelection
	// Description:	Turns on/off the 'Track Selection' option of the control
	//				which shows the colors during the process of selection.
	// Parameters:	BOOL bTrack
	// Return:		None. 
	// Notes:		None.
	//***********************************************************************
	virtual void SetTrackSelection(BOOL bTrack);

	//***********************************************************************
	// Name:		GetTrackSelection
	// Description:	Returns the state of the 'Track Selection' option.
	// Parameters:	void
	// Return:		BOOL 
	// Notes:		None.
	//***********************************************************************
	virtual BOOL GetTrackSelection(void) const;

	//***********************************************************************
	// Name:		SetStyle
	// Description:	Sets the style of control to show.
	// Parameters:	BOOL bComboBoxStyle
	// Return:		None. 
	// Notes:		None.
	//***********************************************************************
	virtual void SetStyle(BOOL bComboBoxStyle);

	//***********************************************************************
	// Name:		GetTrackSelection
	// Description:	Returns TRUE if the style is set on ComboBox and FALSE if
	//				it is set on Button.
	// Parameters:	void
	// Return:		BOOL 
	// Notes:		None.
	//***********************************************************************
	virtual BOOL GetStyle(void) const;

	//***********************************************************************
	// Name:		SetRGBText
	// Description:	Sets the 3 letters used to display the RGB-value.
	// Parameters:	LPCTSTR tszRGB
	// Return:		None. 
	// Notes:		None.
	//***********************************************************************
	virtual void SetRGBText(LPCTSTR tszRGB = _T("RGB"));

	//***********************************************************************
	// Name:		SetAlwaysRGB
	// Description:	If this is set to TRUE the RGB-value of the colour will
	//				be shown even if the colour is a base colour.
	// Parameters:	BOOL bShow
	// Return:		None. 
	// Notes:		None.
	//***********************************************************************
	virtual void SetAlwaysRGB(BOOL bShow);

	//***********************************************************************
	// Name:		GetAlwaysRGB
	// Description:	Returns TRUE if the RGB-value is always shown.
	// Parameters:	void
	// Return:		BOOL 
	// Notes:		None.
	//***********************************************************************
	virtual BOOL GetAlwaysRGB(void) const;

	//{{AFX_VIRTUAL(CColourPickerXP)
    public:
    virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
    protected:
    virtual void PreSubclassWindow();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
    //}}AFX_VIRTUAL

protected:
	//{{AFX_MSG(CColourPickerXP)
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    //}}AFX_MSG
    afx_msg BOOL OnClicked();
	afx_msg void OnNMThemeChanged(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg LRESULT OnSelEndOK(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnSelEndCancel(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnSelChange(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnMouseLeave(WPARAM wParam, LPARAM lParam);

	//***********************************************************************
	// Name:		DrawArrow
	// Description:	None.
	// Parameters:	CDC* pDC
	//				RECT* pRect
	//				int iDirection
	//					0 - Down
	//					1 - Up
	//					2 - Left
	//					3 - Right
	// Return:		static None. 
	// Notes:		None.
	//***********************************************************************
	static void DrawArrow(CDC* pDC, 
						  RECT* pRect, 
						  int iDirection = 0,
						  COLORREF clrArrow = RGB(0,0,0));

	virtual void DrawHotArrow(BOOL bHot);


	DECLARE_MESSAGE_MAP()

	COLORREF m_Color;
	COLORREF m_DefaultColor;
	CString m_strDefaultText;
	CString m_strCustomText;
	BOOL	m_bPopupActive;
	BOOL	m_bTrackSelection;
	BOOL	m_bMouseOver;
	BOOL	m_bFlatMenus;

	BOOL	m_bComboBoxStyle;
	BOOL	m_bAlwaysRGB;
	CString	m_strRGBText;

	CString	m_strRegSection;
	static CString m_strRegSectionStatic;

#ifdef _THEME_H_
	CXPTheme m_xpButton, m_xpEdit, m_xpCombo;
#endif

private:

	typedef CButton _Inherited;
};

//***********************************************************************
//**                  CColourPopupXP class definition                  **
//***********************************************************************

// To hold the colours and their names
typedef struct {
    COLORREF crColour;
	TCHAR    szName[256];
} ColourTableEntry;

/////////////////////////////////////////////////////////////////////////////
// CColourPopupXP window

class CColourPopupXP : public CWnd
{
// Construction
public:
    CColourPopupXP();
    CColourPopupXP(CPoint p, COLORREF crColour, CWnd* pParentWnd,
                 LPCTSTR szDefaultText = NULL, LPCTSTR szCustomText = NULL,
				 LPCTSTR szRegSection = NULL);
    void Initialise();

// Attributes
public:

// Operations
public:
    BOOL Create(CPoint p, COLORREF crColour, CWnd* pParentWnd, 
                LPCTSTR szDefaultText = NULL, LPCTSTR szCustomText = NULL);

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CColourPopupXP)
    public:
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    //}}AFX_VIRTUAL

// Implementation
public:
    virtual ~CColourPopupXP();

protected:
    BOOL GetCellRect(int nIndex, const LPRECT& rect);
    void FindCellFromColour(COLORREF crColour);
    void SetWindowSize();
    void CreateToolTips();
    void ChangeSelection(int nIndex);
    void EndSelection(int nMessage);
    void DrawCell(CDC* pDC, int nIndex);

    int  GetIndex(int row, int col) const;
    int  GetRow(int nIndex) const;
    int  GetColumn(int nIndex) const;

	void DrawBorder(CDC* pDC, CRect rect, UINT nEdge, UINT nBorder);

// public attributes
public:
    static ColourTableEntry m_crColours[];
	static TCHAR m_strInitNames[][256];
    static int m_nNumColours;

    static COLORREF GetColour(int nIndex)              { return m_crColours[nIndex].crColour; }
    static LPCTSTR GetColourName(int nIndex)           { return m_crColours[nIndex].szName; }
// protected attributes
protected:
    int            m_nNumColumns, m_nNumRows;
    int            m_nBoxSize, m_nMargin;
    int            m_nCurrentSel;
    int            m_nChosenColourSel;
    CString        m_strDefaultText;
    CString        m_strCustomText;
    CRect          m_CustomTextRect, m_DefaultTextRect, m_WindowRect, m_BoxesRect;
    CFont          m_Font;
    CPalette       m_Palette;
    COLORREF       m_crInitialColour, m_crColour;
    CToolTipCtrl   m_ToolTip;
    CWnd*          m_pParent;

    BOOL           m_bChildWindowVisible;

	BOOL		   m_bIsXP, m_bFlatmenus;

	COLORREF	   m_clrBackground, 
				   m_clrHiLightBorder, 
				   m_clrHiLight, 
				   m_clrHiLightText, 
				   m_clrText, 
				   m_clrLoLight;

	CString        m_strRegSection;


    // Generated message map functions
protected:
    //{{AFX_MSG(CColourPopupXP)
    afx_msg void OnNcDestroy();
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnPaint();
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg BOOL OnQueryNewPalette();
    afx_msg void OnPaletteChanged(CWnd* pFocusWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
#if _MFC_VER >= 0x0700
	afx_msg void OnActivateApp(BOOL bActive, DWORD dwTask);
#else
	afx_msg void OnActivateApp(BOOL bActive, HTASK hTask);
#endif
	//}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

#endif //!COLOURPICKERXP_INCLUDED
