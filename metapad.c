/****************************************************************************/
/*                                                                          */
/*   metapad 3.6                                                            */
/*                                                                          */
/*   Copyright (C) 2021 SoBiT Corp                                          */
/*   Copyright (C) 2013 Mario Rugiero                                       */
/*   Copyright (C) 1999-2011 Alexander Davidson                             */
/*                                                                          */
/*   This program is free software: you can redistribute it and/or modify   */
/*   it under the terms of the GNU General Public License as published by   */
/*   the Free Software Foundation, either version 3 of the License, or      */
/*   (at your option) any later version.                                    */
/*                                                                          */
/*   This program is distributed in the hope that it will be useful,        */
/*   but WITHOUT ANY WARRANTY; without even the implied warranty of         */
/*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          */
/*   GNU General Public License for more details.                           */
/*                                                                          */
/*   You should have received a copy of the GNU General Public License      */
/*   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */
/*                                                                          */
/****************************************************************************/

/**
 * @file metapad.c
 * @brief Currently, most of the project's code is here.
 */

#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0400

#ifdef BUILD_METAPAD_UNICODE
#include <wchar.h>
#define _CF_TEXT CF_UNICODETEXT
#define _SF_TEXT (SF_TEXT || SF_UNICODE)
#define TM_ENCODING TM_MULTICODEPAGE
#else
#define _CF_TEXT CF_TEXT
#define _SF_TEXT SF_TEXT
#define TM_ENCODING TM_SINGLECODEPAGE
#endif

#include <windows.h>
#include <shellapi.h>
#include <commdlg.h>
#include <commctrl.h>
#include <winuser.h>
#include <tchar.h>

#ifndef TBSTYLE_FLAT
#define TBSTYLE_FLAT 0x0800
#endif

#ifdef USE_RICH_EDIT
#include <richedit.h>
#endif

#include "include/resource.h"

#if defined(__MINGW32__) && !defined(__MINGW64_VERSION_MAJOR)
 #define PROPSHEETHEADER_V1_SIZE 40
#elif !defined(__MINGW32__)
 #include "include/w32crt.h"
#endif

#if !defined(__MINGW64_VERSION_MAJOR)
//extern long _ttol(const TCHAR*);
//extern int _ttoi(const TCHAR*);
#endif

#ifdef _MSC_VER
#pragma intrinsic(memset)
#endif

//#pragma comment(linker, "/OPT:NOWIN98" )

#include "include/consts.h"
#include "include/strings.h"
#include "include/outmacros.h"
#include "include/typedefs.h"
#include "include/language_plugin.h"
#include "include/external_viewers.h"
#include "include/settings_save.h"
#include "include/settings_load.h"
#include "include/file_load.h"
#include "include/file_new.h"
#include "include/file_save.h"
#include "include/file_utils.h"

///// Globals /////

#include "include/globals.h"
#include "include/tmp_protos.h"
SLWA SetLWA = NULL;
HANDLE globalHeap = NULL;
HINSTANCE hinstThis = NULL;
HINSTANCE hinstLang = NULL;
HWND hwnd = NULL;
HWND client = NULL;
HWND status = NULL;
HWND toolbar = NULL;
HWND hdlgCancel = NULL;
HWND hdlgFind = NULL;
HMENU hrecentmenu = NULL;
HFONT hfontmain = NULL;
HFONT hfontfind = NULL;
BOOL g_bIniMode = FALSE;
BOOL bFindOpen = FALSE;
BOOL bReplaceOpen = FALSE;
WINDOWPLACEMENT wpFindPlace;
WINDOWPLACEMENT wpReplacePlace;
int _fltused = 0x9875; // see CMISCDAT.C for more info on this

option_struct options;

///// Implementation /////

LPTSTR GetString(UINT uID)
{
	LoadString(hinstLang, uID, _szString, MAXSTRING);
	return _szString;
}

BOOL EncodeWithEscapeSeqs(TCHAR* szText)
{
	TCHAR szStore[MAXMACRO];
	INT i,j;

	for (i = 0, j = 0; i < MAXMACRO && szText[i]; ++i) {
		switch (szText[i]) {
		case _T('\n'):
			break;
		case _T('\r'):
			szStore[j++] = _T('\\');
			szStore[j++] = _T('n');
			break;
		case _T('\t'):
			szStore[j++] = _T('\\');
			szStore[j++] = _T('t');
			break;
		case _T('\\'):
			szStore[j++] = _T('\\');
			szStore[j++] = _T('\\');
			break;
		default:
			szStore[j++] = szText[i];
		}
		if (j >= MAXMACRO - 1) {
			return FALSE;
		}
	}
	szStore[j] = _T('\0');
	lstrcpy(szText, szStore);
	return TRUE;
}

void ParseForEscapeSeqs(TCHAR* szText)
{
	TCHAR szStore[MAXMACRO];
	INT i,j;
	BOOL bSlashFound = FALSE;

	for (i = 0, j = 0;i < MAXMACRO && szText[i]; ++i) {
		if (bSlashFound) {
			switch (szText[i]) {
			case _T('n'):
#ifdef USE_RICH_EDIT
				szStore[j] = _T('\r');
#else
				szStore[j++] = _T('\r');
				szStore[j] = _T('\n');
#endif
				break;
			case _T('t'):
				szStore[j] = _T('\t');
				break;
			default:
				szStore[j] = szText[i];
			}
			bSlashFound = FALSE;
		}
		else {
			if (szText[i] == _T('\\')) {
				bSlashFound = TRUE;
				continue;
			}
			szStore[j] = szText[i];
		}
		++j;
	}
	szStore[j] = _T('\0');
	lstrcpy(szText, szStore);
}

/**
 * Reverse byte pairs.
 *
 * @param[in] buffer Pointer to the start of the data to be reversed.
 * @param[in] size Size of the data to be reversed.
 */
void ReverseBytes(LPBYTE buffer, LONG size)
{
	BYTE temp;
	long i, end;

	end = size - 2;
	for (i = 0; i <= end; i+=2) {
		temp = buffer[i];
		buffer[i] = buffer[i+1];
		buffer[i+1] = temp;
	}
}

#ifndef USE_RICH_EDIT
void GetClientRange(int min, int max, LPTSTR szDest)
{
	long lFileSize;
	LPTSTR szBuffer;
	TCHAR ch;

	lFileSize = GetWindowTextLength(client);
	szBuffer = (LPTSTR)GetShadowBuffer();
	ch = szBuffer[max];
	szBuffer[max] = _T('\0');
	lstrcpy(szDest, szBuffer + min);
	szBuffer[max] = ch;
}
#endif

/**
 * Updates window title to reflect current working file's name and state.
 */
void UpdateCaption(void)
{
	TCHAR szBuffer[MAXFN];

	ExpandFilename(szFile);

	if (bDirtyFile) {
		szBuffer[0] = _T(' ');
		szBuffer[1] = _T('*');
		szBuffer[2] = _T(' ');
		wsprintf(szBuffer+3, STR_CAPTION_FILE, szCaptionFile);
	}
	else
		wsprintf(szBuffer, STR_CAPTION_FILE, szCaptionFile);

	if (bReadOnly) {
		lstrcat(szBuffer, _T(" "));
		lstrcat(szBuffer, GetString(IDS_READONLY_INDICATOR));
	}

	SetWindowText(hwnd, szBuffer);
}

/**
 * Cleanup objects.
 */
void CleanUp(void)
{
	if (hrecentmenu)
		DestroyMenu(hrecentmenu);
	if (lpszShadow)
		HeapFree(globalHeap, 0, (HGLOBAL) lpszShadow);
	if (hfontmain)
		DeleteObject(hfontmain);
	if (hfontfind)
		DeleteObject(hfontfind);
	if (hthread)
		CloseHandle(hthread);

	if (hinstLang != hinstThis)
		FreeLibrary(hinstLang);

#ifdef USE_RICH_EDIT
	DestroyWindow(client);
	FreeLibrary(GetModuleHandle(STR_RICHDLL));
#else
	if (BackBrush)
		DeleteObject(BackBrush);
#endif
}

#ifdef USE_RICH_EDIT
void UpdateWindowText(void)
{
	LPCTSTR szBuffer = GetShadowBuffer();
	bLoading = TRUE;
	SetWindowText(client, szBuffer);
	bLoading = FALSE;
	SetTabStops();
	SendMessage(client, EM_EMPTYUNDOBUFFER, 0, 0);
	UpdateStatus();
	InvalidateRect(client, NULL, TRUE);
}
#endif

/**
 * Get status bar height.
 *
 * @return Statusbar's height if bShowStatus, 0 otherwise.
 */
int GetStatusHeight(void)
{
	return (bShowStatus ? nStatusHeight : 0);
}

/**
 * Get toolbar height.
 *
 * @return Toolbar's height if bShowToolbar, 0 otherwise.
 */
int GetToolbarHeight(void)
{
	return (bShowToolbar ? nToolbarHeight : 0);
}

void SwitchReadOnly(BOOL bNewVal)
{
	bReadOnly = bNewVal;
	if (GetCheckedState(GetMenu(hwnd), ID_READONLY, FALSE) != bNewVal) {
		GetCheckedState(GetMenu(hwnd), ID_READONLY, TRUE);
	}
}

BOOL GetCheckedState(HMENU hmenu, UINT nID, BOOL bToggle)
{
	MENUITEMINFO mio;

	mio.cbSize = sizeof(MENUITEMINFO);
	mio.fMask = MIIM_STATE;
	GetMenuItemInfo(hmenu, nID, FALSE, &mio);
	if (mio.fState == MFS_CHECKED) {
		if (bToggle) {
			mio.fState = MFS_UNCHECKED;
			SetMenuItemInfo(hmenu, nID, FALSE, &mio);
		}
		return TRUE;
	}
	else {
		if (bToggle) {
			mio.fState = MFS_CHECKED;
			SetMenuItemInfo(hmenu, nID, FALSE, &mio);
		}
		return FALSE;
	}
}

void CreateToolbar(void)
{
	DWORD dwStyle = WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | TBSTYLE_TOOLTIPS;
	TBADDBITMAP tbab = {hinstThis, IDB_TOOLBAR};

	TBBUTTON tbButtons [] = {
	{0, 0, TBSTATE_ENABLED, TBSTYLE_SEP},
//{CUSTOMBMPBASE+6, ID_NEW_INSTANCE, TBSTATE_ENABLED, TBSTYLE_BUTTON},
{STD_FILENEW, ID_MYFILE_NEW, TBSTATE_ENABLED, TBSTYLE_BUTTON},
{STD_FILEOPEN, ID_MYFILE_OPEN, TBSTATE_ENABLED, TBSTYLE_BUTTON},
{STD_FILESAVE, ID_MYFILE_SAVE, TBSTATE_ENABLED, TBSTYLE_BUTTON},
{CUSTOMBMPBASE+1, ID_RELOAD_CURRENT, TBSTATE_ENABLED, TBSTYLE_BUTTON},
{0, 0, TBSTATE_ENABLED, TBSTYLE_SEP},
{CUSTOMBMPBASE+2, ID_FILE_LAUNCHVIEWER, TBSTATE_ENABLED, TBSTYLE_BUTTON},
{CUSTOMBMPBASE+3, ID_LAUNCH_SECONDARY_VIEWER, TBSTATE_ENABLED, TBSTYLE_BUTTON},
{0, 0, TBSTATE_ENABLED, TBSTYLE_SEP},
{STD_PRINT, ID_PRINT, TBSTATE_ENABLED, TBSTYLE_BUTTON},
{0, 0, TBSTATE_ENABLED, TBSTYLE_SEP},
{STD_FIND, ID_FIND, TBSTATE_ENABLED, TBSTYLE_BUTTON},
{STD_REPLACE, ID_REPLACE, TBSTATE_ENABLED, TBSTYLE_BUTTON},
{0, 0, TBSTATE_ENABLED, TBSTYLE_SEP},
{STD_CUT, ID_MYEDIT_CUT, TBSTATE_ENABLED, TBSTYLE_BUTTON},
{STD_COPY, ID_MYEDIT_COPY, TBSTATE_ENABLED, TBSTYLE_BUTTON},
{STD_PASTE, ID_MYEDIT_PASTE, TBSTATE_ENABLED, TBSTYLE_BUTTON},
{0, 0, TBSTATE_ENABLED, TBSTYLE_SEP},
{STD_UNDO, ID_MYEDIT_UNDO, TBSTATE_ENABLED, TBSTYLE_BUTTON},
#ifdef USE_RICH_EDIT
{STD_REDOW, ID_MYEDIT_REDO, TBSTATE_ENABLED, TBSTYLE_BUTTON},
#endif
{0, 0, TBSTATE_ENABLED, TBSTYLE_SEP},
{CUSTOMBMPBASE, ID_EDIT_WORDWRAP, TBSTATE_ENABLED, TBSTYLE_CHECK},
{CUSTOMBMPBASE+4, ID_FONT_PRIMARY, TBSTATE_ENABLED, TBSTYLE_BUTTON},
{CUSTOMBMPBASE+5, ID_ALWAYSONTOP, TBSTATE_ENABLED, TBSTYLE_BUTTON},
{0, 0, TBSTATE_ENABLED, TBSTYLE_SEP},
{STD_PROPERTIES, ID_VIEW_OPTIONS, TBSTATE_ENABLED, TBSTYLE_BUTTON},
};
	if (!options.bUnFlatToolbar)
		dwStyle |= TBSTYLE_FLAT;

	toolbar = CreateToolbarEx(hwnd, dwStyle,
		ID_TOOLBAR, CUSTOMBMPBASE, HINST_COMMCTRL, IDB_STD_SMALL_COLOR,
		(LPCTBBUTTON)&tbButtons, NUMBUTTONS, 0, 0, 16, 16, sizeof(TBBUTTON));

	if (SendMessage(toolbar, TB_ADDBITMAP, (WPARAM)NUMCUSTOMBITMAPS, (LPARAM)&tbab) < 0)
		ReportLastError();

	{
		RECT rect;
		GetWindowRect(toolbar, &rect);
		nToolbarHeight = rect.bottom - rect.top;
	}
}

void CreateStatusbar(void)
{
	int nPaneSizes[NUMPANES] = {0,0,0};

	status = CreateWindowEx(
		WS_EX_DLGMODALFRAME,
		STATUSCLASSNAME,
		_T(""),
		WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | SBT_NOBORDERS | SBARS_SIZEGRIP,
		0, 0, 0, 0,
		hwnd,
		(HMENU) ID_STATUSBAR,
		hinstThis,
		NULL);

	SendMessage(status, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), 0);

	{
		RECT rect;
		GetWindowRect(status, &rect);
		nStatusHeight = rect.bottom - rect.top - 5;
	}

	SendMessage(status, SB_SETPARTS, NUMPANES, (DWORD_PTR)(LPINT)nPaneSizes);
}

void CreateClient(HWND hParent, LPCTSTR szText, BOOL bWrap)
{
#ifdef USE_RICH_EDIT
	DWORD dwStyle =
					ES_AUTOHSCROLL |
					ES_AUTOVSCROLL |
					ES_NOHIDESEL |
					ES_MULTILINE |
					WS_HSCROLL |
					WS_VSCROLL |
					WS_CHILD |
					0;

	if (!options.bHideScrollbars) {
		dwStyle |= ES_DISABLENOSCROLL;
	}

	client = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		RICHEDIT_CLASS,
		szText,
		dwStyle,
		0, 0, 0, 0,
		hParent,
		(HMENU)ID_CLIENT,
		hinstThis,
		NULL);

	if (!client) ReportLastError();

	SendMessage(client, EM_SETTARGETDEVICE, (WPARAM)0, (LPARAM)(LONG) !bWrap);
	SendMessage(client, EM_AUTOURLDETECT, (WPARAM)bHyperlinks, 0);
	SendMessage(client, EM_SETEVENTMASK, 0, (LPARAM)(ENM_LINK | ENM_CHANGE));
	SendMessage(client, EM_EXLIMITTEXT, 0, (LPARAM)(DWORD)0x7fffffff);
	/** @fixme Commented out code. */
	// sort of fixes font problems but cannot set tab size
	SendMessage(client, EM_SETTEXTMODE, (WPARAM)TM_PLAINTEXT || (WPARAM)TM_ENCODING, 0);

	wpOrigEditProc = (WNDPROC) SetWindowLongPtr(client, GWLP_WNDPROC, (LONG) EditProc);
#else
	DWORD dwStyle = ES_NOHIDESEL | WS_VSCROLL | ES_MULTILINE | WS_CHILD;

	if (!bWrap)
		dwStyle |= WS_HSCROLL;
	/** @fixme Commented out code. */
	/*
	if (bReadOnly)
		dwStyle |= ES_READONLY;
	*/

	client = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		_T("EDIT"),
		szText,
		dwStyle,
		0, 0, 0, 0,
		hParent,
		(HMENU)ID_CLIENT,
		hinstThis,
		NULL);

	SendMessage(client, EM_LIMITTEXT, 0, 0);
	wpOrigEditProc = (WNDPROC) SetWindowLongPtr(client, GWLP_WNDPROC, (LONG_PTR) EditProc);
#endif
}

void UpdateStatus(void)
{
	LONG lLine, lLineIndex, lLines;
	TCHAR szPane[50];
	int nPaneSizes[NUMPANES];
	LPTSTR szBuffer;
	long lLineLen, lCol = 1;
	int i = 0;
	CHARRANGE cr;

	if (toolbar && bShowToolbar) {

#ifdef USE_RICH_EDIT
		SendMessage(client, EM_EXGETSEL, 0, (LPARAM)&cr);
#else
		SendMessage(client, EM_GETSEL, (WPARAM)&cr.cpMin, (LPARAM)&cr.cpMax);
#endif
		SendMessage(toolbar, TB_CHECKBUTTON, (WPARAM)ID_EDIT_WORDWRAP, MAKELONG(bWordWrap, 0));
		SendMessage(toolbar, TB_CHECKBUTTON, (WPARAM)ID_FONT_PRIMARY, MAKELONG(bPrimaryFont, 0));
		SendMessage(toolbar, TB_CHECKBUTTON, (WPARAM)ID_ALWAYSONTOP, MAKELONG(bAlwaysOnTop, 0));
		SendMessage(toolbar, TB_ENABLEBUTTON, (WPARAM)ID_RELOAD_CURRENT, MAKELONG(szFile[0] != _T('\0'), 0));
		SendMessage(toolbar, TB_ENABLEBUTTON, (WPARAM)ID_MYEDIT_CUT, MAKELONG((cr.cpMin != cr.cpMax), 0));
		SendMessage(toolbar, TB_ENABLEBUTTON, (WPARAM)ID_MYEDIT_COPY, MAKELONG((cr.cpMin != cr.cpMax), 0));
		SendMessage(toolbar, TB_ENABLEBUTTON, (WPARAM)ID_MYEDIT_UNDO, MAKELONG(SendMessage(client, EM_CANUNDO, 0, 0), 0));
		SendMessage(toolbar, TB_ENABLEBUTTON, (WPARAM)ID_MYEDIT_PASTE, MAKELONG(IsClipboardFormatAvailable(_CF_TEXT), 0));
#ifdef USE_RICH_EDIT
		SendMessage(toolbar, TB_ENABLEBUTTON, (WPARAM)ID_MYEDIT_REDO, MAKELONG(SendMessage(client, EM_CANREDO, 0, 0), 0));
#endif
	}

	if (status == NULL || !bShowStatus)
		return;

#ifdef USE_RICH_EDIT
	if (!bUpdated)
		return;
#endif

	nPaneSizes[SBPANE_TYPE] = 4 * options.nStatusFontWidth;

	if (bBinaryFile) {
		wsprintf(szPane, _T("  BIN"));
	}
	else if (nEncodingType == TYPE_UTF_8) {
		wsprintf(szPane, _T(" UTF-8"));
		nPaneSizes[SBPANE_TYPE] = 5 * options.nStatusFontWidth + 4;
	}
	else if (nEncodingType == TYPE_UTF_16) {
		wsprintf(szPane, _T(" Unicode"));
		nPaneSizes[SBPANE_TYPE] = 6 * options.nStatusFontWidth + 4;
	}
	else if (nEncodingType == TYPE_UTF_16_BE) {
		wsprintf(szPane, _T(" Unicode BE"));
		nPaneSizes[SBPANE_TYPE] = 8 * options.nStatusFontWidth + 4;
	}
	else if (bUnix) {
		wsprintf(szPane, _T("UNIX"));
	}
	else {
		wsprintf(szPane, _T(" DOS"));
	}

	SendMessage(status, SB_SETTEXT, (WPARAM) SBPANE_TYPE, (LPARAM)(LPTSTR)szPane);

	lLines = SendMessage(client, EM_GETLINECOUNT, 0, 0);

#ifdef USE_RICH_EDIT
	if (bInsertMode)
		wsprintf(szPane, _T(" INS"));
	else
		wsprintf(szPane, _T("OVR"));
	nPaneSizes[SBPANE_INS] = nPaneSizes[SBPANE_INS - 1] + 4 * options.nStatusFontWidth - 3;
	SendMessage(status, SB_SETTEXT, (WPARAM) SBPANE_INS, (LPARAM)(LPTSTR)szPane);
#else
	wsprintf(szPane, _T(" Bytes: %d "), CalculateFileSize());
	nPaneSizes[SBPANE_INS] = nPaneSizes[SBPANE_INS - 1] + (int)((options.nStatusFontWidth/STATUS_FONT_CONST) * lstrlen(szPane));
	SendMessage(status, SB_SETTEXT, (WPARAM) SBPANE_INS, (LPARAM)(LPTSTR)szPane);
#endif
	/** @fixme Commented out code. */
	/*
	nPaneSizes[SBPANE_READ] = nPaneSizes[SBPANE_READ - 1] + 2 * options.nStatusFontWidth + 4;
	if (bReadOnly)
		wsprintf(szPane, _T("READ"));
	else
		wsprintf(szPane, _T(" WRI"));
	SendMessage(status, SB_SETTEXT, (WPARAM) SBPANE_READ, (LPARAM)(LPTSTR)szPane);
	*/

#ifdef USE_RICH_EDIT
	SendMessage(client, EM_EXGETSEL, 0, (LPARAM)&cr);
	lLine = SendMessage(client, EM_EXLINEFROMCHAR, 0, (LPARAM)cr.cpMax);
#else
	SendMessage(client, EM_GETSEL, (WPARAM)&cr.cpMin, (LPARAM)&cr.cpMax);
	lLine = SendMessage(client, EM_LINEFROMCHAR, (WPARAM)cr.cpMax, 0);
#endif
	wsprintf(szPane, _T(" Line: %d/%d"), lLine+1, lLines);
	SendMessage(status, SB_SETTEXT, (WPARAM) SBPANE_LINE, (LPARAM)(LPTSTR)szPane);

	nPaneSizes[SBPANE_LINE] = nPaneSizes[SBPANE_LINE - 1] + (int)((options.nStatusFontWidth/STATUS_FONT_CONST) * lstrlen(szPane) + 2);

	lLineLen = SendMessage(client, EM_LINELENGTH, (WPARAM)cr.cpMax, 0);
	szBuffer = (LPTSTR) HeapAlloc(globalHeap, HEAP_ZERO_MEMORY, (lLineLen+2) * sizeof(TCHAR));
	*((LPWORD)szBuffer) = (USHORT)(lLineLen + 1);
	SendMessage(client, EM_GETLINE, (WPARAM)lLine, (LPARAM) (LPCTSTR)szBuffer);
	szBuffer[lLineLen] = _T('\0');

	lLineIndex = SendMessage(client, EM_LINEINDEX, (WPARAM)lLine, 0);
	while (lLineLen && i < (cr.cpMax - lLineIndex) && szBuffer[i]) {
		if (szBuffer[i] == _T('\t'))
			lCol += options.nTabStops - (lCol-1) % options.nTabStops;
		else
			lCol++;
		i++;
	}
	wsprintf(szPane, _T(" Col: %d"), lCol);
	SendMessage(status, SB_SETTEXT, (WPARAM) SBPANE_COL, (LPARAM)(LPTSTR)szPane);
	nPaneSizes[SBPANE_COL] = nPaneSizes[SBPANE_COL - 1] + (int)((options.nStatusFontWidth/STATUS_FONT_CONST) * lstrlen(szPane) + 2);
	/** @fixme Commented out code. */
	/*
	if (bHideMessage)
		szPane[0] = _T('\0');
	else
		lstrcpy(szPane, szStatusMessage);
	*/

	nPaneSizes[SBPANE_MESSAGE] = nPaneSizes[SBPANE_MESSAGE - 1] + 1000;//(int)((options.nStatusFontWidth/STATUS_FONT_CONST) * lstrlen(szPane));
	SendMessage(status, SB_SETTEXT, (WPARAM) SBPANE_MESSAGE | SBT_NOBORDERS, (LPARAM)(bHideMessage ? _T("") : szStatusMessage));

	SendMessage(status, SB_SETPARTS, NUMPANES, (DWORD_PTR)(LPINT)nPaneSizes);
	HeapFree(globalHeap, 0, (HGLOBAL)szBuffer);
}

LRESULT APIENTRY FindProc(HWND hwndFind, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_COMMAND:
		if ((LOWORD(wParam) == IDC_ESCAPE || LOWORD(wParam) == IDC_ESCAPE2) && HIWORD(wParam) == BN_CLICKED) {
			HMENU hmenu = LoadMenu(hinstLang, (LPCTSTR)IDR_ESCAPE_SEQUENCES);
			HMENU hsub = GetSubMenu(hmenu, 0);
			RECT rect;
			UINT id;
			TCHAR szText[MAXFIND];

			SendMessage(hwnd, WM_INITMENUPOPUP, (WPARAM)hsub, MAKELPARAM(1, FALSE));
			if (LOWORD(wParam) == IDC_ESCAPE) {
				GetWindowRect(GetDlgItem(hwndFind, IDC_ESCAPE), &rect);
				SendDlgItemMessage(hwndFind, ID_DROP_FIND, WM_GETTEXT, (WPARAM)MAXFIND, (LPARAM)szText);
			}
			else {
				GetWindowRect(GetDlgItem(hwndFind, IDC_ESCAPE2), &rect);
				SendDlgItemMessage(hwndFind, ID_DROP_REPLACE, WM_GETTEXT, (WPARAM)MAXFIND, (LPARAM)szText);
			}

			if (bNoFindHidden) {
				MENUITEMINFO mio;

				mio.cbSize = sizeof(MENUITEMINFO);
				mio.fMask = MIIM_STATE;
				mio.fState = MFS_CHECKED;
				SetMenuItemInfo(hsub, ID_ESCAPE_DISABLE, FALSE, &mio);
				EnableMenuItem(hsub, ID_ESCAPE_NEWLINE, MF_BYCOMMAND | MF_GRAYED);
				EnableMenuItem(hsub, ID_ESCAPE_TAB, MF_BYCOMMAND | MF_GRAYED);
				EnableMenuItem(hsub, ID_ESCAPE_BACKSLASH, MF_BYCOMMAND | MF_GRAYED);
			}

			id = TrackPopupMenuEx(hsub, TPM_RETURNCMD | TPM_TOPALIGN | TPM_LEFTALIGN | TPM_RIGHTBUTTON, rect.left, rect.bottom, hwnd, NULL);

			switch (id) {
			case ID_ESCAPE_NEWLINE:
				lstrcat(szText, _T("\\n"));
				break;
			case ID_ESCAPE_TAB:
				lstrcat(szText, _T("\\t"));
				break;
			case ID_ESCAPE_BACKSLASH:
				lstrcat(szText, _T("\\\\"));
				break;
			case ID_ESCAPE_DISABLE:
				bNoFindHidden = !GetCheckedState(hsub, ID_ESCAPE_DISABLE, TRUE);
				break;
			}

			if (LOWORD(wParam) == IDC_ESCAPE) {
				SendDlgItemMessage(hwndFind, ID_DROP_FIND, WM_SETTEXT, (WPARAM)(BOOL)FALSE, (LPARAM)szText);
			}
			else {
				SendDlgItemMessage(hwndFind, ID_DROP_REPLACE, WM_SETTEXT, (WPARAM)(BOOL)FALSE, (LPARAM)szText);
			}

			DestroyMenu(hmenu);
		}
		break;
	}
	return CallWindowProc(wpOrigFindProc, hwndFind, uMsg, wParam, lParam);
}

LRESULT APIENTRY EditProc(HWND hwndEdit, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
#ifdef USE_RICH_EDIT
	case WM_VSCROLL:
	case WM_HSCROLL:
		if ((uMsg == WM_HSCROLL && (LOWORD(wParam) == SB_LINERIGHT || LOWORD(wParam) == SB_PAGERIGHT)) ||
		   (uMsg == WM_VSCROLL && (LOWORD(wParam) == SB_LINEDOWN || LOWORD(wParam) == SB_PAGEDOWN))) {
			SCROLLINFO si;
			UINT nSbType = (uMsg == WM_VSCROLL ? SB_VERT : SB_HORZ);

			SendMessage(hwndEdit, WM_SETREDRAW, (WPARAM)FALSE, 0);
			if (EnableScrollBar(hwndEdit, nSbType, ESB_ENABLE_BOTH)) {
				EnableScrollBar(hwndEdit, nSbType, ESB_DISABLE_BOTH);
				SendMessage(hwndEdit, WM_SETREDRAW, (WPARAM)TRUE, 0);
				return 0;
			}
			SendMessage(hwndEdit, WM_SETREDRAW, (WPARAM)TRUE, 0);

			si.cbSize = sizeof(si);
			si.fMask = SIF_PAGE|SIF_POS|SIF_RANGE;
			GetScrollInfo(hwndEdit, nSbType, &si);
			if (si.nPos >= si.nMax - (int)si.nPage + 1)
				return 0;

			CallWindowProc(wpOrigEditProc, hwndEdit, uMsg, wParam, lParam);

			if (LOWORD(wParam) == SB_PAGERIGHT) {
				GetScrollInfo(hwndEdit, nSbType, &si);
				SendMessage(hwndEdit, uMsg, MAKEWPARAM(SB_THUMBPOSITION, si.nPos), 0);
			}
		}
		else {
			CallWindowProc(wpOrigEditProc, hwndEdit, uMsg, wParam, lParam);
		}
		return 0;
	case WM_MOUSEWHEEL:
		{
			UINT i, nLines = 3;

			SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &nLines, 0);
			if (nLines > 1000) {
				PostMessage(hwndEdit, WM_VSCROLL, (short)HIWORD(wParam) > 0 ? SB_PAGEUP : SB_PAGEDOWN, 0);
			}
			else {
				for (i = 0; i < nLines; ++i) {
					PostMessage(hwndEdit, WM_VSCROLL, (short)HIWORD(wParam) > 0 ? SB_LINEUP : SB_LINEDOWN, 0);
				}
			}
		}
		return 0;
	case WM_PASTE:
		return 0;
#endif

#ifdef USE_BOOKMARKS
	case WM_PAINT:
		{
			HDC clientDC = GetDC(client);
			TEXTMETRIC tm;
			LRESULT lRes = CallWindowProc(wpOrigEditProc, hwndEdit, uMsg, wParam, lParam);
			INT nFontHeight, nVis;

			INT nLine = 5;

			SelectObject (clientDC, hfontmain);

			if (!GetTextMetrics(clientDC, &tm))
				ReportLastError();
			/** @fixme Commented out code. */
			nFontHeight = (tm.tmHeight /*- tm.tmInternalLeading*/);

			nVis = CallWindowProc(wpOrigEditProc, hwndEdit, EM_GETFIRSTVISIBLELINE, 0, 0);

			if (nVis < nLine) {
				nLine -= nVis;
				SetPixel(clientDC, 4, -(nFontHeight/2) + nFontHeight * nLine, RGB(255, 0, 255));
				SetPixel(clientDC, 3, -(1+nFontHeight/2) + nFontHeight * nLine, RGB(255, 0, 255));
				SetPixel(clientDC, 4, -(1+nFontHeight/2) + nFontHeight * nLine, RGB(255, 0, 255));
				SetPixel(clientDC, 5, -(1+nFontHeight/2) + nFontHeight * nLine, RGB(255, 0, 255));
				SetPixel(clientDC, 4, -(2+nFontHeight/2) + nFontHeight * nLine, RGB(255, 0, 255));
			}

			ReleaseDC(client, clientDC);
			return lRes;
		}
		break;
#endif
	case WM_LBUTTONUP:
	case WM_LBUTTONDOWN:
		{
			LRESULT lRes = CallWindowProc(wpOrigEditProc, hwndEdit, uMsg, wParam, lParam);
			UpdateStatus();
			return lRes;
		}
	case WM_LBUTTONDBLCLK:
#ifndef USE_RICH_EDIT
		if (bSmartSelect) {

			CHARRANGE cr;

			SendMessage(client, EM_GETSEL, (WPARAM)&cr.cpMin, (LPARAM)&cr.cpMax);
			CallWindowProc(wpOrigEditProc, hwndEdit, uMsg, wParam, lParam);
			SendMessage(client, EM_SETSEL, (WPARAM)cr.cpMin, (LPARAM)cr.cpMax);
			SelectWord(FALSE, TRUE, TRUE);
			return 0;
		}
		else {
			return CallWindowProc(wpOrigEditProc, hwndEdit, uMsg, wParam, lParam);
		}
#else
		{
			CHARRANGE cr;

			SendMessage(client, EM_EXGETSEL, 0, (LPARAM)&cr);
			CallWindowProc(wpOrigEditProc, hwndEdit, uMsg, wParam, lParam);
			SendMessage(client, EM_EXSETSEL, 0, (LPARAM)&cr);
			SelectWord(FALSE, bSmartSelect, TRUE);
			return 0;
		}
/** @fixme Commented out code. */
//		if (!bSmartSelect) {
/*		}
		else {
			return CallWindowProc(wpOrigEditProc, hwndEdit, uMsg, wParam, lParam);
		}*/
#endif
	case WM_RBUTTONUP:
		{
			HMENU hmenu = LoadMenu(hinstLang, MAKEINTRESOURCE(IDR_POPUP));
			HMENU hsub = GetSubMenu(hmenu, 0);
			POINT pt;
			UINT id;
			CHARRANGE cr;

#ifndef USE_RICH_EDIT
			DeleteMenu(hsub, 1, MF_BYPOSITION);
#endif
/** @fixme Commented code. */
/*			if (bLinkMenu) {
				bLinkMenu = FALSE;
			}
*/
#ifdef USE_RICH_EDIT
			SendMessage(client, EM_EXGETSEL, 0, (LPARAM)&cr);
#else
			SendMessage(client, EM_GETSEL, (WPARAM)&cr.cpMin, (LPARAM)&cr.cpMax);
#endif
			if (options.bContextCursor) {
				if (cr.cpMin == cr.cpMax) {
#ifdef USE_RICH_EDIT
					BOOL bOld = options.bLinkDoubleClick;
					options.bLinkDoubleClick = TRUE;
#endif
					SendMessage(hwndEdit, WM_LBUTTONDOWN, wParam, lParam);
					SendMessage(hwndEdit, WM_LBUTTONUP, wParam, lParam);
#ifdef USE_RICH_EDIT
					options.bLinkDoubleClick = bOld;
#endif
				}
			}
			SendMessage(hwnd, WM_INITMENUPOPUP, (WPARAM)hsub, MAKELPARAM(1, FALSE));
			GetCursorPos(&pt);
			id = TrackPopupMenuEx(hsub, TPM_RETURNCMD | TPM_TOPALIGN | TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, hwnd, NULL);
			PostMessage(hwnd, WM_COMMAND, MAKEWPARAM(id, 0), 0);
			DestroyMenu(hmenu);
			return 0;
		}
	case WM_KEYUP:
	case WM_KEYDOWN:
		{
			LRESULT lRes = CallWindowProc(wpOrigEditProc, hwndEdit, uMsg, wParam, lParam);
			UpdateStatus();
			return lRes;
		}
	case WM_CHAR:
		if ((TCHAR) wParam == _T('\r')) {
			CallWindowProc(wpOrigEditProc, hwndEdit, uMsg, wParam, lParam);
			if (options.bAutoIndent) {
				int i = 0;
				LONG lLineLen, lLine;
				LPTSTR szBuffer, szIndent;
				CHARRANGE cr;

#ifdef USE_RICH_EDIT
				CallWindowProc(wpOrigEditProc, client, EM_EXGETSEL, 0, (LPARAM)&cr);
				lLine = CallWindowProc(wpOrigEditProc, client, EM_EXLINEFROMCHAR, 0, (LPARAM)cr.cpMin);
#else
				CallWindowProc(wpOrigEditProc, client, EM_GETSEL, (WPARAM)&cr.cpMin, 0);
				lLine = CallWindowProc(wpOrigEditProc, client, EM_LINEFROMCHAR, (WPARAM)cr.cpMin, 0);
#endif
				lLineLen = CallWindowProc(wpOrigEditProc, client, EM_LINELENGTH, (WPARAM)cr.cpMin, 0);

				lLineLen = CallWindowProc(wpOrigEditProc, client, EM_LINELENGTH, (WPARAM)cr.cpMin - 1, 0);

				szBuffer = (LPTSTR) HeapAlloc(globalHeap, HEAP_ZERO_MEMORY, (lLineLen+2) * sizeof(TCHAR));
				szIndent = (LPTSTR) HeapAlloc(globalHeap, HEAP_ZERO_MEMORY, (lLineLen+2) * sizeof(TCHAR));

				*((LPWORD)szBuffer) = (USHORT)(lLineLen + 1);
				CallWindowProc(wpOrigEditProc, client, EM_GETLINE, (WPARAM)lLine - 1, (LPARAM)(LPCTSTR)szBuffer);
				szBuffer[lLineLen] = _T('\0');
				while (szBuffer[i] == _T('\t') || szBuffer[i] == _T(' ')) {
					szIndent[i] = szBuffer[i];
					i++;
				}
				szIndent[i] = _T('\0');
				CallWindowProc(wpOrigEditProc, client, EM_REPLACESEL, (WPARAM)TRUE, (LPARAM)szIndent);
				HeapFree(globalHeap, 0, (HGLOBAL)szBuffer);
				HeapFree(globalHeap, 0, (HGLOBAL)szIndent);
			}
			return 0;
		}
		break;
	case EM_REPLACESEL:
		{
			LRESULT lTmp;
			LONG lStartLine, lEndLine;
			CHARRANGE cr;

#ifdef USE_RICH_EDIT
			SendMessage(client, EM_EXGETSEL, 0, (LPARAM)&cr);
			lStartLine = SendMessage(client, EM_EXLINEFROMCHAR, 0, (LPARAM)cr.cpMin);
			lEndLine = SendMessage(client, EM_EXLINEFROMCHAR, 0, (LPARAM)cr.cpMax);
#else
			SendMessage(client, EM_GETSEL, (WPARAM)&cr.cpMin, (LPARAM)&cr.cpMax);
			lStartLine = SendMessage(client, EM_LINEFROMCHAR, (WPARAM)cr.cpMin, 0);
			lEndLine = SendMessage(client, EM_LINEFROMCHAR, (WPARAM)cr.cpMax, 0);
#endif

			if (cr.cpMin == cr.cpMax || lStartLine == lEndLine) break;

			if (!bReplacingAll)
				SendMessage(hwndEdit, WM_SETREDRAW, (WPARAM)FALSE, 0);
			lTmp = CallWindowProc(wpOrigEditProc, hwndEdit, uMsg, wParam, lParam);
			if (!bReplacingAll)
				SendMessage(hwndEdit, WM_SETREDRAW, (WPARAM)TRUE, 0);
			SetWindowPos(client, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

			return lTmp;
		}
	}
	return CallWindowProc(wpOrigEditProc, hwndEdit, uMsg, wParam, lParam);
}

BOOL CALLBACK AbortDlgProc(HDC hdc, int nCode)
{
	MSG msg;

	while (PeekMessage((LPMSG) &msg, (HWND) NULL, 0, 0, PM_REMOVE)) {
		if (!IsDialogMessage(hdlgCancel, (LPMSG) &msg)) {
			TranslateMessage((LPMSG) &msg);
			DispatchMessage((LPMSG) &msg);
		}
	}
	return bPrint;
}

LRESULT CALLBACK AbortPrintJob(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
		case WM_INITDIALOG:
			CenterWindow(hwndDlg);
			SetDlgItemText(hwndDlg, IDD_FILE, szFile);
			return TRUE;
		case WM_COMMAND:
			bPrint = FALSE;
			return TRUE;
		default:
			return FALSE;
	}
}

#ifndef USE_RICH_EDIT
void PrintContents()
{
	PRINTDLG pd;
	DOCINFO di;
	RECT rectdev, rect;
	int nHeight, nError;
	LPCTSTR startAt;
	LONG lStringLen;
	int totalDone = 0;
	UINT page;
	HFONT hprintfont = NULL, *oldfont = NULL;
	LOGFONT storefont;
	HDC clientDC;
	LONG lHeight;
	CHARRANGE cr;
	LPTSTR szBuffer;
	BOOL bUseDefault;

	ZeroMemory(&pd, sizeof(PRINTDLG));
	pd.lStructSize = sizeof(PRINTDLG);
	pd.hDevMode = (HANDLE) NULL;
	pd.hDevNames = (HANDLE) NULL;
	pd.Flags = PD_RETURNDC | PD_NOPAGENUMS;
	pd.hwndOwner = hwnd;
	pd.hDC = (HDC) NULL;
	pd.nCopies = 1;

	SendMessage(client, EM_GETSEL, (WPARAM)&cr.cpMin, (LPARAM)&cr.cpMax);

	if (cr.cpMin != cr.cpMax) {
		pd.Flags |= PD_SELECTION;
	}
	else {
		pd.Flags |= PD_NOSELECTION;
	}

	if (!PrintDlg(&pd)) {
		DWORD x = CommDlgExtendedError();

		if (x && pd.hDC == NULL) {
			ERROROUT(GetString(IDS_PRINTER_NOT_FOUND));
			return;
		}

		if (x) {
			TCHAR szBuff[50];
			wsprintf(szBuff, GetString(IDS_PRINT_INIT_ERROR), x);
			ERROROUT(szBuff);
		}
		return;
	}

	if (pd.Flags & PD_SELECTION) {
		szBuffer = (LPTSTR) HeapAlloc(globalHeap, HEAP_ZERO_MEMORY, (cr.cpMax - cr.cpMin + 1) * sizeof(TCHAR));
		GetClientRange(cr.cpMin, cr.cpMax, szBuffer);
	}
	else {
		szBuffer = (LPTSTR)GetShadowBuffer();
		if (szBuffer == NULL) {
			ReportLastError();
			return;
		}
	}
	lStringLen = lstrlen(szBuffer);

	bPrint = TRUE;

	if (SetAbortProc(pd.hDC, AbortDlgProc) == SP_ERROR) {
		HeapFree(globalHeap, 0, szBuffer);
		ERROROUT(GetString(IDS_PRINT_ABORT_ERROR));
		return;
	}

	hdlgCancel = CreateDialog(hinstLang, MAKEINTRESOURCE(IDD_ABORT_PRINT), hwnd, (DLGPROC) AbortPrintJob);
	ShowWindow(hdlgCancel, SW_SHOW);

	EnableWindow(hwnd, FALSE);

	bUseDefault = options.bDefaultPrintFont;

	if (bPrimaryFont)
		bUseDefault |= (options.nPrimaryFont == 0);
	else
		bUseDefault |= (options.nSecondaryFont == 0);

	if (!bUseDefault) {
		long lScale = 1;
		int nPrinter, nScreen;

		clientDC = GetDC(client);
		nPrinter = GetDeviceCaps(pd.hDC, LOGPIXELSY);
		nScreen = GetDeviceCaps(clientDC, LOGPIXELSY);

		//if (nPrinter > nScreen)
			lScale = (long)(nPrinter / nScreen);
		//else
		//	lScale = (long)(nScreen / nPrinter);

		if (bPrimaryFont) {
			storefont = options.PrimaryFont;
			lHeight = options.PrimaryFont.lfHeight * lScale;
			options.PrimaryFont.lfHeight = lHeight;
			SetFont(&hprintfont, bPrimaryFont);
			options.PrimaryFont = storefont;
		}
		else {
			storefont = options.SecondaryFont;
			lHeight = options.SecondaryFont.lfHeight * lScale;
			options.SecondaryFont.lfHeight = lHeight;
			SetFont(&hprintfont, bPrimaryFont);
			options.SecondaryFont = storefont;
		}
		ReleaseDC(client, clientDC);
		oldfont = SelectObject(pd.hDC, hprintfont);
	}

	ZeroMemory(&di, sizeof(DOCINFO));
	di.cbSize = sizeof(DOCINFO);
	di.lpszDocName = szFile;

	nError = StartDoc(pd.hDC, &di);
	if (nError <= 0) {
		ReportLastError();
		ERROROUT(GetString(IDS_PRINT_START_ERROR));
		goto Error;
	}
	/** @fixme Several blocks of code commented out. */
	/*
	rectdev.left = rectdev.top = 0;
	rectdev.right = GetDeviceCaps(pd.hDC, HORZRES);
	rectdev.bottom = GetDeviceCaps(pd.hDC, VERTRES);
	*/
/*
	rectdev.left = (long)(options.rMargins.left / 1000. * 1440);
	rectdev.top = (long)(options.rMargins.top / 1000. * 1440);
	rectdev.right = GetDeviceCaps(pd.hDC, HORZRES) - (long)(options.rMargins.right / 1000. * 1440);
	rectdev.bottom = GetDeviceCaps(pd.hDC, VERTRES) - (long)(options.rMargins.bottom / 1000. * 1440);
*/
	{
		int nDPIx = GetDeviceCaps(pd.hDC, LOGPIXELSX);
		int nDPIy = GetDeviceCaps(pd.hDC, LOGPIXELSY);
		//int nMarginX = GetDeviceCaps(pd.hDC, PHYSICALOFFSETX);
		//int nMarginY = GetDeviceCaps(pd.hDC, PHYSICALOFFSETY);

		rectdev.left = (long)(options.rMargins.left / 1000. * nDPIx);
		rectdev.top = (long)(options.rMargins.top / 1000. * nDPIy);
		rectdev.right = GetDeviceCaps(pd.hDC, HORZRES) - (long)(options.rMargins.right / 1000. * nDPIx);
		rectdev.bottom = GetDeviceCaps(pd.hDC, VERTRES) - (long)(options.rMargins.bottom / 1000. * nDPIy);
	}

	startAt = szBuffer;

	for (page = pd.nMinPage; (nError > 0) && (totalDone < lStringLen); page++)
	{
		int nLo = 0;
		int nHi = lStringLen - totalDone;
		int nCount = nHi;
		int nRet = 0;

		nError = StartPage(pd.hDC);
		if (nError <= 0) break;

		SetMapMode(pd.hDC, MM_TEXT);
		if (hprintfont != NULL)
			SelectObject(pd.hDC, hprintfont);

		rect = rectdev;
		while (nLo < nHi) {
			rect.right = rectdev.right;
			nHeight = DrawText(pd.hDC, startAt, nCount, &rect, DT_CALCRECT|DT_WORDBREAK|DT_NOCLIP|DT_EXPANDTABS|DT_NOPREFIX);
			if (nHeight < rectdev.bottom)
				nLo = nCount;
			if (nHeight > rectdev.bottom)
				nHi = nCount;
			if (nLo == nHi - 1)
				nLo = nHi;
			if (nLo < nHi)
				nCount = nLo + (nHi - nLo)/2;
		}
		nRet = DrawText(pd.hDC, startAt, nCount, &rect, DT_WORDBREAK|DT_NOCLIP|DT_EXPANDTABS|DT_NOPREFIX);
		if (nRet == 0) {
			ERROROUT(GetString(IDS_DRAWTEXT_ERROR));
			break;
		}
		startAt += nCount;
		totalDone += nCount;

		nError = EndPage(pd.hDC);
	}

	if (nError > 0) {
		EndDoc(pd.hDC);
	}
	else {
		AbortDoc(pd.hDC);
		ReportLastError();
		ERROROUT(GetString(IDS_PRINT_ERROR));
	}

Error:
	if (!options.bDefaultPrintFont) {
		SelectObject(pd.hDC, oldfont);
		DeleteObject(hprintfont);
	}
	if (pd.Flags & PD_SELECTION) {
		HeapFree(globalHeap, 0, (HGLOBAL)szBuffer);
	}

	EnableWindow(hwnd, TRUE);
	DestroyWindow(hdlgCancel);
	DeleteDC(pd.hDC);
}

#else // USE_RICH_EDIT

void PrintContents(void)
{
	PRINTDLG pd;
	DOCINFO di;
	int nError;
	FORMATRANGE fr;
	int nHorizRes;
	int nVertRes;
	int nLogPixelsX;
	int nLogPixelsY;
	LONG lTextLength;
	LONG lTextPrinted;
	int i;
	CHARRANGE cr;

	ZeroMemory(&pd, sizeof(PRINTDLG));
	pd.lStructSize = sizeof(PRINTDLG);
	pd.hDevMode = (HANDLE) NULL;
	pd.hDevNames = (HANDLE) NULL;
	pd.Flags = PD_RETURNDC | PD_NOPAGENUMS;

	SendMessage(client, EM_EXGETSEL, 0, (LPARAM)&cr);

	if (cr.cpMin != cr.cpMax) {
		pd.Flags |= PD_SELECTION;
	}
	else {
		pd.Flags |= PD_NOSELECTION;
	}

	pd.hwndOwner = hwnd;
	pd.hDC = (HDC) NULL;
	pd.nCopies = 1;

	if (!PrintDlg(&pd)) {
		DWORD x = CommDlgExtendedError();

		if (x && pd.hDC == NULL) {
			ERROROUT(GetString(IDS_PRINTER_NOT_FOUND));
			return;
		}

		if (x) {
			TCHAR szBuff[50];
			wsprintf(szBuff, GetString(IDS_PRINT_INIT_ERROR), x);
			ERROROUT(szBuff);
		}
		return;
	}

	bPrint = TRUE;

	if (SetAbortProc(pd.hDC, AbortDlgProc) == SP_ERROR) {
		ERROROUT(GetString(IDS_PRINT_ABORT_ERROR));
		return;
	}

	hdlgCancel = CreateDialog(hinstLang, MAKEINTRESOURCE(IDD_ABORT_PRINT), hwnd, (DLGPROC) AbortPrintJob);
	ShowWindow(hdlgCancel, SW_SHOW);

	EnableWindow(hwnd, FALSE);

	nHorizRes = GetDeviceCaps(pd.hDC, HORZRES);
	nVertRes = GetDeviceCaps(pd.hDC, VERTRES);
	nLogPixelsX = GetDeviceCaps(pd.hDC, LOGPIXELSX);
	nLogPixelsY = GetDeviceCaps(pd.hDC, LOGPIXELSY);

	SetMapMode(pd.hDC, MM_TEXT);

	ZeroMemory(&fr, sizeof(fr));
	fr.hdc = fr.hdcTarget = pd.hDC;

	fr.rcPage.left = fr.rcPage.top = 0;
	fr.rcPage.right = (nHorizRes/nLogPixelsX) * 1440;
	fr.rcPage.bottom = (nVertRes/nLogPixelsY) * 1440;

	fr.rc.left = fr.rcPage.left + (long)(options.rMargins.left / 1000. * 1440);
	fr.rc.top = fr.rcPage.top + (long)(options.rMargins.top / 1000. * 1440);
	fr.rc.right = fr.rcPage.right - (long)(options.rMargins.right / 1000. * 1440);
	fr.rc.bottom = fr.rcPage.bottom - (long)(options.rMargins.bottom / 1000. * 1440);

	ZeroMemory(&di, sizeof(di));
	di.cbSize = sizeof(di);
	di.lpszDocName = szCaptionFile;
	di.lpszOutput = NULL;

	lTextLength = GetWindowTextLength(client) - (SendMessage(client, EM_GETLINECOUNT, 0, 0) - 1);

	for (i = 0; i < pd.nCopies; ++i) {

		if (!(pd.Flags & PD_SELECTION)) {
			fr.chrg.cpMin = 0;
			fr.chrg.cpMax = -1;
		}
		else fr.chrg = cr;

		if (StartDoc(pd.hDC, &di) <= 0) {
			ReportLastError();
			ERROROUT(GetString(IDS_PRINT_START_ERROR));
			goto Error;
		}

		do {
			nError = StartPage(pd.hDC);
			if (nError <= 0) break;

			lTextPrinted = SendMessage(client, EM_FORMATRANGE, FALSE, (LPARAM)&fr);
			SendMessage(client, EM_DISPLAYBAND, 0, (LPARAM)&fr.rc);

			nError = EndPage(pd.hDC);
			if (nError <= 0) break;

			fr.chrg.cpMin = lTextPrinted;

		} while (lTextPrinted < lTextLength && fr.chrg.cpMin != fr.chrg.cpMax);

		SendMessage(client, EM_FORMATRANGE, 0, (LPARAM)NULL);

		if (nError > 0) {
			EndDoc(pd.hDC);
		}
		else {
			AbortDoc(pd.hDC);
			ReportLastError();
			ERROROUT(GetString(IDS_PRINT_ERROR));
			break;
		}
	}

Error:
	EnableWindow(hwnd, TRUE);
	DestroyWindow(hdlgCancel);
	DeleteDC(pd.hDC);
}
#endif

void ReportLastError(void)
{
	LPVOID lpMsgBuf;
	UINT err = GetLastError(), i;
	LPTSTR szBuffer, errMsg = GetString(IDS_ERROR_MSG);

	i = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL,
				err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				(LPTSTR) &lpMsgBuf, 0, NULL);
	szBuffer = (LPTSTR) HeapAlloc(globalHeap, HEAP_ZERO_MEMORY, (i + wcslen(errMsg) + 10) * sizeof(TCHAR));
	wsprintf(szBuffer, errMsg, lpMsgBuf, err);
	MessageBox(NULL, szBuffer, STR_METAPAD, MB_OK | MB_ICONSTOP);

	LocalFree(lpMsgBuf);
	HeapFree(globalHeap, 0, (HGLOBAL)szBuffer);
/*
#ifndef	_DEBUG
	PostQuitMessage(0);
#endif
*/
}

void SaveMRUInfo(LPCTSTR szFullPath)
{
	HKEY key = NULL;
	TCHAR szKey[7];
	TCHAR szBuffer[MAXFN];
	TCHAR szTopVal[MAXFN];
	DWORD dwBufferSize;
	UINT i = 1;

	if (options.nMaxMRU == 0)
		return;

	if (!g_bIniMode && RegCreateKeyEx(HKEY_CURRENT_USER, STR_REGKEY, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &key, NULL) != ERROR_SUCCESS) {
		ReportLastError();
		return;
	}

	wsprintf(szKey, _T("mru_%d"), nMRUTop);
	dwBufferSize = sizeof(szBuffer);
	LoadOptionNumeric(key, szKey, (LPBYTE)(&szBuffer), dwBufferSize);

	if (lstrcmp(szFullPath, szBuffer) != 0) {
		if (++nMRUTop > options.nMaxMRU) {
			nMRUTop = 1;
		}

		SaveOption(key, _T("mru_Top"), REG_DWORD, (LPBYTE)&nMRUTop, sizeof(int));
		wsprintf(szKey, _T("mru_%d"), nMRUTop);
		dwBufferSize = sizeof(szTopVal);
		szTopVal[0] = _T('\0');
		LoadOptionString(key, szKey, (LPBYTE)&szTopVal, dwBufferSize);
		SaveOption(key, szKey, REG_SZ, (LPBYTE)szFullPath, sizeof(TCHAR) * lstrlen(szFullPath) + 1);

		for (i = 1; i <= options.nMaxMRU; ++i) {
			if (i == nMRUTop) continue;

			szBuffer[0] = szKey[0] = _T('\0');
			dwBufferSize = sizeof(szBuffer);
			wsprintf(szKey, _T("mru_%d"), i);
			LoadOptionString(key, szKey, (LPBYTE)&szBuffer, dwBufferSize);
			if (lstrcmpi(szBuffer, szFullPath) == 0) {
				SaveOption(key, szKey, REG_SZ, (LPBYTE)szTopVal, sizeof(TCHAR) * lstrlen(szTopVal) + 1);
				break;
			}
		}
	}

	if (key != NULL) {
		RegCloseKey(key);
	}

	PopulateMRUList();
}

void PopulateFavourites(void)
{
	TCHAR szBuffer[MAXFAVESIZE];
	TCHAR szName[MAXFN], szMenu[MAXFN];
	INT i, j, cnt, accel;
	MENUITEMINFO mio;
	HMENU hmenu = GetMenu(hwnd);
	HMENU hsub = GetSubMenu(hmenu, FAVEPOS);

	bHasFaves = FALSE;

	while (GetMenuItemCount(hsub) > 4) {
		DeleteMenu(hsub, 4, MF_BYPOSITION);
	}

	mio.cbSize = sizeof(MENUITEMINFO);
	mio.fMask = MIIM_TYPE | MIIM_ID;

	if (GetPrivateProfileString(STR_FAV_APPNAME, NULL, NULL, szBuffer, MAXFAVESIZE, szFav)) {
		bHasFaves = TRUE;
		/** @fixme Commented out condition for a for loop. The whole
				block is probably never executed. */
		for (i = 0, j = 0, cnt = accel = 1; /*cnt <= MAXFAVES*/; ++j, ++i) {
			szName[j] = szBuffer[i];
			if (szBuffer[i] == _T('\0')) {
				if (lstrcmp(szName, _T("-")) == 0) {
					mio.fType = MFT_SEPARATOR;
					InsertMenuItem(hsub, cnt + 3, TRUE, &mio);
					++cnt;
					j = -1;
				}
				else {
					mio.fType = MFT_STRING;
					wsprintf(szMenu, (accel < 10 ? _T("&%d ") : _T("%d ")), accel);
					++accel;
					lstrcat(szMenu, szName);
					j = -1;
					mio.dwTypeData = szMenu;
					mio.wID = ID_FAV_RANGE_BASE + cnt;
					InsertMenuItem(hsub, cnt + 3, TRUE, &mio);
					++cnt;
				}
			}
			if (szBuffer[i] == 0 && szBuffer[i+1] == 0) break;
		}
	}
}

void PopulateMRUList(void)
{
	HKEY key = NULL;
	DWORD nPrevSave = 0;
	TCHAR szBuffer[MAXFN+4];
	TCHAR szBuff2[MAXFN];
	HMENU hmenu = GetMenu(hwnd);
	HMENU hsub;
	TCHAR szKey[7];
	MENUITEMINFO mio;

	if (options.bRecentOnOwn)
		hsub = GetSubMenu(hmenu, 1);
	else
		hsub = GetSubMenu(GetSubMenu(hmenu, 0), RECENTPOS);

	if (options.nMaxMRU == 0) {
		if (options.bRecentOnOwn)
			EnableMenuItem(hmenu, 1, MF_BYPOSITION | MF_GRAYED);
		else
			EnableMenuItem(GetSubMenu(hmenu, 0), RECENTPOS, MF_BYPOSITION | MF_GRAYED);
		return;
	}
	else {
		if (options.bRecentOnOwn)
			EnableMenuItem(hmenu, 1, MF_BYPOSITION | MF_ENABLED);
		else
			EnableMenuItem(GetSubMenu(hmenu, 0), RECENTPOS, MF_BYPOSITION | MF_ENABLED);
	}

	if (!g_bIniMode && RegCreateKeyEx(HKEY_CURRENT_USER, STR_REGKEY, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &key, &nPrevSave) != ERROR_SUCCESS)
		ReportLastError();

	if (g_bIniMode || nPrevSave == REG_OPENED_EXISTING_KEY) {
		UINT i, num = 1, cnt = 0;
		DWORD dwBufferSize = sizeof(int);

		while (GetMenuItemCount(hsub)) {
			DeleteMenu(hsub, 0, MF_BYPOSITION);
		}

		LoadOptionNumeric(key, _T("mru_Top"), (LPBYTE)&nMRUTop, dwBufferSize);
		mio.cbSize = sizeof(MENUITEMINFO);
		mio.fMask = MIIM_TYPE | MIIM_ID;
		mio.fType = MFT_STRING;

		i = nMRUTop;
		while (cnt < options.nMaxMRU) {
			szBuff2[0] = _T('\0');
			wsprintf(szKey, _T("mru_%d"), i);
			wsprintf(szBuffer, (num < 10 ? _T("&%d ") : _T("%d ")), num);

			dwBufferSize = sizeof(szBuff2);
			LoadOptionString(key, szKey, (LPBYTE)(&szBuff2), dwBufferSize);

			if (lstrlen(szBuff2) > 0) {
				lstrcat(szBuffer, szBuff2);
				mio.dwTypeData = szBuffer;
				mio.wID = ID_MRU_BASE + i;
				InsertMenuItem(hsub, num, TRUE, &mio);
				num++;
			}
			if (i < 2)
				i = options.nMaxMRU;
			else
				i--;
			cnt++;
		}
	}
	if (key != NULL) {
		RegCloseKey(key);
	}
}

void CenterWindow(HWND hwndCenter)
{
	RECT r1, r2;
	GetWindowRect(GetParent(hwndCenter), &r1);
	GetWindowRect(hwndCenter, &r2);
	SetWindowPos(hwndCenter, HWND_TOP, (((r1.right - r1.left) - (r2.right - r2.left)) / 2) + r1.left, (((r1.bottom - r1.top) - (r2.bottom - r2.top)) / 2) + r1.top, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
}

void SelectWord(BOOL bFinding, BOOL bSmart, BOOL bAutoSelect)
{
	LONG lLine, lLineIndex, lLineLen;
	LPTSTR szBuffer;
	CHARRANGE cr;

#ifdef USE_RICH_EDIT
	SendMessage(client, EM_EXGETSEL, 0, (LPARAM)&cr);
	lLine = SendMessage(client, EM_EXLINEFROMCHAR, 0, (LPARAM)cr.cpMin);
	if (lLine == SendMessage(client, EM_EXLINEFROMCHAR, 0, (LPARAM)cr.cpMax)) {
#else
	SendMessage(client, EM_GETSEL, (WPARAM)&cr.cpMin, (LPARAM)&cr.cpMax);
	lLine = SendMessage(client, EM_LINEFROMCHAR, (WPARAM)cr.cpMin, 0);
	if (lLine == SendMessage(client, EM_LINEFROMCHAR, (WPARAM)cr.cpMax, 0)) {
#endif
		lLineLen = SendMessage(client, EM_LINELENGTH, (WPARAM)cr.cpMin, 0);
		szBuffer = (LPTSTR) HeapAlloc(globalHeap, HEAP_ZERO_MEMORY, (lLineLen+2) * sizeof(TCHAR));
		*((LPWORD)szBuffer) = (USHORT)(lLineLen + 1);
		SendMessage(client, EM_GETLINE, (WPARAM)lLine, (LPARAM) (LPCTSTR)szBuffer);
		szBuffer[lLineLen] = _T('\0');
		lLineIndex = SendMessage(client, EM_LINEINDEX, (WPARAM)lLine, 0);
		cr.cpMin -= lLineIndex;
		cr.cpMax -= lLineIndex;
		if (cr.cpMin == cr.cpMax && bAutoSelect) {
			if (bSmart) {
				if (_istprint(szBuffer[cr.cpMax]) && !_istspace(szBuffer[cr.cpMax])) {
					cr.cpMax++;
				}
				else {
					while (cr.cpMin && (_istalnum(szBuffer[cr.cpMin-1]) || szBuffer[cr.cpMin-1] == _T('_')))
						cr.cpMin--;
				}
				if (_istalnum(szBuffer[cr.cpMin]) || szBuffer[cr.cpMin] == _T('_')) {
					while (cr.cpMin && (_istalnum(szBuffer[cr.cpMin-1]) || szBuffer[cr.cpMin-1] == _T('_')))
						cr.cpMin--;
					while (cr.cpMax < lLineLen && (_istalnum(szBuffer[cr.cpMax]) || szBuffer[cr.cpMax] == _T('_')))
						cr.cpMax++;
				}
			}
			else {
				if (_istprint(szBuffer[cr.cpMax])) {
					while (cr.cpMin && (!_istspace(szBuffer[cr.cpMin-1])))
						cr.cpMin--;
					while (cr.cpMax < lLineLen && (!_istspace(szBuffer[cr.cpMax])))
						cr.cpMax++;
				}
				else {
					while (cr.cpMin && (_istspace(szBuffer[cr.cpMin-1])))
						cr.cpMin--;
					while (cr.cpMin && (!_istspace(szBuffer[cr.cpMin-1])))
						cr.cpMin--;
				}
				while (cr.cpMax < lLineLen && _istspace(szBuffer[cr.cpMax]) && szBuffer[cr.cpMax])
					cr.cpMax++;
			}
		}
		if (bAutoSelect || cr.cpMin != cr.cpMax) {
			if (bFinding) {
				if (cr.cpMax - cr.cpMin > MAXFIND) {
					lstrcpyn(szFindText, szBuffer + cr.cpMin, MAXFIND);
					szFindText[MAXFIND - 1] = _T('\0');
				}
				else {
					lstrcpyn(szFindText, szBuffer + cr.cpMin, cr.cpMax - cr.cpMin + 1);
					szFindText[cr.cpMax - cr.cpMin] = _T('\0');
				}
			}
			cr.cpMin += lLineIndex;
			cr.cpMax += lLineIndex;
#ifdef USE_RICH_EDIT
			SendMessage(client, EM_EXSETSEL, 0, (LPARAM)&cr);
#else
			SendMessage(client, EM_SETSEL, (WPARAM)cr.cpMin, (LPARAM)cr.cpMax);
#endif
		}
		HeapFree(globalHeap, 0, (HGLOBAL)szBuffer);
		UpdateStatus();
	}
}

#ifdef STREAMING
DWORD CALLBACK EditStreamIn(DWORD dwCookie, LPBYTE pbBuff, LONG cb, LONG *pcb)
{
	LPTSTR szBuffer = (LPTSTR)dwCookie;
	LONG lBufferLength = lstrlen(szBuffer);
	static LONG nBytesDone = 0;

	wsprintf(szStatusMessage, _T("Loading file... %d"), nBytesDone);
	UpdateStatus();

	if (*pcb == 1) nBytesDone = 0;

	if (nBytesDone == lBufferLength) {
		*pcb = 0;
		return 0;
	}

	if (cb > lBufferLength - nBytesDone) {
		cb = lBufferLength - nBytesDone;
	}

	memcpy(pbBuff, szBuffer + nBytesDone, cb * sizeof(TCHAR));
	nBytesDone += cb;
	*pcb = cb;
	return 0;
}
#endif

void SetFont(HFONT* phfnt, BOOL bPrimary)
{
	LOGFONT logfind;

	if (*phfnt)
		DeleteObject(*phfnt);

	if (hfontfind)
		DeleteObject(hfontfind);

	if (bPrimary) {
		if (options.nPrimaryFont == 0) {
			*phfnt = GetStockObject(SYSTEM_FIXED_FONT);
			hfontfind = *phfnt;
			return;
		}
		else {
			*phfnt = CreateFontIndirect(&options.PrimaryFont);
			CopyMemory((PVOID)&logfind, (CONST VOID*)&options.PrimaryFont, sizeof(LOGFONT));
		}
	}
	else {
		if (options.nSecondaryFont == 0) {
			*phfnt = GetStockObject(ANSI_FIXED_FONT);
			hfontfind = *phfnt;
			return;
		}
		else {
			*phfnt = CreateFontIndirect(&options.SecondaryFont);
			CopyMemory((PVOID)&logfind, (CONST VOID*)&options.SecondaryFont, sizeof(LOGFONT));
		}
	}

	{
		HDC clientdc = GetDC(client);
		logfind.lfHeight = -MulDiv(LOWORD(GetDialogBaseUnits())+2, GetDeviceCaps(clientdc, LOGPIXELSY), 72);
		hfontfind = CreateFontIndirect(&logfind);
		ReleaseDC(client, clientdc);
	}
}

BOOL SetClientFont(BOOL bPrimary)
{
#ifdef USE_RICH_EDIT
	HDC clientDC;
	CHARFORMAT cf;
	TCHAR szFace[MAXFONT];
	TEXTMETRIC tm;
	CHARRANGE cr;
	BOOL bUseSystem;

	bUseSystem = bPrimary ? options.bSystemColours : options.bSystemColours2;
	//if (!bPrimary && options.bSecondarySystemColour) bUseSystem = TRUE;

	if (SendMessage(client, EM_CANUNDO, 0, 0)) {
		if (!options.bSuppressUndoBufferPrompt && MessageBox(hwnd, GetString(IDS_FONT_UNDO_WARNING), STR_METAPAD, MB_ICONEXCLAMATION | MB_OKCANCEL) == IDCANCEL) {
			return FALSE;
		}
	}

	SendMessage(client, EM_EXGETSEL, 0, (LPARAM)&cr);

	SendMessage(client, WM_SETREDRAW, (WPARAM)FALSE, 0);

	SetFont(&hfontmain, bPrimary);

	cf.cbSize = sizeof (cf);
	cf.dwMask = CFM_COLOR | CFM_SIZE | CFM_FACE | CFM_BOLD | CFM_CHARSET | CFM_ITALIC;

	SendMessage(client, EM_GETCHARFORMAT, TRUE, (LPARAM)&cf);

	clientDC = GetDC(client);
	SelectObject (clientDC, hfontmain);

	if (!GetTextFace(clientDC, MAXFONT, szFace))
		ReportLastError();

	lstrcpy(cf.szFaceName, szFace);

	if (bPrimary) {
		cf.bCharSet = options.PrimaryFont.lfCharSet;
		cf.bPitchAndFamily = options.PrimaryFont.lfPitchAndFamily;
	}
	else {
		cf.bCharSet = options.SecondaryFont.lfCharSet;
		cf.bPitchAndFamily = options.SecondaryFont.lfPitchAndFamily;
	}

	if (!GetTextMetrics(clientDC, &tm))
		ReportLastError();

	cf.yHeight = 15 * (tm.tmHeight - tm.tmInternalLeading);

	cf.dwEffects = 0;
	if (tm.tmWeight == FW_BOLD) {
		cf.dwEffects |= CFE_BOLD;
	}
	if (tm.tmItalic) {
		cf.dwEffects |= CFE_ITALIC;
	}
	if (bUseSystem)
		cf.dwEffects |= CFE_AUTOCOLOR;
	else
		cf.crTextColor = bPrimary ? options.FontColour : options.FontColour2;

	bLoading = TRUE;
	SendMessage(client, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);
	bLoading = FALSE;
	ReleaseDC(client, clientDC);

	UpdateWindowText();

	SendMessage(client, EM_SETBKGNDCOLOR, (WPARAM)bUseSystem, (LPARAM)(bPrimary ? options.BackColour : options.BackColour2));
	SendMessage(client, EM_EXSETSEL, 0, (LPARAM)&cr);
	SendMessage(client, WM_SETREDRAW, (WPARAM)TRUE, 0);

	InvalidateRect(hwnd, NULL, TRUE);

	return TRUE;
#else
	SetFont(&hfontmain, bPrimary);
	SendMessage(client, WM_SETFONT, (WPARAM)hfontmain, 0);
	SendMessage(client, EM_SETMARGINS, (WPARAM)EC_LEFTMARGIN, MAKELPARAM(options.nSelectionMarginWidth, 0));
	SetTabStops();
	return TRUE;
#endif
}

void SetTabStops(void)
{
#ifdef USE_RICH_EDIT
	UINT nTmp;
	PARAFORMAT pf;
	int nWidth;
	HDC clientDC;
	BOOL bOldDirty = bDirtyFile;
	CHARRANGE cr, cr2;

	SendMessage(client, EM_EXGETSEL, 0, (LPARAM)&cr);

	SendMessage(client, WM_SETREDRAW, (WPARAM)FALSE, 0);
	cr2.cpMin = 0;
	cr2.cpMax = -1;
	SendMessage(client, EM_EXSETSEL, 0, (LPARAM)&cr2);

	clientDC = GetDC(client);
	SelectObject (clientDC, hfontmain);

	if (!GetCharWidth32(clientDC, (UINT)VK_SPACE, (UINT)VK_SPACE, &nWidth))
		ERROROUT(GetString(IDS_TCHAR_WIDTH_ERROR));

	nTmp = nWidth * 15 * options.nTabStops;

	ZeroMemory(&pf, sizeof(pf));
	pf.cbSize = sizeof(PARAFORMAT);
	SendMessage(client, EM_GETPARAFORMAT, 0, (LPARAM)&pf);

	pf.dwMask = PFM_TABSTOPS;
	pf.cTabCount = MAX_TAB_STOPS;
	{
		int itab;
		for (itab = 0; itab < pf.cTabCount; itab++)
			pf.rgxTabs[itab] = (itab+1) * nTmp;
	}
	ReleaseDC(client, clientDC);

	bDirtyFile = TRUE;

#ifndef BUILD_METAPAD_UNICODE
	//TODO: This fails, for now commented by preprocessor (Rich-Unicode build / "Release")
	if (!SendMessage(client, EM_SETPARAFORMAT, 0, (LPARAM)(PARAFORMAT FAR *)&pf))
		ERROROUT(GetString(IDS_PARA_FORMAT_ERROR));
#endif

	bDirtyFile = bOldDirty;

	SendMessage(client, EM_EXSETSEL, 0, (LPARAM)&cr);
	SendMessage(client, WM_SETREDRAW, (WPARAM)TRUE, 0);
	InvalidateRect(hwnd, NULL, TRUE);
#else
	UINT nTmp = options.nTabStops * 4;

	SendMessage(client, EM_SETTABSTOPS, (WPARAM)1, (LPARAM)&nTmp);
#endif
}

LPCTSTR GetShadowBuffer(void)
{
	if (lpszShadow == NULL || SendMessage(client, EM_GETMODIFY, 0, 0)) {
		UINT nSize = GetWindowTextLength(client)+1;
		if (nSize > nShadowSize) {
			if (lpszShadow != NULL)
				HeapFree(globalHeap, 0, (HGLOBAL) lpszShadow);
			lpszShadow = (LPTSTR) HeapAlloc(globalHeap, HEAP_ZERO_MEMORY, nSize * sizeof(TCHAR));
			nShadowSize = nSize;
		}

#ifdef USE_RICH_EDIT
		{
			TEXTRANGE tr;
			tr.chrg.cpMin = 0;
			tr.chrg.cpMax = -1;
			tr.lpstrText = lpszShadow;

			SendMessage(client, EM_GETTEXTRANGE, 0, (LPARAM)&tr);
		}
#else
		GetWindowText(client, lpszShadow, nSize);
#endif
		SendMessage(client, EM_SETMODIFY, (WPARAM) FALSE, 0);
	}

	return lpszShadow;
}

#ifdef USE_RICH_EDIT
BOOL DoSearch(LPCTSTR szText, LONG lStart, LONG lEnd, BOOL bDown, BOOL bWholeWord, BOOL bCase, BOOL bFromTop)
{
	LONG lPrevStart = 0;
	UINT nFlags = FR_DOWN;
	FINDTEXT ft;
	CHARRANGE cr;

	if (bWholeWord)
		nFlags |= FR_WHOLEWORD;

	if (bCase)
		nFlags |= FR_MATCHCASE;

	ft.lpstrText = (LPTSTR)szText;

	if (bDown) {
		if (bFromTop)
			ft.chrg.cpMin = 0;
		else
			ft.chrg.cpMin = lStart + (lStart == lEnd ? 0 : 1);
		ft.chrg.cpMax = nReplaceMax;

		cr.cpMin = SendMessage(client, EM_FINDTEXT, (WPARAM)nFlags, (LPARAM)&ft);

		if (_tcschr(szText, _T('\r'))) {
			LONG lLine, lLines;

			lLine = SendMessage(client, EM_EXLINEFROMCHAR, 0, (LPARAM)cr.cpMin);
			lLines = SendMessage(client, EM_GETLINECOUNT, 0, 0);

			if (lLine == lLines - 1) {
				return FALSE;
			}
		}

		if (cr.cpMin == -1)
			return FALSE;
		cr.cpMax = cr.cpMin + lstrlen(szText);

		SendMessage(client, EM_EXSETSEL, 0, (LPARAM)&cr);
	}
	else {
		ft.chrg.cpMin = 0;
		if (bFromTop)
			ft.chrg.cpMax = -1;
		else
			ft.chrg.cpMax = (lStart == lEnd ? lEnd : lEnd - 1);
		lStart = SendMessage(client, EM_FINDTEXT, (WPARAM)nFlags, (LPARAM)&ft);
		if (lStart == -1)
			return FALSE;
		else {
			while (lStart != -1) {
				lPrevStart = lStart;
				lStart = SendMessage(client, EM_FINDTEXT, (WPARAM)nFlags, (LPARAM)&ft);
				ft.chrg.cpMin = lStart + 1;
			}
		}
		cr.cpMin = lPrevStart;
		cr.cpMax = lPrevStart + lstrlen(szText);
		SendMessage(client, EM_EXSETSEL, 0, (LPARAM)&cr);
	}
	if (!bReplacingAll)	UpdateStatus();
	return TRUE;
}
#else
typedef int (WINAPI* CMPFUNC)(LPCTSTR str1, LPCTSTR str2);

BOOL DoSearch(LPCTSTR szText, LONG lStart, LONG lEnd, BOOL bDown, BOOL bWholeWord, BOOL bCase, BOOL bFromTop)
{
	LONG lSize;
	int nFindLen = lstrlen(szText);
	LPCTSTR szBuffer = GetShadowBuffer();
	LPCTSTR lpszStop, lpsz, lpszFound = NULL;
	CMPFUNC pfnCompare = bCase ? lstrcmp : lstrcmpi;

	lSize = GetWindowTextLength(client);

	if (!szBuffer) {
		ReportLastError();
		return FALSE;
	}

	if (bDown) {
		if (nReplaceMax > -1) {
			lpszStop = szBuffer + nReplaceMax - 1;
		}
		else {
			lpszStop = szBuffer + lSize - 1;
		}
		lpsz = szBuffer + (bFromTop ? 0 : lStart + (lStart == lEnd ? 0 : 1));
	}
	else {
		lpszStop = szBuffer + (bFromTop ? lSize : lStart + (nFindLen == 1 && lStart > 0 ? -1 : 0));
		lpsz = szBuffer;
	}

	while (lpszStop != szBuffer && lpsz <= lpszStop - (bDown ? 0 : nFindLen-1) && (!bDown || (bDown && lpszFound == NULL))) {
		if ((bCase && *lpsz == *szText) || (TCHAR)(DWORD_PTR)CharLower((LPTSTR)(DWORD_PTR)(BYTE)*lpsz) == (TCHAR)(DWORD_PTR)CharLower((LPTSTR)(DWORD_PTR)(BYTE)*szText)) {
			LPTSTR lpch = (LPTSTR)(lpsz + nFindLen);
			TCHAR chSave = *lpch;
			int nResult;

			*lpch = _T('\0');
			nResult = (*pfnCompare)(lpsz, szText);
			*lpch = chSave;


			if (bWholeWord && lpsz > szBuffer && (_istalnum(*(lpsz-1)) || *(lpsz-1) == _T('_') || _istalnum(*(lpsz + nFindLen)) || *(lpsz + nFindLen) == _T('_')))
			;
			else if (nResult == 0) {
				lpszFound = lpsz;
			}
		}
		lpsz++;
	}

	if (lpszFound != NULL) {
		LONG lEnd;

		lStart = lpszFound - szBuffer;
		lEnd = lStart + nFindLen;
		SendMessage(client, EM_SETSEL, (WPARAM)lStart, (LPARAM)lEnd);
		if (!bReplacingAll)	UpdateStatus();
		return TRUE;
	}
	return FALSE;
}
#endif

void FixReadOnlyMenu(void)
{
	HMENU hmenu = GetMenu(hwnd);
	if (options.bReadOnlyMenu) {
		MENUITEMINFO mio;
		mio.cbSize = sizeof(MENUITEMINFO);
		mio.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
		mio.fType = MFT_STRING;
		mio.dwTypeData = GetString(IDS_READONLY_MENU);
		mio.wID = ID_READONLY;
		if (bReadOnly)
			mio.fState = MFS_CHECKED;
		else
			mio.fState = 0;

		InsertMenuItem(GetSubMenu(hmenu, 0), READONLYPOS, TRUE, &mio);
	}
	else {
		DeleteMenu(GetSubMenu(hmenu, 0), READONLYPOS, MF_BYPOSITION);
	}
}

void FixMRUMenus(void)
{
	HMENU hmenu = GetMenu(hwnd);
	MENUITEMINFO mio;

	mio.cbSize = sizeof(MENUITEMINFO);
	mio.fMask = MIIM_TYPE | MIIM_SUBMENU;
	mio.fType = MFT_STRING;
	mio.hSubMenu = CreateMenu();

	if (options.bRecentOnOwn) {
		mio.dwTypeData = GetString(IDS_RECENT_MENU);
		InsertMenuItem(hmenu, 1, TRUE, &mio);
		if (hrecentmenu)
			DestroyMenu(hrecentmenu);
		hrecentmenu = mio.hSubMenu;

		DeleteMenu(GetSubMenu(hmenu, 0), RECENTPOS, MF_BYPOSITION);
		DeleteMenu(GetSubMenu(hmenu, 0), RECENTPOS, MF_BYPOSITION);
	}
	else {
		mio.dwTypeData = GetString(IDS_RECENT_FILES_MENU);
		InsertMenuItem(GetSubMenu(hmenu, 0), RECENTPOS, TRUE, &mio);
		if (hrecentmenu)
			DestroyMenu(hrecentmenu);
		hrecentmenu = mio.hSubMenu;
		mio.hSubMenu = 0;
		mio.fType = MFT_SEPARATOR;
		mio.fMask = MIIM_TYPE;
		InsertMenuItem(GetSubMenu(hmenu, 0), RECENTPOS + 1, TRUE, &mio);
		DeleteMenu(hmenu, 1, MF_BYPOSITION);
	}
	DrawMenuBar(hwnd);
}

void GotoLine(LONG lLine, LONG lOffset)
{
	LONG lTmp, lLineLen;
	CHARRANGE cr;

	if (lLine == -1 || lOffset == -1) return;

	lTmp = SendMessage(client, EM_GETLINECOUNT, 0, 0);
	if (lLine > lTmp)
		lLine = lTmp;
	else if (lLine < 1)
		lLine = 1;
	lTmp = SendMessage(client, EM_LINEINDEX, (WPARAM)lLine-1, 0);
	lLineLen = SendMessage(client, EM_LINELENGTH, (WPARAM)lTmp, 0);

	if (!options.bHideGotoOffset) {

		if (lOffset > lLineLen)
			lOffset = lLineLen + 1;
		else if (lOffset < 1)
			lOffset = 1;
		lTmp += lOffset - 1;

		cr.cpMin = cr.cpMax = lTmp;
#ifdef USE_RICH_EDIT
		SendMessage(client, EM_EXSETSEL, 0, (LPARAM)&cr);
#else
		SendMessage(client, EM_SETSEL, (WPARAM)cr.cpMin, (LPARAM)cr.cpMax);
#endif
	}
	else {
		cr.cpMin = lTmp;
		cr.cpMax = lTmp + lLineLen;
#ifdef USE_RICH_EDIT
		SendMessage(client, EM_EXSETSEL, 0, (LPARAM)&cr);
#else
		SendMessage(client, EM_SETSEL, (WPARAM)cr.cpMin, (LPARAM)cr.cpMax);
#endif
	}
	SendMessage(client, EM_SCROLLCARET, 0, 0);
	UpdateStatus();
}

BOOL CALLBACK AboutDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static BOOL bIcon = FALSE;
	switch (uMsg) {
		case WM_INITDIALOG: {
			if (bIcon) {
				SendDlgItemMessage(hwndDlg, IDC_DLGICON, STM_SETICON, (WPARAM)LoadIcon(hinstThis, MAKEINTRESOURCE(IDI_EYE)), 0);
				SetDlgItemText(hwndDlg, IDC_STATICX, STR_ABOUT_HACKER);
			}
			else {
				SetDlgItemText(hwndDlg, IDC_STATICX, STR_ABOUT_NORMAL);
			}

			CenterWindow(hwndDlg);
			SetWindowPos(client, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED); // hack for icon jitter
			SetDlgItemText(hwndDlg, IDC_EDIT_URL, STR_URL);
			SetDlgItemText(hwndDlg, IDOK, GetString(IDS_OK_BUTTON));
			SetDlgItemText(hwndDlg, IDC_STATIC_COPYRIGHT, STR_COPYRIGHT);
			SetDlgItemText(hwndDlg, IDC_STATIC_COPYRIGHT2, GetString(IDS_ALLRIGHTS));
			break;
		}
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDOK:
				case IDCANCEL:
					EndDialog(hwndDlg, wParam);
					break;
				case IDC_DLGICON:
					if (HIWORD(wParam) == STN_DBLCLK) {
						HICON hicon;
						int i;
						RECT r;
						int rands[EGGNUM] = {0, -3, 6, -8, 0, -6, 10, -3, -8, 5, 9, 2, -3, -4, 0};

						GetWindowRect(hwndDlg, &r);

						for (i = 0; i < 300; ++i)
							SetWindowPos(hwndDlg, HWND_TOP, r.left+rands[i % EGGNUM], r.top+rands[(i+1) % EGGNUM], 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);

						if (bIcon) {
							hicon = LoadIcon(hinstThis, MAKEINTRESOURCE(IDI_PAD));
							SetDlgItemText(hwndDlg, IDC_STATICX, STR_ABOUT_NORMAL);

							{
								HMENU hmenu = GetSubMenu(GetSubMenu(GetMenu(hwnd), EDITPOS), CONVERTPOS);
								DeleteMenu(hmenu, 0, MF_BYPOSITION);
								DeleteMenu(hmenu, 0, MF_BYPOSITION);
							}
						}
						else {
							hicon = LoadIcon(hinstThis, MAKEINTRESOURCE(IDI_EYE));
							SetDlgItemText(hwndDlg, IDC_STATICX, STR_ABOUT_HACKER);

							{
								HMENU hmenu = GetSubMenu(GetSubMenu(GetMenu(hwnd), EDITPOS), CONVERTPOS);

								MENUITEMINFO mio;
								mio.cbSize = sizeof(MENUITEMINFO);
								mio.fMask = MIIM_TYPE | MIIM_ID;
								mio.fType = MFT_STRING;
								mio.dwTypeData = _T("31337 h4Ck3r\tCtrl+*");
								mio.wID = ID_HACKER;

								InsertMenuItem(hmenu, 0, TRUE, &mio);
								mio.fType = MFT_SEPARATOR;
								mio.fMask = MIIM_TYPE;
								InsertMenuItem(hmenu, 1, TRUE, &mio);
							}
						}

						SetClassLongPtr(GetParent(hwndDlg), GCLP_HICON, (LONG_PTR) hicon);
						SendDlgItemMessage(hwndDlg, IDC_DLGICON, STM_SETICON, (WPARAM)hicon, 0);
						bIcon = !bIcon;

					}
			}
			break;
		default:
			return FALSE;
	}
	return TRUE;
}

BOOL CALLBACK AboutPluginDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
		case WM_INITDIALOG: {
			SetDlgItemText(hwndDlg, IDC_EDIT_PLUGIN_LANG, GetString(IDS_PLUGIN_LANGUAGE));
			SetDlgItemText(hwndDlg, IDC_EDIT_PLUGIN_RELEASE, GetString(IDS_PLUGIN_RELEASE));
			SetDlgItemText(hwndDlg, IDC_EDIT_PLUGIN_TRANSLATOR, GetString(IDS_PLUGIN_TRANSLATOR));
			SetDlgItemText(hwndDlg, IDC_EDIT_PLUGIN_EMAIL, GetString(IDS_PLUGIN_EMAIL));
			CenterWindow(hwndDlg);
			return TRUE;
		}
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDOK:
				case IDCANCEL:
					EndDialog(hwndDlg, wParam);
			}
			return TRUE;
		default:
			return FALSE;
	}
}


BOOL CALLBACK GotoDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
		case WM_INITDIALOG: {
			LONG lLine, lLineIndex;
			TCHAR szLine[6];
			CHARRANGE cr;

#ifdef USE_RICH_EDIT
			SendMessage(client, EM_EXGETSEL, 0, (LPARAM)&cr);
			CenterWindow(hwndDlg);
			lLine = SendMessage(client, EM_EXLINEFROMCHAR, 0, (LPARAM)cr.cpMax);
#else
			SendMessage(client, EM_GETSEL, (WPARAM)&cr.cpMin, (LPARAM)&cr.cpMax);
			CenterWindow(hwndDlg);
			lLine = SendMessage(client, EM_LINEFROMCHAR, (LPARAM)cr.cpMax, 0);
#endif
			wsprintf(szLine, _T("%d"), lLine+1);
			SetDlgItemText(hwndDlg, IDC_LINE, szLine);

			if (options.bHideGotoOffset) {
				HWND hwndItem = GetDlgItem(hwndDlg, IDC_OFFSET);
				ShowWindow(hwndItem, SW_HIDE);
				hwndItem = GetDlgItem(hwndDlg, IDC_OFFSET_TEXT);
				ShowWindow(hwndItem, SW_HIDE);
			}
			else {
				lLineIndex = SendMessage(client, EM_LINEINDEX, (WPARAM)lLine, 0);
				wsprintf(szLine, _T("%d"), 1 + cr.cpMax - lLineIndex);
				SetDlgItemText(hwndDlg, IDC_OFFSET, szLine);
				SendDlgItemMessage(hwndDlg, IDC_LINE, EM_SETSEL, 0, (LPARAM)-1);
			}
			return TRUE;
		}
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDOK: {
					LONG lLine, lCol = 0;
					TCHAR szLine[10];

					GetDlgItemText(hwndDlg, IDC_LINE, szLine, 10);
					lLine = _ttol(szLine);
					if (!options.bHideGotoOffset) {
						GetDlgItemText(hwndDlg, IDC_OFFSET, szLine, 10);
						lCol = _ttol(szLine);
					}
					GotoLine(lLine, lCol);
				}
				case IDCANCEL:
					EndDialog(hwndDlg, wParam);
			}
			return TRUE;
		default:
			return FALSE;
	}
}

BOOL CALLBACK AddFavDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
		case WM_INITDIALOG: {
			SetDlgItemText(hwndDlg, IDC_DATA, szFile);
			CenterWindow(hwndDlg);
			return TRUE;
		}
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDOK: {
					TCHAR szName[MAXFN];
					GetDlgItemText(hwndDlg, IDC_DATA, szName, MAXFN);
					WritePrivateProfileString(STR_FAV_APPNAME, szName, szFile, szFav);
					PopulateFavourites();
				}
				case IDCANCEL:
					EndDialog(hwndDlg, wParam);
			}
			return TRUE;
		default:
			return FALSE;
	}
}

BOOL CALLBACK AdvancedPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			TCHAR szInt[5];

			SendDlgItemMessage(hwndDlg, IDC_GOTO_OFFSET, BM_SETCHECK, (WPARAM) options.bHideGotoOffset, 0);
			SendDlgItemMessage(hwndDlg, IDC_RECENT, BM_SETCHECK, (WPARAM) options.bRecentOnOwn, 0);
			SendDlgItemMessage(hwndDlg, IDC_SMARTHOME, BM_SETCHECK, (WPARAM) options.bNoSmartHome, 0);
			SendDlgItemMessage(hwndDlg, IDC_NO_SAVE_EXTENSIONS, BM_SETCHECK, (WPARAM) options.bNoAutoSaveExt, 0);
			SendDlgItemMessage(hwndDlg, IDC_CONTEXT_CURSOR, BM_SETCHECK, (WPARAM) options.bContextCursor, 0);
			SendDlgItemMessage(hwndDlg, IDC_CURRENT_FIND_FONT, BM_SETCHECK, (WPARAM) options.bCurrentFindFont, 0);
			SendDlgItemMessage(hwndDlg, IDC_SECONDARY_PRINT_FONT, BM_SETCHECK, (WPARAM) options.bPrintWithSecondaryFont, 0);
			SendDlgItemMessage(hwndDlg, IDC_NO_SAVE_HISTORY, BM_SETCHECK, (WPARAM) options.bNoSaveHistory, 0);
			SendDlgItemMessage(hwndDlg, IDC_NO_FIND_SELECT, BM_SETCHECK, (WPARAM) options.bNoFindAutoSelect, 0);
			SendDlgItemMessage(hwndDlg, IDC_NO_FAVES, BM_SETCHECK, (WPARAM) options.bNoFaves, 0);
			SendDlgItemMessage(hwndDlg, IDC_INSERT_TIME, BM_SETCHECK, (WPARAM) options.bDontInsertTime, 0);
			SendDlgItemMessage(hwndDlg, IDC_PROMPT_BINARY, BM_SETCHECK, (WPARAM) options.bNoWarningPrompt, 0);
			SendDlgItemMessage(hwndDlg, IDC_STICKY_WINDOW, BM_SETCHECK, (WPARAM) options.bStickyWindow, 0);
			SendDlgItemMessage(hwndDlg, IDC_READONLY_MENU, BM_SETCHECK, (WPARAM) options.bReadOnlyMenu, 0);
#ifdef USE_RICH_EDIT
			SendDlgItemMessage(hwndDlg, IDC_LINK_DC, BM_SETCHECK, (WPARAM) options.bLinkDoubleClick, 0);
			SendDlgItemMessage(hwndDlg, IDC_HIDE_SCROLLBARS, BM_SETCHECK, (WPARAM) options.bHideScrollbars, 0);
			SendDlgItemMessage(hwndDlg, IDC_SUPPRESS_UNDO_PROMPT, BM_SETCHECK, (WPARAM) options.bSuppressUndoBufferPrompt, 0);
#else
			SendDlgItemMessage(hwndDlg, IDC_DEFAULT_PRINT, BM_SETCHECK, (WPARAM) options.bDefaultPrintFont, 0);
			SendDlgItemMessage(hwndDlg, IDC_ALWAYS_LAUNCH, BM_SETCHECK, (WPARAM) options.bAlwaysLaunch, 0);
			SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_DEFAULT_PRINT, 0), 0);
#endif
			SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_STICKY_WINDOW, 0), 0);

			SendDlgItemMessage(hwndDlg, IDC_COMBO_FORMAT, CB_ADDSTRING, 0, (LPARAM)_T("DOS Text"));
			SendDlgItemMessage(hwndDlg, IDC_COMBO_FORMAT, CB_ADDSTRING, 0, (LPARAM)_T("UNIX Text"));
			SendDlgItemMessage(hwndDlg, IDC_COMBO_FORMAT, CB_ADDSTRING, 0, (LPARAM)_T("Unicode"));
			SendDlgItemMessage(hwndDlg, IDC_COMBO_FORMAT, CB_ADDSTRING, 0, (LPARAM)_T("Unicode BE"));
			SendDlgItemMessage(hwndDlg, IDC_COMBO_FORMAT, CB_ADDSTRING, 0, (LPARAM)_T("UTF-8"));


			SendDlgItemMessage(hwndDlg, IDC_COMBO_FORMAT, CB_SETCURSEL, (WPARAM)options.nFormatIndex, 0);

			wsprintf(szInt, _T("%d"), options.nMaxMRU);
			SetDlgItemText(hwndDlg, IDC_EDIT_MAX_MRU, szInt);

			return TRUE;
		}
	case WM_NOTIFY:
		switch (((NMHDR FAR *)lParam)->code) {
		case PSN_KILLACTIVE:
			{
				TCHAR szInt[10];
				INT nTmp;

				GetDlgItemText(hwndDlg, IDC_EDIT_MAX_MRU, szInt, 10);
				nTmp = _ttoi(szInt);
				if (nTmp < 0 || nTmp > 16) {
					ERROROUT(GetString(IDS_MAX_RECENT_WARNING));
					SetFocus(GetDlgItem(hwndDlg, IDC_EDIT_MAX_MRU));
					SetWindowLong (hwndDlg, DWLP_MSGRESULT, TRUE);
				}
				return TRUE;
			}
		case PSN_APPLY:
			{
				TCHAR szInt[5];
				INT nTmp;

				GetDlgItemText(hwndDlg, IDC_EDIT_MAX_MRU, szInt, 3);
				nTmp = _ttoi(szInt);
				options.nMaxMRU = nTmp;

				options.bHideGotoOffset = (BST_CHECKED == SendDlgItemMessage(hwndDlg, IDC_GOTO_OFFSET, BM_GETCHECK, 0, 0));
				options.bRecentOnOwn = (BST_CHECKED == SendDlgItemMessage(hwndDlg, IDC_RECENT, BM_GETCHECK, 0, 0));
				options.bNoSmartHome = (BST_CHECKED == SendDlgItemMessage(hwndDlg, IDC_SMARTHOME, BM_GETCHECK, 0, 0));
				options.bNoAutoSaveExt = (BST_CHECKED == SendDlgItemMessage(hwndDlg, IDC_NO_SAVE_EXTENSIONS, BM_GETCHECK, 0, 0));
				options.bContextCursor = (BST_CHECKED == SendDlgItemMessage(hwndDlg, IDC_CONTEXT_CURSOR, BM_GETCHECK, 0, 0));
				options.bCurrentFindFont = (BST_CHECKED == SendDlgItemMessage(hwndDlg, IDC_CURRENT_FIND_FONT, BM_GETCHECK, 0, 0));
				options.bPrintWithSecondaryFont = (BST_CHECKED == SendDlgItemMessage(hwndDlg, IDC_SECONDARY_PRINT_FONT, BM_GETCHECK, 0, 0));
				options.bNoSaveHistory = (BST_CHECKED == SendDlgItemMessage(hwndDlg, IDC_NO_SAVE_HISTORY, BM_GETCHECK, 0, 0));
				options.bNoFindAutoSelect = (BST_CHECKED == SendDlgItemMessage(hwndDlg, IDC_NO_FIND_SELECT, BM_GETCHECK, 0, 0));
				options.bNoFaves = (BST_CHECKED == SendDlgItemMessage(hwndDlg, IDC_NO_FAVES, BM_GETCHECK, 0, 0));
				options.bDontInsertTime = (BST_CHECKED == SendDlgItemMessage(hwndDlg, IDC_INSERT_TIME, BM_GETCHECK, 0, 0));
				options.bNoWarningPrompt = (BST_CHECKED == SendDlgItemMessage(hwndDlg, IDC_PROMPT_BINARY, BM_GETCHECK, 0, 0));
				options.bStickyWindow = (BST_CHECKED == SendDlgItemMessage(hwndDlg, IDC_STICKY_WINDOW, BM_GETCHECK, 0, 0));
				options.bReadOnlyMenu = (BST_CHECKED == SendDlgItemMessage(hwndDlg, IDC_READONLY_MENU, BM_GETCHECK, 0, 0));
#ifndef USE_RICH_EDIT
				options.bDefaultPrintFont = (BST_CHECKED == SendDlgItemMessage(hwndDlg, IDC_DEFAULT_PRINT, BM_GETCHECK, 0, 0));
				options.bAlwaysLaunch = (BST_CHECKED == SendDlgItemMessage(hwndDlg, IDC_ALWAYS_LAUNCH, BM_GETCHECK, 0, 0));
#else
				options.bLinkDoubleClick = (BST_CHECKED == SendDlgItemMessage(hwndDlg, IDC_LINK_DC, BM_GETCHECK, 0, 0));
				options.bHideScrollbars = (BST_CHECKED == SendDlgItemMessage(hwndDlg, IDC_HIDE_SCROLLBARS, BM_GETCHECK, 0, 0));
				options.bSuppressUndoBufferPrompt = (BST_CHECKED == SendDlgItemMessage(hwndDlg, IDC_SUPPRESS_UNDO_PROMPT, BM_GETCHECK, 0, 0));
#endif
				options.nFormatIndex = SendDlgItemMessage(hwndDlg, IDC_COMBO_FORMAT, CB_GETCURSEL, 0, 0);
				if (options.nFormatIndex == CB_ERR)
					options.nFormatIndex = 0;

				return TRUE;
			}
		}
		return FALSE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_STICKY_WINDOW:
			if (BST_CHECKED == SendDlgItemMessage(hwndDlg, IDC_STICKY_WINDOW, BM_GETCHECK, 0, 0))
				EnableWindow(GetDlgItem(hwndDlg, IDC_BUTTON_STICK), TRUE);
			else
				EnableWindow(GetDlgItem(hwndDlg, IDC_BUTTON_STICK), FALSE);
			break;
#ifndef USE_RICH_EDIT
		case IDC_DEFAULT_PRINT:
			if (BST_CHECKED == SendDlgItemMessage(hwndDlg, IDC_DEFAULT_PRINT, BM_GETCHECK, 0, 0))
				EnableWindow(GetDlgItem(hwndDlg, IDC_SECONDARY_PRINT_FONT), FALSE);
			else
				EnableWindow(GetDlgItem(hwndDlg, IDC_SECONDARY_PRINT_FONT), TRUE);
			break;
#endif
		case IDC_BUTTON_STICK:
			SaveWindowPlacement(hwnd);
			MessageBox(hwndDlg, GetString(IDS_STICKY_MESSAGE), STR_METAPAD, MB_ICONINFORMATION);
			break;
		case IDC_BUTTON_CLEAR_FIND:
			if (MessageBox(hwndDlg, GetString(IDS_CLEAR_FIND_WARNING), STR_METAPAD, MB_ICONEXCLAMATION | MB_OKCANCEL) == IDOK) {
				ZeroMemory(FindArray, sizeof(FindArray));
				ZeroMemory(ReplaceArray, sizeof(ReplaceArray));
			}
			break;
		case IDC_BUTTON_CLEAR_RECENT:
			if (MessageBox(hwndDlg, GetString(IDS_CLEAR_RECENT_WARNING), STR_METAPAD, MB_ICONEXCLAMATION | MB_OKCANCEL) == IDOK) {
				HKEY key = NULL;
				TCHAR szKey[6];
				UINT i;

				if (!g_bIniMode && RegCreateKeyEx(HKEY_CURRENT_USER, STR_REGKEY, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &key, NULL) != ERROR_SUCCESS) {
					ReportLastError();
					break;
				}

				for (i = 1; i <= options.nMaxMRU; ++i) {
					wsprintf(szKey, _T("mru_%d"), i);
					SaveOption(key, szKey, REG_SZ, (LPBYTE)_T(""), 1);
				}

				if (key != NULL) {
					RegCloseKey(key);
				}

				PopulateMRUList();
			}
			break;
		}
		return FALSE;
	default:
		return FALSE;
	}
}

BOOL CALLBACK Advanced2PageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			SendDlgItemMessage(hwndDlg, IDC_EDIT_LANG_PLUGIN, EM_LIMITTEXT, (WPARAM)MAXFN-1, 0);
			SendDlgItemMessage(hwndDlg, IDC_MACRO_1, EM_LIMITTEXT, (WPARAM)MAXMACRO-1, 0);
			SendDlgItemMessage(hwndDlg, IDC_MACRO_2, EM_LIMITTEXT, (WPARAM)MAXMACRO-1, 0);
			SendDlgItemMessage(hwndDlg, IDC_MACRO_3, EM_LIMITTEXT, (WPARAM)MAXMACRO-1, 0);
			SendDlgItemMessage(hwndDlg, IDC_MACRO_4, EM_LIMITTEXT, (WPARAM)MAXMACRO-1, 0);
			SendDlgItemMessage(hwndDlg, IDC_MACRO_5, EM_LIMITTEXT, (WPARAM)MAXMACRO-1, 0);
			SendDlgItemMessage(hwndDlg, IDC_MACRO_6, EM_LIMITTEXT, (WPARAM)MAXMACRO-1, 0);
			SendDlgItemMessage(hwndDlg, IDC_MACRO_7, EM_LIMITTEXT, (WPARAM)MAXMACRO-1, 0);
			SendDlgItemMessage(hwndDlg, IDC_MACRO_8, EM_LIMITTEXT, (WPARAM)MAXMACRO-1, 0);
			SendDlgItemMessage(hwndDlg, IDC_MACRO_9, EM_LIMITTEXT, (WPARAM)MAXMACRO-1, 0);
			SendDlgItemMessage(hwndDlg, IDC_MACRO_10, EM_LIMITTEXT, (WPARAM)MAXMACRO-1, 0);
			SetDlgItemText(hwndDlg, IDC_EDIT_LANG_PLUGIN, options.szLangPlugin);

			SetDlgItemText(hwndDlg, IDC_MACRO_1, options.MacroArray[0]);
			SetDlgItemText(hwndDlg, IDC_MACRO_2, options.MacroArray[1]);
			SetDlgItemText(hwndDlg, IDC_MACRO_3, options.MacroArray[2]);
			SetDlgItemText(hwndDlg, IDC_MACRO_4, options.MacroArray[3]);
			SetDlgItemText(hwndDlg, IDC_MACRO_5, options.MacroArray[4]);
			SetDlgItemText(hwndDlg, IDC_MACRO_6, options.MacroArray[5]);
			SetDlgItemText(hwndDlg, IDC_MACRO_7, options.MacroArray[6]);
			SetDlgItemText(hwndDlg, IDC_MACRO_8, options.MacroArray[7]);
			SetDlgItemText(hwndDlg, IDC_MACRO_9, options.MacroArray[8]);
			SetDlgItemText(hwndDlg, IDC_MACRO_10, options.MacroArray[9]);

			if (options.szLangPlugin[0] == _T('\0')) {
				SendDlgItemMessage(hwndDlg, IDC_RADIO_LANG_DEFAULT, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
				SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_RADIO_LANG_DEFAULT, 0), 0);
			}
			else {
				SendDlgItemMessage(hwndDlg, IDC_RADIO_LANG_PLUGIN, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
				SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_RADIO_LANG_PLUGIN, 0), 0);
			}
		}
		return TRUE;
	case WM_NOTIFY:
		switch (((NMHDR FAR *)lParam)->code) {
		case PSN_KILLACTIVE:
			{
				if (BST_CHECKED == SendDlgItemMessage(hwndDlg, IDC_RADIO_LANG_PLUGIN, BM_GETCHECK, 0, 0)) {
					TCHAR szPlugin[MAXFN];
					GetDlgItemText(hwndDlg, IDC_EDIT_LANG_PLUGIN, szPlugin, MAXFN);
					if (szPlugin[0] == _T('\0')) {
						ERROROUT(GetString(IDS_SELECT_PLUGIN_WARNING));
						SetFocus(GetDlgItem(hwndDlg, IDC_EDIT_LANG_PLUGIN));
						SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, TRUE);
					}
				}
				return TRUE;
			}
			break;
		case PSN_APPLY:
			{
				GetDlgItemText(hwndDlg, IDC_MACRO_1, options.MacroArray[0], MAXMACRO);
				GetDlgItemText(hwndDlg, IDC_MACRO_2, options.MacroArray[1], MAXMACRO);
				GetDlgItemText(hwndDlg, IDC_MACRO_3, options.MacroArray[2], MAXMACRO);
				GetDlgItemText(hwndDlg, IDC_MACRO_4, options.MacroArray[3], MAXMACRO);
				GetDlgItemText(hwndDlg, IDC_MACRO_5, options.MacroArray[4], MAXMACRO);
				GetDlgItemText(hwndDlg, IDC_MACRO_6, options.MacroArray[5], MAXMACRO);
				GetDlgItemText(hwndDlg, IDC_MACRO_7, options.MacroArray[6], MAXMACRO);
				GetDlgItemText(hwndDlg, IDC_MACRO_8, options.MacroArray[7], MAXMACRO);
				GetDlgItemText(hwndDlg, IDC_MACRO_9, options.MacroArray[8], MAXMACRO);
				GetDlgItemText(hwndDlg, IDC_MACRO_10, options.MacroArray[9], MAXMACRO);

				if (BST_CHECKED == SendDlgItemMessage(hwndDlg, IDC_RADIO_LANG_DEFAULT, BM_GETCHECK, 0, 0))
					options.szLangPlugin[0] = _T('\0');
				else if (BST_CHECKED == SendDlgItemMessage(hwndDlg, IDC_RADIO_LANG_PLUGIN, BM_GETCHECK, 0, 0))
					GetDlgItemText(hwndDlg, IDC_EDIT_LANG_PLUGIN, options.szLangPlugin, MAXFN);
			}
			break;
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_RADIO_LANG_DEFAULT:
			EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT_LANG_PLUGIN), FALSE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_BUTTON_BROWSE), FALSE);
			break;
		case IDC_RADIO_LANG_PLUGIN:
			EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT_LANG_PLUGIN), TRUE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_BUTTON_BROWSE), TRUE);
			break;
		case IDC_BUTTON_BROWSE:
			{
				OPENFILENAME ofn;
				TCHAR szDefExt[] = _T("dll");
				TCHAR szFilter[] = _T("metapad language plugins (*.dll)\0*.dll\0All Files (*.*)\0*.*\0");
				TCHAR szResult[MAXFN];

				szResult[0] = _T('\0');

				ofn.lStructSize = sizeof(OPENFILENAME);
				ofn.hwndOwner = hwndDlg;
				ofn.lpstrFilter = szFilter;
				ofn.lpstrCustomFilter = (LPTSTR)NULL;
				ofn.nMaxCustFilter = 0L;
				ofn.nFilterIndex = 1L;
				ofn.lpstrFile = szResult;
				ofn.nMaxFile = sizeof(szResult);
				ofn.lpstrFileTitle = (LPTSTR)NULL;
				ofn.nMaxFileTitle = 0L;
				ofn.lpstrInitialDir = (LPTSTR) NULL;
				ofn.lpstrTitle = (LPTSTR)NULL;
				ofn.Flags = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
				ofn.nFileOffset = 0;
				ofn.nFileExtension = 0;
				ofn.lpstrDefExt = szDefExt;

				if (GetOpenFileName(&ofn)) {
					if (g_bDisablePluginVersionChecking) {
						SetDlgItemText(hwndDlg, IDC_EDIT_LANG_PLUGIN, szResult);
					}
					else {
						HINSTANCE hinstTemp = LoadAndVerifyLanguagePlugin(szResult);
						if (hinstTemp) {
							SetDlgItemText(hwndDlg, IDC_EDIT_LANG_PLUGIN, szResult);
							FreeLibrary(hinstTemp);
						}
						else {
							SendDlgItemMessage(hwndDlg, IDC_RADIO_LANG_DEFAULT, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
							SendDlgItemMessage(hwndDlg, IDC_RADIO_LANG_PLUGIN, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
							SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_RADIO_LANG_DEFAULT, 0), 0);
						}
					}
				}
			}
		}
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

BOOL CALLBACK ViewPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static LOGFONT TmpPrimaryFont, TmpSecondaryFont;
	static HFONT hfont = NULL, hfont2 = NULL;
	static COLORREF CustomClrs[16], TmpBackColour, TmpFontColour, TmpBackColour2, TmpFontColour2;
	static HBRUSH hbackbrush, hfontbrush;
	static HBRUSH hbackbrush2, hfontbrush2;

	switch (uMsg) {
	case WM_INITDIALOG:
		{
			TCHAR szInt[5];

			TmpPrimaryFont = options.PrimaryFont;
			TmpSecondaryFont = options.SecondaryFont;
			TmpBackColour = options.BackColour;
			TmpFontColour = options.FontColour;
			TmpBackColour2 = options.BackColour2;
			TmpFontColour2 = options.FontColour2;

			hbackbrush = CreateSolidBrush(TmpBackColour);
			hfontbrush = CreateSolidBrush(TmpFontColour);
			hbackbrush2 = CreateSolidBrush(TmpBackColour2);
			hfontbrush2 = CreateSolidBrush(TmpFontColour2);

			if (options.nPrimaryFont == 0)
				SendDlgItemMessage(hwndDlg, IDC_FONT_PRIMARY, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
			else
				SendDlgItemMessage(hwndDlg, IDC_FONT_PRIMARY_2, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);

			if (options.nSecondaryFont == 0)
				SendDlgItemMessage(hwndDlg, IDC_FONT_SECONDARY, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
			else
				SendDlgItemMessage(hwndDlg, IDC_FONT_SECONDARY_2, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);

			/*
			wsprintf(szInt, _T("%d"), options.nStatusFontWidth);
			SetDlgItemText(hwndDlg, IDC_EDIT_SB_FONT_WIDTH, szInt);
			*/

			wsprintf(szInt, _T("%d"), options.nSelectionMarginWidth);
			SetDlgItemText(hwndDlg, IDC_EDIT_SM_WIDTH, szInt);

			if (SetLWA) {
				wsprintf(szInt, _T("%d"), options.nTransparentPct);
				SetDlgItemText(hwndDlg, IDC_EDIT_TRANSPARENT, szInt);
			}
			else {
				EnableWindow(GetDlgItem(hwndDlg, IDC_STAT_TRANS), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT_TRANSPARENT), FALSE);
			}

			SendDlgItemMessage(hwndDlg, IDC_FLAT_TOOLBAR, BM_SETCHECK, (WPARAM) options.bUnFlatToolbar, 0);
			SendDlgItemMessage(hwndDlg, IDC_SYSTEM_COLOURS, BM_SETCHECK, (WPARAM) options.bSystemColours, 0);
			SendDlgItemMessage(hwndDlg, IDC_SYSTEM_COLOURS2, BM_SETCHECK, (WPARAM) options.bSystemColours2, 0);
			SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_SYSTEM_COLOURS, 0), 0);
			SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_SYSTEM_COLOURS2, 0), 0);

			hfont = CreateFontIndirect(&TmpPrimaryFont);
			SendDlgItemMessage(hwndDlg, IDC_BTN_FONT1, WM_SETFONT, (WPARAM)hfont, 0);
			hfont2 = CreateFontIndirect(&TmpSecondaryFont);
			SendDlgItemMessage(hwndDlg, IDC_BTN_FONT2, WM_SETFONT, (WPARAM)hfont2, 0);
		}
		return TRUE;
	case WM_NOTIFY:
		switch (((NMHDR FAR *)lParam)->code) {
		case PSN_KILLACTIVE:
			{
				TCHAR szInt[10];
				INT nTmp;

				/*
				GetDlgItemText(hwndDlg, IDC_EDIT_SB_FONT_WIDTH, szInt, 10);
				nTmp = _ttoi(szInt);
				if (nTmp < 1 || nTmp > 1000) {
					ERROROUT(_T("Enter a font size between 1 and 1000"));
					SetFocus(GetDlgItem(hwndDlg, IDC_EDIT_SB_FONT_WIDTH));
					SetWindowLong (hwndDlg, DWLP_MSGRESULT, TRUE);
				}
				*/

				GetDlgItemText(hwndDlg, IDC_EDIT_SM_WIDTH, szInt, 10);
				nTmp = _ttoi(szInt);
				if (nTmp < 0 || nTmp > 300) {
					ERROROUT(GetString(IDS_MARGIN_WIDTH_WARNING));
					SetFocus(GetDlgItem(hwndDlg, IDC_EDIT_SM_WIDTH));
					SetWindowLong (hwndDlg, DWLP_MSGRESULT, TRUE);
				}


				GetDlgItemText(hwndDlg, IDC_EDIT_TRANSPARENT, szInt, 10);
				nTmp = _ttoi(szInt);
				if (SetLWA && (nTmp < 1 || nTmp > 99)) {
					ERROROUT(GetString(IDS_TRANSPARENCY_WARNING));
					SetFocus(GetDlgItem(hwndDlg, IDC_EDIT_TRANSPARENT));
					SetWindowLong (hwndDlg, DWLP_MSGRESULT, TRUE);
				}

				return TRUE;
			}
		case PSN_APPLY:
			{

				TCHAR szInt[5];
				INT nTmp;
/*
				GetDlgItemText(hwndDlg, IDC_EDIT_SB_FONT_WIDTH, szInt, 5);
				nTmp = _ttoi(szInt);
				options.nStatusFontWidth = nTmp;
*/
				GetDlgItemText(hwndDlg, IDC_EDIT_SM_WIDTH, szInt, 5);
				nTmp = _ttoi(szInt);
				options.nSelectionMarginWidth = nTmp;

				GetDlgItemText(hwndDlg, IDC_EDIT_TRANSPARENT, szInt, 5);
				nTmp = _ttoi(szInt);
				options.nTransparentPct = nTmp;

				if (BST_CHECKED == SendDlgItemMessage(hwndDlg, IDC_FONT_PRIMARY, BM_GETCHECK, 0, 0))
					options.nPrimaryFont = 0;
				else
					options.nPrimaryFont = 1;

				if (BST_CHECKED == SendDlgItemMessage(hwndDlg, IDC_FONT_SECONDARY, BM_GETCHECK, 0, 0))
					options.nSecondaryFont = 0;
				else
					options.nSecondaryFont = 1;

				if (BST_CHECKED == SendDlgItemMessage(hwndDlg, IDC_SYSTEM_COLOURS, BM_GETCHECK, 0, 0))
					options.bSystemColours = TRUE;
				else
					options.bSystemColours = FALSE;

				if (BST_CHECKED == SendDlgItemMessage(hwndDlg, IDC_SYSTEM_COLOURS2, BM_GETCHECK, 0, 0))
					options.bSystemColours2 = TRUE;
				else
					options.bSystemColours2 = FALSE;

				options.bUnFlatToolbar = (BST_CHECKED == SendDlgItemMessage(hwndDlg, IDC_FLAT_TOOLBAR, BM_GETCHECK, 0, 0));
				options.PrimaryFont = TmpPrimaryFont;
				options.SecondaryFont = TmpSecondaryFont;
				options.BackColour = TmpBackColour;
				options.FontColour = TmpFontColour;
				options.BackColour2 = TmpBackColour2;
				options.FontColour2 = TmpFontColour2;
			}
		case PSN_RESET:
			if (hfont)
				DeleteObject(hfont);
			if (hfont2)
				DeleteObject(hfont2);
			if (hbackbrush)
				DeleteObject(hbackbrush);
			if (hfontbrush)
				DeleteObject(hfontbrush);
			if (hbackbrush2)
				DeleteObject(hbackbrush2);
			if (hfontbrush2)
				DeleteObject(hfontbrush2);
			break;
		}
		break;
	case WM_CTLCOLORBTN:
		if (GetDlgItem(hwndDlg, IDC_COLOUR_BACK) == (HWND)lParam) {
			SetBkColor((HDC)wParam, TmpBackColour);
			return (BOOL)(!(!hbackbrush));
		}
		if (GetDlgItem(hwndDlg, IDC_COLOUR_FONT) == (HWND)lParam) {
			SetBkColor((HDC)wParam, TmpFontColour);
			return (BOOL)(!(!hfontbrush));
		}
		if (GetDlgItem(hwndDlg, IDC_COLOUR_BACK2) == (HWND)lParam) {
			SetBkColor((HDC)wParam, TmpBackColour2);
			return (BOOL)(!(!hbackbrush2));
		}
		if (GetDlgItem(hwndDlg, IDC_COLOUR_FONT2) == (HWND)lParam) {
			SetBkColor((HDC)wParam, TmpFontColour2);
			return (BOOL)(!(!hfontbrush2));
		}
		return (BOOL)(!(!NULL));
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_COLOUR_BACK:
		case IDC_COLOUR_FONT:
		case IDC_COLOUR_BACK2:
		case IDC_COLOUR_FONT2:
			{
				CHOOSECOLOR cc;

				ZeroMemory(&cc, sizeof(CHOOSECOLOR));
				cc.lStructSize = sizeof(CHOOSECOLOR);
				cc.hwndOwner = hwndDlg;
				cc.lpCustColors = (LPDWORD) CustomClrs;
				cc.Flags = CC_FULLOPEN | CC_RGBINIT;
				switch (LOWORD(wParam)) {
				case IDC_COLOUR_BACK:
					cc.rgbResult = TmpBackColour;
					if (ChooseColor(&cc)) {
						TmpBackColour = cc.rgbResult;
						if (hbackbrush)
							DeleteObject(hbackbrush);
						hbackbrush = CreateSolidBrush(TmpBackColour);
						InvalidateRect(GetDlgItem(hwndDlg, IDC_COLOUR_BACK), NULL, TRUE);
					}
					break;
				case IDC_COLOUR_FONT:
					cc.rgbResult = TmpFontColour;
					if (ChooseColor(&cc)) {
						TmpFontColour = cc.rgbResult;
						if (hfontbrush)
							DeleteObject(hfontbrush);
						hfontbrush = CreateSolidBrush(TmpFontColour);
						InvalidateRect(GetDlgItem(hwndDlg, IDC_COLOUR_FONT), NULL, TRUE);
					}
					break;
				case IDC_COLOUR_BACK2:
					cc.rgbResult = TmpBackColour2;
					if (ChooseColor(&cc)) {
						TmpBackColour2 = cc.rgbResult;
						if (hbackbrush2)
							DeleteObject(hbackbrush2);
						hbackbrush2 = CreateSolidBrush(TmpBackColour2);
						InvalidateRect(GetDlgItem(hwndDlg, IDC_COLOUR_BACK2), NULL, TRUE);
					}
					break;
				case IDC_COLOUR_FONT2:
					cc.rgbResult = TmpFontColour2;
					if (ChooseColor(&cc)) {
						TmpFontColour2 = cc.rgbResult;
						if (hfontbrush2)
							DeleteObject(hfontbrush2);
						hfontbrush2 = CreateSolidBrush(TmpFontColour2);
						InvalidateRect(GetDlgItem(hwndDlg, IDC_COLOUR_FONT2), NULL, TRUE);
					}
					break;
				}
			}
			return TRUE;
		case IDC_BTN_FONT1:
			{
				CHOOSEFONT cf;

				ZeroMemory(&cf, sizeof(CHOOSEFONT));
				cf.lStructSize = sizeof (CHOOSEFONT);
				cf.hwndOwner = hwndDlg;
				cf.lpLogFont = &TmpPrimaryFont;
				cf.Flags = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT;
				if (ChooseFont(&cf)) {
					if (hfont)
						DeleteObject(hfont);

					hfont = CreateFontIndirect(&TmpPrimaryFont);
					SendDlgItemMessage(hwndDlg, IDC_BTN_FONT1, WM_SETFONT, (WPARAM)hfont, 0);
					SendDlgItemMessage(hwndDlg, IDC_FONT_PRIMARY, BM_SETCHECK, (WPARAM)FALSE, 0);
					SendDlgItemMessage(hwndDlg, IDC_FONT_PRIMARY_2, BM_SETCHECK, (WPARAM)TRUE, 0);
				}
				InvalidateRect(GetDlgItem(hwndDlg, IDC_BTN_FONT1), NULL, TRUE);
			}
			break;
		case IDC_BTN_FONT2:
			{
				CHOOSEFONT cf;

				ZeroMemory(&cf, sizeof(CHOOSEFONT));
				cf.lStructSize = sizeof(CHOOSEFONT);
				cf.hwndOwner = hwndDlg;
				cf.lpLogFont = &TmpSecondaryFont;
				cf.Flags = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT;
				if (ChooseFont(&cf)) {
					if (hfont2)
						DeleteObject(hfont2);
					hfont2 = CreateFontIndirect(&TmpSecondaryFont);
					SendDlgItemMessage(hwndDlg, IDC_BTN_FONT2, WM_SETFONT, (WPARAM)hfont2, 0);
					SendDlgItemMessage(hwndDlg, IDC_FONT_SECONDARY, BM_SETCHECK, (WPARAM)FALSE, 0);
					SendDlgItemMessage(hwndDlg, IDC_FONT_SECONDARY_2, BM_SETCHECK, (WPARAM)TRUE, 0);
				}
				InvalidateRect(GetDlgItem(hwndDlg, IDC_BTN_FONT2), NULL, TRUE);
			}
			break;
		case IDC_SYSTEM_COLOURS:
			{
				BOOL bEnable;
				if (BST_CHECKED == SendDlgItemMessage(hwndDlg, IDC_SYSTEM_COLOURS, BM_GETCHECK, 0, 0)) {
					if (hfontbrush)
						DeleteObject(hfontbrush);
					hfontbrush = GetSysColorBrush(COLOR_WINDOWTEXT);
					TmpFontColour = GetSysColor(COLOR_WINDOWTEXT);

					if (hbackbrush)
						DeleteObject(hbackbrush);
					hbackbrush = GetSysColorBrush(COLOR_WINDOW);
					TmpBackColour = GetSysColor(COLOR_WINDOW);

					InvalidateRect(GetDlgItem(hwndDlg, IDC_COLOUR_BACK), NULL, TRUE);
					InvalidateRect(GetDlgItem(hwndDlg, IDC_COLOUR_FONT), NULL, TRUE);
					bEnable = FALSE;
				}
				else
					bEnable = TRUE;
				EnableWindow(GetDlgItem(hwndDlg, IDC_STAT_FONT), bEnable);
				EnableWindow(GetDlgItem(hwndDlg, IDC_STAT_WIND), bEnable);
				EnableWindow(GetDlgItem(hwndDlg, IDC_COLOUR_BACK), bEnable);
				EnableWindow(GetDlgItem(hwndDlg, IDC_COLOUR_FONT), bEnable);
			}
			return FALSE;
		case IDC_SYSTEM_COLOURS2:
			{
				BOOL bEnable;
				if (BST_CHECKED == SendDlgItemMessage(hwndDlg, IDC_SYSTEM_COLOURS2, BM_GETCHECK, 0, 0)) {
					if (hfontbrush2)
						DeleteObject(hfontbrush2);
					hfontbrush2 = GetSysColorBrush(COLOR_WINDOWTEXT);
					TmpFontColour2 = GetSysColor(COLOR_WINDOWTEXT);

					if (hbackbrush2)
						DeleteObject(hbackbrush2);
					hbackbrush2 = GetSysColorBrush(COLOR_WINDOW);
					TmpBackColour2 = GetSysColor(COLOR_WINDOW);

					InvalidateRect(GetDlgItem(hwndDlg, IDC_COLOUR_BACK2), NULL, TRUE);
					InvalidateRect(GetDlgItem(hwndDlg, IDC_COLOUR_FONT2), NULL, TRUE);
					bEnable = FALSE;
				}
				else
					bEnable = TRUE;
				EnableWindow(GetDlgItem(hwndDlg, IDC_STAT_FONT2), bEnable);
				EnableWindow(GetDlgItem(hwndDlg, IDC_STAT_WIND2), bEnable);
				EnableWindow(GetDlgItem(hwndDlg, IDC_COLOUR_BACK2), bEnable);
				EnableWindow(GetDlgItem(hwndDlg, IDC_COLOUR_FONT2), bEnable);
			}
			return FALSE;
		}
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

BOOL CALLBACK GeneralPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			TCHAR szInt[5];

			CenterWindow(hwndSheet);

			SendDlgItemMessage(hwndDlg, IDC_EDIT_BROWSER, EM_LIMITTEXT, (WPARAM)MAXFN-1, 0);
			SendDlgItemMessage(hwndDlg, IDC_EDIT_ARGS, EM_LIMITTEXT, (WPARAM)MAXARGS-1, 0);
			SendDlgItemMessage(hwndDlg, IDC_EDIT_QUOTE, EM_LIMITTEXT, (WPARAM)MAXQUOTE-1, 0);
			SetDlgItemText(hwndDlg, IDC_EDIT_BROWSER, options.szBrowser);
			SetDlgItemText(hwndDlg, IDC_EDIT_ARGS, options.szArgs);
			SetDlgItemText(hwndDlg, IDC_EDIT_BROWSER2, options.szBrowser2);
			SetDlgItemText(hwndDlg, IDC_EDIT_ARGS2, options.szArgs2);
			SetDlgItemText(hwndDlg, IDC_EDIT_QUOTE, options.szQuote);
			SendDlgItemMessage(hwndDlg, IDC_CHECK_QUICKEXIT, BM_SETCHECK, (WPARAM) options.bQuickExit, 0);
			SendDlgItemMessage(hwndDlg, IDC_CHECK_SAVEMENUSETTINGS, BM_SETCHECK, (WPARAM) options.bSaveMenuSettings, 0);
			SendDlgItemMessage(hwndDlg, IDC_CHECK_SAVEWINDOWPLACEMENT, BM_SETCHECK, (WPARAM) options.bSaveWindowPlacement, 0);
			SendDlgItemMessage(hwndDlg, IDC_CHECK_SAVEDIRECTORY, BM_SETCHECK, (WPARAM) options.bSaveDirectory, 0);
			SendDlgItemMessage(hwndDlg, IDC_CHECK_LAUNCH_CLOSE, BM_SETCHECK, (WPARAM) options.bLaunchClose, 0);
			SendDlgItemMessage(hwndDlg, IDC_FIND_AUTO_WRAP, BM_SETCHECK, (WPARAM)options.bFindAutoWrap, 0);
			SendDlgItemMessage(hwndDlg, IDC_AUTO_INDENT, BM_SETCHECK, (WPARAM)options.bAutoIndent, 0);
			SendDlgItemMessage(hwndDlg, IDC_INSERT_SPACES, BM_SETCHECK, (WPARAM)options.bInsertSpaces, 0);
			SendDlgItemMessage(hwndDlg, IDC_NO_CAPTION_DIR, BM_SETCHECK, (WPARAM)options.bNoCaptionDir, 0);

			wsprintf(szInt, _T("%d"), options.nTabStops);
			SetDlgItemText(hwndDlg, IDC_TAB_STOP, szInt);

			if (options.nLaunchSave == 0)
				SendDlgItemMessage(hwndDlg, IDC_LAUNCH_SAVE0, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
			else if (options.nLaunchSave == 1)
				SendDlgItemMessage(hwndDlg, IDC_LAUNCH_SAVE1, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
			else
				SendDlgItemMessage(hwndDlg, IDC_LAUNCH_SAVE2, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
		}
		return TRUE;
	case WM_NOTIFY:
		switch (((NMHDR FAR *)lParam)->code) {
		case PSN_KILLACTIVE:
			{
				TCHAR szInt[5];
				int nTmp;

				GetDlgItemText(hwndDlg, IDC_TAB_STOP, szInt, 5);
				nTmp = _ttoi(szInt);
				if (nTmp < 1 || nTmp > 100) {
					ERROROUT(GetString(IDS_TAB_SIZE_WARNING));
					SetFocus(GetDlgItem(hwndDlg, IDC_TAB_STOP));
					SetWindowLong (hwndDlg, DWLP_MSGRESULT, TRUE);
				}
				return TRUE;
			}
			break;
		case PSN_APPLY:
			{
				int nTmp;
				TCHAR szInt[5];

				GetDlgItemText(hwndDlg, IDC_TAB_STOP, szInt, 5);
				nTmp = _ttoi(szInt);
				options.nTabStops = nTmp;

				GetDlgItemText(hwndDlg, IDC_EDIT_BROWSER, options.szBrowser, MAXFN);
				GetDlgItemText(hwndDlg, IDC_EDIT_ARGS, options.szArgs, MAXARGS);
				GetDlgItemText(hwndDlg, IDC_EDIT_BROWSER2, options.szBrowser2, MAXFN);
				GetDlgItemText(hwndDlg, IDC_EDIT_ARGS2, options.szArgs2, MAXARGS);
				GetDlgItemText(hwndDlg, IDC_EDIT_QUOTE, options.szQuote, MAXQUOTE);

				if (BST_CHECKED == SendDlgItemMessage(hwndDlg, IDC_LAUNCH_SAVE0, BM_GETCHECK, 0, 0))
					options.nLaunchSave = 0;
				else if (BST_CHECKED == SendDlgItemMessage(hwndDlg, IDC_LAUNCH_SAVE1, BM_GETCHECK, 0, 0))
					options.nLaunchSave = 1;
				else
					options.nLaunchSave = 2;

				options.bNoCaptionDir = (BST_CHECKED == SendDlgItemMessage(hwndDlg, IDC_NO_CAPTION_DIR, BM_GETCHECK, 0, 0));
				options.bAutoIndent = (BST_CHECKED == SendDlgItemMessage(hwndDlg, IDC_AUTO_INDENT, BM_GETCHECK, 0, 0));
				options.bInsertSpaces = (BST_CHECKED == SendDlgItemMessage(hwndDlg, IDC_INSERT_SPACES, BM_GETCHECK, 0, 0));
				options.bFindAutoWrap = (BST_CHECKED == SendDlgItemMessage(hwndDlg, IDC_FIND_AUTO_WRAP, BM_GETCHECK, 0, 0));
				options.bLaunchClose = (BST_CHECKED == SendDlgItemMessage(hwndDlg, IDC_CHECK_LAUNCH_CLOSE, BM_GETCHECK, 0, 0));
				options.bQuickExit = (BST_CHECKED == SendDlgItemMessage(hwndDlg, IDC_CHECK_QUICKEXIT, BM_GETCHECK, 0, 0));
				options.bSaveWindowPlacement = (BST_CHECKED == SendDlgItemMessage(hwndDlg, IDC_CHECK_SAVEWINDOWPLACEMENT, BM_GETCHECK, 0, 0));
				options.bSaveMenuSettings = (BST_CHECKED == SendDlgItemMessage(hwndDlg, IDC_CHECK_SAVEMENUSETTINGS, BM_GETCHECK, 0, 0));
				options.bSaveDirectory = (BST_CHECKED == SendDlgItemMessage(hwndDlg, IDC_CHECK_SAVEDIRECTORY, BM_GETCHECK, 0, 0));
			}
			break;
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_BUTTON_BROWSE:
		case IDC_BUTTON_BROWSE2:
			{
				OPENFILENAME ofn;
				TCHAR szDefExt[] = _T("exe");
				TCHAR szFilter[] = _T("Executable Files (*.exe)\0*.exe\0All Files (*.*)\0*.*\0");
				TCHAR szResult[MAXFN];

				szResult[0] = _T('\0');

				ofn.lStructSize = sizeof(OPENFILENAME);
				ofn.hwndOwner = hwndDlg;
				ofn.lpstrFilter = szFilter;
				ofn.lpstrCustomFilter = (LPTSTR)NULL;
				ofn.nMaxCustFilter = 0L;
				ofn.nFilterIndex = 1L;
				ofn.lpstrFile = szResult;
				ofn.nMaxFile = sizeof(szResult);
				ofn.lpstrFileTitle = (LPTSTR)NULL;
				ofn.nMaxFileTitle = 0L;
				ofn.lpstrInitialDir = (LPTSTR) NULL;
				ofn.lpstrTitle = (LPTSTR)NULL;
				ofn.Flags = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
				ofn.nFileOffset = 0;
				ofn.nFileExtension = 0;
				ofn.lpstrDefExt = szDefExt;

				if (GetOpenFileName(&ofn)) {
					if (LOWORD(wParam) == IDC_BUTTON_BROWSE) {
						SetDlgItemText(hwndDlg, IDC_EDIT_BROWSER, szResult);
					}
					else {
						SetDlgItemText(hwndDlg, IDC_EDIT_BROWSER2, szResult);
					}
				}
			}
		}
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

int CALLBACK SheetInitProc(HWND hwndDlg, UINT uMsg, LPARAM lParam)
{
	if (uMsg == PSCB_PRECREATE) {
		if (((LPDLGTEMPLATEEX)lParam)->signature == 0xFFFF) {
			((LPDLGTEMPLATEEX)lParam)->style &= ~DS_CONTEXTHELP;
		}
		else {
			((LPDLGTEMPLATE)lParam)->style &= ~DS_CONTEXTHELP;
		}
	}
	else if (hwndDlg) {
		hwndSheet = hwndDlg;
	}
	return TRUE;
}

LRESULT WINAPI MainWndProc(HWND hwndMain, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	if (Msg == WM_COMMAND && ID_FAV_RANGE_BASE < LOWORD(wParam) && LOWORD(wParam) <= ID_FAV_RANGE_MAX) {
		LoadFileFromMenu(LOWORD(wParam), FALSE);
		return FALSE;
	}

	switch(Msg) {
//	case WM_ERASEBKGND:
//		return (LRESULT)1;
	case WM_DESTROY:
		if (options.bSaveWindowPlacement && !options.bStickyWindow)
			SaveWindowPlacement(hwndMain);
		SaveMenusAndData();
		CleanUp();
		PostQuitMessage(0);
		break;
	case WM_ACTIVATE:
		UpdateStatus();
		SetFocus(client);
		break;
	case WM_DROPFILES:
		{
			HDROP hDrop = (HDROP) wParam;

			if (!SaveIfDirty())
				break;

			DragQueryFile(hDrop, 0, szFile, MAXFN);
			DragFinish(hDrop);

			bLoading = TRUE;
			bHideMessage = FALSE;
			lstrcpy(szStatusMessage, GetString(IDS_FILE_LOADING));
			UpdateStatus();
			LoadFile(szFile, FALSE, TRUE);
			if (bLoading) {
				bLoading = FALSE;
				bDirtyFile = FALSE;
				UpdateCaption();
			}
			else {
				MakeNewFile();
			}
			break;
		}
	case WM_SIZING:
		InvalidateRect(client, NULL, FALSE); // ML: for decreasing window size, update scroll bar
		break;
	case WM_SIZE: {
		if (client == NULL) break;
		SetWindowPos(client, 0, 0, GetToolbarHeight(), LOWORD(lParam), HIWORD(lParam) - GetStatusHeight() - GetToolbarHeight(), SWP_SHOWWINDOW);
		if (bWordWrap) {
			SetWindowLongPtr(client, GWL_STYLE, GetWindowLongPtr(client, GWL_STYLE) & ~WS_HSCROLL);
			SetWindowPos(client, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
			UpdateStatus();
		}
		if (bShowStatus) {
			SendMessage(status, WM_SIZE, 0, 0);
			InvalidateRect(status, NULL, TRUE);
		}

		if (bShowToolbar)
			SendMessage(toolbar, WM_SIZE, 0, 0);

		break;
	}
#ifndef USE_RICH_EDIT
	case WM_CTLCOLOREDIT:
		if ((HWND)lParam == client) {
			/*
			if (!bPrimaryFont && options.bSecondarySystemColour) {
				SetBkColor((HDC)wParam, GetSysColor(COLOR_WINDOW));
				SetTextColor((HDC)wParam, GetSysColor(COLOR_WINDOWTEXT));
				if (BackBrush)
					DeleteObject(BackBrush);
				BackBrush = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
			}
			*/
			SetBkColor((HDC)wParam, bPrimaryFont ? options.BackColour : options.BackColour2);
			SetTextColor((HDC)wParam, bPrimaryFont ? options.FontColour : options.FontColour2);
			if (BackBrush)
				DeleteObject(BackBrush);
			BackBrush = CreateSolidBrush(bPrimaryFont ? options.BackColour : options.BackColour2);
			return (LONG_PTR)BackBrush;
		}
		break;
#endif
	case WM_CLOSE:
		if (!SaveIfDirty())
			break;
		DestroyWindow(hwndMain);
		break;
	case WM_QUERYENDSESSION:
		if (!SaveIfDirty())
			return FALSE;
		else
			return TRUE;
	case WM_INITMENUPOPUP: {
		HMENU hmenuPopup = (HMENU) wParam;
		INT nPos = LOWORD(lParam);
		if ((BOOL)HIWORD(lParam) == TRUE)
			break;

		if (nPos == EDITPOS) {
			CHARRANGE cr;
			if (IsClipboardFormatAvailable(_CF_TEXT))
				EnableMenuItem(hmenuPopup, ID_MYEDIT_PASTE, MF_BYCOMMAND | MF_ENABLED);
			else
				EnableMenuItem(hmenuPopup, ID_MYEDIT_PASTE, MF_BYCOMMAND | MF_GRAYED);
/*
#ifdef USE_RICH_EDIT
			if (SendMessage(client, EM_CANPASTE, 0, 0))
				EnableMenuItem(hmenuPopup, ID_MYEDIT_PASTE, MF_BYCOMMAND | MF_ENABLED);
			else
				EnableMenuItem(hmenuPopup, ID_MYEDIT_PASTE, MF_BYCOMMAND | MF_GRAYED);
#endif
*/
			if (bWordWrap)
				EnableMenuItem(hmenuPopup, ID_COMMIT_WORDWRAP, MF_BYCOMMAND | MF_ENABLED);
			else
				EnableMenuItem(hmenuPopup, ID_COMMIT_WORDWRAP, MF_BYCOMMAND | MF_GRAYED);

#ifdef USE_RICH_EDIT
			SendMessage(client, EM_EXGETSEL, 0, (LPARAM)&cr);
#else
			SendMessage(client, EM_GETSEL, (WPARAM)&cr.cpMin, (LPARAM)&cr.cpMax);
#endif
			if (cr.cpMin == cr.cpMax) {
				EnableMenuItem(hmenuPopup, ID_MYEDIT_CUT, MF_BYCOMMAND | MF_GRAYED);
				EnableMenuItem(hmenuPopup, ID_MYEDIT_COPY, MF_BYCOMMAND | MF_GRAYED);
				EnableMenuItem(hmenuPopup, ID_MYEDIT_DELETE, MF_BYCOMMAND | MF_GRAYED);
			}
			else {
				EnableMenuItem(hmenuPopup, ID_MYEDIT_CUT, MF_BYCOMMAND | MF_ENABLED);
				EnableMenuItem(hmenuPopup, ID_MYEDIT_COPY, MF_BYCOMMAND | MF_ENABLED);
				EnableMenuItem(hmenuPopup, ID_MYEDIT_DELETE, MF_BYCOMMAND | MF_ENABLED);
			}
			if (SendMessage(client, EM_CANUNDO, 0, 0))
				EnableMenuItem(hmenuPopup, ID_MYEDIT_UNDO, MF_BYCOMMAND | MF_ENABLED);
			else
				EnableMenuItem(hmenuPopup, ID_MYEDIT_UNDO, MF_BYCOMMAND | MF_GRAYED);
#ifdef USE_RICH_EDIT
			if (SendMessage(client, EM_CANREDO, 0, 0))
				EnableMenuItem(hmenuPopup, ID_MYEDIT_REDO, MF_BYCOMMAND | MF_ENABLED);
			else
				EnableMenuItem(hmenuPopup, ID_MYEDIT_REDO, MF_BYCOMMAND | MF_GRAYED);
#endif
		}
		else if (nPos == 0) {
			if (szFile[0] != _T('\0')) {
				EnableMenuItem(hmenuPopup, ID_RELOAD_CURRENT, MF_BYCOMMAND | MF_ENABLED);
				EnableMenuItem(hmenuPopup, ID_READONLY, MF_BYCOMMAND | MF_ENABLED);
			}
			else {
				EnableMenuItem(hmenuPopup, ID_RELOAD_CURRENT, MF_BYCOMMAND | MF_GRAYED);
				EnableMenuItem(hmenuPopup, ID_READONLY, MF_BYCOMMAND | MF_GRAYED);
			}
		}
		else if (!options.bNoFaves && nPos == FAVEPOS) {
			if (szFile[0] != _T('\0')) {
				EnableMenuItem(hmenuPopup, ID_FAV_ADD, MF_BYCOMMAND | MF_ENABLED);
			}
			else {
				EnableMenuItem(hmenuPopup, ID_FAV_ADD, MF_BYCOMMAND | MF_GRAYED);
			}
			if (bHasFaves) {
				EnableMenuItem(hmenuPopup, ID_FAV_EDIT, MF_BYCOMMAND | MF_ENABLED);
			}
			else {
				EnableMenuItem(hmenuPopup, ID_FAV_EDIT, MF_BYCOMMAND | MF_GRAYED);
			}

		}
		break;
	}
	case WM_NOTIFY: {
		switch (((LPNMHDR)lParam)->code) {
			case TTN_NEEDTEXT: {
				LPTOOLTIPTEXT lpttt = (LPTOOLTIPTEXT)lParam;
				switch (lpttt->hdr.idFrom) {
					/*
					case ID_NEW_INSTANCE:
						lpttt->lpszText = GetString(IDS_NEW_INSTANCE);
						break;
					*/
					case ID_MYFILE_NEW:
						lpttt->lpszText = GetString(IDS_TB_NEWFILE);
						break;
					case ID_MYFILE_OPEN:
						lpttt->lpszText = GetString(IDS_TB_OPENFILE);
						break;
					case ID_MYFILE_SAVE:
						lpttt->lpszText = GetString(IDS_TB_SAVEFILE);
						break;
					case ID_PRINT:
						lpttt->lpszText = GetString(IDS_TB_PRINT);
						break;
					case ID_FIND:
						lpttt->lpszText = GetString(IDS_TB_FIND);
						break;
					case ID_REPLACE:
						lpttt->lpszText = GetString(IDS_TB_REPLACE);
						break;
					case ID_MYEDIT_CUT:
						lpttt->lpszText = GetString(IDS_TB_CUT);
						break;
					case ID_MYEDIT_COPY:
						lpttt->lpszText = GetString(IDS_TB_COPY);
						break;
					case ID_MYEDIT_PASTE:
						lpttt->lpszText = GetString(IDS_TB_PASTE);
						break;
					case ID_MYEDIT_UNDO:
						lpttt->lpszText = GetString(IDS_TB_UNDO);
						break;
#ifdef USE_RICH_EDIT
					case ID_MYEDIT_REDO:
						lpttt->lpszText = GetString(IDS_TB_REDO);
						break;
#endif
					case ID_VIEW_OPTIONS:
						lpttt->lpszText = GetString(IDS_TB_SETTINGS);
						break;
					case ID_RELOAD_CURRENT:
						lpttt->lpszText = GetString(IDS_TB_REFRESH);
						break;
					case ID_EDIT_WORDWRAP:
						lpttt->lpszText = GetString(IDS_TB_WORDWRAP);
						break;
					case ID_FONT_PRIMARY:
						lpttt->lpszText = GetString(IDS_TB_PRIMARYFONT);
						break;
					case ID_ALWAYSONTOP:
						lpttt->lpszText = GetString(IDS_TB_ONTOP);
						break;
					case ID_FILE_LAUNCHVIEWER:
						lpttt->lpszText = GetString(IDS_TB_PRIMARYVIEWER);
						break;
					case ID_LAUNCH_SECONDARY_VIEWER:
						lpttt->lpszText = GetString(IDS_TB_SECONDARYVIEWER);
						break;
				}
				break;
			}
#ifdef USE_RICH_EDIT
			case EN_LINK: {
				ENLINK* pLink = (ENLINK *) lParam;
				switch (pLink->msg) {
					case WM_SETCURSOR: {
						HCURSOR hcur;
						if (options.bLinkDoubleClick)
							hcur = LoadCursor(NULL, IDC_ARROW);
						else
							hcur = LoadCursor(hinstThis, MAKEINTRESOURCE(IDC_MYHAND));
						SetCursor(hcur);
						return (LRESULT)1;
					}
/*
					case WM_RBUTTONDOWN:
						bLinkMenu = TRUE;
						break;
					case WM_RBUTTONUP:
						bLinkMenu = FALSE;
						break;
*/
					case WM_LBUTTONUP:
						if (options.bLinkDoubleClick) break;
					case WM_LBUTTONDBLCLK: {
						TEXTRANGE tr;

						tr.chrg.cpMin = pLink->chrg.cpMin;
						tr.chrg.cpMax = pLink->chrg.cpMax;
						tr.lpstrText = (LPTSTR) HeapAlloc(globalHeap, HEAP_ZERO_MEMORY, (pLink->chrg.cpMax - pLink->chrg.cpMin + 1) * sizeof(TCHAR));

						SendMessage(client, EM_GETTEXTRANGE, 0, (LPARAM)&tr);

						ShellExecute(NULL, NULL, tr.lpstrText, NULL, NULL, SW_SHOWNORMAL);
						HeapFree(globalHeap, 0, (HGLOBAL)tr.lpstrText);
						break;
					}
				}
			}
#endif
		}
		break;
	}
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
			case ID_CLIENT:
				switch (HIWORD(wParam)) {
#ifdef USE_RICH_EDIT
					case EN_UPDATE:
						if (!bUpdated) {
							bUpdated = TRUE;
							UpdateStatus();
						}
						break;
#endif
					case EN_CHANGE: {
						if (!bDirtyFile && !bLoading) {
							int nMax = GetWindowTextLength(hwndMain);
							TCHAR szTmp[MAXFN];
							szTmp[0] = _T(' ');
							szTmp[1] = _T('*');
							szTmp[2] = _T(' ');
							GetWindowText(hwndMain, szTmp + 3, nMax + 1);
							SetWindowText(hwndMain, szTmp);
							bDirtyFile = TRUE;
						}
						bHideMessage = TRUE;
						break;
					}
#ifdef USE_RICH_EDIT
					case EN_STOPNOUNDO:
						ERROROUT(GetString(IDS_CANT_UNDO_WARNING));
						break;
#endif
					case EN_ERRSPACE:
					case EN_MAXTEXT:
#ifdef USE_RICH_EDIT
						{
							TCHAR szBuffer[100];
							wsprintf(szBuffer, GetString(IDS_MEMORY_LIMIT), GetWindowTextLength(client));
							ERROROUT(szBuffer);
						}
#else
						if (bLoading) {
							if (options.bAlwaysLaunch || MessageBox(hwnd, GetString(IDS_QUERY_LAUNCH_VIEWER), STR_METAPAD, MB_ICONQUESTION | MB_YESNO) == IDYES) {
								if (options.szBrowser[0] == _T('\0')) {
									MessageBox(hwnd, GetString(IDS_PRIMARY_VIEWER_MISSING), STR_METAPAD, MB_OK|MB_ICONEXCLAMATION);
									SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(ID_VIEW_OPTIONS, 0), 0);
									szFile[0] = _T('\0');
									break;
								}
								LaunchPrimaryExternalViewer();
							}
							bLoading = FALSE;
							if (!IsWindowVisible(hwnd))
								bQuitApp = TRUE;
						}
						else {
							MessageBox(hwnd, GetString(IDS_LE_MEMORY_LIMIT), STR_METAPAD, MB_ICONEXCLAMATION | MB_OK);
						}
#endif
				}
				break;
			case ID_SAVE_AND_QUIT:
				if (!SaveCurrentFile())
					break;
				options.bQuickExit = TRUE;
			case ID_MYFILE_QUICK_EXIT:
				if (!options.bQuickExit || bLoading)
					break;
			case ID_MYFILE_EXIT:
				SendMessage(hwndMain, WM_CLOSE, 0, 0L);
				break;
			case ID_FIND_NEXT:
				if (szFindText[0] != _T('\0')) {
					SearchFile(szFindText, bMatchCase, FALSE, TRUE, bWholeWord);
					SendMessage(client, EM_SCROLLCARET, 0, 0);
				}
				else {
					bDown = TRUE;
					SendMessage(hwndMain, WM_COMMAND, MAKEWPARAM(ID_FIND, 0), 0);
				}
				break;
			case ID_FIND_PREV_WORD:
			case ID_FIND_NEXT_WORD:
				SelectWord(TRUE, TRUE, TRUE);
				if (szFindText[0] != _T('\0')) {
					SearchFile(szFindText, bMatchCase, FALSE, (LOWORD(wParam) == ID_FIND_NEXT_WORD ? TRUE : FALSE), bWholeWord);
					SendMessage(client, EM_SCROLLCARET, 0, 0);
				}
				break;
			case ID_FIND_PREV:
				if (szFindText[0] != _T('\0')) {
					SearchFile(szFindText, bMatchCase, FALSE, FALSE, bWholeWord);
					SendMessage(client, EM_SCROLLCARET, 0, 0);
				}
				else {
					bDown = FALSE;
					SendMessage(hwndMain, WM_COMMAND, MAKEWPARAM(ID_FIND, 0), 0);
				}
				break;
			case ID_FIND: {
				static FINDREPLACE fr;
				static BOOL bFindCalled = FALSE;

				if (hdlgFind) {
					SetFocus(hdlgFind);
					break;
				}

				lstrcpy(szFindText, FindArray[0]);

				SelectWord(TRUE, TRUE, !options.bNoFindAutoSelect);

				ZeroMemory(&fr, sizeof(FINDREPLACE));
				fr.lStructSize = sizeof(FINDREPLACE);
				fr.hwndOwner = hwndMain;
				fr.lpstrFindWhat = szFindText;
				fr.wFindWhatLen = MAXFIND * sizeof(TCHAR);

				fr.hInstance = hinstLang;
				fr.lpTemplateName = MAKEINTRESOURCE(IDD_FIND);
				fr.Flags = FR_ENABLETEMPLATE;

				if (bWholeWord)
					fr.Flags |= FR_WHOLEWORD;
				if (bDown)
					fr.Flags |= FR_DOWN;
				if (bMatchCase)
					fr.Flags |= FR_MATCHCASE;

				hdlgFind = FindText(&fr);
				bFindOpen = TRUE;
				if (bFindCalled) {
					SetWindowPlacement(hdlgFind, &wpFindPlace);
				} else
					bFindCalled = TRUE;

				wpOrigFindProc = (WNDPROC)SetWindowLongPtr(hdlgFind, GWLP_WNDPROC, (LONG_PTR)FindProc);

				{
					CHARRANGE cr;
					int i;

#ifdef USE_RICH_EDIT
					SendMessage(client, EM_EXGETSEL, 0, (LPARAM)&cr);
#else
					SendMessage(client, EM_GETSEL, (WPARAM)&cr.cpMin, (LPARAM)&cr.cpMax);
#endif
					SendDlgItemMessage(hdlgFind, IDC_CLOSE_AFTER_FIND, BM_SETCHECK, (WPARAM) bCloseAfterFind, 0);

					SetWindowText(GetDlgItem(hdlgFind, 1152), _T("dummy_find"));

					{
						HBITMAP hb = CreateMappedBitmap(hinstThis, IDB_DROP_ARROW, 0, NULL, 0);
						SendDlgItemMessage(hdlgFind, IDC_ESCAPE, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)(HANDLE)hb);
					}

					if (options.bCurrentFindFont) {
						SendDlgItemMessage(hdlgFind, ID_DROP_FIND, WM_SETFONT, (WPARAM)hfontfind, 0);
					}
					SendDlgItemMessage(hdlgFind, ID_DROP_FIND, CB_LIMITTEXT, (WPARAM)MAXFIND, 0);

					for (i = 0; i < NUMFINDS; ++i) {
						if (lstrlen(FindArray[i]))
							SendDlgItemMessage(hdlgFind, ID_DROP_FIND, CB_ADDSTRING, 0, (WPARAM)FindArray[i]);
					}

					if (lstrlen(szFindText)) {
						SendDlgItemMessage(hdlgFind, ID_DROP_FIND, WM_SETTEXT, 0, (LPARAM)szFindText);
						SendDlgItemMessage(hdlgFind, ID_DROP_FIND, CB_SETEDITSEL, 0, MAKELPARAM(0, -1));
					}
				}
				break;
			}
			case ID_REPLACE: {
				static FINDREPLACE fr;
				static BOOL bReplaceCalled = FALSE;

				if (hdlgFind) {
					SetFocus(hdlgFind);
					break;
				}

				lstrcpy(szFindText, FindArray[0]);
				lstrcpy(szReplaceText, ReplaceArray[0]);

				SelectWord(TRUE, TRUE, !options.bNoFindAutoSelect);

				ZeroMemory(&fr, sizeof(FINDREPLACE));
				fr.lStructSize = sizeof(FINDREPLACE);
				fr.hwndOwner = hwndMain;
				fr.lpstrFindWhat = szFindText;
				fr.wFindWhatLen = MAXFIND * sizeof(TCHAR);
				fr.lpstrReplaceWith = szReplaceText;
				fr.wReplaceWithLen = MAXFIND * sizeof(TCHAR);

				fr.hInstance = hinstLang;
				fr.lpTemplateName = MAKEINTRESOURCE(IDD_REPLACE);
				fr.Flags = FR_ENABLETEMPLATE;

				if (bWholeWord)
					fr.Flags |= FR_WHOLEWORD;
				if (bDown)
					fr.Flags |= FR_DOWN;
				if (bMatchCase)
					fr.Flags |= FR_MATCHCASE;


				hdlgFind = ReplaceText(&fr);
				bReplaceOpen = TRUE;
				if (bReplaceCalled) {
					SetWindowPlacement(hdlgFind, &wpReplacePlace);
				} else
					bReplaceCalled = TRUE;

				wpOrigFindProc = (WNDPROC)SetWindowLongPtr(hdlgFind, GWLP_WNDPROC, (LONG_PTR)FindProc);

				{
					CHARRANGE cr;
					int i;
#ifdef USE_RICH_EDIT
					SendMessage(client, EM_EXGETSEL, 0, (LPARAM)&cr);
#else
					SendMessage(client, EM_GETSEL, (WPARAM)&cr.cpMin, (LPARAM)&cr.cpMax);
#endif

					SetWindowText(GetDlgItem(hdlgFind, 1152), _T("dummy_repl"));

					{
						HBITMAP hb = CreateMappedBitmap(hinstThis, IDB_DROP_ARROW, 0, NULL, 0);
						SendDlgItemMessage(hdlgFind, IDC_ESCAPE, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)(HANDLE)hb);
						SendDlgItemMessage(hdlgFind, IDC_ESCAPE2, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)(HANDLE)hb);
					}

					SendDlgItemMessage(hdlgFind, ID_DROP_FIND, CB_LIMITTEXT, (WPARAM)MAXFIND, 0);
					SendDlgItemMessage(hdlgFind, ID_DROP_REPLACE, CB_LIMITTEXT, (WPARAM)MAXFIND, 0);

					for (i = 0; i < NUMFINDS; ++i) {
						if (lstrlen(FindArray[i]))
							SendDlgItemMessage(hdlgFind, ID_DROP_FIND, CB_ADDSTRING, 0, (LPARAM)FindArray[i]);
						if (lstrlen(ReplaceArray[i]))
							SendDlgItemMessage(hdlgFind, ID_DROP_REPLACE, CB_ADDSTRING, 0, (LPARAM)ReplaceArray[i]);
					}

					if (options.bCurrentFindFont) {
						SendDlgItemMessage(hdlgFind, ID_DROP_FIND, WM_SETFONT, (WPARAM)hfontfind, 0);
						SendDlgItemMessage(hdlgFind, ID_DROP_REPLACE, WM_SETFONT, (WPARAM)hfontfind, 0);
					}
					SendDlgItemMessage(hdlgFind, ID_DROP_REPLACE, CB_SETEDITSEL, 0, 0);

					if (lstrlen(szFindText)) {
						SendDlgItemMessage(hdlgFind, ID_DROP_FIND, WM_SETTEXT, 0, (LPARAM)szFindText);
						SendDlgItemMessage(hdlgFind, ID_DROP_FIND, CB_SETEDITSEL, 0, MAKELPARAM(0, -1));
					}

#ifdef USE_RICH_EDIT
					if (SendMessage(client, EM_EXLINEFROMCHAR, 0, (LPARAM)cr.cpMin) == SendMessage(client, EM_EXLINEFROMCHAR, 0, (LPARAM)cr.cpMax))
#else
					if (SendMessage(client, EM_LINEFROMCHAR, (WPARAM)cr.cpMin, 0) == SendMessage(client, EM_LINEFROMCHAR, (WPARAM)cr.cpMax, 0))
#endif
						SendDlgItemMessage(hdlgFind, IDC_RADIO_WHOLE, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
					else
						SendDlgItemMessage(hdlgFind, IDC_RADIO_SELECTION, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
				}
				break;
			}
			case ID_MYEDIT_DELETE:
				SendMessage(client, WM_CLEAR, 0, 0);
				UpdateStatus();
				break;
#ifdef USE_RICH_EDIT
			case ID_MYEDIT_REDO:
				SendMessage(client, EM_REDO, 0, 0);
				UpdateStatus();
				break;
#endif
			case ID_MYEDIT_UNDO: {
				/*
				BOOL bOldDirty = FALSE;
				if (bDirtyFile)
					bOldDirty = TRUE;
				*/
				SendMessage(client, EM_UNDO, 0, 0);
				/*
				if (bOldDirty && bDirtyFile && !SendMessage(client, EM_CANUNDO, 0, 0)) {
					TCHAR szBuffer[MAXFN];

					bDirtyFile = FALSE;
					GetWindowText(hwndMain, szBuffer, GetWindowTextLength(hwndMain) + 1);
					SetWindowText(hwndMain, szBuffer + 3);
					bLoading = FALSE;
				}
				*/
				UpdateStatus();
				break;
			}
#if defined(USE_RICH_EDIT) || defined(BUILD_METAPAD_UNICODE)
			case ID_MYEDIT_CUT:
			case ID_MYEDIT_COPY:
			{
#if TRUE // new cut/copy (for chinese crash)
				HGLOBAL hMem, hMem2;
				LPTSTR szOrig, szNew;

				if (LOWORD(wParam) == ID_MYEDIT_CUT) {
					SendMessage(client, WM_CUT, 0, 0);
				}
				else {
					SendMessage(client, WM_COPY, 0, 0);
				}

				if (!OpenClipboard(hwnd)) {
					/**
					 * @BUG The following error pops up spuriously for some users,
					 * but doesn't actually affect the copy.
					 * Might be related to an incorrect assumption about how the
					 * clipboard works or to multiple copy messages.
					 */
					ERROROUT(GetString(IDS_CLIPBOARD_OPEN_ERROR));
					break;
				}
				else {
					if ( (hMem = GetClipboardData(_CF_TEXT)) ) {
						DWORD dwLastError;
						szOrig = GlobalLock(hMem);
						if( szOrig ) {
							hMem2 = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, sizeof(TCHAR) * (lstrlen(szOrig) + 2));
							szNew = GlobalLock(hMem2);
							if (szNew) {
								lstrcpy(szNew, szOrig);
							}
							else {
								ERROROUT(GetString(IDS_GLOBALLOCK_ERROR));
								break;
							}
							SetLastError(NO_ERROR);
							if (!GlobalUnlock(hMem) && (dwLastError = GetLastError()) != NO_ERROR) {
								ERROROUT(GetString(IDS_CLIPBOARD_UNLOCK_ERROR));
								SetLastError(dwLastError);
								ReportLastError();
								break;
							}
							if (!GlobalUnlock(hMem2) && (dwLastError = GetLastError()) != NO_ERROR) {
								ERROROUT(GetString(IDS_CLIPBOARD_UNLOCK_ERROR));
								SetLastError(dwLastError);
								ReportLastError();
								break;
							}
							if (!EmptyClipboard()) {
								ReportLastError();
								break;
							}
							if (!SetClipboardData(_CF_TEXT, hMem2)) {
								ReportLastError();
								break;
							}
						}
						else {
							ERROROUT(GetString(IDS_GLOBALLOCK_ERROR));
						}
					}
					CloseClipboard();
				}
#else // old cut/copy
/**
 * @fixme Huge block of unreachable code. It's here probably as a means of
 * testing the currently used code. Might be wise to remove. It also includes
 * some commented out sections.
 */
					HGLOBAL hMem;
					LPTSTR strTmp;
					LPTSTR szSrc;
					CHARRANGE cr;
					DWORD dwLastError;

					SendMessage(client, EM_EXGETSEL, 0, (LPARAM)&cr);
					if (cr.cpMin == cr.cpMax) break;
/*
{
	TCHAR str[100];
	wsprintf(str, _T("Selection length = %d"), cr.cpMax - cr.cpMin);
	ERROROUT(str);
}
*/
					szSrc = (LPTSTR) GlobalAlloc(GPTR, (cr.cpMax - cr.cpMin + 1) * sizeof(TCHAR));
					SendMessage(client, EM_GETSELTEXT, 0, (LPARAM)szSrc);

//DBGOUT(szSrc, _T("original string"));
					RichModeToDos(&szSrc);

//DBGOUT(szSrc, _T("modified string"));

					hMem = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, sizeof(TCHAR) * (lstrlen(szSrc) + 2));
					strTmp = GlobalLock(hMem);
					if (strTmp) {
						lstrcpy(strTmp, szSrc);
					}
					else {
						ERROROUT(GetString(IDS_GLOBALLOCK_ERROR));
						break;
					}

//DBGOUT(strTmp, _T("adding to clipboard"));

/*
{
	TCHAR str[100];
	wsprintf(str, _T("hMem = %d"), hMem);
	ERROROUT(str);
}
*/
					SetLastError(NO_ERROR);

					if (!GlobalUnlock(hMem) && (dwLastError = GetLastError()) != NO_ERROR) {
						ERROROUT(GetString(IDS_CLIPBOARD_UNLOCK_ERROR));
						SetLastError(dwLastError);
						ReportLastError();
						break;
					}

					if (OpenClipboard(NULL)) {
						EmptyClipboard();
						SetClipboardData(_CF_TEXT, hMem);
						CloseClipboard();
					}
					else {
						ERROROUT(GetString(IDS_CLIPBOARD_OPEN_ERROR));
					}

					GlobalFree((HGLOBAL)szSrc);
/*
{
	HGLOBAL hMem;
	LPTSTR szTmp;
	OpenClipboard(NULL);
	hMem = GetClipboardData(_CF_TEXT);
	if (hMem) {
		szTmp = GlobalLock(hMem);
		DBGOUT(szTmp, _T("clipboard contents"));
		GlobalUnlock(hMem);
	}
	CloseClipboard();
}
*/
					if (LOWORD(wParam) == ID_MYEDIT_CUT) {
						SendMessage(client, EM_REPLACESEL, (WPARAM)TRUE, (LPARAM)NULL);
						InvalidateRect(client, NULL, TRUE);
					}
#endif
				}
				UpdateStatus();
				break;
#else
			case ID_MYEDIT_CUT:
				SendMessage(client, WM_CUT, 0, 0);
				UpdateStatus();
				break;
			case ID_MYEDIT_COPY:
				SendMessage(client, WM_COPY, 0, 0);
				UpdateStatus();
				break;
#endif
			case ID_MYEDIT_PASTE: {
#if defined(USE_RICH_EDIT) || defined(BUILD_METAPAD_UNICODE)
				HGLOBAL hMem;
				LPTSTR szTmp;
				OpenClipboard(NULL);
				hMem = GetClipboardData(_CF_TEXT);
				if (hMem) {
					szTmp = GlobalLock(hMem);
					SendMessage(client, EM_REPLACESEL, (WPARAM)TRUE, (LPARAM)szTmp);
					GlobalUnlock(hMem);
				}
				CloseClipboard();
				InvalidateRect(client, NULL, TRUE);
#else
				SendMessage(client, WM_PASTE, 0, 0);
#endif
				UpdateStatus();
				break;
			}
			case ID_HOME:
				{
					LONG lStartLine, lStartLineIndex, lLineLen, i;
					CHARRANGE cr;
					TCHAR* szTemp;

					if (options.bNoSmartHome) {
						SendMessage(client, WM_KEYDOWN, (WPARAM)VK_HOME, 0);
						SendMessage(client, WM_KEYUP, (WPARAM)VK_HOME, 0);
						break;
					}

#ifdef USE_RICH_EDIT
					SendMessage(client, EM_EXGETSEL, 0, (LPARAM)&cr);
					lStartLine = SendMessage(client, EM_EXLINEFROMCHAR, 0, (LPARAM)cr.cpMin);
#else
					SendMessage(client, EM_GETSEL, (WPARAM)&cr.cpMin, (LPARAM)&cr.cpMax);
					lStartLine = SendMessage(client, EM_LINEFROMCHAR, (WPARAM)cr.cpMin, 0);
#endif
					lStartLineIndex = SendMessage(client, EM_LINEINDEX, (WPARAM)lStartLine, 0);
					lLineLen = SendMessage(client, EM_LINELENGTH, (WPARAM)lStartLineIndex, 0);

					szTemp = (LPTSTR) HeapAlloc(globalHeap, HEAP_ZERO_MEMORY, (lLineLen+2) * sizeof(TCHAR));
					*((LPWORD)szTemp) = (USHORT)(lLineLen + 1);
					SendMessage(client, EM_GETLINE, (WPARAM)lStartLine, (LPARAM)(LPCTSTR)szTemp);
					szTemp[lLineLen] = _T('\0');

					for (i = 0; i < lLineLen; ++i)
						if (szTemp[i] != _T('\t') && szTemp[i] != _T(' '))
							break;

					if (cr.cpMin - lStartLineIndex == i)
						cr.cpMin = cr.cpMax = lStartLineIndex;
					else
						cr.cpMin = cr.cpMax = lStartLineIndex + i;

#ifdef USE_RICH_EDIT
					SendMessage(client, EM_EXSETSEL, 0, (LPARAM)&cr);
#else
					SendMessage(client, EM_SETSEL, (WPARAM)cr.cpMin, (LPARAM)cr.cpMax);
#endif
					HeapFree(globalHeap, 0, (HGLOBAL)szTemp);
					SendMessage(client, EM_SCROLLCARET, 0, 0);
				}
				break;
			case ID_MYEDIT_SELECTALL: {
				CHARRANGE cr;
				cr.cpMin = 0;
				cr.cpMax = -1;

#ifdef USE_RICH_EDIT
				SendMessage(client, EM_EXSETSEL, 0, (LPARAM)&cr);
#else
				SendMessage(client, EM_SETSEL, (WPARAM)cr.cpMin, (LPARAM)cr.cpMax);
#endif
				UpdateStatus();
				break;
			}
#ifdef USE_RICH_EDIT
			case ID_SHOWHYPERLINKS:
				{
					CHARRANGE cr;
					HCURSOR hcur;

					if (SendMessage(client, EM_CANUNDO, 0, 0)) {
						if (!options.bSuppressUndoBufferPrompt && MessageBox(hwnd, GetString(IDS_UNDO_HYPERLINKS_WARNING), STR_METAPAD, MB_ICONEXCLAMATION | MB_OKCANCEL) == IDCANCEL) {
							break;
						}
					}
					hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));
					bHyperlinks = !GetCheckedState(GetMenu(hwndMain), ID_SHOWHYPERLINKS, TRUE);
					SendMessage(client, EM_AUTOURLDETECT, (WPARAM)bHyperlinks, 0);

					SendMessage(client, WM_SETREDRAW, (WPARAM)FALSE, 0);
					SendMessage(client, EM_EXGETSEL, 0, (LPARAM)&cr);
					UpdateWindowText();
					SendMessage(client, EM_EXSETSEL, 0, (LPARAM)&cr);
					SendMessage(client, WM_SETREDRAW, (WPARAM)TRUE, 0);
					SetCursor(hcur);
				}
				break;
#endif
			case ID_SMARTSELECT:
				bSmartSelect = !GetCheckedState(GetMenu(hwndMain), ID_SMARTSELECT, TRUE);
				break;
			case ID_ALWAYSONTOP:
				bAlwaysOnTop = !GetCheckedState(GetMenu(hwndMain), ID_ALWAYSONTOP, TRUE);
				SetWindowPos(hwnd, (bAlwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
				UpdateStatus();
				break;
			case ID_TRANSPARENT: {
				if (SetLWA) {
					bTransparent = !GetCheckedState(GetMenu(hwndMain), ID_TRANSPARENT, TRUE);
					if (bTransparent) {
						SetWindowLongPtr(hwnd, GWL_EXSTYLE, GetWindowLongPtr(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);
						SetLWA(hwnd, 0, (BYTE)((255 * (100 - options.nTransparentPct)) / 100), LWA_ALPHA);
					}
					else {
						SetWindowLongPtr(hwnd, GWL_EXSTYLE, GetWindowLongPtr(hwnd, GWL_EXSTYLE) & ~WS_EX_LAYERED);
						RedrawWindow(hwnd, NULL, NULL, RDW_UPDATENOW | RDW_ALLCHILDREN | RDW_ERASE | RDW_INVALIDATE | RDW_FRAME);
					}
				}
				break;
			}
			case ID_SHOWTOOLBAR: {
				RECT rect;

				if (!IsWindow(toolbar))
					CreateToolbar();

				bShowToolbar = !GetCheckedState(GetMenu(hwndMain), ID_SHOWTOOLBAR, TRUE);
				if (bShowToolbar) {
					UpdateStatus();
					ShowWindow(toolbar, SW_SHOW);
				}
				else
					ShowWindow(toolbar, SW_HIDE);

				SendMessage(toolbar, WM_SIZE, 0, 0);
				GetClientRect(hwndMain, &rect);
				SetWindowPos(client, 0, 0, bShowToolbar ? GetToolbarHeight() : rect.top - GetToolbarHeight(), rect.right - rect.left, rect.bottom - rect.top - GetStatusHeight() - GetToolbarHeight(), SWP_NOZORDER | SWP_SHOWWINDOW);
				break;
			}
			case ID_INSERT_FILE: {
				OPENFILENAME ofn;
				TCHAR szDefExt[] = _T("txt");
				TCHAR szFilename[MAXFN] = _T("");

				ofn.lStructSize = sizeof(OPENFILENAME);
				ofn.hwndOwner = client;
				ofn.lpstrFilter = szCustomFilter;
				ofn.lpstrCustomFilter = (LPTSTR)NULL;
				ofn.nMaxCustFilter = 0L;
				ofn.nFilterIndex = 1L;
				ofn.lpstrFile = szFilename;
				ofn.nMaxFile = sizeof(szFilename);
				ofn.lpstrFileTitle = (LPTSTR)NULL;
				ofn.nMaxFileTitle = 0L;
				ofn.lpstrInitialDir = szDir;
				ofn.lpstrTitle = (LPTSTR)NULL;
				ofn.Flags = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
				ofn.nFileOffset = 0;
				ofn.nFileExtension = 0;
				ofn.lpstrDefExt = szDefExt;

				if (GetOpenFileName(&ofn)) {
					HANDLE hFile = NULL;
					ULONG lBufferLength;
					PBYTE pBuffer = NULL;
					DWORD dwActualBytesRead;
					HCURSOR hcur;
					INT nFileEncoding;

					hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));

					hFile = (HANDLE)CreateFile(szFilename, GENERIC_READ, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
					if (hFile == INVALID_HANDLE_VALUE) {
						ERROROUT(GetString(IDS_FILE_READ_ERROR));
						goto endinsertfile;
					}

					dwActualBytesRead = LoadFileIntoBuffer(hFile, &pBuffer, &lBufferLength, &nFileEncoding);
#ifndef BUILD_METAPAD_UNICODE
					if (memchr((const void*)pBuffer, _T('\0'), lBufferLength) != NULL) {
						if (options.bNoWarningPrompt || MessageBox(hwnd, GetString(IDS_BINARY_FILE_WARNING), STR_METAPAD, MB_ICONQUESTION|MB_YESNO) == IDYES) {
							UINT i;
							for (i = 0; i < lBufferLength; i++) {
								if (pBuffer[i] == _T('\0'))
									pBuffer[i] = _T(' ');
							}
						}
						else goto endinsertfile;
					}
#endif

					if (nFileEncoding != TYPE_UTF_16 && nFileEncoding != TYPE_UTF_16_BE) {
#ifdef USE_RICH_EDIT
						FixTextBuffer((LPTSTR)pBuffer);
#else
						FixTextBufferLE((LPTSTR*)&pBuffer);
#endif
					}
					SendMessage(client, EM_REPLACESEL, (WPARAM)TRUE, (LPARAM)(LPTSTR)pBuffer);
#ifdef USE_RICH_EDIT
					InvalidateRect(client, NULL, TRUE);
#endif
endinsertfile:
					CloseHandle(hFile);
					if (pBuffer) HeapFree(globalHeap, 0, (HGLOBAL) pBuffer);
					SetCursor(hcur);
				}
				break;
			}
			case ID_SHOWSTATUS: {
				RECT rect;

				if (!IsWindow(status))
					CreateStatusbar();

				bShowStatus = !GetCheckedState(GetMenu(hwndMain), ID_SHOWSTATUS, TRUE);
				if (bShowStatus) {
					UpdateStatus();
					ShowWindow(status, SW_SHOW);
				}
				else
					ShowWindow(status, SW_HIDE);

				GetClientRect(hwndMain, &rect);
				SetWindowPos(client, 0, 0, GetToolbarHeight(), rect.right - rect.left, rect.bottom - rect.top - GetStatusHeight() - GetToolbarHeight(), SWP_NOMOVE | SWP_NOZORDER | SWP_SHOWWINDOW);
				SendMessage(status, WM_SIZE, 0, 0);
				break;
			}
			case ID_READONLY:
				{
					TCHAR szTmp[MAXFN];
					int nRes;

					if (!options.bReadOnlyMenu) break;

					bReadOnly = !GetCheckedState(GetMenu(hwndMain), ID_READONLY, FALSE);
					if (bReadOnly)
						nRes = SetFileAttributes(szFile, GetFileAttributes(szFile) | FILE_ATTRIBUTE_READONLY);
					else
						nRes = SetFileAttributes(szFile, ((GetFileAttributes(szFile) & ~FILE_ATTRIBUTE_READONLY) == 0 ? FILE_ATTRIBUTE_NORMAL : GetFileAttributes(szFile) & ~FILE_ATTRIBUTE_READONLY));

					if (nRes == 0) {
						DWORD dwError = GetLastError();
						if (dwError == ERROR_ACCESS_DENIED) {
							ERROROUT(GetString(IDS_CHANGE_READONLY_ERROR));
						}
						else {
							ReportLastError();
						}
						bReadOnly = !bReadOnly;
						break;
					}

					GetWindowText(hwndMain, szTmp, GetWindowTextLength(hwndMain) + 1);

					if (bReadOnly) {
						lstrcat(szTmp, _T(" "));
						lstrcat(szTmp, GetString(IDS_READONLY_INDICATOR));
						SetWindowText(hwndMain, szTmp);
					}
					else
						//szTmp[GetWindowTextLength(hwndMain) - 12] = _T('\0');
						UpdateCaption();

					SwitchReadOnly(bReadOnly);

					UpdateStatus();
				}
				break;
			case ID_FONT_PRIMARY:
				bPrimaryFont = !GetCheckedState(GetMenu(hwndMain), ID_FONT_PRIMARY, TRUE);
				if (!SetClientFont(bPrimaryFont)) {
					GetCheckedState(GetMenu(hwndMain), ID_FONT_PRIMARY, TRUE);
					bPrimaryFont = !bPrimaryFont;
				}
#ifndef USE_RICH_EDIT
				UpdateStatus();
#endif
				break;
			case ID_VIEW_OPTIONS:
				{
					PROPSHEETHEADER psh;
					PROPSHEETPAGE pages[4];

					ZeroMemory(&pages[0], sizeof(pages[0]));
					pages[0].dwSize = sizeof(PROPSHEETPAGE);
					//pages[0].dwSize = PROPSHEETPAGE_V1_SIZE;

					pages[0].dwFlags = PSP_DEFAULT;
					pages[0].hInstance = hinstLang;
					pages[0].pszTemplate = MAKEINTRESOURCE(IDD_PROPPAGE_GENERAL);
					pages[0].pfnDlgProc = (DLGPROC)GeneralPageProc;

					ZeroMemory(&pages[1], sizeof(pages[1]));
					pages[1].dwSize = sizeof(PROPSHEETPAGE);
					//pages[1].dwSize = PROPSHEETPAGE_V1_SIZE;

					pages[1].hInstance = hinstLang;
					pages[1].dwFlags = PSP_DEFAULT;
					pages[1].pszTemplate = MAKEINTRESOURCE(IDD_PROPPAGE_VIEW);
					pages[1].pfnDlgProc = (DLGPROC)ViewPageProc;

					ZeroMemory(&pages[2], sizeof(pages[2]));
					pages[2].dwSize = sizeof(PROPSHEETPAGE);
					//pages[2].dwSize = PROPSHEETPAGE_V1_SIZE;

					pages[2].hInstance = hinstLang;
					pages[2].dwFlags = PSP_DEFAULT;
					pages[2].pszTemplate = MAKEINTRESOURCE(IDD_PROPPAGE_A2);
					pages[2].pfnDlgProc = (DLGPROC)Advanced2PageProc;

					ZeroMemory(&pages[3], sizeof(pages[3]));
					pages[3].dwSize = sizeof(PROPSHEETPAGE);
					//pages[3].dwSize = PROPSHEETPAGE_V1_SIZE;

					pages[3].hInstance = hinstLang;
					pages[3].dwFlags = PSP_DEFAULT;
#ifdef USE_RICH_EDIT
					pages[3].pszTemplate = MAKEINTRESOURCE(IDD_PROPPAGE_A1);
#else
					pages[3].pszTemplate = MAKEINTRESOURCE(IDD_PROPPAGE_A1_LE);
#endif
					pages[3].pfnDlgProc = (DLGPROC)AdvancedPageProc;

					ZeroMemory(&psh, sizeof(psh));
					psh.dwSize = sizeof(PROPSHEETHEADER);

					//psh.dwSize = PROPSHEETHEADER_V1_SIZE;

					psh.dwFlags = PSH_USECALLBACK | PSH_NOAPPLYNOW | PSH_PROPSHEETPAGE;
					psh.hwndParent = hwndMain;
					psh.nPages = 4;
					psh.pszCaption = GetString(IDS_SETTINGS_TITLE);
					psh.ppsp = (LPCPROPSHEETPAGE) pages;
					psh.pfnCallback = SheetInitProc;

					{
						int retval;
/*
						BOOL bOldMRU = options.bRecentOnOwn;
						int nOldFontOption, nOldTabs = options.nTabStops;
						LOGFONT oldFont;
						BOOL bOldReadOnlyMenu = options.bReadOnlyMenu;
						BOOL bOldFlatToolbar = options.bUnFlatToolbar;
						BOOL bOldSystemColours = options.bSystemColours;
						BOOL bOldSystemColours2 = options.bSystemColours2;
						BOOL bOldNoFaves = options.bNoFaves;
#ifdef USE_RICH_EDIT
						BOOL bOldHideScrollbars = options.bHideScrollbars;
#endif
						COLORREF oldBackColour = options.BackColour, oldFontColour = options.FontColour;
						COLORREF oldBackColour2 = options.BackColour2, oldFontColour2 = options.FontColour2;
						TCHAR szOldLangPlugin[MAXFN];

						lstrcpy(szOldLangPlugin, options.szLangPlugin);
*/
						option_struct oldOptions;
						memcpy(&oldOptions, &options, sizeof(option_struct));
						LoadOptions();
/*
						if (bPrimaryFont) {
							nOldFontOption = options.nPrimaryFont;
							oldFont = options.PrimaryFont;
						}
						else {
							nOldFontOption = options.nSecondaryFont;
							oldFont = options.SecondaryFont;
						}
*/
						retval = PropertySheet(&psh);
						if (retval > 0) {
							SaveOptions();

							if (options.bLaunchClose && options.nLaunchSave == 2) {
								MessageBox(hwndMain, GetString(IDS_LAUNCH_WARNING), STR_METAPAD, MB_ICONEXCLAMATION);
							}

							if (options.bReadOnlyMenu != oldOptions.bReadOnlyMenu)
								FixReadOnlyMenu();

							if (options.bRecentOnOwn != oldOptions.bRecentOnOwn) {
								FixMRUMenus();
								PopulateMRUList();
							}

							if (oldOptions.bUnFlatToolbar != options.bUnFlatToolbar) {
								DestroyWindow(toolbar);
								CreateToolbar();
							}

							if ((memcmp((LPVOID)(LPVOID)(bPrimaryFont ? &oldOptions.PrimaryFont : &oldOptions.SecondaryFont), (LPVOID)(bPrimaryFont ? &options.PrimaryFont : &options.SecondaryFont), sizeof(LOGFONT)) != 0) ||
								(oldOptions.nTabStops != options.nTabStops) ||
								((bPrimaryFont && oldOptions.nPrimaryFont != options.nPrimaryFont) || (!bPrimaryFont && oldOptions.nSecondaryFont != options.nSecondaryFont)) ||
								(oldOptions.bSystemColours != options.bSystemColours) ||
								(oldOptions.bSystemColours2 != options.bSystemColours2) ||
								(memcmp((LPVOID)&oldOptions.BackColour, (LPVOID)&options.BackColour, sizeof(COLORREF)) != 0) ||
								(memcmp((LPVOID)&oldOptions.FontColour, (LPVOID)&options.FontColour, sizeof(COLORREF)) != 0) ||
								(memcmp((LPVOID)&oldOptions.BackColour2, (LPVOID)&options.BackColour2, sizeof(COLORREF)) != 0) ||
								(memcmp((LPVOID)&oldOptions.FontColour2, (LPVOID)&options.FontColour2, sizeof(COLORREF)) != 0))
								if (!SetClientFont(bPrimaryFont)) {
									options.nTabStops = oldOptions.nTabStops;
									if (bPrimaryFont) {
										options.nPrimaryFont = oldOptions.nPrimaryFont;
										memcpy((LPVOID)&options.PrimaryFont, (LPVOID)&oldOptions.PrimaryFont, sizeof(LOGFONT));
									}
									else {
										options.nSecondaryFont = oldOptions.nSecondaryFont;
										memcpy((LPVOID)&options.SecondaryFont, (LPVOID)&oldOptions.SecondaryFont, sizeof(LOGFONT));
									}
								}

							if (szFile[0] != _T('\0')) {
								UpdateCaption();
							}

							SendMessage(client, EM_SETMARGINS, (WPARAM)EC_LEFTMARGIN, MAKELPARAM(options.nSelectionMarginWidth, 0));

							if (bTransparent) {
								SetLWA(hwnd, 0, (BYTE)((255 * (100 - options.nTransparentPct)) / 100), LWA_ALPHA);
							}
#ifdef USE_RICH_EDIT
							if (oldOptions.bHideScrollbars != options.bHideScrollbars) {
								ERROROUT(GetString(IDS_RESTART_HIDE_SB));
							}
#endif
							if (oldOptions.bNoFaves != options.bNoFaves) {
								ERROROUT(GetString(IDS_RESTART_FAVES));
							}

							if (lstrcmp(oldOptions.szLangPlugin, options.szLangPlugin) != 0) {
								ERROROUT(GetString(IDS_RESTART_LANG));
							}

							PopulateMRUList();
							UpdateStatus();
						}
						else if (retval < 0) {
							ReportLastError();
						}
					}
					break;
				}
			case ID_HELP_ABOUT:
				DialogBox(hinstThis, MAKEINTRESOURCE(IDD_ABOUT), hwndMain, (DLGPROC)AboutDialogProc);
				break;
			case ID_ABOUT_PLUGIN:
				if (hinstThis != hinstLang)
					DialogBox(hinstLang, MAKEINTRESOURCE(IDD_ABOUT_PLUGIN), hwndMain, (DLGPROC)AboutPluginDialogProc);
				break;
			case ID_MYFILE_OPEN: {
				OPENFILENAME ofn;
				TCHAR szDefExt[] = _T("txt");
				TCHAR szTmp[MAXFN] = _T("");

				SetCurrentDirectory(szDir);
				if (!SaveIfDirty())
					break;

				ofn.lStructSize = sizeof(OPENFILENAME);
				ofn.hwndOwner = client;
				ofn.lpstrFilter = szCustomFilter;
				ofn.lpstrCustomFilter = (LPTSTR)NULL;
				ofn.nMaxCustFilter = 0L;
				ofn.nFilterIndex = 1L;
				ofn.lpstrFile = szTmp;
				ofn.nMaxFile = sizeof(szTmp);
				ofn.lpstrFileTitle = (LPTSTR)NULL;
				ofn.nMaxFileTitle = 0L;
				ofn.lpstrInitialDir = szDir;
				ofn.lpstrTitle = (LPTSTR)NULL;
				ofn.Flags = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
				ofn.nFileOffset = 0;
				ofn.nFileExtension = 0;
				ofn.lpstrDefExt = szDefExt;

				if (GetOpenFileName(&ofn)) {
					GetCurrentDirectory(MAXFN, szDir);
					bLoading = TRUE;
					bHideMessage = FALSE;
					lstrcpy(szStatusMessage, GetString(IDS_FILE_LOADING));
					UpdateStatus();
					lstrcpy(szFile, szTmp);
					LoadFile(szFile, FALSE, TRUE);
					if (bLoading) {
						/*
						bLoading = FALSE;
						ExpandFilename(szFile);
						wsprintf(szTmp, STR_CAPTION_FILE, szCaptionFile);
						SetWindowText(hwndMain, szTmp);
						bDirtyFile = FALSE;
						*/
						bLoading = FALSE;
						bDirtyFile = FALSE;
						UpdateCaption();
					}
					else {
						MakeNewFile();
					}
				}
				UpdateStatus();
				break;
			}
			case ID_MYFILE_NEW: {
				if (!SaveIfDirty())
					break;
				MakeNewFile();
				break;
			}
			case ID_EDIT_SELECTWORD: {
				CHARRANGE cr;

#ifdef USE_RICH_EDIT
				SendMessage(client, EM_EXGETSEL, 0, (LPARAM)&cr);
				cr.cpMin = cr.cpMax;
				SendMessage(client, EM_EXSETSEL, 0, (LPARAM)&cr);
#else
				SendMessage(client, EM_GETSEL, (WPARAM)&cr.cpMin, (LPARAM)&cr.cpMax);
				cr.cpMin = cr.cpMax;
				SendMessage(client, EM_SETSEL, (WPARAM)cr.cpMin, (LPARAM)cr.cpMax);
#endif
				SelectWord(FALSE, bSmartSelect, TRUE);
				break;
			}
			case ID_GOTOLINE: {
				DialogBox(hinstLang, MAKEINTRESOURCE(IDD_GOTO), hwndMain, (DLGPROC)GotoDialogProc);
				break;
			}
			case ID_EDIT_WORDWRAP:
				{
					HCURSOR hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));
#ifdef USE_RICH_EDIT
					bWordWrap = !GetCheckedState(GetMenu(hwndMain), ID_EDIT_WORDWRAP, TRUE);
					SendMessage(client, EM_SETTARGETDEVICE, (WPARAM)0, (LPARAM)(LONG) !bWordWrap);

					SendMessage(client, WM_HSCROLL, (WPARAM)SB_LEFT, 0);
					SendMessage(client, EM_SCROLLCARET, 0, 0);

					if (bWordWrap)
						SetWindowLongPtr(client, GWL_STYLE, GetWindowLongPtr(client, GWL_STYLE) & ~WS_HSCROLL);
					else if (!options.bHideScrollbars)
						SetWindowLongPtr(client, GWL_STYLE, GetWindowLongPtr(client, GWL_STYLE) | WS_HSCROLL);

					SetWindowPos(client, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
#else
					LONG lFileSize = GetWindowTextLength(client);
					LPTSTR szBuffer;
					RECT rect;
					CHARRANGE cr;

					szBuffer = (LPTSTR) HeapAlloc(globalHeap, HEAP_ZERO_MEMORY, (lFileSize+1) * sizeof(TCHAR));

					bWordWrap = !GetCheckedState(GetMenu(hwnd), ID_EDIT_WORDWRAP, TRUE);

					SendMessage(client, EM_GETSEL, (WPARAM)&cr.cpMin, (LPARAM)&cr.cpMax);

					GetWindowText(client, szBuffer, lFileSize+1);
					if (!DestroyWindow(client))
						ReportLastError();

					CreateClient(hwnd, _T(""), bWordWrap);

					SetWindowText(client, szBuffer);

					SendMessage(client, EM_SETSEL, (WPARAM)cr.cpMin, (LPARAM)cr.cpMax);

					HeapFree(globalHeap, 0, (HGLOBAL)szBuffer);
					SetClientFont(bPrimaryFont);

					GetClientRect(hwnd, &rect);
					SetWindowPos(client, 0, 0, bShowToolbar ? GetToolbarHeight() : rect.top - GetToolbarHeight(), rect.right - rect.left, rect.bottom - rect.top - GetStatusHeight() - GetToolbarHeight(), SWP_NOZORDER | SWP_SHOWWINDOW);
					SetFocus(client);
					SendMessage(client, EM_SCROLLCARET, 0, 0);
#endif
					SetCursor(hcur);
					UpdateStatus();
					break;
				}
			case ID_FILE_LAUNCHVIEWER:
				LaunchInViewer(TRUE, FALSE);
				break;
			case ID_LAUNCH_SECONDARY_VIEWER:
				LaunchInViewer(TRUE, TRUE);
				break;
			case ID_MYFILE_SAVE:
				SaveCurrentFile();
				break;
			case ID_MYFILE_SAVEAS:
				SaveCurrentFileAs();
				break;
			/*
			case ID_RUN_HELP:
				{
					TCHAR* pch;
					TCHAR buf[200];

					GetModuleFileName(hinst, buf, MAXFN);

					pch = _tcsrchr(buf, _T('\\'));
					++pch;
					*pch = _T('\0');

					lstrcat(buf, _T("metapad.chm"));

					ShellExecute(NULL, NULL, buf, NULL, szDir, SW_SHOWNORMAL);
				}
				break;
			*/
			case ID_STRIPCHAR:
			case ID_INDENT:
			case ID_UNINDENT: {
				LONG lStartLine, lEndLine, lStartLineIndex, lEndLineIndex, lMaxLine, lEndLineLen;
				BOOL bIndent = LOWORD(wParam) == ID_INDENT;
				LPTSTR szPrefix = (LPTSTR) HeapAlloc(globalHeap, HEAP_ZERO_MEMORY, (options.nTabStops+1) * sizeof(TCHAR));
				BOOL bEnd = FALSE;
				BOOL bStrip = LOWORD(wParam) == ID_STRIPCHAR;
				CHARRANGE cr;
				BOOL bRewrap = FALSE;

#ifdef USE_RICH_EDIT
				SendMessage(client, EM_EXGETSEL, 0, (LPARAM)&cr);
				lStartLine = SendMessage(client, EM_EXLINEFROMCHAR, 0, (LPARAM)cr.cpMin);
				lEndLine = SendMessage(client, EM_EXLINEFROMCHAR, 0, (LPARAM)cr.cpMax);
#else
				SendMessage(client, EM_GETSEL, (WPARAM)&cr.cpMin, (LPARAM)&cr.cpMax);
				lStartLine = SendMessage(client, EM_LINEFROMCHAR, (WPARAM)cr.cpMin, 0);
				lEndLine = SendMessage(client, EM_LINEFROMCHAR, (WPARAM)cr.cpMax, 0);
#endif

				if (bWordWrap && !bStrip && lStartLine != lEndLine) {
					SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(ID_EDIT_WORDWRAP, 0), 0);
#ifdef USE_RICH_EDIT
					SendMessage(client, EM_EXGETSEL, 0, (LPARAM)&cr);
					lStartLine = SendMessage(client, EM_EXLINEFROMCHAR, 0, (LPARAM)cr.cpMin);
					lEndLine = SendMessage(client, EM_EXLINEFROMCHAR, 0, (LPARAM)cr.cpMax);
#else
					SendMessage(client, EM_GETSEL, (WPARAM)&cr.cpMin, (LPARAM)&cr.cpMax);
					lStartLine = SendMessage(client, EM_LINEFROMCHAR, (WPARAM)cr.cpMin, 0);
					lEndLine = SendMessage(client, EM_LINEFROMCHAR, (WPARAM)cr.cpMax, 0);
#endif
					bRewrap = TRUE;
					//ERROROUT(_T("This function will not work with word wrap enabled."));
					//break;
				}

				lStartLineIndex = SendMessage(client, EM_LINEINDEX, (WPARAM)lStartLine, 0);
				lEndLineIndex = SendMessage(client, EM_LINEINDEX, (WPARAM)lEndLine, 0);
				lMaxLine = SendMessage(client, EM_GETLINECOUNT, 0, 0) - 1;

				if (options.bInsertSpaces) {
					memset(szPrefix, _T(' '), options.nTabStops);
					if (cr.cpMin == cr.cpMax) {
						szPrefix[options.nTabStops - (cr.cpMax - lEndLineIndex) % options.nTabStops] = _T('\0');
					}
					else
						szPrefix[options.nTabStops] = _T('\0');
				}
				else {
					lstrcpy(szPrefix, _T("\t"));
				}

				lEndLineLen = SendMessage(client, EM_LINELENGTH, (WPARAM)lEndLineIndex, 0);
				if ((lEndLine == lMaxLine) && (cr.cpMin != cr.cpMax) && (cr.cpMax != lEndLineIndex) &&
					((lEndLine != lStartLine) || (cr.cpMin == lEndLineIndex && (cr.cpMax - cr.cpMin - 1) == lEndLineLen))) {
					bEnd = TRUE;
				}
				if (lStartLine != lEndLine || bEnd || bStrip) {
					ULONG lFileSize = GetWindowTextLength(client);
					LPTSTR szBuffer;
					int i, j = 0, diff = 0;
					LPTSTR szTemp;
					LONG lLineLen, iIndex, lBefore;
					if (lEndLineIndex != cr.cpMax || (bStrip && cr.cpMin == cr.cpMax && lEndLineIndex == cr.cpMax)) {
						long lTmp = SendMessage(client, EM_LINEINDEX, (WPARAM)lEndLine+1, 0);
						lEndLineIndex = lTmp;
						++lEndLine;
					}
					szBuffer = (LPTSTR) HeapAlloc(globalHeap, HEAP_ZERO_MEMORY, (1 + lFileSize + (lMaxLine + 1) * options.nTabStops) * sizeof(TCHAR));
					szBuffer[0] = _T('\0');

					for (i = lStartLine; i < lEndLine; i++) {
						iIndex = SendMessage(client, EM_LINEINDEX, (WPARAM)i, 0);
						lLineLen = SendMessage(client, EM_LINELENGTH, (WPARAM)iIndex, 0);

						diff = 0;
						szTemp = (LPTSTR) HeapAlloc(globalHeap, HEAP_ZERO_MEMORY, (lLineLen+2) * sizeof(TCHAR));
						*((LPWORD)szTemp) = (USHORT)(lLineLen + 1); // + 1 to fix weird xp bug (skipping length 1 lines)!!
						SendMessage(client, EM_GETLINE, (WPARAM)i, (LPARAM)(LPCTSTR)szTemp);
						szTemp[lLineLen] = _T('\0');
						if (bIndent) {
							lstrcat(szBuffer, szPrefix);
							lstrcat(szBuffer, szTemp);
							diff = -(lstrlen(szPrefix));
						}
						else {
							if ((bStrip && lLineLen > 0) || szTemp[0] == _T('\t')) {
								diff = 1;
							}
							else if (szTemp[0] == _T(' ')) {
								diff = 1;
								while (diff < options.nTabStops && szTemp[diff] == _T(' '))
									diff++;
							}
							lstrcpy(szBuffer+j, szTemp + diff);
						}
						if (bEnd && i == lEndLine - 1) {
							j += lLineLen - diff;
						}
						else {
							szBuffer[lLineLen+j-diff] = _T('\r');
							szBuffer[lLineLen+j+1-diff] = _T('\n');
							j += lLineLen + 2 - diff;
						}
						HeapFree(globalHeap, 0, (HGLOBAL)szTemp);
					}

					lBefore = SendMessage(client, EM_GETLINECOUNT, 0, 0);
					cr.cpMin = lStartLineIndex;
					cr.cpMax = lEndLineIndex;
#ifdef USE_RICH_EDIT
					SendMessage(client, EM_EXSETSEL, 0, (LPARAM)&cr);
#else
					SendMessage(client, EM_SETSEL, (WPARAM)cr.cpMin, (LPARAM)cr.cpMax);
#endif
					SendMessage(client, EM_REPLACESEL, (WPARAM)TRUE, (LPARAM)szBuffer);
					if (SendMessage(client, EM_GETLINECOUNT, 0, 0) > lBefore) {
						SendMessage(client, EM_UNDO, 0, 0);
					}
					else {
						lEndLineIndex = SendMessage(client, EM_LINEINDEX, (WPARAM)lEndLine, 0);
						cr.cpMin = lStartLineIndex;
						cr.cpMax = lEndLineIndex;
#ifdef USE_RICH_EDIT
						SendMessage(client, EM_EXSETSEL, 0, (LPARAM)&cr);
#else
						SendMessage(client, EM_SETSEL, (WPARAM)cr.cpMin, (LPARAM)cr.cpMax);
#endif
					}
					HeapFree(globalHeap, 0, (HGLOBAL)szBuffer);
				}
				else {
					if (bIndent) {
						SendMessage(client, EM_REPLACESEL, (WPARAM)TRUE, (LPARAM)szPrefix);
					}
				}
				HeapFree(globalHeap, 0, (HGLOBAL)szPrefix);

				if (bRewrap) {
					SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(ID_EDIT_WORDWRAP, 0), 0);
				}

#ifdef USE_RICH_EDIT
				InvalidateRect(client, NULL, TRUE);
#endif
				UpdateStatus();
				break;
			}
#ifdef USE_RICH_EDIT
			case ID_INSERT_MODE:
				bInsertMode = !bInsertMode;
				SendMessage(client, WM_KEYDOWN, (WPARAM)VK_INSERT, (LPARAM)0x510001);
				SendMessage(client, WM_KEYUP, (WPARAM)VK_INSERT, (LPARAM)0xC0510001);
				UpdateStatus();
				break;
#endif
			case ID_DATE_TIME_LONG:
			case ID_DATE_TIME: {
				TCHAR szTime[100], szDate[100];
				if (bLoading) {
					szTime[0] = _T('\r');
					szTime[1] = _T('\n');
					szTime[2] = _T('\0');
				}
				else {
					szTime[0] = _T('\0');
				}
				if (!options.bDontInsertTime) {
					GetTimeFormat(LOCALE_USER_DEFAULT, TIME_NOSECONDS, NULL, NULL, (bLoading ? szTime + 2 : szTime), 100);
					lstrcat(szTime, _T(" "));
				}
				GetDateFormat(LOCALE_USER_DEFAULT, (LOWORD(wParam) == ID_DATE_TIME_LONG ? DATE_LONGDATE : 0), NULL, NULL, szDate, 100);
				lstrcat(szTime, szDate);
				if (bLoading) {
					lstrcat(szTime, _T("\r\n"));
				}
				SendMessage(client, EM_REPLACESEL, (WPARAM)TRUE, (LPARAM)szTime);
#ifdef USE_RICH_EDIT
				InvalidateRect(client, NULL, TRUE);
#endif

				if (bLoading)
					bDirtyFile = FALSE;
				UpdateStatus();
				break;
			}
			case ID_PAGESETUP:
				{
					LPTSTR szLocale = _T("1");
					BOOL bMetric;
					PAGESETUPDLG psd;

					ZeroMemory(&psd, sizeof(PAGESETUPDLG));
					psd.lStructSize = sizeof(PAGESETUPDLG);
					psd.Flags |= /*PSD_INTHOUSANDTHSOFINCHES | */PSD_DISABLEORIENTATION | PSD_DISABLEPAPER | PSD_DISABLEPRINTER | PSD_ENABLEPAGESETUPTEMPLATE | PSD_MARGINS;
					psd.hwndOwner = hwndMain;
					psd.hInstance = hinstLang;
					psd.rtMargin = options.rMargins;
					psd.lpPageSetupTemplateName = MAKEINTRESOURCE(IDD_PAGE_SETUP);

					if (!GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_IMEASURE, (LPTSTR)&szLocale, sizeof(szLocale))) {
						ReportLastError();
					}
					bMetric = (lstrcmp(szLocale, _T("0")) == 0);

					if (bMetric) {
						psd.rtMargin.bottom = (long)(psd.rtMargin.bottom * 2.54);
						psd.rtMargin.top = (long)(psd.rtMargin.top * 2.54);
						psd.rtMargin.left = (long)(psd.rtMargin.left * 2.54);
						psd.rtMargin.right = (long)(psd.rtMargin.right * 2.54);
					}

					if (PageSetupDlg(&psd)) {

						if (bMetric) {
							psd.rtMargin.bottom = (long)(psd.rtMargin.bottom * 0.3937);
							psd.rtMargin.top = (long)(psd.rtMargin.top * 0.3937);
							psd.rtMargin.left = (long)(psd.rtMargin.left * 0.3937);
							psd.rtMargin.right = (long)(psd.rtMargin.right * 0.3937);
						}

						options.rMargins = psd.rtMargin;
						SaveOptions();
					}
				}
				break;
			case ID_PRINT: {
				BOOL bFontChanged = FALSE;
				if (options.bPrintWithSecondaryFont && bPrimaryFont) {
					SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(ID_FONT_PRIMARY, 0), 0);
					bFontChanged = TRUE;
				}
				PrintContents();
				if (bFontChanged) {
					SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(ID_FONT_PRIMARY, 0), 0);
				}
				break;
			}
			case ID_HACKER:
			case ID_TABIFY:
			case ID_UNTABIFY:
			case ID_QUOTE:
			case ID_STRIP_CR:
			case ID_STRIP_CR_SPACE:
			case ID_STRIP_TRAILING_WS:
			case ID_MAKE_LOWER:
			case ID_MAKE_UPPER:
			case ID_MAKE_INVERSE:
			case ID_MAKE_SENTENCE:
			case ID_MAKE_TITLE:
#ifndef BUILD_METAPAD_UNICODE
			case ID_MAKE_OEM:
			case ID_MAKE_ANSI:
#endif
				{
				CHARRANGE cr;
				LPTSTR szSrc;
				LPTSTR szDest;
				LONG nSize;
				HCURSOR hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));

#ifdef USE_RICH_EDIT
				SendMessage(client, EM_EXGETSEL, 0, (LPARAM)&cr);
#else
				SendMessage(client, EM_GETSEL, (WPARAM)&cr.cpMin, (LPARAM)&cr.cpMax);
#endif
				if (cr.cpMin == cr.cpMax) {
					ERROROUT(GetString(IDS_NO_SELECTED_TEXT));
					break;
				}
				nSize = cr.cpMax - cr.cpMin + 1;

				szSrc = (LPTSTR) HeapAlloc(globalHeap, HEAP_ZERO_MEMORY, nSize * sizeof(TCHAR));
				szDest = (LPTSTR) HeapAlloc(globalHeap, HEAP_ZERO_MEMORY, nSize * sizeof(TCHAR));

#ifdef USE_RICH_EDIT
				SendMessage(client, EM_GETSELTEXT, 0, (LPARAM)szSrc);
#else
				GetClientRange(cr.cpMin, cr.cpMax, szSrc);
#endif
				switch (LOWORD(wParam)) {
				case ID_UNTABIFY:
				case ID_TABIFY:
					{
						CHARRANGE cr2 = cr;
						TCHAR szFnd[101], szRepl[101];

						if (LOWORD(wParam) == ID_UNTABIFY) {
							lstrcpy(szFnd, _T("\t"));
							memset(szRepl, _T(' '), options.nTabStops);
							szRepl[options.nTabStops] = _T('\0');
						}
						else {
							lstrcpy(szRepl, _T("\t"));
							memset(szFnd, _T(' '), options.nTabStops);
							szFnd[options.nTabStops] = _T('\0');
						}

						nReplaceMax = cr.cpMax;
						cr2.cpMax = cr2.cpMin;

#ifdef USE_RICH_EDIT
						SendMessage(client, EM_EXSETSEL, 0, (LPARAM)&cr2);
#else
						SendMessage(client, EM_SETSEL, (WPARAM)cr2.cpMin, (LPARAM)cr2.cpMax);
#endif
						SendMessage(client, WM_SETREDRAW, (WPARAM)FALSE, 0);
						bReplacingAll = TRUE;
						while (SearchFile(szFnd, FALSE, TRUE, TRUE, FALSE)) {
							SendMessage(client, EM_REPLACESEL, (WPARAM)TRUE, (LPARAM)szRepl);
							nReplaceMax -= lstrlen(szFnd) - lstrlen(szRepl);
						}
						bReplacingAll = FALSE;
						cr.cpMax = nReplaceMax;
						nReplaceMax = -1;
						SendMessage(client, WM_SETREDRAW, (WPARAM)TRUE, 0);
						InvalidateRect(hwnd, NULL, TRUE);
					}
					break;
				case ID_QUOTE:
					{
						LONG i, j, nLines = 1;
						INT nQuoteLen = lstrlen(options.szQuote);

						for (i = 0; i < nSize-1; ++i) {
#ifdef USE_RICH_EDIT
							if (szSrc[i] == _T('\r')) ++nLines;
#else
							if (szSrc[i] == _T('\n')) ++nLines;
#endif
						}

						HeapFree(globalHeap, 0, (HGLOBAL)szDest);
						szDest = (LPTSTR) HeapAlloc(globalHeap, HEAP_ZERO_MEMORY, nSize * sizeof(TCHAR) + nLines * nQuoteLen);

						lstrcpy(szDest, options.szQuote);
						for (i = 0, j = nQuoteLen; i < nSize-1; ++i) {
							szDest[j++] = szSrc[i];
#ifdef USE_RICH_EDIT
							if (szSrc[i] == _T('\r')) {
#else
							if (szSrc[i] == _T('\n')) {
#endif
								if (i == nSize - 2) {
									--nLines;
									break;
								}
								lstrcat(szDest, options.szQuote);
								j += nQuoteLen;
							}
						}

						cr.cpMax += nLines * nQuoteLen;
					}
					break;
				case ID_STRIP_CR_SPACE:
				case ID_STRIP_CR:
					{
						LONG i, j;
						for (i = 0, j = 0; i < nSize; ++i) {
#ifdef USE_RICH_EDIT
							if (szSrc[i] != _T('\r')) {
								szDest[j++] = szSrc[i];
							}
							else if (szSrc[i+1] == _T('\r')) {
								szDest[j++] = szSrc[i++];
								szDest[j++] = szSrc[i];
							}
							else if (LOWORD(wParam) == ID_STRIP_CR_SPACE){
								szDest[j++] = _T(' ');
							}
#else
							if (szSrc[i] != _T('\n') && szSrc[i] != _T('\r')) {
								szDest[j++] = szSrc[i];
							}
							else if (szSrc[i] == _T('\r') && szSrc[i+1] == _T('\n') && szSrc[i+2] == _T('\r') && szSrc[i+3] == _T('\n')) {
								szDest[j++] = szSrc[i++];
								szDest[j++] = szSrc[i++];
								szDest[j++] = szSrc[i++];
								szDest[j++] = szSrc[i];
							}
							else if (LOWORD(wParam) == ID_STRIP_CR_SPACE && szSrc[i] == _T('\n')) {
								szDest[j++] = _T(' ');
							}
#endif
						}
						cr.cpMax -= i - j;
					}
					break;
				case ID_STRIP_TRAILING_WS:
					{
						INT i, j;
						BOOL bStrip = TRUE;

						szDest[lstrlen(szSrc)] = _T('\0');
						for (i = lstrlen(szSrc)-1, j = i; i >= 0; --i) {

							if (bStrip) {
								if (szSrc[i] == _T('\t') || szSrc[i] == _T(' ')) {
									continue;
								}
								else if (szSrc[i] != _T('\r') && szSrc[i] != _T('\n')) {
									bStrip = FALSE;
								}
							}
							else {
								if (szSrc[i] == _T('\r')) {
									bStrip = TRUE;
								}
							}
							szDest[j--] = szSrc[i];
						}
						cr.cpMax -= j + 1;
						lstrcpy(szDest, szDest + j + 1);
					}
					break;
				case ID_HACKER:
					{
						LONG i;
						lstrcpy(szDest, szSrc);

						for (i = 0; i < nSize; i++) {
							if (_istascii(szDest[i])) {
								switch (szDest[i]) {
								case _T('a'):
								case _T('A'):
								szDest[i] = _T('4');
									break;
								case _T('T'):
								case _T('t'):
									szDest[i] = _T('7');
									break;
								case _T('E'):
								case _T('e'):
									szDest[i] = _T('3');
									break;
								case _T('L'):
								case _T('l'):
									szDest[i] = _T('1');
									break;
								case _T('S'):
								case _T('s'):
									szDest[i] = _T('5');
									break;
								case _T('G'):
								case _T('g'):
									szDest[i] = _T('6');
									break;
								case _T('O'):
								case _T('o'):
									szDest[i] = _T('0');
									break;
								}
							}
						}
					}
					break;
				case ID_MAKE_LOWER:
					lstrcpy(szDest, szSrc);
					CharLowerBuff(szDest, nSize);
					break;
				case ID_MAKE_UPPER:
					lstrcpy(szDest, szSrc);
					CharUpperBuff(szDest, nSize);
					break;
				case ID_MAKE_INVERSE:
					{
						LONG i;

						lstrcpy(szDest, szSrc);
						for (i = 0; i < nSize; i++) {
							if (IsCharAlpha(szDest[i])) {
								if (IsCharUpper(szDest[i])) {
									CharLowerBuff(szDest + i, 1);
									//szDest[i] = (TCHAR)_tolower(szDest[i]);
								}
								else if (IsCharLower(szDest[i])) {
									CharUpperBuff(szDest + i, 1);
									//szDest[i] = (TCHAR)_toupper(szDest[i]);
								}
							}
						}
					}
					break;
				case ID_MAKE_TITLE:
				case ID_MAKE_SENTENCE:
					{
						LONG i;
						BOOL bNextUpper = TRUE;

						lstrcpy(szDest, szSrc);
						for (i = 0; i < nSize; i++) {
							if (IsCharAlpha(szDest[i])) {
								if (bNextUpper) {
									bNextUpper = FALSE;
									if (IsCharLower(szDest[i])) {
										CharUpperBuff(szDest + i, 1);
										//szDest[i] = (TCHAR)_toupper(szDest[i]);
									}
								}
								else {
									if (IsCharUpper(szDest[i])) {
										CharLowerBuff(szDest + i, 1);
										//szDest[i] = (TCHAR)_tolower(szDest[i]);
									}
								}
							}
							else {
								if (LOWORD(wParam) == ID_MAKE_TITLE && szDest[i] != _T('\'')) {
									bNextUpper = TRUE;
								}
								else if (szDest[i] == _T('.')
									|| szDest[i] == _T('?')
									|| szDest[i] == _T('!')
									|| szDest[i] == _T('\r')) {
									bNextUpper = TRUE;
								}

							}
						}
					}
					break;
#ifndef BUILD_METAPAD_UNICODE
				case ID_MAKE_OEM:
					CharToOemBuff(szSrc, szDest, nSize);
					break;
				case ID_MAKE_ANSI:
					OemToCharBuff(szSrc, szDest, nSize);
					break;
#endif
				}

				if (LOWORD(wParam) != ID_UNTABIFY && LOWORD(wParam) != ID_TABIFY) {
					if (lstrcmp(szSrc, szDest) != 0) {
						SendMessage(client, EM_REPLACESEL, (WPARAM)TRUE, (LPARAM)szDest);
					}
				}
#ifdef USE_RICH_EDIT
				SendMessage(client, EM_EXSETSEL, 0, (LPARAM)&cr);
				InvalidateRect(client, NULL, TRUE);
#else
				SendMessage(client, EM_SETSEL, (WPARAM)cr.cpMin, (LPARAM)cr.cpMax);
#endif
				HeapFree(globalHeap, 0, (HGLOBAL)szDest);
				HeapFree(globalHeap, 0, (HGLOBAL)szSrc);
				SetCursor(hcur);
				break;
			}
			case ID_NEW_INSTANCE:
				{
					TCHAR szBuffer[MAXFN];
					GetModuleFileName(hinstThis, szBuffer, MAXFN);
					//ShellExecute(NULL, NULL, szBuffer, NULL, szDir, SW_SHOWNORMAL);
					ExecuteProgram(szBuffer, _T(""));
				}
				break;
			case ID_LAUNCH_ASSOCIATED_VIEWER:
				LaunchInViewer(FALSE, FALSE);
				break;
			case ID_UTF_8_FILE:
			case ID_UNICODE_FILE:
			case ID_UNICODE_BE_FILE:
			case ID_DOS_FILE:
			case ID_UNIX_FILE: {
				HMENU hmenu = GetSubMenu(GetSubMenu(GetMenu(hwndMain), 0), FILEFORMATPOS);

				bBinaryFile = FALSE;
				if (!bLoading) {
					if (LOWORD(wParam) == ID_UTF_8_FILE && nEncodingType == TYPE_UTF_8)
						break;
					else if (LOWORD(wParam) == ID_UNICODE_FILE && nEncodingType == TYPE_UTF_16)
						break;
					else if (LOWORD(wParam) == ID_UNICODE_BE_FILE && nEncodingType == TYPE_UTF_16_BE)
						break;
					else if (LOWORD(wParam) == ID_UNIX_FILE && bUnix && nEncodingType == TYPE_UNKNOWN)
						break;
					else if (LOWORD(wParam) == ID_DOS_FILE && !bUnix && nEncodingType == TYPE_UNKNOWN)
						break;
				}

				bUnix = LOWORD(wParam) == ID_UNIX_FILE;

				if (LOWORD(wParam) == ID_UTF_8_FILE)
					nEncodingType = TYPE_UTF_8;
				else if (LOWORD(wParam) == ID_UNICODE_FILE)
					nEncodingType = TYPE_UTF_16;
				else if (LOWORD(wParam) == ID_UNICODE_BE_FILE)
					nEncodingType = TYPE_UTF_16_BE;
				else
					nEncodingType = TYPE_UNKNOWN;

				CheckMenuRadioItem(hmenu, ID_DOS_FILE, ID_UTF_8_FILE, LOWORD(wParam), MF_BYCOMMAND);

				if (!bDirtyFile && !bLoading) {
					TCHAR szTmp[MAXFN];
					szTmp[0] = _T(' ');
					szTmp[1] = _T('*');
					szTmp[2] = _T(' ');
					GetWindowText(hwndMain, szTmp + 3, GetWindowTextLength(hwndMain) + 1);
					SetWindowText(hwndMain, szTmp);
					bDirtyFile = TRUE;
				}
				if (!bLoading) {
					bHideMessage = TRUE;
					UpdateStatus();
				}
				break;
			}
			case ID_RELOAD_CURRENT: {
				if (szFile[0] != _T('\0')) {
					CHARRANGE cr;

					if (!SaveIfDirty())
						break;

					bHideMessage = FALSE;
					lstrcpy(szStatusMessage, GetString(IDS_FILE_LOADING));
					UpdateStatus();

					bLoading = TRUE;

#ifdef USE_RICH_EDIT
					SendMessage(client, EM_EXGETSEL, 0, (LPARAM)&cr);
					LoadFile(szFile, FALSE, FALSE);
					SendMessage(client, EM_EXSETSEL, 0, (LPARAM)&cr);
#else
					SendMessage(client, EM_GETSEL, (WPARAM)&cr.cpMin, (LPARAM)&cr.cpMax);
					LoadFile(szFile, FALSE, FALSE);
					SendMessage(client, EM_SETSEL, (WPARAM)cr.cpMin, (LPARAM)cr.cpMax);
					SendMessage(client, EM_SCROLLCARET, 0, 0);
#endif

					UpdateStatus();
					if (bLoading) {
						/*
						TCHAR szBuffer[MAXFN];
						wsprintf(szBuffer, STR_CAPTION_FILE, szCaptionFile);
						SetWindowText(hwndMain, szBuffer);
						bLoading = FALSE;
						bDirtyFile = FALSE;
						*/
						bLoading = FALSE;
						bDirtyFile = FALSE;
						UpdateCaption();
					}
				}
				break;
			}
#ifdef USE_RICH_EDIT
			case ID_SHOWFILESIZE:
				{
					HCURSOR hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));

					bHideMessage = FALSE;
					wsprintf(szStatusMessage, GetString(IDS_BYTE_LENGTH), CalculateFileSize());
					UpdateStatus();
					if (!bShowStatus)
						SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(ID_SHOWSTATUS, 0), 0);
					SetCursor(hcur);
				}
				break;
#endif
			case ID_MRU_1:
			case ID_MRU_2:
			case ID_MRU_3:
			case ID_MRU_4:
			case ID_MRU_5:
			case ID_MRU_6:
			case ID_MRU_7:
			case ID_MRU_8:
			case ID_MRU_9:
			case ID_MRU_10:
			case ID_MRU_11:
			case ID_MRU_12:
			case ID_MRU_13:
			case ID_MRU_14:
			case ID_MRU_15:
			case ID_MRU_16:
				LoadFileFromMenu(LOWORD(wParam), TRUE);
				break;
			case ID_SET_MACRO_1:
			case ID_SET_MACRO_2:
			case ID_SET_MACRO_3:
			case ID_SET_MACRO_4:
			case ID_SET_MACRO_5:
			case ID_SET_MACRO_6:
			case ID_SET_MACRO_7:
			case ID_SET_MACRO_8:
			case ID_SET_MACRO_9:
			case ID_SET_MACRO_10: {
				CHARRANGE cr;
				int macroIndex = LOWORD(wParam) - ID_SET_MACRO_1;

#ifdef USE_RICH_EDIT
				SendMessage(client, EM_EXGETSEL, 0, (LPARAM)&cr);
#else
				SendMessage(client, EM_GETSEL, (WPARAM)&cr.cpMin, (LPARAM)&cr.cpMax);
#endif
				if (cr.cpMax - cr.cpMin > MAXMACRO - 1) {
					ERROROUT(GetString(IDS_MACRO_LENGTH_ERROR));
					break;
				}
#ifdef USE_RICH_EDIT
				SendMessage(client, EM_GETSELTEXT, 0, (LPARAM)options.MacroArray[macroIndex]);
#else
				GetClientRange(cr.cpMin, cr.cpMax, options.MacroArray[macroIndex]);
#endif

				if (!EncodeWithEscapeSeqs(options.MacroArray[macroIndex])) {
					ERROROUT(GetString(IDS_MACRO_LENGTH_ERROR));
					break;
				}
				else {
					HKEY key;

					if (!g_bIniMode) {
						RegCreateKeyEx(HKEY_CURRENT_USER, STR_REGKEY, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &key, NULL);
						RegSetValueEx(key, _T("MacroArray"), 0, REG_BINARY, (LPBYTE)&options.MacroArray, sizeof(options.MacroArray));
						RegCloseKey(key);
					}
					else {
						TCHAR entry[14];
						wsprintf(entry, _T("szMacroArray%d"), macroIndex);
						if (!SaveOption((HKEY)NULL, entry, REG_SZ, (LPBYTE)&options.MacroArray[macroIndex], MAXMACRO)) {
							ReportLastError();
						}
					}
				}

				break;
			}
			case ID_MACRO_1:
			case ID_MACRO_2:
			case ID_MACRO_3:
			case ID_MACRO_4:
			case ID_MACRO_5:
			case ID_MACRO_6:
			case ID_MACRO_7:
			case ID_MACRO_8:
			case ID_MACRO_9:
			case ID_MACRO_10: {
				TCHAR szMacro[MAXMACRO];
				lstrcpy(szMacro, options.MacroArray[LOWORD(wParam) - ID_MACRO_1]);
				ParseForEscapeSeqs(szMacro);
				SendMessage(client, EM_REPLACESEL, (WPARAM)TRUE, (LPARAM)szMacro);
#ifdef USE_RICH_EDIT
				InvalidateRect(client, NULL, TRUE);
#endif
				break;
			}
			case ID_COMMIT_WORDWRAP:
				if (bWordWrap) {
					LONG lFileSize;
					LONG lLineLen, lMaxLine;
					LPTSTR szBuffer;
					LONG i, iIndex;
					LPTSTR szTemp;
					CHARRANGE cr;
					HCURSOR hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));

					lFileSize = GetWindowTextLength(client);
					lMaxLine = SendMessage(client, EM_GETLINECOUNT, 0, 0);
					szBuffer = (LPTSTR) HeapAlloc(globalHeap, HEAP_ZERO_MEMORY, (lFileSize + lMaxLine + 1) * sizeof(TCHAR));

					for (i = 0; i < lMaxLine; i++) {
						iIndex = SendMessage(client, EM_LINEINDEX, (WPARAM)i, 0);
						lLineLen = SendMessage(client, EM_LINELENGTH, (WPARAM)iIndex, 0);

						szTemp = (LPTSTR) HeapAlloc(globalHeap, HEAP_ZERO_MEMORY, (lLineLen+3) * sizeof(TCHAR));
						*((LPWORD)szTemp) = (USHORT)(lLineLen + 1);
						SendMessage(client, EM_GETLINE, (WPARAM)i, (LPARAM)(LPCTSTR)szTemp);
						szTemp[lLineLen] = _T('\0');
						lstrcat(szBuffer, szTemp);
						if (i < lMaxLine - 1) {
#ifdef USE_RICH_EDIT
							lstrcat(szBuffer, _T("\r"));
#else
							lstrcat(szBuffer, _T("\r\n"));
#endif
						}
						HeapFree(globalHeap, 0, (HGLOBAL)szTemp);
					}

					cr.cpMin = 0;
					cr.cpMax = -1;
					SendMessage(client, WM_SETREDRAW, (WPARAM)FALSE, 0);

#ifdef USE_RICH_EDIT
					SendMessage(client, EM_EXSETSEL, 0, (LPARAM)&cr);
#else
					SendMessage(client, EM_SETSEL, (WPARAM)cr.cpMin, (LPARAM)cr.cpMax);
#endif
					SendMessage(client, EM_REPLACESEL, (WPARAM)TRUE, (LPARAM)szBuffer);
					cr.cpMax = 0;
#ifdef USE_RICH_EDIT
					SendMessage(client, EM_EXSETSEL, 0, (LPARAM)&cr);
#else
					SendMessage(client, EM_SETSEL, (WPARAM)cr.cpMin, (LPARAM)cr.cpMax);
#endif
					SendMessage(client, WM_SETREDRAW, (WPARAM)TRUE, 0);
					HeapFree(globalHeap, 0, (HGLOBAL)szBuffer);
					InvalidateRect(client, NULL, TRUE);
					SetCursor(hcur);
				}
				break;
			case ID_SHIFT_ENTER:
			case ID_CONTROL_SHIFT_ENTER:
#ifdef USE_RICH_EDIT
			{
				BYTE keys[256];
				GetKeyboardState(keys);
				keys[VK_SHIFT] &= 0x7F;
				SetKeyboardState(keys);
				SendMessage(client, WM_KEYDOWN, (WPARAM)VK_RETURN, 0);
				keys[VK_SHIFT] |= 0x80;
				SetKeyboardState(keys);
			}
#else
				SendMessage(client, WM_CHAR, (WPARAM)_T('\r'), 0);
#endif
				break;
			case ID_CONTEXTMENU:
				SendMessage(client, WM_RBUTTONUP, 0, 0);
				break;
			case ID_SCROLLUP:
				PostMessage(client, WM_VSCROLL, (WPARAM)SB_LINEUP, 0);
				break;
			case ID_SCROLLDOWN:
				PostMessage(client, WM_VSCROLL, (WPARAM)SB_LINEDOWN, 0);
				break;
			case ID_SCROLLLEFT:
				PostMessage(client, WM_HSCROLL, (WPARAM)SB_PAGELEFT, 0);
//				SendMessage(client, WM_SYSKEYUP, (WPARAM)VK_MENU, (LPARAM)0);
				break;
			case ID_SCROLLRIGHT:
				PostMessage(client, WM_HSCROLL, (WPARAM)SB_PAGERIGHT, 0);
//				SendMessage(client, WM_CHAR, (WPARAM)VK_MENU, (LPARAM)0);
				break;
			case ID_FAV_ADD:
				DialogBox(hinstLang, MAKEINTRESOURCE(IDD_FAV_NAME), hwndMain, (DLGPROC)AddFavDialogProc);
				break;
			case ID_FAV_EDIT: {
				TCHAR szBuffer[MAXFN];
				GetModuleFileName(hinstThis, szBuffer, MAXFN);
				//ShellExecute(NULL, NULL, szBuffer, szFav, szDir, SW_SHOWNORMAL);
				ExecuteProgram(szBuffer, szFav);
				break;
			}
			case ID_FAV_RELOAD:
				PopulateFavourites();
				break;
		}
		break;
	default:
		if (Msg == uFindReplaceMsg) {
			LPFINDREPLACE lpfr = (LPFINDREPLACE)lParam;
			TCHAR szBuffer[MAXFIND];
			int nIdx;
			BOOL bFinding = FALSE;
#ifdef USE_RICH_EDIT
			BOOL bFixCRProblem = FALSE;
			TCHAR szFindHold[MAXFIND];
			TCHAR szReplaceHold[MAXFIND];
#endif

			if (lpfr->Flags & FR_DIALOGTERM) {
				int i;
				if(bFindOpen)
					GetWindowPlacement(hdlgFind, &wpFindPlace);
				else if(bReplaceOpen)
					GetWindowPlacement(hdlgFind, &wpReplacePlace);
				for (i = 0; i < NUMFINDS; i++) {
					SendDlgItemMessage(hdlgFind, ID_DROP_FIND, CB_GETLBTEXT, i, (WPARAM)FindArray[i]);
					if (lpfr->lpstrReplaceWith != NULL)
						SendDlgItemMessage(hdlgFind, ID_DROP_REPLACE, CB_GETLBTEXT, i, (WPARAM)ReplaceArray[i]);
				}
				hdlgFind = NULL;
				bFindOpen = FALSE;
				bReplaceOpen = FALSE;
				return FALSE;
			}

			bFinding = (lstrcmp(lpfr->lpstrFindWhat, _T("dummy_find")) == 0);

			SendDlgItemMessage(hdlgFind, ID_DROP_FIND, WM_GETTEXT, MAXFIND, (WPARAM)szBuffer);
			lstrcpy(lpfr->lpstrFindWhat, szBuffer);

			nIdx = SendDlgItemMessage(hdlgFind, ID_DROP_FIND, CB_FINDSTRINGEXACT, 0, (WPARAM)szBuffer);
			if (nIdx == CB_ERR) {
				if (SendDlgItemMessage(hdlgFind, ID_DROP_FIND, CB_GETCOUNT, 0, 0) >= NUMFINDS) {
					SendDlgItemMessage(hdlgFind, ID_DROP_FIND, CB_DELETESTRING, (LPARAM)NUMFINDS-1, 0);
				}
			}
			else {
				SendDlgItemMessage(hdlgFind, ID_DROP_FIND, CB_DELETESTRING, (LPARAM)nIdx, 0);
			}
			SendDlgItemMessage(hdlgFind, ID_DROP_FIND, CB_INSERTSTRING, 0, (WPARAM)szBuffer);
			SendDlgItemMessage(hdlgFind, ID_DROP_FIND, CB_SETCURSEL, (LPARAM)0, 0);

			if (lpfr->lpstrReplaceWith != NULL) {
				nIdx = SendDlgItemMessage(hdlgFind, ID_DROP_REPLACE, CB_FINDSTRINGEXACT, 0, (WPARAM)lpfr->lpstrReplaceWith);
				if (nIdx == CB_ERR) {
					if (SendDlgItemMessage(hdlgFind, ID_DROP_REPLACE, CB_GETCOUNT, 0, 0) >= NUMFINDS) {
						SendDlgItemMessage(hdlgFind, ID_DROP_REPLACE, CB_DELETESTRING, (LPARAM)NUMFINDS-1, 0);
					}
				}
				else {
					SendDlgItemMessage(hdlgFind, ID_DROP_REPLACE, CB_DELETESTRING, (LPARAM)nIdx, 0);
				}
				SendDlgItemMessage(hdlgFind, ID_DROP_REPLACE, CB_INSERTSTRING, 0, (WPARAM)lpfr->lpstrReplaceWith);
				SendDlgItemMessage(hdlgFind, ID_DROP_REPLACE, CB_SETCURSEL, (LPARAM)0, 0);

				if (!bNoFindHidden) {
					ParseForEscapeSeqs(lpfr->lpstrReplaceWith);
				}
			}

			if (!bNoFindHidden) {
				ParseForEscapeSeqs(lpfr->lpstrFindWhat);
			}

			bMatchCase = (BOOL) (lpfr->Flags & FR_MATCHCASE);
			bDown = (BOOL) (lpfr->Flags & FR_DOWN);
			bWholeWord = (BOOL) (lpfr->Flags & FR_WHOLEWORD);
			if (lpfr->Flags & FR_REPLACEALL) {
				HCURSOR hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));
				UINT nCnt = 0;
				TCHAR szMsg[25];
				CHARRANGE cr, store = {0,0};
				BOOL bSelection = BST_CHECKED == SendDlgItemMessage(hdlgFind, IDC_RADIO_SELECTION, BM_GETCHECK, 0, 0);

				if (bSelection) {
#ifdef USE_RICH_EDIT
					SendMessage(client, EM_EXGETSEL, 0, (LPARAM)&cr);
#else
					SendMessage(client, EM_GETSEL, (WPARAM)&cr.cpMin, (LPARAM)&cr.cpMax);
#endif
					store.cpMax = cr.cpMax;
					store.cpMin = cr.cpMin;

					nReplaceMax = cr.cpMax;
					cr.cpMax = cr.cpMin;
				}
				else
					cr.cpMin = cr.cpMax = 0;

#ifdef USE_RICH_EDIT
				SendMessage(client, EM_EXSETSEL, 0, (LPARAM)&cr);
#else
				SendMessage(client, EM_SETSEL, (WPARAM)cr.cpMin, (LPARAM)cr.cpMax);
#endif
				SendMessage(client, WM_SETREDRAW, (WPARAM)FALSE, 0);
				bReplacingAll = TRUE;

#ifdef USE_RICH_EDIT // warning -- big kluge to fix problem in richedit re newlines!
				if (lpfr->lpstrFindWhat[lstrlen(lpfr->lpstrFindWhat) - 1] == _T('\r')) {
					if (lpfr->lpstrReplaceWith[lstrlen(lpfr->lpstrReplaceWith) - 1] != _T('\r')) {
						bFixCRProblem = TRUE;
					}
				}
#endif
				while (SearchFile(lpfr->lpstrFindWhat, bMatchCase, TRUE, TRUE, bWholeWord)) {
#ifdef USE_RICH_EDIT // warning -- big kluge!
					TCHAR ch[2] = {_T('\0'), _T('\0')};

					lstrcpy(szReplaceHold, lpfr->lpstrReplaceWith);

					if (bFixCRProblem) {
						SendMessage(client, EM_EXGETSEL, 0, (LPARAM)&cr);
						++cr.cpMax;
						SendMessage(client, EM_EXSETSEL, 0, (LPARAM)&cr);
						SendMessage(client, EM_GETSELTEXT, 0, (LPARAM)szFindHold);
						if (lstrlen(szFindHold) == cr.cpMax - cr.cpMin) {
							ch[0] = szFindHold[lstrlen(szFindHold) - 1];
							lstrcat(szReplaceHold, ch);
						}
					}

					SendMessage(client, EM_REPLACESEL, (WPARAM)TRUE, (LPARAM)szReplaceHold);

					if (bFixCRProblem) {
						SendMessage(client, EM_EXGETSEL, 0, (LPARAM)&cr);
						--cr.cpMin;
						--cr.cpMax;
						SendMessage(client, EM_EXSETSEL, 0, (LPARAM)&cr);
					}

#else
					SendMessage(client, EM_REPLACESEL, (WPARAM)TRUE, (LPARAM)lpfr->lpstrReplaceWith);
#endif
					if (bSelection)
						nReplaceMax -= lstrlen(lpfr->lpstrFindWhat) - lstrlen(lpfr->lpstrReplaceWith);
					nCnt++;
				}
				bReplacingAll = FALSE;
				nReplaceMax = -1;

				if (bSelection) {
					cr.cpMin = store.cpMin;
					cr.cpMax = store.cpMax + nCnt * (lstrlen(lpfr->lpstrReplaceWith) - lstrlen(lpfr->lpstrFindWhat));
				}

#ifdef USE_RICH_EDIT
				SendMessage(client, EM_EXSETSEL, 0, (LPARAM)&cr);
#else
				SendMessage(client, EM_SETSEL, (WPARAM)cr.cpMin, (LPARAM)cr.cpMax);
#endif

				SendMessage(client, WM_SETREDRAW, (WPARAM)TRUE, 0);

				InvalidateRect(client, NULL, TRUE);
				SetCursor(hcur);

				UpdateStatus();
				wsprintf(szMsg, GetString(IDS_ITEMS_REPLACED), nCnt);
				MessageBox(hdlgFind, szMsg, STR_METAPAD, MB_OK|MB_ICONINFORMATION);
			}
			else if (lpfr->Flags & FR_FINDNEXT) {
				SearchFile(lpfr->lpstrFindWhat, bMatchCase, FALSE, bDown, bWholeWord);
				if (bFinding) {
					bCloseAfterFind = (BST_CHECKED == SendDlgItemMessage(hdlgFind, IDC_CLOSE_AFTER_FIND, BM_GETCHECK, 0, 0));
					if (bCloseAfterFind) {
						int i;
						for (i = 0; i < NUMFINDS; i++) {
							SendDlgItemMessage(hdlgFind, ID_DROP_FIND, CB_GETLBTEXT, i, (WPARAM)FindArray[i]);
						}
						if (bFindOpen)
							GetWindowPlacement(hdlgFind, &wpFindPlace);
						else if (bReplaceOpen)
							GetWindowPlacement(hdlgFind, &wpReplacePlace);
						DestroyWindow(hdlgFind);
						hdlgFind = NULL;
						bFindOpen = FALSE;
						bReplaceOpen = FALSE;
					}
				}
			}
			else if (lpfr->Flags & FR_REPLACE) {
				CHARRANGE cr;
#ifdef USE_RICH_EDIT
				SendMessage(client, EM_EXGETSEL, 0, (LPARAM)&cr);
				cr.cpMax = cr.cpMin;
				SendMessage(client, EM_EXSETSEL, 0, (LPARAM)&cr);
#else
				SendMessage(client, EM_GETSEL, (WPARAM)&cr.cpMin, (LPARAM)&cr.cpMax);
				cr.cpMax = cr.cpMin;
				SendMessage(client, EM_SETSEL, (WPARAM)cr.cpMin, (LPARAM)cr.cpMax);
#endif

#ifdef USE_RICH_EDIT // warning -- big kluge!
				if (lpfr->lpstrFindWhat[lstrlen(lpfr->lpstrFindWhat) - 1] == _T('\r')) {
					if (lpfr->lpstrReplaceWith[lstrlen(lpfr->lpstrReplaceWith) - 1] != _T('\r')) {
						bFixCRProblem = TRUE;
					}
				}
				lstrcpy(szReplaceHold, lpfr->lpstrReplaceWith);

#endif
				if (SearchFile(lpfr->lpstrFindWhat, bMatchCase, FALSE, bDown, bWholeWord)) {
#ifdef USE_RICH_EDIT // warning -- big kluge!
					TCHAR ch[2] = {_T('\0'), _T('\0')};

					if (bFixCRProblem) {
						SendMessage(client, EM_EXGETSEL, 0, (LPARAM)&cr);
						++cr.cpMax;
						SendMessage(client, EM_EXSETSEL, 0, (LPARAM)&cr);
						SendMessage(client, EM_GETSELTEXT, 0, (LPARAM)szFindHold);
						if (lstrlen(szFindHold) == cr.cpMax - cr.cpMin) {
							ch[0] = szFindHold[lstrlen(szFindHold) - 1];
							lstrcat(szReplaceHold, ch);
						}
					}

					SendMessage(client, EM_REPLACESEL, (WPARAM)TRUE, (LPARAM)szReplaceHold);

					if (bFixCRProblem) {
						SendMessage(client, EM_EXGETSEL, 0, (LPARAM)&cr);
						--cr.cpMin;
						--cr.cpMax;
						SendMessage(client, EM_EXSETSEL, 0, (LPARAM)&cr);
					}

					InvalidateRect(client, NULL, TRUE);
#else
					SendMessage(client, EM_REPLACESEL, (WPARAM)TRUE, (LPARAM)lpfr->lpstrReplaceWith);
#endif
					SearchFile(lpfr->lpstrFindWhat, bMatchCase, FALSE, bDown, bWholeWord);
				}
			}
			SendMessage(client, EM_SCROLLCARET, 0, 0);

			return FALSE;
		}
		return DefWindowProc(hwndMain, Msg, wParam, lParam);
	}
	return FALSE;
}

DWORD WINAPI LoadThread(LPVOID lpParameter)
{
	LoadFile(szFile, TRUE, TRUE);
	if (bLoading) {
		bLoading = FALSE;
		if (lstrlen(szFile) == 0) {
			MakeNewFile();
		}
#ifdef USE_RICH_EDIT
		else {
			if (lpParameter != NULL) {
				GotoLine(((CHARRANGE*)lpParameter)->cpMin, ((CHARRANGE*)lpParameter)->cpMax);
			}
			else {
				SendMessage(client, EM_SCROLLCARET, 0, 0);
			}
		}
#endif
		UpdateCaption();
	}
	else {
		MakeNewFile();
	}
	SendMessage(client, EM_SETREADONLY, (WPARAM)FALSE, 0);

#ifdef USE_RICH_EDIT
	if (!bWordWrap && !options.bHideScrollbars) { // Hack for initially drawing hscrollbar for Richedit
		SetWindowLongPtr(client, GWL_STYLE, GetWindowLongPtr(client, GWL_STYLE) | WS_HSCROLL);
		SetWindowPos(client, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
	}
#endif

	EnableWindow(hwnd, TRUE);
	SendMessage(hwnd, WM_ACTIVATE, 0, 0);
	return 0;
}

#if defined(__MINGW32__) && !defined(__MINGW64_VERSION_MAJOR)
#include "mingw-unicode-gui.c"
#endif
int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	WNDCLASS wc;
	MSG msg;
	HACCEL accel = NULL;
	int left, top, width, height;
	HMENU hmenu;
	MENUITEMINFO mio;
	CHARRANGE crLineCol = {-1, -1};
	LPTSTR szCmdLine;
	BOOL bSkipLanguagePlugin = FALSE;

	if (!hPrevInstance) {
		wc.hbrBackground = (HBRUSH)(COLOR_WINDOW);
		wc.style = /*CS_BYTEALIGNWINDOW CS_VREDRAW | CS_HREDRAW;*/ 0;
		wc.lpfnWndProc = MainWndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = hInstance;
		wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PAD));
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.lpszMenuName = /*MAKEINTRESOURCE(IDR_MENU)*/0;
		wc.lpszClassName = STR_METAPAD;
		if (!RegisterClass(&wc)) {
			ReportLastError();
			return FALSE;
		}
	}

	hinstThis = hInstance;
	globalHeap = GetProcessHeap();

#if defined(BUILD_METAPAD_UNICODE) && ( !defined(__MINGW32__) || defined(__MINGW64_VERSION_MAJOR) )
	szCmdLine = GetCommandLine();
	szCmdLine = _tcschr(szCmdLine, _T(' ')) + 1;
#else
	szCmdLine = lpCmdLine;
#endif

	{
		TCHAR* pch;
		GetModuleFileName(hinstThis, szMetapadIni, MAXFN);

		pch = _tcsrchr(szMetapadIni, _T('\\'));
		++pch;
		*pch = _T('\0');
		lstrcat(szMetapadIni, _T("metapad.ini"));
	}

	if (lstrlen(szCmdLine) > 0) {
		int nCmdLen = lstrlen(szCmdLine);
		if (nCmdLen > 1 && szCmdLine[0] == _T('/')) {

			TCHAR chOption = szCmdLine[1];
			if (chOption == _T('s') || chOption == _T('S')) {
				bSkipLanguagePlugin = TRUE;
				szCmdLine += 2;
				if (szCmdLine[0] == _T(' ')) ++szCmdLine;
			}
			else if (chOption == _T('v') || chOption == _T('V')) {
				g_bDisablePluginVersionChecking = TRUE;
				szCmdLine += 2;
				if (szCmdLine[0] == _T(' ')) ++szCmdLine;
			}
			else if (chOption == _T('i') || chOption == _T('I')) {
				g_bIniMode = TRUE;
				szCmdLine += 2;
				if (szCmdLine[0] == _T(' ')) ++szCmdLine;
			}
			else if (chOption == _T('m') || chOption == _T('M')) {
				LoadOptions();
				if( options.bSaveWindowPlacement || options.bStickyWindow ) {
					int left, top, width, height, nShow;
					LoadWindowPlacement(&left, &top, &width, &height, &nShow);
					SaveOption(NULL, _T("w_WindowState"), REG_DWORD, (LPBYTE)&nShow, sizeof(int));
					SaveOption(NULL, _T("w_Left"), REG_DWORD, (LPBYTE)&left, sizeof(int));
					SaveOption(NULL, _T("w_Top"), REG_DWORD, (LPBYTE)&top, sizeof(int));
					SaveOption(NULL, _T("w_Width"), REG_DWORD, (LPBYTE)&width, sizeof(int));
					SaveOption(NULL, _T("w_Height"), REG_DWORD, (LPBYTE)&height, sizeof(int));
				}
				g_bIniMode = TRUE;
				SaveOptions();
				MSGOUT(_T("Migration to INI completed."));
				return FALSE;
			}
		}
	}

	if (!g_bIniMode) {
		WIN32_FIND_DATA FindFileData;
		HANDLE handle;

		if ((handle = FindFirstFile(szMetapadIni, &FindFileData)) != INVALID_HANDLE_VALUE) {
			FindClose(handle);
			g_bIniMode = TRUE;
		}
	}
	GetCurrentDirectory(MAXFN, szDir);
	LoadOptions();

	options.nStatusFontWidth = LOWORD(GetDialogBaseUnits());

	bWordWrap = FALSE;
	bPrimaryFont = TRUE;
	bSmartSelect = TRUE;
#ifdef USE_RICH_EDIT
	bHyperlinks = TRUE;
#endif
	bShowStatus = TRUE;
	bShowToolbar = TRUE;
	bAlwaysOnTop = FALSE;
	bCloseAfterFind = FALSE;
	bNoFindHidden = TRUE;
	bTransparent = FALSE;

	lstrcpy(szCustomFilter, GetString(IDS_DEFAULT_FILTER));
	LoadMenusAndData();

	FixFilterString(szCustomFilter);

	if (options.bSaveWindowPlacement || options.bStickyWindow)
		LoadWindowPlacement(&left, &top, &width, &height, &nCmdShow);
	else {
		left = top = width = height = CW_USEDEFAULT;
		nCmdShow = SW_SHOWNORMAL;
	}

	{
		TCHAR szBuffer[100];
		wsprintf(szBuffer, STR_CAPTION_FILE, GetString(IDS_NEW_FILE));

		hwnd = CreateWindowEx(
			WS_EX_ACCEPTFILES,
			STR_METAPAD,
			szBuffer,
			WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
			left, top, width, height,
			NULL,
			NULL,
			hInstance,
			NULL);
	}

	if (!hwnd) {
		ReportLastError();
		return FALSE;
	}

	if (!bSkipLanguagePlugin) {
		FindAndLoadLanguagePlugin();
	}

	{
		HMENU hm = LoadMenu(hinstLang, MAKEINTRESOURCE(IDR_MENU));

		if (hm == NULL) {
			ReportLastError();
			return FALSE;
		}

#ifndef USE_RICH_EDIT
		{
			HMENU hsub = GetSubMenu(hm, 0);

			DeleteMenu(hsub, 10, MF_BYPOSITION);
			hsub = GetSubMenu(hm, 3);
			DeleteMenu(hsub, 8, MF_BYPOSITION);
		}
#endif

		SetMenu(hwnd, hm);

		if (hinstLang != hinstThis) {
			HMENU hsub = GetSubMenu(hm, 4);
			MENUITEMINFO mio;

			mio.cbSize = sizeof(MENUITEMINFO);
			mio.fMask = MIIM_TYPE | MIIM_ID;
			mio.fType = MFT_STRING;
			mio.wID = ID_ABOUT_PLUGIN;
			mio.dwTypeData = GetString(IDS_MENU_LANGUAGE_PLUGIN);
			InsertMenuItem(hsub, 1, TRUE, &mio);
		}
	}


	{
		HMODULE hm;

		hm = GetModuleHandle(_T("user32.dll"));
		SetLWA = (SLWA)(GetProcAddress(hm, "SetLayeredWindowAttributes"));

		if (SetLWA) {
			SetLWA(hwnd, 0, 255, LWA_ALPHA);
		}
		else {
			HMENU hsub = GetSubMenu(GetMenu(hwnd), 3);
			DeleteMenu(hsub, 4, MF_BYPOSITION);
		}
	}

	accel = LoadAccelerators(hinstLang, MAKEINTRESOURCE(IDR_ACCELERATOR));
	if (!accel) {
		ReportLastError();
		return FALSE;
	}

#ifdef USE_RICH_EDIT
	if (LoadLibrary(STR_RICHDLL) == NULL) {
		ReportLastError();
		ERROROUT(GetString(IDS_RICHED_MISSING_ERROR));
		return FALSE;
	}
#endif

	CreateClient(hwnd, NULL, bWordWrap);
	if (!client) {
		ReportLastError();
		return FALSE;
	}
	SetClientFont(bPrimaryFont);

	uFindReplaceMsg = RegisterWindowMessage(FINDMSGSTRING);

	hmenu = GetMenu(hwnd);
	mio.cbSize = sizeof(MENUITEMINFO);
	mio.fMask = MIIM_STATE;
	if (bWordWrap) {
		mio.fState = MFS_CHECKED;
		SetMenuItemInfo(hmenu, ID_EDIT_WORDWRAP, 0, &mio);
	}
	if (!bSmartSelect) {
		mio.fState = MFS_UNCHECKED;
		SetMenuItemInfo(hmenu, ID_SMARTSELECT, 0, &mio);
	}
#ifdef USE_RICH_EDIT
	if (!bHyperlinks) {
		mio.fState = MFS_UNCHECKED;
		SetMenuItemInfo(hmenu, ID_SHOWHYPERLINKS, 0, &mio);
	}
#endif
	if (bTransparent && SetLWA) {
		mio.fState = MFS_CHECKED;
		SetMenuItemInfo(hmenu, ID_TRANSPARENT, 0, &mio);
		SetWindowLongPtr(hwnd, GWL_EXSTYLE, GetWindowLongPtr(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);
		SetLWA(hwnd, 0, (BYTE)((255 * (100 - options.nTransparentPct)) / 100), LWA_ALPHA);
	}
	if (bShowStatus) {
		mio.fState = MFS_CHECKED;
		SetMenuItemInfo(hmenu, ID_SHOWSTATUS, 0, &mio);
		CreateStatusbar();
	}
	if (bShowToolbar) {
		mio.fState = MFS_CHECKED;
		SetMenuItemInfo(hmenu, ID_SHOWTOOLBAR, 0, &mio);
		CreateToolbar();
	}
	if (!bPrimaryFont) {
		mio.fState = MFS_UNCHECKED;
		SetMenuItemInfo(hmenu, ID_FONT_PRIMARY, 0, &mio);
	}
	if (bAlwaysOnTop) {
		mio.fState = MFS_CHECKED;
		SetMenuItemInfo(hmenu, ID_ALWAYSONTOP, 0, &mio);
		SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	}

	bDirtyFile = FALSE;
	bDown = TRUE;
	bWholeWord = FALSE;
	bReplacingAll = FALSE;
	nReplaceMax = -1;
	bInsertMode = TRUE;

	InitCommonControls();

	if (options.bRecentOnOwn)
		FixMRUMenus();

	if (!options.bReadOnlyMenu)
		FixReadOnlyMenu();

	PopulateMRUList();

	if (options.bNoFaves) {
		DeleteMenu(hmenu, FAVEPOS, MF_BYPOSITION);
		SetWindowPos(hwnd, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
	}
	else {
		WIN32_FIND_DATA FindFileData;
		HANDLE handle;

		if (options.szFavDir[0] == _T('\0') || (handle = FindFirstFile(options.szFavDir, &FindFileData)) == INVALID_HANDLE_VALUE) {
			TCHAR* pch;
			GetModuleFileName(hInstance, szFav, MAXFN);

			pch = _tcsrchr(szFav, _T('\\'));
			++pch;
			*pch = _T('\0');
		}
		else {
			FindClose(handle);
			lstrcpy(szFav, options.szFavDir);
			lstrcat(szFav, _T("\\"));
		}

		lstrcat(szFav, STR_FAV_FILE);

		PopulateFavourites();
	}

	MakeNewFile();

	if (lstrlen(szCmdLine) > 0) {
		int nCmdLen;

		nCmdLen = lstrlen(szCmdLine);

		if (nCmdLen > 1 && szCmdLine[0] == _T('/')) {
			TCHAR chOption = szCmdLine[1];

			if (chOption == _T('p') || chOption == _T('P')) {
				if (szCmdLine[3] == _T('\"') && szCmdLine[nCmdLen - 1] == _T('\"'))
					lstrcpyn(szFile, szCmdLine + 4, nCmdLen - 1);
				else
					lstrcpyn(szFile, szCmdLine + 3, nCmdLen + 1);

				LoadFile(szFile, FALSE, FALSE);
				lstrcpy(szCaptionFile, szFile);
				PrintContents();
				CleanUp();
				return TRUE;
			}
			else if (chOption == _T('g') || chOption == _T('G')) {
				TCHAR szNum[6];
				int nRlen, nClen;

				ZeroMemory(szNum, sizeof(szNum));
				for (nRlen = 0; _istdigit(szCmdLine[nRlen + 3]); nRlen++)
					szNum[nRlen] = szCmdLine[nRlen + 3];
				crLineCol.cpMin = _ttol(szNum);

				ZeroMemory(szNum, sizeof(szNum));
				for (nClen = 0; _istdigit(szCmdLine[nClen + nRlen + 4]); nClen++)
					szNum[nClen] = szCmdLine[nRlen + nClen + 4];
				crLineCol.cpMax = _ttol(szNum);

				if (szCmdLine[5 + nClen + nRlen] == _T('\"') && szCmdLine[nCmdLen - 1] == _T('\"'))
					lstrcpyn(szFile, szCmdLine + nClen + nRlen + 6, nCmdLen - 1);
				else
					lstrcpyn(szFile, szCmdLine + nClen + nRlen + 5, nCmdLen + 1);
			}
			else if (chOption == _T('e') || chOption == _T('E')) {
				if (szCmdLine[3] == _T('\"') && szCmdLine[nCmdLen - 1] == _T('\"'))
					lstrcpyn(szFile, szCmdLine + 4, nCmdLen - 1);
				else
					lstrcpyn(szFile, szCmdLine + 3, nCmdLen + 1);

				crLineCol.cpMax = crLineCol.cpMin = 0x7fffffff;
			}
			else {
				ERROROUT(GetString(IDS_COMMAND_LINE_OPTIONS));
				CleanUp();
				return TRUE;
			}
		}
		else {
			//ERROROUT(szCmdLine);
			if (szCmdLine[0] == _T('\"')) {
				lstrcpyn(szFile, szCmdLine + 1, _tcschr(szCmdLine+1, _T('\"')) - szCmdLine);
			}
			else {
				lstrcpyn(szFile, szCmdLine, nCmdLen + 1);
			}
		}
		bLoading = TRUE;

		GetFullPathName(szFile, MAXFN, szFile, NULL);

		ExpandFilename(szFile);
#ifdef USE_RICH_EDIT
		{
			DWORD dwID;
			TCHAR szBuffer[MAXFN];

			wsprintf(szBuffer, STR_CAPTION_FILE, szCaptionFile);
			SetWindowText(hwnd, szBuffer);
			SendMessage(client, EM_SETREADONLY, (WPARAM)TRUE, 0);
			ShowWindow(hwnd, nCmdShow);

			EnableWindow(hwnd, FALSE);
			hthread = CreateThread(NULL, 0, LoadThread, (LPVOID)&(crLineCol), 0, &dwID);
		}
#else
		LoadThread(NULL);
		if (bQuitApp)
			PostQuitMessage(0);
		else
			ShowWindow(hwnd, nCmdShow);

		SendMessage(client, EM_SCROLLCARET, 0, 0);
		GotoLine(crLineCol.cpMin, crLineCol.cpMax);
#endif
	}
	else {
		ShowWindow(hwnd, nCmdShow);
	}

#ifdef USE_RICH_EDIT
	if (!bWordWrap && !options.bHideScrollbars) { // Hack for initially drawing hscrollbar for Richedit
		SetWindowLongPtr(client, GWL_STYLE, GetWindowLongPtr(client, GWL_STYLE) | WS_HSCROLL);
		SetWindowPos(client, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
	}
#endif

	UpdateStatus();

	SendMessage(client, EM_SETMARGINS, (WPARAM)EC_LEFTMARGIN, MAKELPARAM(options.nSelectionMarginWidth, 0));
	SetFocus(client);

	while (GetMessage(&msg, NULL, 0,0)) {
		if (!(hdlgFind && IsDialogMessage(hdlgFind, &msg)) && !TranslateAccelerator(hwnd, accel, &msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return msg.wParam;
}
