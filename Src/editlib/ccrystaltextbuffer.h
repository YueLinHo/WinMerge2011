////////////////////////////////////////////////////////////////////////////
//  File:       ccrystaltextbuffer.h
//  Version:    1.0.0.0
//  Created:    29-Dec-1998
//
//  Author:     Stcherbatchenko Andrei
//  E-mail:     windfall@gmx.de
//
//  Interface of the CCrystalTextBuffer class, a part of Crystal Edit -
//  syntax coloring text editor.
//
//  You are free to use or modify this code to the following restrictions:
//  - Acknowledge me somewhere in your about box, simple "Parts of code by.."
//  will be enough. If you can't (or don't want to), contact me personally.
//  - LEAVE THIS HEADER INTACT
////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
//  19-Jul-99
//      Ferdinand Prantl:
//  +   FEATURE: see cpps ...
//
//  ... it's being edited very rapidly so sorry for non-commented
//        and maybe "ugly" code ...
////////////////////////////////////////////////////////////////////////////
/**
 * @file  ccrystaltextbuffer.h
 *
 * @brief Declaration file for CCrystalTextBuffer.
 */
// ID line follows -- this is updated by SVN
// $Id$


#pragma once

#include "LineInfo.h"
#include "UndoRecord.h"
#include "ccrystaltextview.h"

enum LINEFLAGS
{
	LF_BOOKMARK_FIRST = 0x00000001L,
	LF_EXECUTION = 0x00010000L,
	LF_BREAKPOINT = 0x00020000L,
	LF_COMPILATION_ERROR = 0x00040000L,
	LF_BOOKMARKS = 0x00080000L,
	LF_INVALID_BREAKPOINT = 0x00100000L
};

#define LF_BOOKMARK(id)     (LF_BOOKMARK_FIRST << id)

enum CRLFSTYLE
{
	CRLF_STYLE_AUTOMATIC = -1,
	CRLF_STYLE_DOS = 0,
	CRLF_STYLE_UNIX = 1,
	CRLF_STYLE_MAC = 2,
	CRLF_STYLE_MIXED = 3
};

enum EDITMODE
{
	EDIT_MODE_INSERT = 0,
	EDIT_MODE_OVERWRITE = 1,
	EDIT_MODE_READONLY = 2
};

enum
{
	CE_ACTION_UNKNOWN = 0,
	CE_ACTION_PASTE = 1,
	CE_ACTION_DELSEL = 2,
	CE_ACTION_CUT = 3,
	CE_ACTION_TYPING = 4,
	CE_ACTION_BACKSPACE = 5,
	CE_ACTION_INDENT = 6,
	CE_ACTION_DRAGDROP = 7,
	CE_ACTION_REPLACE = 8,
	CE_ACTION_DELETE = 9,
	CE_ACTION_AUTOINDENT = 10,
	CE_ACTION_AUTOCOMPLETE = 11,
	CE_ACTION_AUTOEXPAND = 12,
	CE_ACTION_LOWERCASE = 13,
	CE_ACTION_UPPERCASE = 14,
	CE_ACTION_SWAPCASE = 15,
	CE_ACTION_CAPITALIZE = 16,
	CE_ACTION_SENTENCIZE = 17,
	CE_ACTION_RECODE = 18, 
	CE_ACTION_SPELL = 19
							//  ...
							//  Expandable: user actions allowed
};


/////////////////////////////////////////////////////////////////////////////
// CUpdateContext class

class EDITPADC_CLASS CUpdateContext
{
public:
	virtual void RecalcPoint(POINT & ptPoint) = 0;
};

/////////////////////////////////////////////////////////////////////////////
// CCrystalTextBuffer command target

class EDITPADC_CLASS CCrystalTextBuffer
{
public:
	DWORD m_dwCurrentRevisionNumber;
	DWORD m_dwRevisionNumberOnSave;
	BOOL IsTextBufferInitialized () const { return m_bInit; }

protected :
	BOOL m_bInit;
	BOOL m_bReadOnly;
	BOOL m_bModified;
	CRLFSTYLE m_nCRLFMode;
	BOOL m_IgnoreEol;
	BOOL m_bInsertTabs;
	int  m_nTabSize;
	int FindLineWithFlag (DWORD dwFlag) const;

protected:
	enum
	{
		UNDO_INSERT = 0x0001,
		UNDO_BEGINGROUP = 0x0100
	};

	class EDITPADC_CLASS CInsertContext : public CUpdateContext
	{
	public:
		POINT m_ptStart, m_ptEnd;
		virtual void RecalcPoint(POINT & ptPoint);
	};

	class EDITPADC_CLASS CDeleteContext : public CUpdateContext
	{
	public:
		POINT m_ptStart, m_ptEnd;
		virtual void RecalcPoint(POINT & ptPoint);
	};

	//  Lines of text
	stl::vector<LineInfo> m_aLines; /**< Text lines. */

	//  Undo
	virtual const UndoRecord &GetUndoRecord(size_t i) const = 0;
	virtual size_t GetUndoRecordCount() const = 0;
	/*stl::vector<UndoRecord> m_aUndoBuf;
	virtual const UndoRecord &GetUndoRecord(size_t i) const
	{
		return m_aUndoBuf[i];
	}
	virtual size_t GetUndoRecordCount() const
	{
		return m_aUndoBuf.size();
	}*/

	size_t m_nUndoPosition;
	size_t m_nSyncPosition;
	BOOL m_bUndoGroup, m_bUndoBeginGroup;

	//BEGIN SW
	/** Position where the last change was made. */
	POINT m_ptLastChange;
	//END SW

	//  Connected views
	stl::list<CCrystalTextView *> m_lpViews;

	//  Helper methods
	void InsertLine(LPCTSTR pszLine, int nLength, int nPosition = -1);
	void AppendLine(int nLineIndex, LPCTSTR pszChars, int nLength);
	void MoveLine(int line1, int line2, int newline1);

	//  Implementation
	BOOL InternalInsertText(CCrystalTextView *, int nLine, int nPos, LPCTSTR pszText, int cchText, int &nEndLine, int &nEndChar);
	BOOL InternalDeleteText(CCrystalTextView *, int nStartLine, int nStartPos, int nEndLine, int nEndPos);
	String StripTail(int i, int bytes);

	//  [JRT] Support For Descriptions On Undo/Redo Actions
	/*virtual UndoRecord &AddUndoRecord(BOOL bInsert, const POINT & ptStartPos, const POINT & ptEndPos,
		LPCTSTR pszText, int cchText, int nActionType = CE_ACTION_UNKNOWN);*/

	// Operations
public:
	//  Construction/destruction code
	CCrystalTextBuffer();
	~CCrystalTextBuffer();

	//  Basic functions
	BOOL InitNew(CRLFSTYLE nCrlfStyle = CRLF_STYLE_DOS);
	void FreeAll();

	//  'Dirty' flag
	virtual void SetModified (BOOL bModified = TRUE);
	BOOL IsModified () const { return m_bModified; }

	//  Connect/disconnect views
	void AddView (CCrystalTextView * pView);
	void RemoveView (CCrystalTextView * pView);

	//  Text access functions
	int GetLineCount() const
	{
		return static_cast<int>(m_aLines.size());
	}
	LPCTSTR GetLineEol (int nLine) const;
	BOOL ChangeLineEol (int nLine, LPCTSTR lpEOL);
	const LineInfo &GetLineInfo(int nLine) const;
	// number of characters in line (excluding any trailing eol characters)
	int GetLineLength(int nLine) const
	{
		return GetLineInfo(nLine).Length();
	}
	// number of characters in line (including any trailing eol characters)
	int GetFullLineLength(int nLine) const // including EOLs
	{
		return GetLineInfo(nLine).FullLength();
	}
	LPCTSTR GetLineChars(int nLine) const
	{
		return GetLineInfo(nLine).GetLine();
	}
	DWORD GetLineFlags(int nLine) const
	{
		return GetLineInfo(nLine).m_dwFlags;
	}
	int GetLineWithFlag (DWORD dwFlag) const;
	void SetLineFlag (int nLine, DWORD dwFlag, BOOL bSet,
			BOOL bRemoveFromPreviousLine = TRUE, BOOL bUpdate = TRUE);
	void GetText (int nStartLine, int nStartChar, int nEndLine, int nEndChar,
			String & text, LPCTSTR pszCRLF = NULL) const;
	virtual void GetTextWithoutEmptys (int nStartLine, int nStartChar,
			int nEndLine, int nEndChar, String &text,
			CRLFSTYLE nCrlfStyle = CRLF_STYLE_AUTOMATIC) const;

	//  Attributes
	CRLFSTYLE GetCRLFMode () const;
	void SetCRLFMode (CRLFSTYLE nCRLFMode);
	/// Adjust all the lines in the buffer to the buffer default EOL Mode
	virtual BOOL applyEOLMode();
	LPCTSTR GetDefaultEol() const;
	static LPCTSTR GetStringEol(CRLFSTYLE nCRLFMode);
	BOOL GetReadOnly () const;
	void SetReadOnly (BOOL bReadOnly = TRUE);

	void SetIgnoreEol(BOOL IgnoreEol) { m_IgnoreEol = IgnoreEol; }

	//  Text modification functions
	virtual BOOL InsertText(CCrystalTextView *pSource, int nLine, int nPos, LPCTSTR pszText, int cchText, int &nEndLine, int &nEndChar, int nAction = CE_ACTION_UNKNOWN, BOOL bHistory = TRUE) = 0;
	virtual BOOL DeleteText(CCrystalTextView *pSource, int nStartLine, int nStartPos, int nEndLine, int nEndPos, int nAction = CE_ACTION_UNKNOWN, BOOL bHistory = TRUE) = 0;

	//  Undo/Redo
	BOOL CanUndo() const;
	BOOL CanRedo() const;
	virtual BOOL Undo(CCrystalTextView *pSource, POINT &ptCursorPos) = 0;
	virtual BOOL Redo(CCrystalTextView *pSource, POINT &ptCursorPos) = 0;

	//  Undo grouping
	void BeginUndoGroup(BOOL bMergeWithPrevious = FALSE);
	void FlushUndoGroup(CCrystalTextView *pSource);

	//BEGIN SW
	/**
	Returns the position where the last changes where made.
	*/
	POINT GetLastChangePos() const;
	//END SW
	void RestoreLastChangePos(POINT pt);
	//void DeleteLine(int line, int nCount = 1);


	//  Browse undo sequence
	size_t GetUndoActionCode(int &nAction, size_t pos = 0) const;
	size_t GetRedoActionCode(int &nAction, size_t pos = 0) const;

	//  Notify all connected views about changes in name of file
	CCrystalTextView::TextDefinition *RetypeViews(LPCTSTR lpszFileName);
	//  Notify all connected views about changes in text
	void UpdateViews(CCrystalTextView *pSource, CUpdateContext *pContext,
					 DWORD dwUpdateFlags, int nLineIndex = -1);

	// Tabs/space inserting
	BOOL GetInsertTabs() const { return m_bInsertTabs; }
	void SetInsertTabs(BOOL bInsertTabs) { m_bInsertTabs = bInsertTabs; }

	// Tabbing
	int GetTabSize() const;
	void SetTabSize(int nTabSize);

	// More bookmarks
	int FindNextBookmarkLine(int nCurrentLine = 0) const;
	int FindPrevBookmarkLine(int nCurrentLine = 0) const;
};

/////////////////////////////////////////////////////////////////////////////
