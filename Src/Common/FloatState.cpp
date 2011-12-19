/*/FloatState.cpp

[The MIT license]

Copyright (c) Jochen Tucht

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

DATE:		BY:					DESCRIPTION:
==========	==================	================================================
2006-02-19	Jochen Tucht		Created
2006-09-12	Jochen Neubeck		Be aware of SWP_FRAMECHANGED
2006-09-21	Jochen Neubeck		Avoid unnecessary SWP_NOCOPYBITS
2006-09-23	Jochen Neubeck		New methods AdjustMax(), AdjustHit()
2006-10-08	Jochen Neubeck		Fix avoid unnecessary SWP_NOCOPYBITS
2007-04-25	Jochen Neubeck		Assume transparent STATICs are group boxes
2009-04-30	Jochen Neubeck		Fix AdjustMax() for left or top aligned taskbar
2011-03-27	Jochen Neubeck		New method CallWindowProc() to ease subclassing
2011-04-01	Jochen Neubeck		Preserve edit selection in dropdown comboboxes
2011-05-14	Jochen Neubeck		SWP_FRAMECHANGED | SWP_NOCOPYBITS forces layout
2011-07-24	Jochen Neubeck		Stop moving when SC_SIZE shrinks beyond limits
2011-12-01	Jochen Neubeck		Fix SC_SIZE with left or top aligned taskbar
*/
#include "StdAfx.h"
#include "FloatState.h"

static HWND NTAPI GetNextDlgGroupSibling(HWND hwndThis)
{
	HWND hwndNext = GetWindow(hwndThis, GW_HWNDNEXT);
	if (hwndNext == NULL || GetWindowLong(hwndNext, GWL_STYLE) & WS_GROUP)
	{
		hwndNext = hwndThis;
		while (hwndNext && !(GetWindowLong(hwndNext, GWL_STYLE) & WS_GROUP))
		{
			hwndNext = GetWindow(hwndNext, GW_HWNDPREV);
		}
	}
	return hwndNext;
}

void CFloatState::Clear()
{
	flags = 0;
}

BOOL CFloatState::Float(WINDOWPOS *pParam)
{
	UINT wpflags = pParam->flags;
	BOOL Float = (wpflags & SWP_FRAMECHANGED) && !(wpflags & SWP_NOSIZE) || !(wpflags & (SWP_NOSIZE | SWP_NOCOPYBITS));
	if (Float && FloatScript)
	{
		const LONG *pScript = FloatScript;
		WINDOWPLACEMENT wp;
		wp.length = sizeof wp;
		HWND hwndOuter = pParam->hwnd;
		if (::GetWindowLong(hwndOuter, GWL_STYLE) & WS_CHILD)
			::GetWindowPlacement(hwndOuter, &wp);
		else
			::GetWindowRect(hwndOuter, &wp.rcNormalPosition);
		if (flags == 0)
		{
			cx = wp.rcNormalPosition.right - wp.rcNormalPosition.left;
			cy = wp.rcNormalPosition.bottom - wp.rcNormalPosition.top;
		}
		if (pParam->cx < cx)
		{
			if (pParam->x != wp.rcNormalPosition.left)
				pParam->x -= cx - pParam->cx;
			pParam->cx = cx;
		}
		if (pParam->cy < cy)
		{
			if (pParam->y != wp.rcNormalPosition.top)
				pParam->y -= cy - pParam->cy;
			pParam->cy = cy;
		}
		int cxSize = pParam->cx;
		int cySize = pParam->cy;
		int cxGrow = cxSize - (wp.rcNormalPosition.right - wp.rcNormalPosition.left);
		int cyGrow = cySize - (wp.rcNormalPosition.bottom - wp.rcNormalPosition.top);
		int cxPull = cxSize - cx;
		int cyPull = cySize - cy;
		while (DWORD nIDCtrl = *pScript++)
		{
			DWORD dwFlags = *pScript++;
			flags |= dwFlags;
			HWND hwndInner = ::GetDlgItem(hwndOuter, LOWORD(nIDCtrl));
			RECT rect =
			{
				LOBYTE(LOWORD(dwFlags)), HIBYTE(LOWORD(dwFlags)),
				LOBYTE(HIWORD(dwFlags)), HIBYTE(HIWORD(dwFlags))
			};
			HWND hwndBreak = hwndInner;
			for (;;)
			{
				WORD wIDCtrl = (WORD)::GetDlgCtrlID(hwndInner);
				if (wIDCtrl == LOWORD(nIDCtrl) || wIDCtrl == 0xFFFF)
				{
					::GetWindowPlacement(hwndInner, &wp);
					if (wp.ptMinPosition.x == -1 && wp.ptMinPosition.y == -1)
					{
						*(LPRECT)&wp.ptMinPosition = wp.rcNormalPosition;
					}
					RECT rcNormalPosition = wp.rcNormalPosition;
					if (cxGrow || (wpflags & SWP_NOCOPYBITS))
					{
						wp.rcNormalPosition.left = wp.ptMinPosition.x + MulDiv(cxPull, rect.left, 250);
						wp.rcNormalPosition.right = wp.ptMaxPosition.x + MulDiv(cxPull, rect.right, 250);
					}
					if (cyGrow || (wpflags & SWP_NOCOPYBITS))
					{
						wp.rcNormalPosition.top = wp.ptMinPosition.y + MulDiv(cyPull, rect.top, 250);
						wp.rcNormalPosition.bottom = wp.ptMaxPosition.y + MulDiv(cyPull, rect.bottom, 250);
					}
					if (!::EqualRect(&rcNormalPosition, &wp.rcNormalPosition))
					{
						pParam->flags |= SWP_NOCOPYBITS;
						if ((::GetWindowLong(hwndInner, GWL_STYLE) & WS_VISIBLE) == 0)
						{
							wp.showCmd = SW_HIDE;
						}
						// Guess whether this is a dropdown combobox
						if ((::GetWindowLong(hwndInner, GWL_EXSTYLE) & WS_EX_CONTROLPARENT) == 0 &&
							(::SendDlgItemMessage(hwndInner, 1001, WM_GETDLGCODE, 0, 0) & DLGC_HASSETSEL))
						{
							LPARAM sel = ::SendMessage(hwndInner, CB_GETEDITSEL, 0, 0);
							::SetWindowPlacement(hwndInner, &wp);
							::SendMessage(hwndInner, CB_SETEDITSEL, 0, sel);
						}
						else
						{
							::SetWindowPlacement(hwndInner, &wp);
						}
					}
				}

				if (!HIWORD(nIDCtrl))
					break;

				hwndInner = ::GetNextDlgGroupSibling(hwndInner);
				if (hwndInner == hwndBreak)
					break;

				if ((::GetWindowLong(hwndInner, GWL_STYLE) & WS_GROUP) &&
					!(::GetWindowLong(hwndInner, GWL_EXSTYLE) & WS_EX_TRANSPARENT))
				{
					nIDCtrl &= ~(R2L | R2R | B2T | B2B);
				}

				rect.left = rect.top = rect.right = rect.bottom = 0;
				if (nIDCtrl & L2L)
				{
					rect.left = LOBYTE(LOWORD(dwFlags));
				}
				if (nIDCtrl & L2R)
				{
					rect.right = LOBYTE(LOWORD(dwFlags));
				}
				if (nIDCtrl & T2T)
				{
					rect.top = HIBYTE(LOWORD(dwFlags));
				}
				if (nIDCtrl & T2B)
				{
					rect.bottom = HIBYTE(LOWORD(dwFlags));
				}
				if (nIDCtrl & R2L)
				{
					rect.left = LOBYTE(HIWORD(dwFlags));
				}
				if (nIDCtrl & R2R)
				{
					rect.right = LOBYTE(HIWORD(dwFlags));
				}
				if (nIDCtrl & B2T)
				{
					rect.top = HIBYTE(HIWORD(dwFlags));
				}
				if (nIDCtrl & B2B)
				{
					rect.bottom = HIBYTE(HIWORD(dwFlags));
				}
			}
		}
		if ((flags & 0x00FF00FF) == 0)
		{
			pParam->cx = cx;
		}
		if ((flags & 0xFF00FF00) == 0)
		{
			pParam->cy = cy;
		}
	}
	return Float;
}

BOOL CFloatState::AdjustMax(HWND hWnd, MINMAXINFO *pParam)
{
	DWORD dwStyle = ::GetWindowLong(hWnd, GWL_STYLE);
	DWORD dwExStyle = ::GetWindowLong(hWnd, GWL_EXSTYLE);
	RECT rect;
	if (!::SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0))
		return FALSE;
	if (!::AdjustWindowRectEx(&rect, dwStyle & ~WS_CAPTION, FALSE, dwExStyle))
		return FALSE;
	pParam->ptMaxSize.x = rect.right - rect.left;
	pParam->ptMaxSize.y = rect.bottom - rect.top;
	return TRUE;
}

UINT CFloatState::AdjustHit(UINT uHitTest)
{
	if (uHitTest >= HTSIZEFIRST && uHitTest <= HTSIZELAST)
	{
		const UINT XLOCK = 0x20;
		const UINT YLOCK = 0x40;
		UINT uSwitch = uHitTest;
		if ((flags & 0x00FF00FF) == 0)
			uSwitch |= XLOCK;
		if ((flags & 0xFF00FF00) == 0)
			uSwitch |= YLOCK;
		switch (uSwitch)
		{
		case HTLEFT | XLOCK:
		case HTRIGHT | XLOCK:
		case HTTOP | YLOCK:
		case HTBOTTOM | YLOCK:
			uHitTest = HTNOWHERE;
			break;
		case HTTOPLEFT | YLOCK:
		case HTBOTTOMLEFT | YLOCK:
			uHitTest = HTLEFT;
			break;
		case HTTOPRIGHT | YLOCK:
		case HTBOTTOMRIGHT | YLOCK:
			uHitTest = HTRIGHT;
			break;
		case HTTOPLEFT | XLOCK:
		case HTTOPRIGHT | XLOCK:
			uHitTest = HTTOP;
			break;
		case HTBOTTOMLEFT | XLOCK:
		case HTBOTTOMRIGHT | XLOCK:
			uHitTest = HTBOTTOM;
			break;
		}
	}
	return uHitTest;
}

LRESULT CFloatState::CallWindowProc(WNDPROC pfnSuper, HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT lResult = ::CallWindowProc(pfnSuper, hWnd, uMsg, wParam, lParam);
	switch (uMsg)
	{
	case WM_NCHITTEST:
		lResult = CFloatState::AdjustHit(lResult);
		break;
	case WM_WINDOWPOSCHANGING:
		CFloatState::Float(reinterpret_cast<WINDOWPOS *>(lParam));
		break;
	case WM_GETMINMAXINFO:
		CFloatState::AdjustMax(hWnd, reinterpret_cast<MINMAXINFO *>(lParam));
		break;
	case WM_ERASEBKGND:
		RECT rect;
		::GetClientRect(hWnd, &rect);
		rect.left = rect.right - ::GetSystemMetrics(SM_CXVSCROLL);
		rect.top = rect.bottom - ::GetSystemMetrics(SM_CYHSCROLL);
		::DrawFrameControl((HDC)wParam, &rect, DFC_SCROLL, DFCS_SCROLLSIZEGRIP);
		break;
	}
	return lResult;
}
