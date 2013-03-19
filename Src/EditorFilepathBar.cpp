/////////////////////////////////////////////////////////////////////////////
//    WinMerge:  an interactive diff/merge utility
//    Copyright (C) 1997-2000  Thingamahoochie Software
//    Author: Dean Grimm
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
/////////////////////////////////////////////////////////////////////////////
/** 
 * @file  EditorFilepathBar.cpp
 *
 * @brief Implementation file for CEditorFilepathBar class
 */
// ID line follows -- this is updated by SVN
// $Id$

#include "StdAfx.h"
#include "Merge.h"
#include "MainFrm.h"
#include "LanguageSelect.h"
#include "Common/coretools.h"
#include "paths.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/** 
 * @brief Format the path for display in header control. 
 *
 * Formats path so it fits to given length, tries to end lines after
 * slash characters.
 *
 * @param [in] pDC Pointer to draw context.
 * @param [in] maxWidth Maximum width of the string in the GUI.
 * @param [in,out] sFilepath:
 * - in: string to format
 * - out: formatted string
 * @return Number of lines path is splitted to.
 */
static void FormatFilePathForDisplayWidth(HSurface *pDC, int maxWidth, String & sFilepath)
{
	int iBegin = 0;
	while (1)
	{
		String line;

		// find the next truncation point
		int iEndMin = 0;
		int iEndMax = sFilepath.size() - iBegin + 1;
		while(1)
		{
			int iEnd = (iEndMin + iEndMax) / 2;
			if (iEnd == iEndMin)
				break;
			line = sFilepath.substr(iBegin, iEnd);
			SIZE ext;
			if (pDC->GetTextExtent(line.c_str(), line.size(), &ext))
			{
				if (ext.cx > maxWidth)
					iEndMax = iEnd;
				else
					iEndMin = iEnd;
			}
		};
		ASSERT(iEndMax == iEndMin+1);

		// here iEndMin is the last character displayed in maxWidth

		// exit the loop if we can display the remaining characters with no truncation
		if (iBegin + iEndMin == sFilepath.size())
			break;

		// truncate the text to the previous "\" if possible
		line = sFilepath.substr(iBegin, iEndMin);
		int lastSlash = line.rfind(_T('\\'));
		if (lastSlash >= 0)
			iEndMin = lastSlash + 1;

		sFilepath.insert(iBegin + iEndMin, _T("\n"));
		iBegin += iEndMin + 2;
	}
}

static void RefreshDisplayText(HEdit *pEdit, LPCTSTR szOriginalText)
{
	String line = szOriginalText;
	const BOOL bModify = pEdit->GetModify();
	if (bModify && (pEdit->GetStyle() & ES_READONLY) != 0)
		line.insert(0, _T("* "));
	paths_CompactPath(pEdit, line);
	pEdit->SetWindowText(line.c_str());
	pEdit->SetModify(bModify);
}

/**
 * @brief Constructor.
 */
CEditorFilePathBar::CEditorFilePathBar(
	const LONG *FloatScript, const LONG *SplitScript)
	: CFloatState(FloatScript)
	, CSplitState(SplitScript)
	, ODialog(IDD_EDITOR_HEADERBAR)
	, m_rgOriginalText(2)
	, m_pToolTips(NULL)
{
	m_rgActive[0] = FALSE;
	m_rgActive[1] = FALSE;
}

/**
 * @brief Destructor.
 */
CEditorFilePathBar::~CEditorFilePathBar()
{
}

/**
 * @brief Create the window.
 * This function subclasses the edit controls.
 * @param [in] pParentWnd Parent window for edit controls.
 * @return TRUE if succeeded, FALSE otherwise.
 */
BOOL CEditorFilePathBar::Create(HWND parent)
{
	CFloatState::Clear();
	LanguageSelect.Create(*this, parent);
	SetDlgCtrlID(0x1000);
	m_Edit[0] = static_cast<HEdit *>(GetDlgItem(IDC_STATIC_TITLE_LEFT));
	m_Edit[1] = static_cast<HEdit *>(GetDlgItem(IDC_STATIC_TITLE_RIGHT));
	m_pToolTips = HToolTips::Create(WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP);
	TOOLINFO ti;
	ti.cbSize = TTTOOLINFO_V1_SIZE;
	ti.uFlags = TTF_CENTERTIP | TTF_IDISHWND | TTF_SUBCLASS;
	ti.hwnd = m_hWnd;
	ti.hinst = NULL;
	ti.lpszText = LPSTR_TEXTCALLBACK;
	ti.uId = reinterpret_cast<UINT_PTR>(m_Edit[0]->m_hWnd);
	m_pToolTips->AddTool(&ti);
	ti.uId = reinterpret_cast<UINT_PTR>(m_Edit[1]->m_hWnd);
	m_pToolTips->AddTool(&ti);
	// Send TTM_SETMAXTIPWIDTH to allow for \n in tooltips
	m_pToolTips->SetMaxTipWidth(5000);
	m_pToolTips->Activate();
	return TRUE;
};

/**
 * @brief Called when tooltip is about to be shown.
 * In this function we set the tooltip text shown.
 */
void CEditorFilePathBar::OnToolTipNotify(TOOLTIPTEXT *pTTT)
{
	if (pTTT->uFlags & TTF_IDISHWND)
	{
		// idFrom is actually the HWND of the CEdit 
		UINT nID = reinterpret_cast<HWindow *>(pTTT->hdr.idFrom)->GetDlgCtrlID();
		if (nID == IDC_STATIC_TITLE_LEFT || nID == IDC_STATIC_TITLE_RIGHT)
		{
			int pane = nID - IDC_STATIC_TITLE_LEFT;
			// compute max width as 80% of full screen width
			RECT rect;
			SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);
			int maxWidth = MulDiv(rect.right - rect.left, 80, 100);
			HToolTips *pToolTips = reinterpret_cast<HToolTips *>(pTTT->hdr.hwndFrom);
			// use the tooltip font
			HFont *pFont = pToolTips->GetFont();
			HSurface *pDC = GetDC();
			HGdiObj *pOldFont = pDC->SelectObject(pFont);
			// fill in the returned structure
			m_sToolTipString = GetTitle(pane);
			/* make it VERY long to test formatting
			m_sToolTipString += m_sToolTipString;
			m_sToolTipString += m_sToolTipString;
			m_sToolTipString += m_sToolTipString;
			m_sToolTipString += m_sToolTipString;
			m_sToolTipString += m_sToolTipString;
			m_sToolTipString += m_sToolTipString;/**/
			FormatFilePathForDisplayWidth(pDC, maxWidth, m_sToolTipString);
			pTTT->lpszText = const_cast<LPTSTR>(m_sToolTipString.c_str());
			// set old font back
			pDC->SelectObject(pOldFont);
			ReleaseDC(pDC);
		}
	}
}

const String &CEditorFilePathBar::GetTitle(int pane)
{
	String &sOriginalText = m_rgOriginalText[pane];
	if (m_Edit[pane]->GetModify() && (m_Edit[pane]->GetStyle() & ES_READONLY) == 0)
		m_Edit[pane]->GetWindowText(sOriginalText);
	return sOriginalText;
}

/** 
 * @brief Set the path for one side
 *
 * @param [in] pane Index (0-based) of pane to update.
 * @param [in] lpszString New text for pane.
 * @param [in] bDirty Whether content has been edited after load/save.
 */
void CEditorFilePathBar::SetText(int pane, LPCTSTR lpszString, BOOL bDirty, BUFFERTYPE bufferType)
{
	ASSERT(pane >= 0 && pane < _countof(m_Edit));
	// Check for NULL since window may be closing..
	if (m_hWnd == NULL)
		return;
	// If the buffer is unnamed and unsaved, make the title editable
	if (bufferType == BUFFER_UNNAMED)
	{
		// Once the unnamed/unsaved title is set, leave it as is
		if (m_Edit[pane]->GetModify())
			return;
		m_Edit[pane]->SetReadOnly(FALSE);
		bDirty = TRUE;
	}
	// Hide magic prefix from users
	paths_UndoMagic(m_rgOriginalText[pane] = lpszString);
	SetModify(pane, bDirty);
}

/** 
 * @brief Get the modification flag for one side
 *
 * @param [in] pane Index (0-based) of pane to query.
 */
BOOL CEditorFilePathBar::GetModify(int pane)
{
	return m_Edit[pane]->GetModify();
}

/** 
 * @brief Set the modification flag for one side
 *
 * @param [in] pane Index (0-based) of pane to update.
 * @param [in] bDirty Whether content has been edited after load/save.
 */
void CEditorFilePathBar::SetModify(int pane, BOOL bDirty)
{
	m_Edit[pane]->SetModify(bDirty);
	RefreshDisplayText(m_Edit[pane], m_rgOriginalText[pane].c_str());
}

/** 
 * @brief Set the active status for one status (change the appearance)
 *
 * @param [in] pane Index (0-based) of pane to update.
 * @param [in] bActive If TRUE activates pane, FALSE deactivates.
 */
void CEditorFilePathBar::SetActive(int pane, bool bActive)
{
	ASSERT(pane >= 0 && pane < _countof(m_Edit));
	// Check for NULL since window may be closing..
	if (m_rgActive[pane] == bActive)
		return;
	m_rgActive[pane] = bActive;
	if (m_hWnd)
		m_Edit[pane]->RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_FRAME);
}

HEdit *CEditorFilePathBar::GetControlRect(int pane, LPRECT prc)
{
	ASSERT(pane >= 0 && pane < _countof(m_Edit));
	m_Edit[pane]->GetWindowRect(prc);
	ScreenToClient(prc);
	return m_Edit[pane];
}

HBRUSH CEditorFilePathBar::OnCtlColorPane(HSurface *pDC, bool bActive)
{
	int nTextColor = bActive ? COLOR_CAPTIONTEXT : COLOR_BTNTEXT;
	int nBackColor = bActive ? COLOR_GRADIENTACTIVECAPTION : COLOR_BTNFACE;
	HBRUSH hBackBrush = GetSysColorBrush(nBackColor);
	// If assigned nBackColor is not supported, or the text color is relatively
	// bright (different from black at least), fall back to COLOR_ACTIVECAPTION.
	if (bActive && (hBackBrush == NULL || GetSysColor(nTextColor) != 0))
	{
		nBackColor = COLOR_ACTIVECAPTION;
		hBackBrush = GetSysColorBrush(nBackColor);
	}
	// set text color
	pDC->SetTextColor(GetSysColor(nTextColor));
	// set the text's background color
	pDC->SetBkColor(GetSysColor(nBackColor));
	// return the brush used for background this sets control background
	return hBackBrush;
}

void CEditorFilePathBar::OnContextMenu(POINT point)
{
	HWindow *pWindow = m_pWnd;
	if (point.x == -1 && point.y == -1)
	{
		point.x = point.y = 5;
		pWindow = HWindow::GetFocus();
		pWindow->ClientToScreen(&point);
	}
	else
	{
		POINT pt = point;
		pWindow->ScreenToClient(&pt);
		pWindow = pWindow->ChildWindowFromPoint(pt);
	}
	int pane = pWindow->GetDlgCtrlID();
	switch (pane)
	{
	case IDC_STATIC_TITLE_LEFT:
	case IDC_STATIC_TITLE_RIGHT:
		pane -= IDC_STATIC_TITLE_LEFT;
		break;
	default:
		return;
	}
	const String &sOriginalText = m_rgOriginalText[pane];
	HMenu *const pMenu = LanguageSelect.LoadMenu(IDR_POPUP_EDITOR_HEADERBAR);
	HMenu *const pSub = pMenu->GetSubMenu(0);

	if (paths_EndsWithSlash(sOriginalText.c_str()))
		// no filename, we have to disable the unwanted menu entry
		pSub->EnableMenuItem(ID_EDITOR_COPY_FILENAME, MF_GRAYED);

	// invoke context menu
	// we don't want to use the main application handlers, so we
	// use flags TPM_NONOTIFY | TPM_RETURNCMD
	// and handle the command after TrackPopupMenu
	int command = pSub->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON |
		TPM_NONOTIFY  | TPM_RETURNCMD, point.x, point.y, theApp.m_pMainWnd->m_pWnd);

	pMenu->DestroyMenu();

	// compute the beginning of the text to copy (in OriginalText)
	int iBegin = 0;
	switch (command)
	{
	case ID_EDITOR_COPY_FILENAME:
		iBegin = sOriginalText.rfind(_T('\\')) + 1;
		break;
	case ID_EDITOR_COPY_PATH:
		// pass the heading "*" for modified files
		iBegin = sOriginalText[0] == _T('*') ? 2 : 0;
		break;
	default:
		return;
	}
	if (OpenClipboard())
	{
		UniStdioFile file;
		file.SetUnicoding(UCS2LE);
		if (HGLOBAL hMem = file.CreateStreamOnHGlobal())
		{
			int iEnd = sOriginalText.size();
			file.WriteString(sOriginalText.c_str() + iBegin, iEnd - iBegin);
			file.WriteString(_T(""), 1);
			EmptyClipboard();
			SetClipboardData(CF_UNICODETEXT, hMem);
		}
		CloseClipboard();
	}
}

LRESULT CEditorFilePathBar::WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_WINDOWPOSCHANGING:
		if (CFloatState::Float(reinterpret_cast<WINDOWPOS *>(lParam)))
		{
			RefreshDisplayText(m_Edit[0], m_rgOriginalText[0].c_str());
			RefreshDisplayText(m_Edit[1], m_rgOriginalText[1].c_str());
		}
		break;
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_MOUSEMOVE:
		if (!COptionsMgr::Get(OPT_RESIZE_PANES) &&
			CSplitState::Split(m_hWnd) == 2)
		{
			RefreshDisplayText(m_Edit[0], m_rgOriginalText[0].c_str());
			RefreshDisplayText(m_Edit[1], m_rgOriginalText[1].c_str());
		}
		break;
	case WM_NOTIFY:
		switch (reinterpret_cast<NMHDR *>(lParam)->code)
		{
		case TTN_NEEDTEXT:
			OnToolTipNotify(reinterpret_cast<TOOLTIPTEXT *>(lParam));
			break;
		case NM_CLICK:
			if (wine_version)
			{
				switch (int idFrom = static_cast<int>(
					reinterpret_cast<NMHDR *>(lParam)->idFrom))
				{
				case 0x6000:
				case 0x6001:
					if (HWindow *pContentView = GetDlgItem(idFrom - 0x5000))
						return pContentView->SendMessage(uMsg, wParam, lParam);
					break;
				}
			}
			break;
		}
		break;
	case WM_CTLCOLOREDIT:
	case WM_CTLCOLORSTATIC:
		switch (int pane = reinterpret_cast<HWindow *>(lParam)->GetDlgCtrlID())
		{
		case IDC_STATIC_TITLE_RIGHT:
		case IDC_STATIC_TITLE_LEFT:
			return reinterpret_cast<LRESULT>(OnCtlColorPane(
				reinterpret_cast<HSurface *>(wParam),
				m_rgActive[pane - IDC_STATIC_TITLE_LEFT]));
		}
		break;
	case WM_COMMAND:
		switch (HIWORD(wParam))
		{
		case EN_SETFOCUS:
			RegisterHotKey(m_hWnd, 1, 0, VK_APPS);
			break;
		case EN_KILLFOCUS:
			UnregisterHotKey(m_hWnd, 1);
			break;
		}
		break;
	case WM_MOUSEACTIVATE:
		if (HIWORD(lParam) == WM_RBUTTONDOWN)
		{
			POINT pt;
			GetCursorPos(&pt);
			HWND hwndHit = ::WindowFromPoint(pt);
			if ((hwndHit == m_Edit[0]->m_hWnd) || (hwndHit == m_Edit[1]->m_hWnd))
			{
				SetCapture();
				return MA_NOACTIVATEANDEAT;
			}
		}
		break;
	case WM_RBUTTONUP:
		ReleaseCapture();
		break;
	case WM_HOTKEY:
		wParam = reinterpret_cast<WPARAM>(m_hWnd);
		lParam = MAKELPARAM(-1, -1);
		// fall through
	case WM_CONTEXTMENU:
		if (wParam == reinterpret_cast<WPARAM>(m_hWnd))
		{
			POINT point;
			POINTSTOPOINT(point, lParam);
			OnContextMenu(point);
			return 0;
		}
		break;
	case WM_DESTROY:
		m_pToolTips->DestroyWindow();
		break;
	}
	return ODialog::WindowProc(uMsg, wParam, lParam);
}
