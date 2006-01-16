#include "stdafx.h"
#include "nxuilib.h"


//
// Constants
//

#define IDT_TIMER1 100
#define IDT_TIMER2 101


//
// Data for notification window
//

typedef struct t_NotifyData
{
	int scrollDelay;
	int showTime;
	int shift;

	// data
	int priority;
	TCHAR title[128];
	TCHAR text[1024];
} NOTIFY_DATA;


//
// Load icon for given priority
//

static HANDLE LoadIconForPriority(int p)
{
	int iconId = IDI_SEVERITY_CRITICAL;

	switch(p)
	{
	case 0: // normal
		iconId = IDI_SEVERITY_NORMAL;
		break;
	case 1: // warning
		iconId = IDI_SEVERITY_WARNING;
		break;
	case 2: // minor
		iconId = IDI_SEVERITY_MINOR;
		break;
	case 3: // major
		iconId = IDI_SEVERITY_MAJOR;
		break;
	case 4: // critical
		iconId = IDI_SEVERITY_CRITICAL;
		break;
	}

	HANDLE hIcon = (HICON)LoadImage(g_hInstance,
						MAKEINTRESOURCE(iconId),
						IMAGE_ICON,
						0, 0,
						LR_DEFAULTCOLOR | LR_SHARED);

	return hIcon;
}


//
// Dialog procedure for popup window
//

static BOOL CALLBACK DlgFunc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static RECT dialogRect;
	static RECT screenRect;
	static NOTIFY_DATA nd;
	static HICON hIcon;
	static HFONT hFont1, hFont2;
	static int fontHeight;

	switch(message)
	{
	//////////////////////////////////////////////////////////////////////////
	case WM_INITDIALOG:
		{
			memcpy(&nd, (const void *)lParam, sizeof(NOTIFY_DATA));

			hIcon = (HICON)LoadIconForPriority(nd.priority);

			HDC hdc = GetDC(NULL);
			hFont1 = CreateFont(
				-MulDiv(8, GetDeviceCaps(hdc, LOGPIXELSY), 72),
				0,
				0,
				0,
				FW_NORMAL,
				FALSE,
				FALSE,
				FALSE,
				DEFAULT_CHARSET,
				OUT_DEFAULT_PRECIS,
				CLIP_DEFAULT_PRECIS,
				ANTIALIASED_QUALITY,
				VARIABLE_PITCH,
				_T("MS Sans Serif"));

			hFont2 = CreateFont(
				-MulDiv(9, GetDeviceCaps(hdc, LOGPIXELSY), 72),
				0,
				0,
				0,
				FW_BOLD,
				FALSE,
				FALSE,
				FALSE,
				DEFAULT_CHARSET,
				OUT_DEFAULT_PRECIS,
				CLIP_DEFAULT_PRECIS,
				ANTIALIASED_QUALITY,
				VARIABLE_PITCH,
				_T("MS Sans Serif"));

			TEXTMETRIC tm;
			SelectObject(hdc, hFont1);
			GetTextMetrics(hdc, &tm);
			ReleaseDC(NULL, hdc);
			fontHeight = tm.tmHeight;

			memset(&screenRect, 0, sizeof(screenRect));
			GetWindowRect(hWnd, &dialogRect);
			SystemParametersInfo(SPI_GETWORKAREA, 0, &screenRect, 0);
			MoveWindow(hWnd,
				screenRect.right - dialogRect.left - dialogRect.right,
				screenRect.bottom,
				dialogRect.right - dialogRect.left,
				dialogRect.bottom - dialogRect.top,
			TRUE);
			SetTimer(hWnd, IDT_TIMER1, nd.scrollDelay, NULL);
			if (nd.showTime > 0)
			{
				SetTimer(hWnd, IDT_TIMER2, nd.showTime, NULL);
			}
		}
		break;
	//////////////////////////////////////////////////////////////////////////
	case WM_CLOSE:
		KillTimer(hWnd, IDT_TIMER1);
		KillTimer(hWnd, IDT_TIMER2);
		EndDialog(hWnd, 0);
		DeleteObject(hIcon);
		DeleteObject(hFont1);
		DeleteObject(hFont2);
		break;
	//////////////////////////////////////////////////////////////////////////
	case WM_COMMAND:
		SendMessage(hWnd, WM_CLOSE, 0, 0);
		break;
	//////////////////////////////////////////////////////////////////////////
	case WM_LBUTTONDOWN:
		MessageBox(NULL, _T("start smth..."), _T("demo"), MB_OK);
		break;
	//////////////////////////////////////////////////////////////////////////
	case WM_RBUTTONDOWN:
		SendMessage(hWnd, WM_CLOSE, 0, 0);
		break;
	//////////////////////////////////////////////////////////////////////////
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hWnd, &ps);

			SetBkMode(hdc, TRANSPARENT);

			// icon
			DrawIcon(hdc, 4, 4, hIcon);

			// title
			SelectObject(hdc, hFont1);
			RECT r;
			r.left = 43;
			r.right = (dialogRect.right - dialogRect.left) - 4;
			r.top = 13;
			r.bottom = fontHeight + r.top;
			DrawText(hdc, nd.title, _tcslen(nd.title), &r, DT_NOPREFIX);

			// text
			SelectObject(hdc, hFont2);
			r.left = 2;
			r.right = dialogRect.right - dialogRect.left - 2;
			r.top = 40;
			r.bottom = dialogRect.bottom - dialogRect.top - 2;
			DrawText(hdc, nd.text, _tcslen(nd.text), &r,
				DT_NOPREFIX | DT_WORDBREAK | DT_EDITCONTROL);

			EndPaint(hWnd, &ps);
		}
		break;
	//////////////////////////////////////////////////////////////////////////
	case WM_TIMER:
		switch(wParam)
		{
		case IDT_TIMER1:
			{
				int newY;

				GetWindowRect(hWnd, &dialogRect);

				if (dialogRect.bottom - nd.shift < screenRect.bottom)
				{
					newY = screenRect.bottom - (dialogRect.bottom - dialogRect.top);
				}
				else
				{
					newY = dialogRect.top - nd.shift;
				}

				MoveWindow(hWnd,
					dialogRect.left,
					newY,
					dialogRect.right - dialogRect.left,
					dialogRect.bottom - dialogRect.top,
					TRUE);

				if (dialogRect.bottom <= screenRect.bottom)
				{
					KillTimer(hWnd, IDT_TIMER1); 
				}
			}
			break;
		case IDT_TIMER2:
			SendMessage(hWnd, WM_CLOSE, 0, 0);
			break;
		default:
			break;
		}
		break;
	//////////////////////////////////////////////////////////////////////////
	default:
		return FALSE;
	}

	return TRUE;
}


//
// Display popup window in bottom right corner of the screen
//

void NXUILIB_EXPORTABLE ShowNetXMSPopUp(int priority, int showTime,
                                        TCHAR *title, TCHAR *text)
{
	NOTIFY_DATA nd;

	nd.scrollDelay = 25;
	nd.showTime = showTime * 1000;
	nd.shift = 25;

	nx_strncpy(nd.title, title, (sizeof(nd.title) / sizeof(TCHAR)));
	nx_strncpy(nd.text, text, (sizeof(nd.text) / sizeof(TCHAR)));
	nd.priority = priority;

	DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_NOTIFY),
                  NULL, DlgFunc, (LONG)&nd);
}
