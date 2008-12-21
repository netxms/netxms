// AdvSplitter.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "AdvSplitter.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAdvSplitter

CAdvSplitter::CAdvSplitter()
{
   m_bInitialized = FALSE;
}

CAdvSplitter::~CAdvSplitter()
{
}

void CAdvSplitter::AssertValid(void) const
{
   CWnd::AssertValid();
	ASSERT(m_nMaxRows >= 1);
	ASSERT(m_nMaxCols >= 1);
	ASSERT(m_nRows >= 1);
	ASSERT(m_nCols >= 1);
	ASSERT(m_nRows <= m_nMaxRows);
	ASSERT(m_nCols <= m_nMaxCols);
}

BEGIN_MESSAGE_MAP(CAdvSplitter, CSplitterWnd)
	//{{AFX_MSG_MAP(CAdvSplitter)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAdvSplitter message handlers

//
// Setup splitter's pane
//

void CAdvSplitter::SetupView(int nRow, int nCol, SIZE sizeInit)
{
	m_pColInfo[nCol].nIdealSize = sizeInit.cx;
	m_pRowInfo[nRow].nIdealSize = sizeInit.cy;
}


//
// Mark splitter as initialized
//

void CAdvSplitter::InitComplete()
{
   m_bInitialized = TRUE;
}


//
// WM_SIZE message handler
//

void CAdvSplitter::OnSize(UINT nType, int cx, int cy) 
{
   if (m_bInitialized)
	   CSplitterWnd::OnSize(nType, cx, cy);
}


//
// Hide row
//

void CAdvSplitter::HideRow(int nRow)
{
   int i, nCol;
   CWnd *pWnd;

   if (m_nRows == 1)
      return;  // Cannot hide last row

   for(nCol = 0; nCol < m_nCols; nCol++)
   {
      pWnd = GetPane(nRow, nCol);
      pWnd->ShowWindow(SW_HIDE);
      pWnd->SetDlgCtrlID(-1);
      for(i = nRow + 1; i < m_nRows; i++)
      {
         pWnd = GetPane(i, nCol);
         pWnd->SetDlgCtrlID(IdFromRowCol(i - 1, nCol));
      }
   }
   m_nMaxRows--;
   m_nRows--;
   RecalcLayout();
}


//
// Hide column
//

void CAdvSplitter::HideColumn(int nCol)
{
   int i, nRow;
   CWnd *pWnd;

   if (m_nCols == 1)
      return;  // Cannot hide last column

   for(nRow = 0; nRow < m_nRows; nRow++)
   {
      pWnd = GetPane(nRow, nCol);
      pWnd->ShowWindow(SW_HIDE);
      pWnd->SetDlgCtrlID(-1);
      for(i = nCol + 1; i < m_nCols; i++)
      {
         pWnd = GetPane(nRow, i);
         pWnd->SetDlgCtrlID(IdFromRowCol(nRow, i - 1));
      }
   }
   m_nMaxCols--;
   m_nCols--;
   RecalcLayout();
}


//
// Show row
// Caller should provide pointers to all CWnd objects for new row
//

void CAdvSplitter::ShowRow(int nRow, ...)
{
   va_list args;
   CWnd *pWnd;
   int i, nCol;

   m_nMaxRows++;
   m_nRows++;
   va_start(args, nRow);
   for(nCol = 0; nCol < m_nCols; nCol++)
   {
      for(i = nRow; i < m_nRows - 1; i++)
      {
         pWnd = GetPane(i, nCol);
         pWnd->SetDlgCtrlID(IdFromRowCol(i + 1, nCol));
      }
      pWnd = va_arg(args, CWnd *);
      pWnd->SetDlgCtrlID(IdFromRowCol(nRow, nCol));
      pWnd->ShowWindow(SW_SHOW);
   }
   va_end(args);
   RecalcLayout();
}


//
// Show column
// Caller should provide pointers to all CWnd objects for new column
//

void CAdvSplitter::ShowColumn(int nCol, ...)
{
   va_list args;
   CWnd *pWnd;
   int i, nRow;

   m_nMaxCols++;
   m_nCols++;
   va_start(args, nCol);
   for(nRow = 0; nRow < m_nRows; nRow++)
   {
      for(i = nCol; i < m_nCols - 1; i++)
      {
         pWnd = GetPane(nRow, i);
         pWnd->SetDlgCtrlID(IdFromRowCol(nRow, i + 1));
      }
      pWnd = va_arg(args, CWnd *);
      pWnd->SetDlgCtrlID(IdFromRowCol(nRow, nCol));
      pWnd->ShowWindow(SW_SHOW);
   }
   va_end(args);
   RecalcLayout();
}
