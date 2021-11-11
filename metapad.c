/****************************************************************************/
/*                                                                          */
/*   metapad 3.6+                                                           */
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

#ifdef _MSC_VER
#pragma intrinsic(memset)
#endif

//#pragma comment(linker, "/OPT:NOWIN98" )

#include "include/consts.h"
#include "include/strings.h"
#include "include/macros.h"
#include "include/typedefs.h"
#include "include/language_plugin.h"
#include "include/external_viewers.h"
#include "include/settings_save.h"
#include "include/settings_load.h"
#include "include/file_load.h"
#include "include/file_new.h"
#include "include/file_save.h"
#include "include/file_utils.h"
#include "include/encoding.h"

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
DWORD frDlgId = -1;
HMENU hrecentmenu = NULL;
HFONT hfontmain = NULL;
HFONT hfontfind = NULL;
BOOL g_bIniMode = FALSE;
LPTSTR szInsert = NULL;
TCHAR gTmpBuf[MAX(MAX(MAXFN,MAXMACRO),MAX(MAXFIND,MAXINSERT))+MAXSTRING], gTmpBuf2[MAX(MAXFN,MAXMACRO)];
TCHAR gDummyBuf[1], gDummyBuf2[1];
#ifdef USE_RICH_EDIT
	TCHAR gTmpBuf3[MAXFIND], gTmpBuf4[MAXFIND];
#endif
int _fltused = 0x9875; // see CMISCDAT.C for more info on this

option_struct options;

#ifdef _DEBUG
	size_t __stktop;
	void __stkchk(){
		int x = 0;
		TCHAR m[32];
		wsprintf(m, _T("Stack: %d"), __stktop - (size_t)&x);
		MSGOUT(m);
	}
#endif

///// Implementation /////

LPTSTR GetString(UINT uID)
{
	LoadString(hinstLang, uID, _szString, MAXSTRING);
	return _szString;
}

BOOL EncodeWithEscapeSeqs(TCHAR* szText)
{
	TCHAR* szStore = gTmpBuf2;
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

BOOL ParseForEscapeSeqs(LPTSTR buf, LPBYTE anys, LPCTSTR errContext)
{
	LPTSTR op = buf, bout, end, szErr, szErr2, szErr3, dbuf;
	INT l, m, base = 0, mul = 3, uni = 0, str, expl = 0, dbufalloc = 0;
	DWORD p = 0;

	if (!SCNUL(buf)[0]) return TRUE;
	for (bout = buf; *buf; buf++, base = uni = expl = 0, p++) {
		if (*buf == _T('\\')) {
			switch(*++buf){
			case _T('n'):
#ifdef USE_RICH_EDIT
				*bout++ = _T('\r');
#else
				*bout++ = _T('\r');
				*bout++ = _T('\n');
#endif
				continue;
			case _T('R'): *bout++ = _T('\r'); continue;
			case _T('N'): *bout++ = _T('\n'); continue;
			case _T('t'): *bout++ = _T('\t'); continue;
			case _T('a'): *bout++ = _T('\a'); continue;
			case _T('e'): *bout++ = _T('\x1e'); continue;
			case _T('f'): *bout++ = _T('\f'); continue;
			case _T('v'): *bout++ = _T('\v'); continue;
			case _T('?'):
#ifdef UNICODE
				*bout++ = _T('\xFFFD');
#else
				*bout++ = _T('?');
#endif
				if (anys)
					anys[p] = 1;
				continue;
			case _T('U'):
			case _T('u'): uni = 1; 
			case _T('X'):
			case _T('x'): if (!base) { base = 16; mul = 2; } if (!uni) uni = 2;
			case _T('W'):
			case _T('w'): if (!uni) uni = 1;
			case _T('S'): 
			case _T('s'): if (!base) { base = 64; mul = 4; }
			case _T('B'):
			case _T('b'): if (!base) { base = 2; mul = 8; }
			case _T('D'):
			case _T('d'): if (!base) base = 10;
			case _T('O'):
			case _T('o'): buf++; expl = 1;
			default:	  if (!base) base = 8;
				str = (expl && (*(buf-1) & _T('_')) == *(buf-1));
				end = buf;
				if (uni == 1) mul *= 2;
				if (sizeof(TCHAR) >= 2 && uni != 1) {
					if (!dbufalloc) {
						dbuf = (LPTSTR)HeapAlloc(globalHeap, 0, (MAXFIND + 2) * sizeof(TCHAR));
						dbufalloc = 1;
					}
				} else
					dbuf = bout;
				l = DecodeBase(base, buf, (LPBYTE)dbuf, str ? -1 : mul, 1, str && base != 64 ? 0 : 1, FALSE, &end);
				if (l > 0) {
					m = l;
					if (sizeof(TCHAR) >= 2 && uni == 1){
						while (m % sizeof(TCHAR))
							*((LPBYTE)(dbuf)+(m++)) = 0;
						m /= sizeof(TCHAR);
					}
					buf = end;
					if (str && *buf && *buf != '\\') 
						l = -1;
#ifdef UNICODE
					else if (sizeof(TCHAR) >= 2 && uni != 1 && !(m = MultiByteToWideChar(CP_ACP, 0, (LPCSTR)dbuf, m, bout, m * sizeof(TCHAR))))
						l = -3;
#endif
					else if (str && uni == 1 && l % sizeof(TCHAR))
						l = -2;
					else if (uni == 1 && l >= sizeof(TCHAR) && nEncodingType == TYPE_UTF_16_BE)
						ReverseBytes((LPBYTE)bout, l * sizeof(TCHAR));
					if (!str) buf--;
				} else if (l == -2 && str && *end && *end != '\\') l = -1;
				if (l > 0) {
					
#ifdef UNICODE
					for (; m; m--, bout++) {
						if (*bout == _T('\0'))
							*bout = _T('\x2400');
					}
#else
					bout += m;
#endif
					p += m-1;
					if (str && !*buf) buf--;
					continue;
				}
				if (l == 0 && !expl) break;
				szErr2 = (LPTSTR)HeapAlloc(globalHeap, 0, (MAXSTRING * 2 + 32) * sizeof(TCHAR));
				szErr3 = szErr2 + MAXSTRING * 2;
				lstrcpy(szErr3, SCNUL(errContext));
				switch(l) {
					case 0: szErr = GetString(IDS_ESCAPE_EXPECTED); break;
					case -1: szErr = GetString(IDS_ESCAPE_BADCHARS); break;
					case -2: szErr = GetString(IDS_ESCAPE_BADALIGN); break;
					case -3: szErr = GetString(IDS_UNICODE_CONV_ERROR); break;
					default: szErr = GetString(IDS_ERROR); break;
				}
				wsprintf(szErr2, szErr, base, mul);
				szErr = szErr2 + MAXSTRING;
				wsprintf(szErr, GetString(IDS_ESCAPE_ERROR), szErr2, szErr3);
				lstrcat(szErr, _T("\n'"));
				lstrcpyn(szErr3, &op[MAX(0, end-op-15)], MIN(end-op, 15)+1);
				lstrcat(szErr, szErr3);
				lstrcat(szErr, _T(" *** "));
				lstrcpyn(szErr3, end, 16);
				lstrcat(szErr, szErr3);
				lstrcat(szErr, _T("'"));
				ERROROUT(szErr);
				FREE(szErr2);
				if (dbufalloc) FREE(dbuf);
				return FALSE;
			}
		}
		*bout++ = *buf;
	}
	*bout = _T('\0');
	if (dbufalloc) FREE(dbuf);
	return TRUE;
	// abcdefghijklmnopqrstuvwxyz ?
	// xM Mxx       XM  XMxMxMM  Mx
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

/**
 * Updates window title to reflect current working file's name and state.
 */
void UpdateCaption(void)
{
	TCHAR* szBuffer = gTmpBuf;

	ExpandFilename(szFile, &szFile);

	if (bDirtyFile) {
		szBuffer[0] = _T(' ');
		szBuffer[1] = _T('*');
		szBuffer[2] = _T(' ');
		wsprintf(szBuffer+3, STR_CAPTION_FILE, SCNUL(szCaptionFile));
	}
	else
		wsprintf(szBuffer, STR_CAPTION_FILE, SCNUL(szCaptionFile));

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
	FREE(szShadow);
	if (hrecentmenu) DestroyMenu(hrecentmenu);
	if (hfontmain) DeleteObject(hfontmain);
	if (hfontfind) DeleteObject(hfontfind);
	if (hthread) CloseHandle(hthread);

	if (hinstLang != hinstThis) FreeLibrary(hinstLang);

#ifdef USE_RICH_EDIT
	DestroyWindow(client);
	FreeLibrary(GetModuleHandle(STR_RICHDLL));
#else
	if (BackBrush) DeleteObject(BackBrush);
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
	static TCHAR szPane[50];
	static int lastw = 0;
	LONG lLine, lLineIndex, lLines;
	int nPaneSizes[NUMPANES];
	LPTSTR szBuffer;
	long lLineLen, lCol = 1;
	int i = 0;
	CHARRANGE cr;
	RECT rc;

	if (toolbar && bShowToolbar) {

#ifdef USE_RICH_EDIT
		SendMessage(client, EM_EXGETSEL, 0, (LPARAM)&cr);
#else
		SendMessage(client, EM_GETSEL, (WPARAM)&cr.cpMin, (LPARAM)&cr.cpMax);
#endif
		SendMessage(toolbar, TB_CHECKBUTTON, (WPARAM)ID_EDIT_WORDWRAP, MAKELONG(bWordWrap, 0));
		SendMessage(toolbar, TB_CHECKBUTTON, (WPARAM)ID_FONT_PRIMARY, MAKELONG(bPrimaryFont, 0));
		SendMessage(toolbar, TB_CHECKBUTTON, (WPARAM)ID_ALWAYSONTOP, MAKELONG(bAlwaysOnTop, 0));
		SendMessage(toolbar, TB_ENABLEBUTTON, (WPARAM)ID_RELOAD_CURRENT, MAKELONG(SCNUL(szFile)[0] != _T('\0'), 0));
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
		lstrcpy(szPane, _T("  BIN"));
	}
	else if (nEncodingType == TYPE_UTF_8) {
		lstrcpy(szPane, bUnix ? _T(" UTF-8 UNIX") : _T(" UTF-8"));
		nPaneSizes[SBPANE_TYPE] = (bUnix ? 9 : 5) * options.nStatusFontWidth + 4;
	}
	else if (nEncodingType == TYPE_UTF_16) {
		lstrcpy(szPane, _T(" Unicode"));
		nPaneSizes[SBPANE_TYPE] = 6 * options.nStatusFontWidth + 4;
	}
	else if (nEncodingType == TYPE_UTF_16_BE) {
		lstrcpy(szPane, _T(" Unicode BE"));
		nPaneSizes[SBPANE_TYPE] = 8 * options.nStatusFontWidth + 4;
	}
	else if (bUnix) {
		lstrcpy(szPane, _T("UNIX"));
	}
	else {
		lstrcpy(szPane, _T(" DOS"));
	}

	SendMessage(status, WM_SETREDRAW, FALSE, 0);
	SendMessage(status, SB_SETTEXT, (WPARAM) SBPANE_TYPE, (LPARAM)(LPTSTR)szPane);

	lLines = SendMessage(client, EM_GETLINECOUNT, 0, 0);

#ifdef USE_RICH_EDIT
	if (bInsertMode)
		lstrcpyf(szPane, _T(" INS"));
	else
		lstrcpy(szPane, _T("OVR"));
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

	if (lLineLen = SendMessage(client, EM_LINELENGTH, (WPARAM)cr.cpMax, 0)) {
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
		FREE(szBuffer);
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

	szBuffer = (bHideMessage ? _T("") : SCNUL(szStatusMessage));
	nPaneSizes[SBPANE_MESSAGE] = nPaneSizes[SBPANE_MESSAGE - 1] + (int)((options.nStatusFontWidth/STATUS_FONT_CONST) * lstrlen(szBuffer) + 5);
	SendMessage(status, SB_SETTEXT, (WPARAM) SBPANE_MESSAGE | SBT_NOBORDERS, (LPARAM)szBuffer);

	SendMessage(status, SB_SETPARTS, NUMPANES, (DWORD_PTR)(LPINT)nPaneSizes);
	SendMessage(status, WM_SETREDRAW, TRUE, 0);
	rc.top = 0; rc.left = 0; rc.bottom = 32; rc.right = MAX(nPaneSizes[SBPANE_MESSAGE], lastw);
	if (lastw > nPaneSizes[SBPANE_MESSAGE]) lastw-=2;
	else lastw = nPaneSizes[SBPANE_MESSAGE];
	InvalidateRect(status, &rc, TRUE);
}

LRESULT APIENTRY FindProc(HWND hwndFind, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	HMENU hmenu, hsub;
	BOOL bOkEna = TRUE;
	//printf("%X %X %X\n", uMsg, wParam, lParam);
	switch (uMsg) {
	case WM_COMMAND:
		switch(LOWORD(wParam)){
		case IDC_ESCAPE:
		case IDC_ESCAPE2: {
			RECT rect;
			UINT id;
			TCHAR* szText = gTmpBuf;
			DWORD i, j = 0;
			if (HIWORD(wParam) != BN_CLICKED) break;
			hmenu = LoadMenu(hinstLang, (LPCTSTR)IDR_ESCAPE_SEQUENCES);
			hsub = GetSubMenu(hmenu, 0);

			SendMessage(hwnd, WM_INITMENUPOPUP, (WPARAM)hsub, MAKELPARAM(1, FALSE));
			switch(LOWORD(wParam)){
				case IDC_ESCAPE:
					GetWindowRect(GetDlgItem(hwndFind, IDC_ESCAPE), &rect);
					SendDlgItemMessage(hwndFind, (frDlgId == ID_FIND || frDlgId == ID_REPLACE? ID_DROP_FIND : ID_DROP_INSERT), WM_GETTEXT, (WPARAM)MAXFIND, (LPARAM)szText);
					break;
				case IDC_ESCAPE2:
					GetWindowRect(GetDlgItem(hwndFind, IDC_ESCAPE2), &rect);
					SendDlgItemMessage(hwndFind, ID_DROP_REPLACE, WM_GETTEXT, (WPARAM)MAXFIND, (LPARAM)szText);
					break;
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
				EnableMenuItem(hsub, ID_ESCAPE_ANY, MF_BYCOMMAND | MF_GRAYED);
				EnableMenuItem(hsub, ID_ESCAPE_HEX, MF_BYCOMMAND | MF_GRAYED);
				EnableMenuItem(hsub, ID_ESCAPE_DEC, MF_BYCOMMAND | MF_GRAYED);
				EnableMenuItem(hsub, ID_ESCAPE_OCT, MF_BYCOMMAND | MF_GRAYED);
				EnableMenuItem(hsub, ID_ESCAPE_BIN, MF_BYCOMMAND | MF_GRAYED);
				EnableMenuItem(hsub, ID_ESCAPE_HEXS, MF_BYCOMMAND | MF_GRAYED);
				EnableMenuItem(hsub, ID_ESCAPE_B64S, MF_BYCOMMAND | MF_GRAYED);
#ifdef UNICODE
				EnableMenuItem(hsub, ID_ESCAPE_HEXU, MF_BYCOMMAND | MF_GRAYED);
				EnableMenuItem(hsub, ID_ESCAPE_B64SU, MF_BYCOMMAND | MF_GRAYED);
				EnableMenuItem(hsub, ID_ESCAPE_HEXSU, MF_BYCOMMAND | MF_GRAYED);
#endif
			}

			id = TrackPopupMenuEx(hsub, TPM_RETURNCMD | TPM_TOPALIGN | TPM_LEFTALIGN | TPM_RIGHTBUTTON, rect.left, rect.bottom, hwnd, NULL);

			switch (id) {
				case ID_ESCAPE_NEWLINE:		lstrcat(szText, _T("\\n")); break;
				case ID_ESCAPE_TAB:			lstrcat(szText, _T("\\t")); break;
				case ID_ESCAPE_BACKSLASH:	lstrcat(szText, _T("\\\\")); break;
				case ID_ESCAPE_ANY:			lstrcat(szText, _T("\\?")); break;
				case ID_ESCAPE_HEX: 		lstrcat(szText, _T("\\x")); break;
				case ID_ESCAPE_DEC: 		lstrcat(szText, _T("\\d")); break;
				case ID_ESCAPE_OCT: 		lstrcat(szText, _T("\\")); break;
				case ID_ESCAPE_BIN: 		lstrcat(szText, _T("\\b")); break;
				case ID_ESCAPE_HEXU: 		lstrcat(szText, _T("\\u")); break;
				case ID_ESCAPE_HEXS: 		lstrcat(szText, _T("\\X\\")); j = 1; break;
				case ID_ESCAPE_HEXSU: 		lstrcat(szText, _T("\\U\\")); j = 1; break;
				case ID_ESCAPE_B64S: 		lstrcat(szText, _T("\\S\\")); j = 1; break;
				case ID_ESCAPE_B64SU: 		lstrcat(szText, _T("\\W\\")); j = 1; break;
				case ID_ESCAPE_DISABLE:
					bNoFindHidden = !GetCheckedState(hsub, ID_ESCAPE_DISABLE, TRUE);
					break;
			}

			switch(LOWORD(wParam)){
				case IDC_ESCAPE:
					i = (frDlgId == ID_FIND || frDlgId == ID_REPLACE? ID_DROP_FIND : ID_DROP_INSERT); break;
				case IDC_ESCAPE2:
					i = ID_DROP_REPLACE; break;
			}
			SendDlgItemMessage(hwndFind, i, WM_SETTEXT, (WPARAM)(BOOL)FALSE, (LPARAM)szText);
			SetFocus(GetDlgItem(hwndFind, i));
			if (j) {
				j = lstrlen(szText);
				SendDlgItemMessage(hwndFind, i, CB_SETEDITSEL, 0, MAKELPARAM(j-1, j-1));
			} else
				SendDlgItemMessage(hwndFind, i, CB_SETEDITSEL, 0, -1);

			DestroyMenu(hmenu);
			break; }
		case IDC_NUM:
		case ID_DROP_INSERT:
			if (HIWORD(wParam) != EN_CHANGE && HIWORD(wParam) != CBN_EDITCHANGE && HIWORD(wParam) != CBN_SELCHANGE && HIWORD(wParam) != CBN_SETFOCUS) break;
			bOkEna &= (0 != SendDlgItemMessage(hdlgFind, ID_DROP_INSERT, WM_GETTEXTLENGTH, 0, 0) || SendDlgItemMessage(hdlgFind, ID_DROP_INSERT, CB_GETCURSEL, 0, 0) >= 0);
			bOkEna &= (0 != SendDlgItemMessage(hdlgFind, IDC_NUM, WM_GETTEXTLENGTH, 0, 0));
		case ID_DROP_FIND:
			if (HIWORD(wParam) != EN_CHANGE && HIWORD(wParam) != CBN_EDITCHANGE && HIWORD(wParam) != CBN_SELCHANGE && HIWORD(wParam) != CBN_SETFOCUS) break;
			bOkEna &= (0 != SendDlgItemMessage(hdlgFind, ID_DROP_FIND, WM_GETTEXTLENGTH, 0, 0) || SendDlgItemMessage(hdlgFind, ID_DROP_FIND, CB_GETCURSEL, 0, 0) >= 0);
			EnableWindow(GetDlgItem(hdlgFind, IDOK), bOkEna);
			if (frDlgId == ID_REPLACE){
				EnableWindow(GetDlgItem(hdlgFind, 1024), bOkEna);
				EnableWindow(GetDlgItem(hdlgFind, 1025), bOkEna);
			}
			break;
		}
		break;
	case WM_SHOWWINDOW:
		return TRUE;
	}
	return FALSE;
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
			SelectWord(NULL, TRUE, TRUE);
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
			SelectWord(NULL, bSmartSelect, TRUE);
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
			SetDlgItemText(hwndDlg, IDD_FILE, SCNUL(szFile));
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
	LPCTSTR szBuffer;
	BOOL bUseDefault;
	DWORD l;

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

	szBuffer = GetShadowRange(cr.cpMin, cr.cpMax, &l);
	lStringLen = (LONG)l;
	bPrint = TRUE;

	if (SetAbortProc(pd.hDC, AbortDlgProc) == SP_ERROR) {
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
	di.lpszDocName = SCNUL(szFile);

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
	di.lpszDocName = SCNUL(szCaptionFile);
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

void ReportError(UINT err) {
	UINT i;
	LPCTSTR errMsg = GetString(IDS_ERROR_MSG);
	TCHAR msgBuf[MAXSTRING];

	i = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), msgBuf, MAXSTRING-lstrlen(errMsg)-12, NULL);
	wsprintf(&msgBuf[i], errMsg, err);
	MessageBox(NULL, msgBuf, STR_METAPAD, MB_OK | MB_ICONSTOP);
/*
#ifndef	_DEBUG
	PostQuitMessage(0);
#endif
*/
}
void ReportLastError(void) {
	ReportError(GetLastError());
}

void SaveMRUInfo(LPCTSTR szFullPath)
{
	HKEY key = NULL;
	TCHAR szKey[16];
	LPTSTR szBuffer = NULL;
	LPTSTR szTopVal = NULL;
	LPTSTR szExpPath = NULL;
	UINT i = 1;

	if (options.nMaxMRU == 0)
		return;

	if (!g_bIniMode && RegCreateKeyEx(HKEY_CURRENT_USER, STR_REGKEY, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &key, NULL) != ERROR_SUCCESS) {
		ReportLastError();
		return;
	}

	wsprintf(szKey, _T("mru_%d"), nMRUTop);
	LoadOptionString(key, szKey, &szBuffer, MAXFN);
	ExpandFilename(szFullPath, &szExpPath);
	szFullPath = szExpPath;

	if (lstrcmp(SCNUL(szFullPath), SCNUL(szBuffer)) != 0) {
		if (++nMRUTop > options.nMaxMRU) {
			nMRUTop = 1;
		}

		SaveOption(key, _T("mru_Top"), REG_DWORD, (LPBYTE)&nMRUTop, sizeof(int));
		wsprintf(szKey, _T("mru_%d"), nMRUTop);
		LoadOptionString(key, szKey, &szTopVal, MAXFN);
		SaveOption(key, szKey, REG_SZ, (LPBYTE)szFullPath, MAXFN);

		for (i = 1; i <= options.nMaxMRU; ++i) {
			if (i == nMRUTop) continue;
			wsprintf(szKey, _T("mru_%d"), i);
			LoadOptionString(key, szKey, &szBuffer, MAXFN);
			if (lstrcmpi(szBuffer, szFullPath) == 0) {
				SaveOption(key, szKey, REG_SZ, (LPBYTE)szTopVal, MAXFN);
				break;
			}
		}
	}

	if (key)
		RegCloseKey(key);

	PopulateMRUList();
	FREE(szBuffer);
	FREE(szTopVal);
	FREE(szExpPath);
}

void PopulateFavourites(void)
{
	TCHAR szBuffer[MAXFAVESIZE];
	TCHAR *szName = gTmpBuf2, *szMenu = gTmpBuf;
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

	if (GetPrivateProfileString(STR_FAV_APPNAME, NULL, NULL, szBuffer, MAXFAVESIZE, SCNUL(szFav))) {
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
	TCHAR* szBuffer = gTmpBuf;
	LPTSTR szBuff2 = NULL;
	HMENU hmenu = GetMenu(hwnd);
	HMENU hsub;
	TCHAR szKey[16];
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

		while (GetMenuItemCount(hsub)) {
			DeleteMenu(hsub, 0, MF_BYPOSITION);
		}

		LoadOptionNumeric(key, _T("mru_Top"), (LPBYTE)&nMRUTop, sizeof(int));
		mio.cbSize = sizeof(MENUITEMINFO);
		mio.fMask = MIIM_TYPE | MIIM_ID;
		mio.fType = MFT_STRING;

		i = nMRUTop;
		while (cnt < options.nMaxMRU) {
			wsprintf(szKey, _T("mru_%d"), i);
			wsprintf(szBuffer, (num < 10 ? _T("&%d ") : _T("%d ")), num);
			LoadOptionString(key, szKey, &szBuff2, MAXFN);

			if (szBuff2 && lstrlen(szBuff2)) {
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
	FREE(szBuff2);
}

void CenterWindow(HWND hwndCenter)
{
	RECT r1, r2;
	GetWindowRect(GetParent(hwndCenter), &r1);
	GetWindowRect(hwndCenter, &r2);
	SetWindowPos(hwndCenter, HWND_TOP, (((r1.right - r1.left) - (r2.right - r2.left)) / 2) + r1.left, (((r1.bottom - r1.top) - (r2.bottom - r2.top)) / 2) + r1.top, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
}

void SelectWord(LPTSTR* target, BOOL bSmart, BOOL bAutoSelect)
{
	LONG lLine, lLineIndex, lLineLen;
	LPTSTR szBuffer;
	CHARRANGE cr;
	LONG l;

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
			if (target) {
				l = MIN(cr.cpMax - cr.cpMin, MAXFIND);
				FREE(*target);
				*target = (LPTSTR)HeapAlloc(globalHeap, 0, (l+1) * sizeof(TCHAR));
				lstrcpyn(*target, szBuffer + cr.cpMin, l+1);
				(*target)[l] = _T('\0');
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
			SetDlgItemText(hwndDlg, IDC_DATA, SCNUL(szFile));
			CenterWindow(hwndDlg);
			return TRUE;
		}
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDOK: {
					TCHAR* szName = gTmpBuf;
					GetDlgItemText(hwndDlg, IDC_DATA, szName, MAXFN);
					WritePrivateProfileString(STR_FAV_APPNAME, szName, SCNUL(szFile), SCNUL(szFav));
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
	int i;
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
			SendDlgItemMessage(hwndDlg, IDC_COMBO_FORMAT, CB_ADDSTRING, 0, (LPARAM)_T("UTF-8 DOS"));
			SendDlgItemMessage(hwndDlg, IDC_COMBO_FORMAT, CB_ADDSTRING, 0, (LPARAM)_T("UTF-8 UNIX"));
			SendDlgItemMessage(hwndDlg, IDC_COMBO_FORMAT, CB_ADDSTRING, 0, (LPARAM)_T("Binary"));

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
				for (i = 0; i < NUMFINDS; i++)
					FREE(FindArray[i]);
				for (i = 0; i < NUMFINDS; i++)
					FREE(ReplaceArray[i]);
				for (i = 0; i < NUMINSERTS; i++)
					FREE(InsertArray[i]);
				ZeroMemory(FindArray, sizeof(FindArray));
				ZeroMemory(ReplaceArray, sizeof(ReplaceArray));
				ZeroMemory(InsertArray, sizeof(InsertArray));
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
					SaveOption(key, szKey, REG_SZ, NULL, 1);
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
			SendDlgItemMessage(hwndDlg, IDC_CUSTOMDATE, EM_LIMITTEXT, (WPARAM)MAXDATEFORMAT-1, 0);
			SendDlgItemMessage(hwndDlg, IDC_CUSTOMDATE2, EM_LIMITTEXT, (WPARAM)MAXDATEFORMAT-1, 0);

			SetDlgItemText(hwndDlg, IDC_EDIT_LANG_PLUGIN, SCNUL(options.szLangPlugin));
			SetDlgItemText(hwndDlg, IDC_MACRO_1, SCNUL(options.MacroArray[0]));
			SetDlgItemText(hwndDlg, IDC_MACRO_2, SCNUL(options.MacroArray[1]));
			SetDlgItemText(hwndDlg, IDC_MACRO_3, SCNUL(options.MacroArray[2]));
			SetDlgItemText(hwndDlg, IDC_MACRO_4, SCNUL(options.MacroArray[3]));
			SetDlgItemText(hwndDlg, IDC_MACRO_5, SCNUL(options.MacroArray[4]));
			SetDlgItemText(hwndDlg, IDC_MACRO_6, SCNUL(options.MacroArray[5]));
			SetDlgItemText(hwndDlg, IDC_MACRO_7, SCNUL(options.MacroArray[6]));
			SetDlgItemText(hwndDlg, IDC_MACRO_8, SCNUL(options.MacroArray[7]));
			SetDlgItemText(hwndDlg, IDC_MACRO_9, SCNUL(options.MacroArray[8]));
			SetDlgItemText(hwndDlg, IDC_MACRO_10, SCNUL(options.MacroArray[9]));
			SetDlgItemText(hwndDlg, IDC_CUSTOMDATE, SCNUL(options.szCustomDate));
			SetDlgItemText(hwndDlg, IDC_CUSTOMDATE2, SCNUL(options.szCustomDate2));

			if (!SCNUL(options.szLangPlugin)[0]) {
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
					TCHAR* szPlugin = gTmpBuf;
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
				TCHAR* buf = gTmpBuf;
				GetDlgItemText(hwndDlg, IDC_MACRO_1, buf, MAXMACRO);	SSTRCPY(options.MacroArray[0], buf);
				GetDlgItemText(hwndDlg, IDC_MACRO_2, buf, MAXMACRO);	SSTRCPY(options.MacroArray[1], buf);
				GetDlgItemText(hwndDlg, IDC_MACRO_3, buf, MAXMACRO);	SSTRCPY(options.MacroArray[2], buf);
				GetDlgItemText(hwndDlg, IDC_MACRO_4, buf, MAXMACRO);	SSTRCPY(options.MacroArray[3], buf);
				GetDlgItemText(hwndDlg, IDC_MACRO_5, buf, MAXMACRO);	SSTRCPY(options.MacroArray[4], buf);
				GetDlgItemText(hwndDlg, IDC_MACRO_6, buf, MAXMACRO);	SSTRCPY(options.MacroArray[5], buf);
				GetDlgItemText(hwndDlg, IDC_MACRO_7, buf, MAXMACRO);	SSTRCPY(options.MacroArray[6], buf);
				GetDlgItemText(hwndDlg, IDC_MACRO_8, buf, MAXMACRO);	SSTRCPY(options.MacroArray[7], buf);
				GetDlgItemText(hwndDlg, IDC_MACRO_9, buf, MAXMACRO);	SSTRCPY(options.MacroArray[8], buf);
				GetDlgItemText(hwndDlg, IDC_MACRO_10, buf, MAXMACRO);	SSTRCPY(options.MacroArray[9], buf);
				GetDlgItemText(hwndDlg, IDC_CUSTOMDATE, buf, MAXDATEFORMAT);	SSTRCPY(options.szCustomDate, buf);
				GetDlgItemText(hwndDlg, IDC_CUSTOMDATE2, buf, MAXDATEFORMAT);	SSTRCPY(options.szCustomDate2, buf);

				if (BST_CHECKED == SendDlgItemMessage(hwndDlg, IDC_RADIO_LANG_DEFAULT, BM_GETCHECK, 0, 0)) {
					FREE(options.szLangPlugin);
				} else if (BST_CHECKED == SendDlgItemMessage(hwndDlg, IDC_RADIO_LANG_PLUGIN, BM_GETCHECK, 0, 0)) {
					GetDlgItemText(hwndDlg, IDC_EDIT_LANG_PLUGIN, buf, MAXFN);		SSTRCPY(options.szLangPlugin, buf);
				}
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
				TCHAR* szResult = gTmpBuf;

				szResult[0] = _T('\0');

				ofn.lStructSize = sizeof(OPENFILENAME);
				ofn.hwndOwner = hwndDlg;
				ofn.lpstrFilter = szFilter;
				ofn.lpstrCustomFilter = (LPTSTR)NULL;
				ofn.nMaxCustFilter = 0L;
				ofn.nFilterIndex = 1L;
				ofn.lpstrFile = szResult;
				ofn.nMaxFile = MAXFN;
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
			SetDlgItemText(hwndDlg, IDC_EDIT_BROWSER, SCNUL(options.szBrowser));
			SetDlgItemText(hwndDlg, IDC_EDIT_ARGS, SCNUL(options.szArgs));
			SetDlgItemText(hwndDlg, IDC_EDIT_BROWSER2, SCNUL(options.szBrowser2));
			SetDlgItemText(hwndDlg, IDC_EDIT_ARGS2, SCNUL(options.szArgs2));
			SetDlgItemText(hwndDlg, IDC_EDIT_QUOTE, SCNUL(options.szQuote));
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
				TCHAR* buf = gTmpBuf;

				GetDlgItemText(hwndDlg, IDC_TAB_STOP, szInt, 5);
				nTmp = _ttoi(szInt);
				options.nTabStops = nTmp;

				GetDlgItemText(hwndDlg, IDC_EDIT_BROWSER, buf, MAXFN);		SSTRCPY(options.szBrowser, buf);
				GetDlgItemText(hwndDlg, IDC_EDIT_ARGS, buf, MAXARGS);		SSTRCPY(options.szArgs, buf);
				GetDlgItemText(hwndDlg, IDC_EDIT_BROWSER2, buf, MAXFN);		SSTRCPY(options.szBrowser2, buf);
				GetDlgItemText(hwndDlg, IDC_EDIT_ARGS2, buf, MAXARGS);		SSTRCPY(options.szArgs2, buf);
				GetDlgItemText(hwndDlg, IDC_EDIT_QUOTE, buf, MAXQUOTE);		SSTRCPY(options.szQuote, buf);

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
				TCHAR* szResult = gTmpBuf;

				szResult[0] = _T('\0');

				ofn.lStructSize = sizeof(OPENFILENAME);
				ofn.hwndOwner = hwndDlg;
				ofn.lpstrFilter = szFilter;
				ofn.lpstrCustomFilter = (LPTSTR)NULL;
				ofn.nMaxCustFilter = 0L;
				ofn.nFilterIndex = 1L;
				ofn.lpstrFile = szResult;
				ofn.nMaxFile = MAXFN;
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
#ifdef USE_RICH_EDIT
	static TEXTRANGE tr;
	static BYTE keys[256];
	static TCHAR szFindHold[MAXFIND];
	static TCHAR szReplaceHold[MAXFIND];
#endif
	static CHARRANGE cr, cr2;
	static FINDREPLACE fr;
	static OPENFILENAME ofn;
	static PAGESETUPDLG psd;
	static PROPSHEETHEADER psh;
	static PROPSHEETPAGE pages[4];
	static RECT rect;
	static WINDOWPLACEMENT wp, wp2;
	static option_struct tmpOptions;
	static TCHAR szMsg[MAXSTRING];

#ifdef USE_RICH_EDIT
	ENLINK* pLink;
#endif
	HANDLE hFile;
	HBITMAP hb;
	HCURSOR hCur;
	HDROP hDrop;
	HGLOBAL hMem, hMem2;
	HKEY key;
	HMENU hMenu;
	LPFINDREPLACE lpfr;
	LPTOOLTIPTEXT lpttt;
	LPBYTE pBuf;
	LPCTSTR szOld;
	LPTSTR sz, szNew, szBuf, szFind, szRepl;
	LPTSTR szTmp = gTmpBuf, szFileName = gTmpBuf;
	LPTSTR szTmp2 = gTmpBuf2;
	BOOL b, abort, uni;
	BYTE base;
	DWORD l, sl, len, lread, err;
	LONG i, j, k, nPos, ena, enc;


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
		if (status && bShowStatus)
			InvalidateRect(status, NULL, TRUE);
		SetFocus(client);
		break;
	case WM_DROPFILES:
		hDrop = (HDROP) wParam;
		if (!SaveIfDirty())
			break;

		DragQueryFile(hDrop, 0, szFileName, MAXFN);
		DragFinish(hDrop);
		SSTRCPY(szFile, szFileName);

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
	case WM_SIZING:
		InvalidateRect(client, NULL, FALSE); // ML: for decreasing window size, update scroll bar
		break;
	case WM_SIZE:
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
	case WM_INITMENUPOPUP:
		hMenu = (HMENU)wParam;
		nPos = LOWORD(lParam);
		if ((BOOL)HIWORD(lParam) == TRUE)
			break;

		if (nPos == EDITPOS) {
			ena = IsClipboardFormatAvailable(_CF_TEXT) ? MF_ENABLED : MF_GRAYED;
			EnableMenuItem(hMenu, ID_MYEDIT_PASTE, MF_BYCOMMAND | ena);
			EnableMenuItem(hMenu, ID_PASTE_B64, MF_BYCOMMAND | ena);
			EnableMenuItem(hMenu, ID_PASTE_HEX, MF_BYCOMMAND | ena);
			EnableMenuItem(hMenu, ID_PASTE_MUL, MF_BYCOMMAND | ena);
			EnableMenuItem(hMenu, ID_COMMIT_WORDWRAP, MF_BYCOMMAND | (bWordWrap ? MF_ENABLED : MF_GRAYED));
#ifdef USE_RICH_EDIT
			SendMessage(client, EM_EXGETSEL, 0, (LPARAM)&cr);
#else
			SendMessage(client, EM_GETSEL, (WPARAM)&cr.cpMin, (LPARAM)&cr.cpMax);
#endif
			ena = (cr.cpMin != cr.cpMax ? MF_ENABLED : MF_GRAYED);
			EnableMenuItem(hMenu, ID_MYEDIT_CUT, MF_BYCOMMAND | ena);
			EnableMenuItem(hMenu, ID_MYEDIT_COPY, MF_BYCOMMAND | ena);
			EnableMenuItem(hMenu, ID_MYEDIT_DELETE, MF_BYCOMMAND | ena);
			EnableMenuItem(hMenu, ID_COPY_B64, MF_BYCOMMAND | ena);
			EnableMenuItem(hMenu, ID_COPY_HEX, MF_BYCOMMAND | ena);
			EnableMenuItem(hMenu, ID_MYEDIT_UNDO, MF_BYCOMMAND | (SendMessage(client, EM_CANUNDO, 0, 0) ? MF_ENABLED : MF_GRAYED));
#ifdef USE_RICH_EDIT
			EnableMenuItem(hMenu, ID_MYEDIT_REDO, MF_BYCOMMAND | (SendMessage(client, EM_CANREDO, 0, 0) ? MF_ENABLED : MF_GRAYED));
#endif
		} else if (nPos == 0) {
			ena = SCNUL(szFile)[0] ? MF_ENABLED : MF_GRAYED;
			EnableMenuItem(hMenu, ID_RELOAD_CURRENT, MF_BYCOMMAND | ena);
			EnableMenuItem(hMenu, ID_READONLY, MF_BYCOMMAND | ena);
		} else if (!options.bNoFaves && nPos == FAVEPOS) {
			EnableMenuItem(hMenu, ID_FAV_ADD, MF_BYCOMMAND | (SCNUL(szFile)[0] ? MF_ENABLED : MF_GRAYED));
			EnableMenuItem(hMenu, ID_FAV_EDIT, MF_BYCOMMAND | (bHasFaves ? MF_ENABLED : MF_GRAYED));
		}
		break;
	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
			case TTN_NEEDTEXT:
				lpttt = (LPTOOLTIPTEXT)lParam;
				switch (lpttt->hdr.idFrom) {
					/*
					case ID_NEW_INSTANCE: lpttt->lpszText = GetString(IDS_NEW_INSTANCE); break;
					*/
					case ID_MYFILE_NEW: lpttt->lpszText = GetString(IDS_TB_NEWFILE); break;
					case ID_MYFILE_OPEN: lpttt->lpszText = GetString(IDS_TB_OPENFILE); break;
					case ID_MYFILE_SAVE: lpttt->lpszText = GetString(IDS_TB_SAVEFILE); break;
					case ID_PRINT: lpttt->lpszText = GetString(IDS_TB_PRINT); break;
					case ID_FIND: lpttt->lpszText = GetString(IDS_TB_FIND); break;
					case ID_REPLACE: lpttt->lpszText = GetString(IDS_TB_REPLACE); break;
					case ID_MYEDIT_CUT: lpttt->lpszText = GetString(IDS_TB_CUT); break;
					case ID_MYEDIT_COPY: lpttt->lpszText = GetString(IDS_TB_COPY); break;
					case ID_MYEDIT_PASTE: lpttt->lpszText = GetString(IDS_TB_PASTE); break;
					case ID_MYEDIT_UNDO: lpttt->lpszText = GetString(IDS_TB_UNDO); break;
#ifdef USE_RICH_EDIT
					case ID_MYEDIT_REDO: lpttt->lpszText = GetString(IDS_TB_REDO); break;
#endif
					case ID_VIEW_OPTIONS: lpttt->lpszText = GetString(IDS_TB_SETTINGS); break;
					case ID_RELOAD_CURRENT: lpttt->lpszText = GetString(IDS_TB_REFRESH); break;
					case ID_EDIT_WORDWRAP: lpttt->lpszText = GetString(IDS_TB_WORDWRAP); break;
					case ID_FONT_PRIMARY: lpttt->lpszText = GetString(IDS_TB_PRIMARYFONT); break;
					case ID_ALWAYSONTOP: lpttt->lpszText = GetString(IDS_TB_ONTOP); break;
					case ID_FILE_LAUNCHVIEWER: lpttt->lpszText = GetString(IDS_TB_PRIMARYVIEWER); break;
					case ID_LAUNCH_SECONDARY_VIEWER: lpttt->lpszText = GetString(IDS_TB_SECONDARYVIEWER); break;
				}
				break;
#ifdef USE_RICH_EDIT
			case EN_LINK:
				pLink = (ENLINK *) lParam;
				switch (pLink->msg) {
					case WM_SETCURSOR:
						if (options.bLinkDoubleClick)
							hCur = LoadCursor(NULL, IDC_ARROW);
						else
							hCur = LoadCursor(hinstThis, MAKEINTRESOURCE(IDC_MYHAND));
						SetCursor(hCur);
						return (LRESULT)1;
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
					case WM_LBUTTONDBLCLK:
						tr.chrg.cpMin = pLink->chrg.cpMin;
						tr.chrg.cpMax = pLink->chrg.cpMax;
						tr.lpstrText = (LPTSTR) HeapAlloc(globalHeap, HEAP_ZERO_MEMORY, (pLink->chrg.cpMax - pLink->chrg.cpMin + 1) * sizeof(TCHAR));

						SendMessage(client, EM_GETTEXTRANGE, 0, (LPARAM)&tr);

						ShellExecute(NULL, NULL, tr.lpstrText, NULL, NULL, SW_SHOWNORMAL);
						HeapFree(globalHeap, 0, (HGLOBAL)tr.lpstrText);
						break;
				}
#endif
		}
		break;
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
					case EN_CHANGE:
						if (!bDirtyFile && !bLoading) {
							nPos = GetWindowTextLength(hwndMain);
							szTmp[0] = _T(' ');
							szTmp[1] = _T('*');
							szTmp[2] = _T(' ');
							GetWindowText(hwndMain, szTmp + 3, nPos + 1);
							SetWindowText(hwndMain, szTmp);
							bDirtyFile = TRUE;
						}
						bHideMessage = TRUE;
						break;
#ifdef USE_RICH_EDIT
					case EN_STOPNOUNDO:
						ERROROUT(GetString(IDS_CANT_UNDO_WARNING));
						break;
#endif
					case EN_ERRSPACE:
					case EN_MAXTEXT:
#ifdef USE_RICH_EDIT
						wsprintf(szTmp, GetString(IDS_MEMORY_LIMIT), GetWindowTextLength(client));
						ERROROUT(szTmp);
#else
						if (bLoading) {
							if (options.bAlwaysLaunch || MessageBox(hwnd, GetString(IDS_QUERY_LAUNCH_VIEWER), STR_METAPAD, MB_ICONQUESTION | MB_YESNO) == IDYES) {
								if (!SCNUL(options.szBrowser)[0]) {
									MessageBox(hwnd, GetString(IDS_PRIMARY_VIEWER_MISSING), STR_METAPAD, MB_OK|MB_ICONEXCLAMATION);
									SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(ID_VIEW_OPTIONS, 0), 0);
									FREE(szFile);
									break;
								}
								LaunchExternalViewer(0);
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
				if (SCNUL(szFindText)[0]) {
					SearchFile(szFindText, bMatchCase, FALSE, TRUE, bWholeWord, pbFindTextAny);
					SendMessage(client, EM_SCROLLCARET, 0, 0);
				}
				else {
					bDown = TRUE;
					SendMessage(hwndMain, WM_COMMAND, MAKEWPARAM(ID_FIND, 0), 0);
				}
				break;
			case ID_FIND_PREV_WORD:
			case ID_FIND_NEXT_WORD:
				SelectWord(&szFindText, TRUE, TRUE);
				if (SCNUL(szFindText)[0]) {
					SearchFile(szFindText, bMatchCase, FALSE, (LOWORD(wParam) == ID_FIND_NEXT_WORD ? TRUE : FALSE), bWholeWord, pbFindTextAny);
					SendMessage(client, EM_SCROLLCARET, 0, 0);
				}
				break;
			case ID_FIND_PREV:
				if (SCNUL(szFindText)[0]) {
					SearchFile(szFindText, bMatchCase, FALSE, FALSE, bWholeWord, pbFindTextAny);
					SendMessage(client, EM_SCROLLCARET, 0, 0);
				}
				else {
					bDown = FALSE;
					SendMessage(hwndMain, WM_COMMAND, MAKEWPARAM(ID_FIND, 0), 0);
				}
				break;
			case ID_FIND:
			case ID_REPLACE:
			case ID_INSERT_TEXT:
			case ID_PASTE_MUL:
				abort = FALSE;
				ZeroMemory(&wp, sizeof(WINDOWPLACEMENT));
				if (hdlgFind) {
					if (frDlgId == LOWORD(wParam) && frDlgId != ID_PASTE_MUL) {
						SetFocus(hdlgFind);
						break;
					} else {
						GetWindowPlacement(hdlgFind, &wp);
						SendMessage(hdlgFind, WM_CLOSE, 0, 0);
					}
				}

				frDlgId = LOWORD(wParam);
				ZeroMemory(&fr, sizeof(FINDREPLACE));
				fr.lStructSize = sizeof(FINDREPLACE);
				fr.hwndOwner = hwndMain;
				fr.lpstrFindWhat = gDummyBuf;
				fr.wFindWhatLen = sizeof(TCHAR);
				fr.lpstrReplaceWith = gDummyBuf2;
				fr.wReplaceWithLen = sizeof(TCHAR);
				fr.hInstance = hinstLang;
				fr.Flags = FR_ENABLETEMPLATE | FR_ENABLEHOOK;
				fr.lpfnHook = (LPFRHOOKPROC)FindProc;
				hb = CreateMappedBitmap(hinstThis, IDB_DROP_ARROW, 0, NULL, 0);
				switch (LOWORD(wParam)) {
					case ID_REPLACE:
						SSTRCPY(szReplaceText, ReplaceArray[0]);
					case ID_FIND:
						SSTRCPY(szFindText, FindArray[0]);
						SelectWord(&szFindText, TRUE, !options.bNoFindAutoSelect);
						if (bWholeWord) fr.Flags |= FR_WHOLEWORD;
						if (bDown) fr.Flags |= FR_DOWN;
						if (bMatchCase) fr.Flags |= FR_MATCHCASE;
#ifdef USE_RICH_EDIT
						SendMessage(client, EM_EXGETSEL, 0, (LPARAM)&cr);
#else
						SendMessage(client, EM_GETSEL, (WPARAM)&cr.cpMin, (LPARAM)&cr.cpMax);
#endif
						break;
					case ID_PASTE_MUL:
						if (!OpenClipboard(NULL)) {
							ERROROUT(GetString(IDS_CLIPBOARD_OPEN_ERROR));
							abort = TRUE;
							break;
						}
						if ( hMem = GetClipboardData(_CF_TEXT) ) {
							if (!(szTmp = GlobalLock(hMem))) {
								ERROROUT(GetString(IDS_GLOBALLOCK_ERROR));
								abort = TRUE;
							} else {
								SSTRCPY(szInsert, szTmp);
								GlobalUnlock(hMem);
							}
						} else
							abort = TRUE;
						CloseClipboard();
					case ID_INSERT_TEXT:
						if (!szInsert){
							SSTRCPY(szInsert, InsertArray[0]);
							SelectWord(&szInsert, TRUE, !options.bNoFindAutoSelect);
						}
						break;
				}
				if (abort) break;

				switch (LOWORD(wParam)) {
					case ID_INSERT_TEXT:
					case ID_PASTE_MUL:
						fr.lpTemplateName = MAKEINTRESOURCE(IDD_INSERT);
						hdlgFind = FindText(&fr);
						SendDlgItemMessage(hdlgFind, ID_DROP_INSERT, CB_LIMITTEXT, (WPARAM)MAXINSERT, 0);
						SendDlgItemMessage(hdlgFind, IDC_NUM, EM_LIMITTEXT, (WPARAM)9, 0);
						SendDlgItemMessage(hdlgFind, IDC_CLOSE_AFTER_INSERT, BM_SETCHECK, (WPARAM) bCloseAfterInsert, 0);
						for (i = 0; i < NUMINSERTS; ++i)
							if (SCNUL(InsertArray[i])[0])
								SendDlgItemMessage(hdlgFind, ID_DROP_INSERT, CB_ADDSTRING, 0, (LPARAM)InsertArray[i]);
						if (options.bCurrentFindFont) SendDlgItemMessage(hdlgFind, ID_DROP_INSERT, WM_SETFONT, (WPARAM)hfontfind, 0);
						if (SCNUL(szInsert)[0]) {
							SendDlgItemMessage(hdlgFind, ID_DROP_INSERT, WM_SETTEXT, 0, (LPARAM)szInsert);
							SendDlgItemMessage(hdlgFind, ID_DROP_INSERT, CB_SETEDITSEL, 0, MAKELPARAM(0, -1));
						}
						SendDlgItemMessage(hdlgFind, IDC_NUM, WM_SETTEXT, 0, (LPARAM)_T("1"));
						break;
					case ID_REPLACE:
						fr.lpTemplateName = MAKEINTRESOURCE(IDD_REPLACE);
						hdlgFind = ReplaceText(&fr);
						SendDlgItemMessage(hdlgFind, IDC_ESCAPE2, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)(HANDLE)hb);
						SendDlgItemMessage(hdlgFind, ID_DROP_REPLACE, CB_LIMITTEXT, (WPARAM)MAXFIND, 0);
						for (i = 0; i < NUMFINDS; ++i)
							if (SCNUL(ReplaceArray[i])[0])
								SendDlgItemMessage(hdlgFind, ID_DROP_REPLACE, CB_ADDSTRING, 0, (LPARAM)ReplaceArray[i]);
						if (options.bCurrentFindFont) SendDlgItemMessage(hdlgFind, ID_DROP_REPLACE, WM_SETFONT, (WPARAM)hfontfind, 0);
#ifdef USE_RICH_EDIT
						if (SendMessage(client, EM_EXLINEFROMCHAR, 0, (LPARAM)cr.cpMin) == SendMessage(client, EM_EXLINEFROMCHAR, 0, (LPARAM)cr.cpMax))
#else
						if (SendMessage(client, EM_LINEFROMCHAR, (WPARAM)cr.cpMin, 0) == SendMessage(client, EM_LINEFROMCHAR, (WPARAM)cr.cpMax, 0))
#endif
							SendDlgItemMessage(hdlgFind, IDC_RADIO_WHOLE, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
						else
							SendDlgItemMessage(hdlgFind, IDC_RADIO_SELECTION, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
						if (SCNUL(szReplaceText)[0]) {
							SendDlgItemMessage(hdlgFind, ID_DROP_REPLACE, WM_SETTEXT, 0, (LPARAM)szReplaceText);
							SendDlgItemMessage(hdlgFind, ID_DROP_REPLACE, CB_SETEDITSEL, 0, MAKELPARAM(0, -1));
						}
						break;
					case ID_FIND:
						fr.lpTemplateName = MAKEINTRESOURCE(IDD_FIND);
						hdlgFind = FindText(&fr);
						SendDlgItemMessage(hdlgFind, IDC_CLOSE_AFTER_FIND, BM_SETCHECK, (WPARAM) bCloseAfterFind, 0);
						break;
				}
				
				SendDlgItemMessage(hdlgFind, IDC_ESCAPE, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)(HANDLE)hb);
				switch (LOWORD(wParam)) {
					case ID_REPLACE:
					case ID_FIND:
						SendDlgItemMessage(hdlgFind, ID_DROP_FIND, CB_LIMITTEXT, (WPARAM)MAXFIND, 0);
						for (i = 0; i < NUMFINDS; ++i)
							if (SCNUL(FindArray[i])[0])
								SendDlgItemMessage(hdlgFind, ID_DROP_FIND, CB_ADDSTRING, 0, (WPARAM)FindArray[i]);
						if (options.bCurrentFindFont) SendDlgItemMessage(hdlgFind, ID_DROP_FIND, WM_SETFONT, (WPARAM)hfontfind, 0);
						if (SCNUL(szFindText)[0]) {
							SendDlgItemMessage(hdlgFind, ID_DROP_FIND, WM_SETTEXT, 0, (LPARAM)szFindText);
							SendDlgItemMessage(hdlgFind, ID_DROP_FIND, CB_SETEDITSEL, 0, MAKELPARAM(0, -1));
						}
						break;
				}

				GetWindowPlacement(hdlgFind, &wp2);
				if (wp.showCmd){
					i = WIDTH(wp2.rcNormalPosition);
					wp2.rcNormalPosition.left = wp.rcNormalPosition.left;
					wp2.rcNormalPosition.right = i + wp2.rcNormalPosition.left;
					i = HEIGHT(wp2.rcNormalPosition);
					wp2.rcNormalPosition.top = wp.rcNormalPosition.top;
					wp2.rcNormalPosition.bottom = i + wp2.rcNormalPosition.top;
					SetWindowPlacement(hdlgFind, &wp2);
				} else {
					GetWindowPlacement(hwnd, &wp);
					i = WIDTH(wp2.rcNormalPosition);
					wp2.rcNormalPosition.right = wp.rcNormalPosition.right - 32;
					wp2.rcNormalPosition.left = wp2.rcNormalPosition.right - i;
					i = HEIGHT(wp2.rcNormalPosition);
					wp2.rcNormalPosition.top = wp.rcNormalPosition.top + 85;
					wp2.rcNormalPosition.bottom = i + wp2.rcNormalPosition.top;
				}
				SetWindowPlacement(hdlgFind, &wp2);
				break;
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
			case ID_MYEDIT_UNDO:
				/*
				BOOL bOldDirty = FALSE;
				if (bDirtyFile)
					bOldDirty = TRUE;
				*/
				SendMessage(client, EM_UNDO, 0, 0);
				/*
				if (bOldDirty && bDirtyFile && !SendMessage(client, EM_CANUNDO, 0, 0)) {
					TCHAR* szBuffer = gTmpBuf;

					bDirtyFile = FALSE;
					GetWindowText(hwndMain, szBuffer, GetWindowTextLength(hwndMain) + 1);
					SetWindowText(hwndMain, szBuffer + 3);
					bLoading = FALSE;
				}
				*/
				UpdateStatus();
				break;
			case ID_CLEAR_CLIPBRD:
				if (!OpenClipboard(NULL)) {
					ERROROUT(GetString(IDS_CLIPBOARD_OPEN_ERROR));
					break;
				}
				if (!EmptyClipboard())
					ReportLastError();
				CloseClipboard();
				UpdateStatus();
				break;
			case ID_MYEDIT_CUT:
			case ID_MYEDIT_COPY:
			case ID_COPY_B64:
			case ID_COPY_HEX:
				szTmp = NULL;
				l = 0;
				base = 0;

				switch(LOWORD(wParam)){
					case ID_MYEDIT_CUT:
						SendMessage(client, WM_CUT, 0, 0); break;
					case ID_COPY_B64: if (!base) base = 64;
					case ID_COPY_HEX: if (!base) base = 16;
					default:
						SendMessage(client, WM_COPY, 0, 0); break;
				}

				if (!OpenClipboard(NULL)) {
					//This happens if another window has the clipboard open. [https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-openclipboard]
					ERROROUT(GetString(IDS_CLIPBOARD_OPEN_ERROR));
					break;
				}
				if ( hMem = GetClipboardData(_CF_TEXT) ) {
					if (!(szOld = GlobalLock(hMem)))
						ERROROUT(GetString(IDS_GLOBALLOCK_ERROR));
					else {
						sl =  lstrlen(szOld);
						switch (base){
							case 16: if (!l) l = sl * 2;
							case 64: if (!l) l = ((sl + 2) / 3) * 4;
								if (nEncodingType == TYPE_UTF_16 || nEncodingType == TYPE_UTF_16_BE)
									l *= 2;
							default: if (!l) l = sl; break;
						}
						hMem2 = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, sizeof(TCHAR) * (l+1));
						if (!(szNew = GlobalLock(hMem2)))
							ERROROUT(GetString(IDS_GLOBALLOCK_ERROR));
						else {
							if (base) {
								if (sizeof(TCHAR) >= 2 && nEncodingType != TYPE_UTF_16 && nEncodingType != TYPE_UTF_16_BE) {
#ifdef UNICODE
									l = WideCharToMultiByte(CP_ACP, 0, szOld, sl, NULL, 0, NULL, NULL);
									szTmp = (LPTSTR)HeapAlloc(globalHeap, 0, l);
									WideCharToMultiByte(CP_ACP, 0, szOld, sl, (LPSTR)szTmp, l, NULL, NULL);
#endif
									szOld = szTmp;
								} else if (sizeof(TCHAR) < 2 && (nEncodingType == TYPE_UTF_16 || nEncodingType == TYPE_UTF_16_BE)) {
									l = 2 * MultiByteToWideChar(CP_ACP, 0, (LPCSTR)szOld, sl, NULL, 0);
									szTmp = (LPTSTR)HeapAlloc(globalHeap, 0, l);
									MultiByteToWideChar(CP_ACP, 0, (LPCSTR)szOld, sl, (LPWSTR)szTmp, l);
									szOld = szTmp;
								} else l = sl * sizeof(TCHAR);
								if (nEncodingType == TYPE_UTF_16_BE)
									ReverseBytes((LPBYTE)szOld, l);
								EncodeBase(base, (LPBYTE)szOld, szNew, l, NULL);
							} else lstrcpy(szNew, szOld);
							SetLastError(NO_ERROR);
							if ((!GlobalUnlock(hMem) && (l = GetLastError())) || (!GlobalUnlock(hMem2) && (l = GetLastError()))) {
								ERROROUT(GetString(IDS_CLIPBOARD_UNLOCK_ERROR));
								ReportError(l);
							}
							if (!EmptyClipboard() || !SetClipboardData(_CF_TEXT, hMem2))
								ReportLastError();
						}
					}
				}
				CloseClipboard();
				UpdateStatus();
				FREE(szTmp);
				break;
			case ID_MYEDIT_PASTE: 
			case ID_PASTE_B64:
			case ID_PASTE_HEX:
				szTmp2 = NULL;
				base = 0;

				if (!OpenClipboard(NULL)) {
					ERROROUT(GetString(IDS_CLIPBOARD_OPEN_ERROR));
					break;
				}
				if ( hMem = GetClipboardData(_CF_TEXT) ) {
					if (!(szTmp = GlobalLock(hMem)))
						ERROROUT(GetString(IDS_GLOBALLOCK_ERROR));
					else {
						uni = (nEncodingType == TYPE_UTF_16 || nEncodingType == TYPE_UTF_16_BE);
						switch(LOWORD(wParam)){
							case ID_PASTE_B64: if (!base) { base = 64; sz = uni ? _T("\\W") : _T("\\S"); }
							case ID_PASTE_HEX: if (!base) { base = 16; sz = uni ? _T("\\U") : _T("\\X"); }
								szTmp2 = (LPTSTR)HeapAlloc(globalHeap, 0, (lstrlen(szTmp)+3) * sizeof(TCHAR));
								lstrcpy(szTmp2, sz);
								lstrcat(szTmp2, szTmp);
								if (!ParseForEscapeSeqs(szTmp2, NULL, GetString(IDS_ESCAPE_CTX_CLIPBRD)))
									FREE(szTmp2);
								break;
							default:
								SSTRCPY(szTmp2, szTmp);
								break;
						}
						GlobalUnlock(hMem);
						if (szTmp2) {
#ifdef USE_RICH_EDIT
							FixTextBuffer(szTmp2);
#else
							FixTextBufferLE(&szTmp2);
#endif
							SendMessage(client, EM_REPLACESEL, (WPARAM)TRUE, (LPARAM)szTmp2);
							FREE(szTmp2);
						}
					}
				}
				CloseClipboard();
				InvalidateRect(client, NULL, TRUE);
				UpdateStatus();
				break;
			case ID_HOME: {
				LONG lStartLine, lStartLineIndex, lLineLen;

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

				szTmp = (LPTSTR) HeapAlloc(globalHeap, HEAP_ZERO_MEMORY, (lLineLen+2) * sizeof(TCHAR));
				*((LPWORD)szTmp) = (USHORT)(lLineLen + 1);
				SendMessage(client, EM_GETLINE, (WPARAM)lStartLine, (LPARAM)(LPCTSTR)szTmp);
				szTmp[lLineLen] = _T('\0');

				for (i = 0; i < lLineLen; ++i)
					if (szTmp[i] != _T('\t') && szTmp[i] != _T(' '))
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
				FREE(szTmp);
				SendMessage(client, EM_SCROLLCARET, 0, 0);
				break;
			}
			case ID_MYEDIT_SELECTALL:
				cr.cpMin = 0;
				cr.cpMax = -1;

#ifdef USE_RICH_EDIT
				SendMessage(client, EM_EXSETSEL, 0, (LPARAM)&cr);
#else
				SendMessage(client, EM_SETSEL, (WPARAM)cr.cpMin, (LPARAM)cr.cpMax);
#endif
				UpdateStatus();
				break;
#ifdef USE_RICH_EDIT
			case ID_SHOWHYPERLINKS:
				if (SendMessage(client, EM_CANUNDO, 0, 0)) {
					if (!options.bSuppressUndoBufferPrompt && MessageBox(hwnd, GetString(IDS_UNDO_HYPERLINKS_WARNING), STR_METAPAD, MB_ICONEXCLAMATION | MB_OKCANCEL) == IDCANCEL)
						break;
				}
				hCur = SetCursor(LoadCursor(NULL, IDC_WAIT));
				bHyperlinks = !GetCheckedState(GetMenu(hwndMain), ID_SHOWHYPERLINKS, TRUE);
				SendMessage(client, EM_AUTOURLDETECT, (WPARAM)bHyperlinks, 0);

				SendMessage(client, WM_SETREDRAW, (WPARAM)FALSE, 0);
				SendMessage(client, EM_EXGETSEL, 0, (LPARAM)&cr);
				UpdateWindowText();
				SendMessage(client, EM_EXSETSEL, 0, (LPARAM)&cr);
				SendMessage(client, WM_SETREDRAW, (WPARAM)TRUE, 0);
				SetCursor(hCur);
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
			case ID_TRANSPARENT:
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
			case ID_SHOWTOOLBAR:
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
			case ID_INSERT_FILE:
				sz = _T("txt");
				szFileName = szTmp + 4;
				lstrcpy(szTmp, _T("\\\\?\\"));

				ofn.lStructSize = sizeof(OPENFILENAME);
				ofn.hwndOwner = client;
				ofn.lpstrFilter = SCNUL(szCustomFilter);
				ofn.lpstrCustomFilter = (LPTSTR)NULL;
				ofn.nMaxCustFilter = 0L;
				ofn.nFilterIndex = 1L;
				ofn.lpstrFile = szFileName;
				ofn.nMaxFile = MAXFN;
				ofn.lpstrFileTitle = (LPTSTR)NULL;
				ofn.nMaxFileTitle = 0L;
				ofn.lpstrInitialDir = SCNUL(szDir);
				ofn.lpstrTitle = (LPTSTR)NULL;
				ofn.Flags = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
				ofn.nFileOffset = 0;
				ofn.nFileExtension = 0;
				ofn.lpstrDefExt = sz;

				if (GetOpenFileName(&ofn)) {
					hFile = NULL;
					pBuf = NULL;
					hCur = SetCursor(LoadCursor(NULL, IDC_WAIT));
					
					hFile = (HANDLE)CreateFile(szTmp, GENERIC_READ, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
					if (hFile == INVALID_HANDLE_VALUE) {
						ERROROUT(GetString(IDS_FILE_READ_ERROR));
						goto endinsertfile;
					}

					lread = LoadFileIntoBuffer(hFile, &pBuf, &len, &enc);
#ifndef BUILD_METAPAD_UNICODE
					if (memchr((const void*)pBuffer, _T('\0'), len) != NULL) {
						if (options.bNoWarningPrompt || MessageBox(hwnd, GetString(IDS_BINARY_FILE_WARNING), STR_METAPAD, MB_ICONQUESTION|MB_YESNO) == IDYES) {
							for (l = 0; l < len; l++) {
								if (pBuf[l] == _T('\0'))
									pBuf[l] = _T(' ');
							}
						}
						else goto endinsertfile;
					}
#endif

					if (enc != TYPE_UTF_16 && enc != TYPE_UTF_16_BE) {
#ifdef USE_RICH_EDIT
						FixTextBuffer((LPTSTR)pBuf);
#else
						FixTextBufferLE((LPTSTR*)&pBuf);
#endif
					}
					SendMessage(client, EM_REPLACESEL, (WPARAM)TRUE, (LPARAM)(LPTSTR)pBuf);
#ifdef USE_RICH_EDIT
					InvalidateRect(client, NULL, TRUE);
#endif
endinsertfile:
					CloseHandle(hFile);
					if (pBuf) HeapFree(globalHeap, 0, (HGLOBAL) pBuf);
					SetCursor(hCur);
				}
				break;
			case ID_SHOWSTATUS:
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
			case ID_READONLY:
				if (!options.bReadOnlyMenu || !szFile) break;

				lstrcpy(szTmp, _T("\\\\?\\"));
				lstrcat(szTmp, szFile);
				bReadOnly = !GetCheckedState(GetMenu(hwndMain), ID_READONLY, FALSE);
				if (bReadOnly)
					ena = SetFileAttributes(szTmp, GetFileAttributes(szTmp) | FILE_ATTRIBUTE_READONLY);
				else
					ena = SetFileAttributes(szTmp, ((GetFileAttributes(szTmp) & ~FILE_ATTRIBUTE_READONLY) == 0 ? FILE_ATTRIBUTE_NORMAL : GetFileAttributes(szTmp) & ~FILE_ATTRIBUTE_READONLY));

				if (ena) {
					err = GetLastError();
					if (err == ERROR_ACCESS_DENIED)
						ERROROUT(GetString(IDS_CHANGE_READONLY_ERROR));
					else
						ReportLastError();
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
				TCHAR* szOldLangPlugin = gTmpBuf;

				lstrcpy(szOldLangPlugin, SCNUL(options.szLangPlugin));
*/
				memcpy(&tmpOptions, &options, sizeof(option_struct));
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
				i = PropertySheet(&psh);
				if (i > 0) {
					SaveOptions();

					if (options.bLaunchClose && options.nLaunchSave == 2) {
						MessageBox(hwndMain, GetString(IDS_LAUNCH_WARNING), STR_METAPAD, MB_ICONEXCLAMATION);
					}

					if (options.bReadOnlyMenu != tmpOptions.bReadOnlyMenu)
						FixReadOnlyMenu();

					if (options.bRecentOnOwn != tmpOptions.bRecentOnOwn) {
						FixMRUMenus();
						PopulateMRUList();
					}

					if (tmpOptions.bUnFlatToolbar != options.bUnFlatToolbar) {
						DestroyWindow(toolbar);
						CreateToolbar();
					}

					if ((memcmp((LPVOID)(LPVOID)(bPrimaryFont ? &tmpOptions.PrimaryFont : &tmpOptions.SecondaryFont), (LPVOID)(bPrimaryFont ? &options.PrimaryFont : &options.SecondaryFont), sizeof(LOGFONT)) != 0) ||
						(tmpOptions.nTabStops != options.nTabStops) ||
						((bPrimaryFont && tmpOptions.nPrimaryFont != options.nPrimaryFont) || (!bPrimaryFont && tmpOptions.nSecondaryFont != options.nSecondaryFont)) ||
						(tmpOptions.bSystemColours != options.bSystemColours) ||
						(tmpOptions.bSystemColours2 != options.bSystemColours2) ||
						(memcmp((LPVOID)&tmpOptions.BackColour, (LPVOID)&options.BackColour, sizeof(COLORREF)) != 0) ||
						(memcmp((LPVOID)&tmpOptions.FontColour, (LPVOID)&options.FontColour, sizeof(COLORREF)) != 0) ||
						(memcmp((LPVOID)&tmpOptions.BackColour2, (LPVOID)&options.BackColour2, sizeof(COLORREF)) != 0) ||
						(memcmp((LPVOID)&tmpOptions.FontColour2, (LPVOID)&options.FontColour2, sizeof(COLORREF)) != 0))
						if (!SetClientFont(bPrimaryFont)) {
							options.nTabStops = tmpOptions.nTabStops;
							if (bPrimaryFont) {
								options.nPrimaryFont = tmpOptions.nPrimaryFont;
								memcpy((LPVOID)&options.PrimaryFont, (LPVOID)&tmpOptions.PrimaryFont, sizeof(LOGFONT));
							}
							else {
								options.nSecondaryFont = tmpOptions.nSecondaryFont;
								memcpy((LPVOID)&options.SecondaryFont, (LPVOID)&tmpOptions.SecondaryFont, sizeof(LOGFONT));
							}
						}

					if (SCNUL(szFile)[0] != _T('\0'))
						UpdateCaption();

					SendMessage(client, EM_SETMARGINS, (WPARAM)EC_LEFTMARGIN, MAKELPARAM(options.nSelectionMarginWidth, 0));

					if (bTransparent) {
						SetLWA(hwnd, 0, (BYTE)((255 * (100 - options.nTransparentPct)) / 100), LWA_ALPHA);
					}
#ifdef USE_RICH_EDIT
					if (tmpOptions.bHideScrollbars != options.bHideScrollbars) {
						ERROROUT(GetString(IDS_RESTART_HIDE_SB));
					}
#endif
					if (tmpOptions.bNoFaves != options.bNoFaves)
						ERROROUT(GetString(IDS_RESTART_FAVES));

					if (lstrcmp(SCNUL(tmpOptions.szLangPlugin), SCNUL(options.szLangPlugin)) != 0)
						ERROROUT(GetString(IDS_RESTART_LANG));

					PopulateMRUList();
					UpdateStatus();
				} else if (i < 0)
					ReportLastError();
				break;
			case ID_HELP_ABOUT:
				DialogBox(hinstThis, MAKEINTRESOURCE(IDD_ABOUT), hwndMain, (DLGPROC)AboutDialogProc);
				break;
			case ID_ABOUT_PLUGIN:
				if (hinstThis != hinstLang)
					DialogBox(hinstLang, MAKEINTRESOURCE(IDD_ABOUT_PLUGIN), hwndMain, (DLGPROC)AboutPluginDialogProc);
				break;
			case ID_MYFILE_OPEN:
				sz = _T("txt");
				szTmp = gTmpBuf2;
				szTmp[0] = '\0';

				SetCurrentDirectory(SCNUL(szDir));
				if (!SaveIfDirty())
					break;

				ofn.lStructSize = sizeof(OPENFILENAME);
				ofn.hwndOwner = client;
				ofn.lpstrFilter = SCNUL(szCustomFilter);
				ofn.lpstrCustomFilter = (LPTSTR)NULL;
				ofn.nMaxCustFilter = 0L;
				ofn.nFilterIndex = 1L;
				ofn.lpstrFile = szTmp;
				ofn.nMaxFile = MAXFN;
				ofn.lpstrFileTitle = (LPTSTR)NULL;
				ofn.nMaxFileTitle = 0L;
				ofn.lpstrInitialDir = SCNUL(szDir);
				ofn.lpstrTitle = (LPTSTR)NULL;
				ofn.Flags = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
				ofn.nFileOffset = 0;
				ofn.nFileExtension = 0;
				ofn.lpstrDefExt = sz;

				if (GetOpenFileName(&ofn)) {
					GetCurrentDirectory(MAXFN, szFileName);
					SSTRCPY(szDir, szFileName);
					LoadOptions();
					bLoading = TRUE;
					bHideMessage = FALSE;
					lstrcpy(szStatusMessage, GetString(IDS_FILE_LOADING));
					UpdateStatus();
					SSTRCPY(szFile, szTmp);
					LoadFile(szFile, FALSE, TRUE);
					if (bLoading) {
						/*
						bLoading = FALSE;
						ExpandFilename(szFile);
						wsprintf(szTmp, STR_CAPTION_FILE, SCNUL(szCaptionFile));
						SetWindowText(hwndMain, szTmp);
						bDirtyFile = FALSE;
						*/
						bLoading = FALSE;
						bDirtyFile = FALSE;
						UpdateCaption();
					}
					else
						MakeNewFile();
				}
				UpdateStatus();
				break;
			case ID_MYFILE_NEW:
				if (!SaveIfDirty())
					break;
				MakeNewFile();
				break;
			case ID_EDIT_SELECTWORD:
#ifdef USE_RICH_EDIT
				SendMessage(client, EM_EXGETSEL, 0, (LPARAM)&cr);
				cr.cpMin = cr.cpMax;
				SendMessage(client, EM_EXSETSEL, 0, (LPARAM)&cr);
#else
				SendMessage(client, EM_GETSEL, (WPARAM)&cr.cpMin, (LPARAM)&cr.cpMax);
				cr.cpMin = cr.cpMax;
				SendMessage(client, EM_SETSEL, (WPARAM)cr.cpMin, (LPARAM)cr.cpMax);
#endif
				SelectWord(NULL, bSmartSelect, TRUE);
				break;
			case ID_GOTOLINE:
				DialogBox(hinstLang, MAKEINTRESOURCE(IDD_GOTO), hwndMain, (DLGPROC)GotoDialogProc);
				break;
			case ID_EDIT_WORDWRAP:
				hCur = SetCursor(LoadCursor(NULL, IDC_WAIT));
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
				szOld = GetShadowBuffer(NULL);
				bWordWrap = !GetCheckedState(GetMenu(hwnd), ID_EDIT_WORDWRAP, TRUE);
				SendMessage(client, EM_GETSEL, (WPARAM)&cr.cpMin, (LPARAM)&cr.cpMax);
				if (!DestroyWindow(client))
					ReportLastError();
				CreateClient(hwnd, _T(""), bWordWrap);
				SetWindowText(client, szOld);
				SendMessage(client, EM_SETSEL, (WPARAM)cr.cpMin, (LPARAM)cr.cpMax);
				SetClientFont(bPrimaryFont);

				GetClientRect(hwnd, &rect);
				SetWindowPos(client, 0, 0, bShowToolbar ? GetToolbarHeight() : rect.top - GetToolbarHeight(), rect.right - rect.left, rect.bottom - rect.top - GetStatusHeight() - GetToolbarHeight(), SWP_NOZORDER | SWP_SHOWWINDOW);
				SetFocus(client);
				SendMessage(client, EM_SCROLLCARET, 0, 0);
#endif
				SetCursor(hCur);
				UpdateStatus();
				break;
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

					ShellExecute(NULL, NULL, buf, NULL, SCNUL(szDir), SW_SHOWNORMAL);
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
					if (cr.cpMin == cr.cpMax)
						szPrefix[options.nTabStops - (cr.cpMax - lEndLineIndex) % options.nTabStops] = _T('\0');
					else
						szPrefix[options.nTabStops] = _T('\0');
				}
				else
					lstrcpy(szPrefix, _T("\t"));

				lEndLineLen = SendMessage(client, EM_LINELENGTH, (WPARAM)lEndLineIndex, 0);
				if ((lEndLine == lMaxLine) && (cr.cpMin != cr.cpMax) && (cr.cpMax != lEndLineIndex) &&
					((lEndLine != lStartLine) || (cr.cpMin == lEndLineIndex && (cr.cpMax - cr.cpMin - 1) == lEndLineLen))) {
					bEnd = TRUE;
				}
				if (lStartLine != lEndLine || bEnd || bStrip) {
					LONG lLineLen, iIndex, lBefore;
					int i, j = 0, diff = 0;
					len = GetWindowTextLength(client);
					
					if (lEndLineIndex != cr.cpMax || (bStrip && cr.cpMin == cr.cpMax && lEndLineIndex == cr.cpMax)) {
						nPos = SendMessage(client, EM_LINEINDEX, (WPARAM)lEndLine+1, 0);
						lEndLineIndex = nPos;
						++lEndLine;
					}
					szBuf = (LPTSTR) HeapAlloc(globalHeap, HEAP_ZERO_MEMORY, (1 + len + (lMaxLine + 1) * options.nTabStops) * sizeof(TCHAR));
					szBuf[0] = _T('\0');

					for (i = lStartLine; i < lEndLine; i++) {
						iIndex = SendMessage(client, EM_LINEINDEX, (WPARAM)i, 0);
						lLineLen = SendMessage(client, EM_LINELENGTH, (WPARAM)iIndex, 0);

						diff = 0;
						szTmp = (LPTSTR) HeapAlloc(globalHeap, HEAP_ZERO_MEMORY, (lLineLen+2) * sizeof(TCHAR));
						*((LPWORD)szTmp) = (USHORT)(lLineLen + 1); // + 1 to fix weird xp bug (skipping length 1 lines)!!
						SendMessage(client, EM_GETLINE, (WPARAM)i, (LPARAM)(LPCTSTR)szTmp);
						szTmp[lLineLen] = _T('\0');
						if (bIndent) {
							lstrcat(szBuf, szPrefix);
							lstrcat(szBuf, szTmp);
							diff = -(lstrlen(szPrefix));
						}
						else {
							if ((bStrip && lLineLen > 0) || szTmp[0] == _T('\t'))
								diff = 1;
							else if (szTmp[0] == _T(' ')) {
								diff = 1;
								while (diff < options.nTabStops && szTmp[diff] == _T(' '))
									diff++;
							}
							lstrcpy(szBuf+j, szTmp + diff);
						}
						if (bEnd && i == lEndLine - 1) {
							j += lLineLen - diff;
						} else {
							szBuf[lLineLen+j-diff] = _T('\r');
							szBuf[lLineLen+j+1-diff] = _T('\n');
							j += lLineLen + 2 - diff;
						}
						FREE(szTmp);
					}

					lBefore = SendMessage(client, EM_GETLINECOUNT, 0, 0);
					cr.cpMin = lStartLineIndex;
					cr.cpMax = lEndLineIndex;
#ifdef USE_RICH_EDIT
					SendMessage(client, EM_EXSETSEL, 0, (LPARAM)&cr);
#else
					SendMessage(client, EM_SETSEL, (WPARAM)cr.cpMin, (LPARAM)cr.cpMax);
#endif
					SendMessage(client, EM_REPLACESEL, (WPARAM)TRUE, (LPARAM)szBuf);
					if (SendMessage(client, EM_GETLINECOUNT, 0, 0) > lBefore) {
						SendMessage(client, EM_UNDO, 0, 0);
					} else {
						lEndLineIndex = SendMessage(client, EM_LINEINDEX, (WPARAM)lEndLine, 0);
						cr.cpMin = lStartLineIndex;
						cr.cpMax = lEndLineIndex;
#ifdef USE_RICH_EDIT
						SendMessage(client, EM_EXSETSEL, 0, (LPARAM)&cr);
#else
						SendMessage(client, EM_SETSEL, (WPARAM)cr.cpMin, (LPARAM)cr.cpMax);
#endif
					}
					FREE(szBuf);
				} else if (bIndent)
					SendMessage(client, EM_REPLACESEL, (WPARAM)TRUE, (LPARAM)szPrefix);
				HeapFree(globalHeap, 0, (HGLOBAL)szPrefix);

				if (bRewrap)
					SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(ID_EDIT_WORDWRAP, 0), 0);

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
			case ID_DATE_TIME: 
			case ID_DATE_TIME_CUSTOM:
			case ID_DATE_TIME_CUSTOM2:
				if (bLoading) {
					szTmp[0] = _T('\r');
					szTmp[1] = _T('\n');
					szTmp[2] = _T('\0');
				}
				else {
					szTmp[0] = _T('\0');
				}
				if (LOWORD(wParam) == ID_DATE_TIME_CUSTOM || LOWORD(wParam) == ID_DATE_TIME_CUSTOM2) {
					sz = (LOWORD(wParam) == ID_DATE_TIME_CUSTOM ? options.szCustomDate : options.szCustomDate2);
					GetTimeFormat(LOCALE_USER_DEFAULT, 0, NULL, SCNUL(sz), (bLoading ? szTmp + 2 : szTmp), 100);
					GetDateFormat(LOCALE_USER_DEFAULT, 0, NULL, szTmp, szTmp, 100);
				} else {
					if (!options.bDontInsertTime) {
						GetTimeFormat(LOCALE_USER_DEFAULT, TIME_NOSECONDS, NULL, NULL, (bLoading ? szTmp + 2 : szTmp), 100);
						lstrcat(szTmp, _T(" "));
					}
					GetDateFormat(LOCALE_USER_DEFAULT, (LOWORD(wParam) == ID_DATE_TIME_LONG ? DATE_LONGDATE : 0), NULL, NULL, szTmp2, 100);
					lstrcat(szTmp, szTmp2);
				}
				if (bLoading)
					lstrcat(szTmp, _T("\r\n"));
				SendMessage(client, EM_REPLACESEL, (WPARAM)TRUE, (LPARAM)szTmp);
#ifdef USE_RICH_EDIT
				InvalidateRect(client, NULL, TRUE);
#endif

				if (bLoading)
					bDirtyFile = FALSE;
				UpdateStatus();
				break;
			case ID_PAGESETUP:
				ZeroMemory(&psd, sizeof(PAGESETUPDLG));
				psd.lStructSize = sizeof(PAGESETUPDLG);
				psd.Flags |= /*PSD_INTHOUSANDTHSOFINCHES | */PSD_DISABLEORIENTATION | PSD_DISABLEPAPER | PSD_DISABLEPRINTER | PSD_ENABLEPAGESETUPTEMPLATE | PSD_MARGINS;
				psd.hwndOwner = hwndMain;
				psd.hInstance = hinstLang;
				psd.rtMargin = options.rMargins;
				psd.lpPageSetupTemplateName = MAKEINTRESOURCE(IDD_PAGE_SETUP);

				if (!GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_IMEASURE, szTmp, 8))
					ReportLastError();
				b = (lstrcmp(szTmp, _T("0")) == 0);

				if (b) {
					psd.rtMargin.bottom = (long)(psd.rtMargin.bottom * 2.54);
					psd.rtMargin.top = (long)(psd.rtMargin.top * 2.54);
					psd.rtMargin.left = (long)(psd.rtMargin.left * 2.54);
					psd.rtMargin.right = (long)(psd.rtMargin.right * 2.54);
				}

				if (PageSetupDlg(&psd)) {
					if (b) {
						psd.rtMargin.bottom = (long)(psd.rtMargin.bottom * 0.3937);
						psd.rtMargin.top = (long)(psd.rtMargin.top * 0.3937);
						psd.rtMargin.left = (long)(psd.rtMargin.left * 0.3937);
						psd.rtMargin.right = (long)(psd.rtMargin.right * 0.3937);
					}
					options.rMargins = psd.rtMargin;
					SaveOptions();
				}
				break;
			case ID_PRINT:
				b = FALSE;
				if (options.bPrintWithSecondaryFont && bPrimaryFont) {
					SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(ID_FONT_PRIMARY, 0), 0);
					b = TRUE;
				}
				PrintContents();
				if (b)
					SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(ID_FONT_PRIMARY, 0), 0);
				break;
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
			case ID_MAKE_OEM:
			case ID_MAKE_ANSI:
				hCur = SetCursor(LoadCursor(NULL, IDC_WAIT));

#ifdef USE_RICH_EDIT
				SendMessage(client, EM_EXGETSEL, 0, (LPARAM)&cr);
#else
				SendMessage(client, EM_GETSEL, (WPARAM)&cr.cpMin, (LPARAM)&cr.cpMax);
#endif
				if (cr.cpMin == cr.cpMax) {
					ERROROUT(GetString(IDS_NO_SELECTED_TEXT));
					break;
				}
				szOld = GetShadowRange(cr.cpMin, cr.cpMax, &l);
				k = (LONG)l + 1;
				szNew = (LPTSTR) HeapAlloc(globalHeap, HEAP_ZERO_MEMORY, k * sizeof(TCHAR));

				switch (LOWORD(wParam)) {
				case ID_UNTABIFY:
				case ID_TABIFY:
					cr2 = cr;
					szFind = szTmp;
					szRepl = szTmp + MAXSTRING;

					if (LOWORD(wParam) == ID_UNTABIFY) {
						lstrcpy(szFind, _T("\t"));
						memset(szRepl, _T(' '), options.nTabStops);
						szRepl[options.nTabStops] = _T('\0');
					}
					else {
						lstrcpy(szRepl, _T("\t"));
						memset(szFind, _T(' '), options.nTabStops);
						szFind[options.nTabStops] = _T('\0');
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
					while (SearchFile(szFind, FALSE, TRUE, TRUE, FALSE, NULL)) {
						SendMessage(client, EM_REPLACESEL, (WPARAM)TRUE, (LPARAM)szRepl);
						nReplaceMax -= lstrlen(szFind) - lstrlen(szRepl);
					}
					bReplacingAll = FALSE;
					cr.cpMax = nReplaceMax;
					nReplaceMax = -1;
					SendMessage(client, WM_SETREDRAW, (WPARAM)TRUE, 0);
					InvalidateRect(hwnd, NULL, TRUE);
					break;
				case ID_QUOTE:
					nPos = 1;
					sl = lstrlen(SCNUL(options.szQuote));
					if (!sl) break;

					for (i = 0; i < k-1; ++i) {
#ifdef USE_RICH_EDIT
						if (szOld[i] == _T('\r')) ++nPos;
#else
						if (szOld[i] == _T('\n')) ++nPos;
#endif
					}

					HeapFree(globalHeap, 0, (HGLOBAL)szNew);
					szNew = (LPTSTR) HeapAlloc(globalHeap, HEAP_ZERO_MEMORY, k * sizeof(TCHAR) + nPos * sl);

					lstrcpy(szNew, options.szQuote);
					for (i = 0, j = sl; i < k-1; ++i) {
						szNew[j++] = szOld[i];
#ifdef USE_RICH_EDIT
						if (szOld[i] == _T('\r')) {
#else
						if (szOld[i] == _T('\n')) {
#endif
							if (i == k - 2) {
								--nPos;
								break;
							}
							lstrcat(szNew, options.szQuote);
							j += sl;
						}
					}

					cr.cpMax += nPos * sl;
					break;
				case ID_STRIP_CR_SPACE:
				case ID_STRIP_CR:
					for (i = 0, j = 0; i < k; ++i) {
#ifdef USE_RICH_EDIT
						if (szOld[i] != _T('\r')) {
							szNew[j++] = szOld[i];
						}
						else if (szOld[i+1] == _T('\r')) {
							szNew[j++] = szOld[i++];
							szNew[j++] = szOld[i];
						}
						else if (LOWORD(wParam) == ID_STRIP_CR_SPACE){
							szNew[j++] = _T(' ');
						}
#else
						if (szOld[i] != _T('\n') && szOld[i] != _T('\r')) {
							szNew[j++] = szOld[i];
						}
						else if (szOld[i] == _T('\r') && szOld[i+1] == _T('\n') && szOld[i+2] == _T('\r') && szOld[i+3] == _T('\n')) {
							szNew[j++] = szOld[i++];
							szNew[j++] = szOld[i++];
							szNew[j++] = szOld[i++];
							szNew[j++] = szOld[i];
						}
						else if (LOWORD(wParam) == ID_STRIP_CR_SPACE && szOld[i] == _T('\n')) {
							szNew[j++] = _T(' ');
						}
#endif
					}
					cr.cpMax -= i - j;
					break;
				case ID_STRIP_TRAILING_WS:
					b = TRUE;

					szNew[l] = _T('\0');
					for (i = l-1, j = i; i >= 0; --i) {

						if (b) {
							if (szOld[i] == _T('\t') || szOld[i] == _T(' '))
								continue;
							else if (szOld[i] != _T('\r') && szOld[i] != _T('\n'))
								b = FALSE;
						}
						else {
							if (szOld[i] == _T('\r'))
								b = TRUE;
						}
						szNew[j--] = szOld[i];
					}
					cr.cpMax -= j + 1;
					lstrcpy(szNew, szNew + j + 1);
					break;
				case ID_HACKER:
					lstrcpy(szNew, szOld);

					for (i = 0; i < k; i++) {
						if (_istascii(szNew[i])) {
							switch (szNew[i]) {
							case _T('a'):
							case _T('A'):
							szNew[i] = _T('4');
								break;
							case _T('T'):
							case _T('t'):
								szNew[i] = _T('7');
								break;
							case _T('E'):
							case _T('e'):
								szNew[i] = _T('3');
								break;
							case _T('L'):
							case _T('l'):
								szNew[i] = _T('1');
								break;
							case _T('S'):
							case _T('s'):
								szNew[i] = _T('5');
								break;
							case _T('G'):
							case _T('g'):
								szNew[i] = _T('6');
								break;
							case _T('O'):
							case _T('o'):
								szNew[i] = _T('0');
								break;
							}
						}
					}
					break;
				case ID_MAKE_LOWER:
					lstrcpy(szNew, szOld);
					CharLowerBuff(szNew, k);
					break;
				case ID_MAKE_UPPER:
					lstrcpy(szNew, szOld);
					CharUpperBuff(szNew, k);
					break;
				case ID_MAKE_INVERSE:
					lstrcpy(szNew, szOld);
					for (i = 0; i < k; i++) {
						if (IsCharAlpha(szNew[i])) {
							if (IsCharUpper(szNew[i])) {
								CharLowerBuff(szNew + i, 1);
								//szNew[i] = (TCHAR)_tolower(szNew[i]);
							}
							else if (IsCharLower(szNew[i])) {
								CharUpperBuff(szNew + i, 1);
								//szNew[i] = (TCHAR)_toupper(szNew[i]);
							}
						}
					}
					break;
				case ID_MAKE_TITLE:
				case ID_MAKE_SENTENCE:
					b = TRUE;
					lstrcpy(szNew, szOld);
					for (i = 0; i < k; i++) {
						if (IsCharAlpha(szNew[i])) {
							if (b) {
								b = FALSE;
								if (IsCharLower(szNew[i])) {
									CharUpperBuff(szNew + i, 1);
									//szNew[i] = (TCHAR)_toupper(szNew[i]);
								}
							}
							else {
								if (IsCharUpper(szNew[i])) {
									CharLowerBuff(szNew + i, 1);
									//szNew[i] = (TCHAR)_tolower(szNew[i]);
								}
							}
						}
						else {
							if (LOWORD(wParam) == ID_MAKE_TITLE && szNew[i] != _T('\'')) {
								b = TRUE;
							} else if (szNew[i] == _T('.')
								|| szNew[i] == _T('?')
								|| szNew[i] == _T('!')
								|| szNew[i] == _T('\r')) {
								b = TRUE;
							}

						}
					}
					break;
				case ID_MAKE_OEM:
					CharToOemBuff(szOld, (LPSTR)szNew, k);
#ifdef UNICODE
					if (sizeof(TCHAR) > 1) {
						lstrcpy((LPTSTR)szOld, szNew);
						MultiByteToWideChar(CP_ACP, 0, (LPSTR)szOld, k, szNew, k);
					}
#endif
					break;
				case ID_MAKE_ANSI:
#ifdef UNICODE
					if (sizeof(TCHAR) > 1) {
						lstrcpy(szNew, szOld);
						WideCharToMultiByte(CP_ACP, 0, szNew, k, (LPSTR)szOld, k, NULL, NULL);
					}
#endif
					OemToCharBuffA((LPSTR)szOld, (LPSTR)szNew, k);
#ifdef UNICODE
					if (sizeof(TCHAR) > 1) {
						//OemToCharBuffW corrupts newlines, use original ANSI function instead and re-convert to unicode here
						lstrcpy((LPTSTR)szOld, szNew);
						MultiByteToWideChar(CP_ACP, 0, (LPSTR)szOld, k, szNew, k);
					}
#endif
					break;
				}

				if (LOWORD(wParam) != ID_UNTABIFY && LOWORD(wParam) != ID_TABIFY) {
					if (lstrcmp(szOld, szNew) != 0) {
						SendMessage(client, EM_REPLACESEL, (WPARAM)TRUE, (LPARAM)szNew);
					}
				}
#ifdef USE_RICH_EDIT
				SendMessage(client, EM_EXSETSEL, 0, (LPARAM)&cr);
				InvalidateRect(client, NULL, TRUE);
#else
				SendMessage(client, EM_SETSEL, (WPARAM)cr.cpMin, (LPARAM)cr.cpMax);
#endif
				FREE(szNew);
				SetCursor(hCur);
				break;
			case ID_NEW_INSTANCE:
				GetModuleFileName(hinstThis, szTmp, MAXFN);
				//ShellExecute(NULL, NULL, szTmp, NULL, SCNUL(szDir), SW_SHOWNORMAL);
				ExecuteProgram(szTmp, _T(""));
				break;
			case ID_LAUNCH_ASSOCIATED_VIEWER:
				LaunchInViewer(FALSE, FALSE);
				break;
			case ID_BINARY_FILE:
			case ID_UTF8_FILE:
			case ID_UTF8_UNIX_FILE:
			case ID_UNICODE_FILE:
			case ID_UNICODE_BE_FILE:
			case ID_DOS_FILE:
			case ID_UNIX_FILE:
				hMenu = GetSubMenu(GetSubMenu(GetMenu(hwndMain), 0), FILEFORMATPOS);

				if (!bLoading) {
					if (LOWORD(wParam) == ID_UTF8_FILE && !bUnix && nEncodingType == TYPE_UTF_8)
						break;
					else if (LOWORD(wParam) == ID_UTF8_UNIX_FILE && bUnix && nEncodingType == TYPE_UTF_8)
						break;
					else if (LOWORD(wParam) == ID_UNICODE_FILE && nEncodingType == TYPE_UTF_16)
						break;
					else if (LOWORD(wParam) == ID_UNICODE_BE_FILE && nEncodingType == TYPE_UTF_16_BE)
						break;
					else if (LOWORD(wParam) == ID_UNIX_FILE && bUnix && !bBinaryFile && nEncodingType == TYPE_UNKNOWN)
						break;
					else if (LOWORD(wParam) == ID_DOS_FILE && !bUnix && !bBinaryFile && nEncodingType == TYPE_UNKNOWN)
						break;
					else if (LOWORD(wParam) == ID_BINARY_FILE && bBinaryFile && nEncodingType == TYPE_UNKNOWN)
						break;
				}

				bBinaryFile = (LOWORD(wParam) == ID_BINARY_FILE);
				bUnix = (LOWORD(wParam) == ID_UNIX_FILE || LOWORD(wParam) == ID_UTF8_UNIX_FILE);

				if (LOWORD(wParam) == ID_UTF8_FILE || LOWORD(wParam) == ID_UTF8_UNIX_FILE)
					nEncodingType = TYPE_UTF_8;
				else if (LOWORD(wParam) == ID_UNICODE_FILE)
					nEncodingType = TYPE_UTF_16;
				else if (LOWORD(wParam) == ID_UNICODE_BE_FILE)
					nEncodingType = TYPE_UTF_16_BE;
				else
					nEncodingType = TYPE_UNKNOWN;

				CheckMenuRadioItem(hMenu, ID_DOS_FILE, ID_BINARY_FILE, LOWORD(wParam), MF_BYCOMMAND);

				if (!bDirtyFile && !bLoading) {
					TCHAR* szTmp = gTmpBuf;
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
			case ID_RELOAD_CURRENT:
				if (SCNUL(szFile)[0]) {
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
						TCHAR* szBuffer = gTmpBuf;
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
#ifdef USE_RICH_EDIT
			case ID_SHOWFILESIZE:
				hCur = SetCursor(LoadCursor(NULL, IDC_WAIT));

				bHideMessage = FALSE;
				wsprintf(szStatusMessage, GetString(IDS_BYTE_LENGTH), CalculateFileSize());
				UpdateStatus();
				if (!bShowStatus)
					SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(ID_SHOWSTATUS, 0), 0);
				SetCursor(hCur);
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
			case ID_SET_MACRO_10:
				i = LOWORD(wParam) - ID_SET_MACRO_1;
				key = NULL;
				szTmp = (LPTSTR)GetShadowSelection(&l);
				if (l > MAXMACRO - 1) {
					ERROROUT(GetString(IDS_MACRO_LENGTH_ERROR));
					break;
				}
				if (!EncodeWithEscapeSeqs(szTmp)) {
					ERROROUT(GetString(IDS_MACRO_LENGTH_ERROR));
					break;
				}
				SSTRCPY(options.MacroArray[i], szTmp);
				wsprintf(szTmp2, _T("szMacroArray%d"), i);
				if (!g_bIniMode)
					RegCreateKeyEx(HKEY_CURRENT_USER, STR_REGKEY, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &key, NULL);
				if (!SaveOption(key, szTmp2, REG_SZ, (LPBYTE)options.MacroArray[i], MAXMACRO))
					ReportLastError();
				if (!g_bIniMode)
					RegCloseKey(key);

				break;
			case ID_MACRO_1:
			case ID_MACRO_2:
			case ID_MACRO_3:
			case ID_MACRO_4:
			case ID_MACRO_5:
			case ID_MACRO_6:
			case ID_MACRO_7:
			case ID_MACRO_8:
			case ID_MACRO_9:
			case ID_MACRO_10:
				szTmp = options.MacroArray[LOWORD(wParam) - ID_MACRO_1];
				if (!SCNUL(szTmp)[0]) break;
				lstrcpy(szTmp2, szTmp);
				if (!ParseForEscapeSeqs(szTmp2, NULL, GetString(IDS_ESCAPE_CTX_MACRO))) break;
				SendMessage(client, EM_REPLACESEL, (WPARAM)TRUE, (LPARAM)szTmp2);
#ifdef USE_RICH_EDIT
				InvalidateRect(client, NULL, TRUE);
#endif
				break;
			case ID_COMMIT_WORDWRAP:
				if (bWordWrap) {
					LONG lLineLen, lMaxLine;
					hCur = SetCursor(LoadCursor(NULL, IDC_WAIT));

					l = GetWindowTextLength(client);
					lMaxLine = SendMessage(client, EM_GETLINECOUNT, 0, 0);
					szBuf = (LPTSTR) HeapAlloc(globalHeap, HEAP_ZERO_MEMORY, (l + lMaxLine + 1) * sizeof(TCHAR));

					for (i = 0; i < lMaxLine; i++) {
						j = SendMessage(client, EM_LINEINDEX, (WPARAM)i, 0);
						lLineLen = SendMessage(client, EM_LINELENGTH, (WPARAM)j, 0);

						szTmp = (LPTSTR) HeapAlloc(globalHeap, HEAP_ZERO_MEMORY, (lLineLen+3) * sizeof(TCHAR));
						*((LPWORD)szTmp) = (USHORT)(lLineLen + 1);
						SendMessage(client, EM_GETLINE, (WPARAM)i, (LPARAM)(LPCTSTR)szTmp);
						szTmp[lLineLen] = _T('\0');
						lstrcat(szBuf, szTmp);
						if (i < lMaxLine - 1) {
#ifdef USE_RICH_EDIT
							lstrcat(szBuf, _T("\r"));
#else
							lstrcat(szBuf, _T("\r\n"));
#endif
						}
						HeapFree(globalHeap, 0, (HGLOBAL)szTmp);
					}

					cr.cpMin = 0;
					cr.cpMax = -1;
					SendMessage(client, WM_SETREDRAW, (WPARAM)FALSE, 0);

#ifdef USE_RICH_EDIT
					SendMessage(client, EM_EXSETSEL, 0, (LPARAM)&cr);
#else
					SendMessage(client, EM_SETSEL, (WPARAM)cr.cpMin, (LPARAM)cr.cpMax);
#endif
					SendMessage(client, EM_REPLACESEL, (WPARAM)TRUE, (LPARAM)szBuf);
					cr.cpMax = 0;
#ifdef USE_RICH_EDIT
					SendMessage(client, EM_EXSETSEL, 0, (LPARAM)&cr);
#else
					SendMessage(client, EM_SETSEL, (WPARAM)cr.cpMin, (LPARAM)cr.cpMax);
#endif
					SendMessage(client, WM_SETREDRAW, (WPARAM)TRUE, 0);
					HeapFree(globalHeap, 0, (HGLOBAL)szBuf);
					InvalidateRect(client, NULL, TRUE);
					SetCursor(hCur);
				}
				break;
			case ID_SHIFT_ENTER:
			case ID_CONTROL_SHIFT_ENTER:
#ifdef USE_RICH_EDIT
				GetKeyboardState(keys);
				keys[VK_SHIFT] &= 0x7F;
				SetKeyboardState(keys);
				SendMessage(client, WM_KEYDOWN, (WPARAM)VK_RETURN, 0);
				keys[VK_SHIFT] |= 0x80;
				SetKeyboardState(keys);
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
			case ID_FAV_EDIT:
				GetModuleFileName(hinstThis, szTmp, MAXFN);
				//ShellExecute(NULL, NULL, szTmp, SCNUL(szFav), SCNUL(szDir), SW_SHOWNORMAL);
				ExecuteProgram(szTmp, szFav);
				break;
			case ID_FAV_RELOAD:
				PopulateFavourites();
				break;
		}
		break;
	default:
		if (Msg == uFindReplaceMsg) {
			lpfr = (LPFINDREPLACE)lParam;
			szBuf = gTmpBuf;
			szRepl = gTmpBuf2;

			if (lpfr->Flags & FR_DIALOGTERM) {
				switch (frDlgId){
					case ID_REPLACE:
						for (i = 0; i < NUMFINDS; i++) {
							SendDlgItemMessage(hdlgFind, ID_DROP_REPLACE, CB_GETLBTEXT, i, (WPARAM)szBuf);
							SSTRCPY(ReplaceArray[i], szBuf);
						}
					case ID_FIND:
						for (i = 0; i < NUMFINDS; i++) {
							SendDlgItemMessage(hdlgFind, ID_DROP_FIND, CB_GETLBTEXT, i, (WPARAM)szBuf);
							SSTRCPY(FindArray[i], szBuf);
						}
						break;
					case ID_INSERT_TEXT:
						for (i = 0; i < NUMINSERTS; i++) {
							SendDlgItemMessage(hdlgFind, ID_DROP_INSERT, CB_GETLBTEXT, i, (WPARAM)szBuf);
							SSTRCPY(InsertArray[i], szBuf);
						}
						break;
					break;
				}
				hdlgFind = NULL;
				frDlgId = -1;
				FREE(szInsert);
				return FALSE;
			}

			szRepl[0] = _T('\0');
			switch (frDlgId) {
				case ID_REPLACE:
					SendDlgItemMessage(hdlgFind, ID_DROP_REPLACE, WM_GETTEXT, MAXFIND, (WPARAM)szRepl);
					nPos = SendDlgItemMessage(hdlgFind, ID_DROP_REPLACE, CB_FINDSTRINGEXACT, 0, (WPARAM)szRepl);
					if (nPos == CB_ERR) {
						if (SendDlgItemMessage(hdlgFind, ID_DROP_REPLACE, CB_GETCOUNT, 0, 0) >= NUMFINDS)
							SendDlgItemMessage(hdlgFind, ID_DROP_REPLACE, CB_DELETESTRING, (LPARAM)NUMFINDS-1, 0);
					} else {
						SendDlgItemMessage(hdlgFind, ID_DROP_REPLACE, CB_DELETESTRING, (LPARAM)nPos, 0);
					}
					SendDlgItemMessage(hdlgFind, ID_DROP_REPLACE, CB_INSERTSTRING, 0, (WPARAM)szRepl);
					SendDlgItemMessage(hdlgFind, ID_DROP_REPLACE, CB_SETCURSEL, (LPARAM)0, 0);
					if (!bNoFindHidden && !ParseForEscapeSeqs(szRepl, NULL, GetString(IDS_ESCAPE_CTX_REPLACE))) return FALSE;
				case ID_FIND:
					SendDlgItemMessage(hdlgFind, ID_DROP_FIND, WM_GETTEXT, MAXFIND, (WPARAM)szBuf);
					nPos = SendDlgItemMessage(hdlgFind, ID_DROP_FIND, CB_FINDSTRINGEXACT, 0, (WPARAM)szBuf);
					if (nPos == CB_ERR) {
						if (SendDlgItemMessage(hdlgFind, ID_DROP_FIND, CB_GETCOUNT, 0, 0) >= NUMFINDS)
							SendDlgItemMessage(hdlgFind, ID_DROP_FIND, CB_DELETESTRING, (LPARAM)NUMFINDS-1, 0);
					} else {
						SendDlgItemMessage(hdlgFind, ID_DROP_FIND, CB_DELETESTRING, (LPARAM)nPos, 0);
					}
					SendDlgItemMessage(hdlgFind, ID_DROP_FIND, CB_INSERTSTRING, 0, (WPARAM)szBuf);
					SendDlgItemMessage(hdlgFind, ID_DROP_FIND, CB_SETCURSEL, (LPARAM)0, 0);
					if (!bNoFindHidden) {
						FREE(pbFindTextAny);
						pbFindTextAny = (LPBYTE)HeapAlloc(globalHeap, HEAP_ZERO_MEMORY, lstrlen(szBuf)+1);
						if (!ParseForEscapeSeqs(szBuf, pbFindTextAny, GetString(IDS_ESCAPE_CTX_FIND))) return FALSE;
					}
					break;
				case ID_INSERT_TEXT:
				case ID_PASTE_MUL:
					SendDlgItemMessage(hdlgFind, IDC_NUM, WM_GETTEXT, 16, (WPARAM)szMsg);
					i = _ttol(szMsg);
					if (!i) return FALSE;
					SendDlgItemMessage(hdlgFind, ID_DROP_INSERT, WM_GETTEXT, MAXINSERT, (WPARAM)szBuf);
					if (frDlgId == ID_PASTE_MUL && lstrcmp(szBuf, SCNUL(szInsert)) != 0)
						frDlgId = ID_INSERT_TEXT;
					if (frDlgId == ID_INSERT_TEXT) {
						nPos = SendDlgItemMessage(hdlgFind, ID_DROP_INSERT, CB_FINDSTRINGEXACT, 0, (WPARAM)szBuf);
						if (nPos == CB_ERR) {
							if (SendDlgItemMessage(hdlgFind, ID_DROP_INSERT, CB_GETCOUNT, 0, 0) >= NUMINSERTS)
								SendDlgItemMessage(hdlgFind, ID_DROP_INSERT, CB_DELETESTRING, (LPARAM)NUMINSERTS-1, 0);
						} else {
							SendDlgItemMessage(hdlgFind, ID_DROP_INSERT, CB_DELETESTRING, (LPARAM)nPos, 0);
						}
						SendDlgItemMessage(hdlgFind, ID_DROP_INSERT, CB_INSERTSTRING, 0, (WPARAM)szBuf);
						SendDlgItemMessage(hdlgFind, ID_DROP_INSERT, CB_SETCURSEL, (LPARAM)0, 0);
					}
					if (!bNoFindHidden && !ParseForEscapeSeqs(szBuf, NULL, GetString(IDS_ESCAPE_CTX_INSERT))) return FALSE;
					l = lstrlen(szBuf);
					if (!options.bNoWarningPrompt && (LONGLONG)l * i > LARGEPASTEWARN) {
						wsprintf(szMsg, GetString(IDS_LARGE_PASTE_WARNING), (LONGLONG)l * i);
						nPos = MessageBox(hdlgFind, szMsg, STR_METAPAD, MB_ICONQUESTION|MB_OKCANCEL);
						if (nPos == IDCANCEL)
							return FALSE;
					}
					SSTRCPY(szInsert, szBuf);
#ifdef USE_RICH_EDIT
					FixTextBuffer(szInsert);
#else
					FixTextBufferLE(&szInsert);
#endif
					szNew = (LPTSTR)HeapAlloc(globalHeap, 0, ((LONGLONG)l * i + 1) * sizeof(TCHAR));
					if (!szNew) {
						ReportLastError();
						return FALSE;
					}
					for (sz = szNew, l = lstrlen(szInsert); i; i--, sz += l)
						lstrcpy(sz, szInsert);
					*sz = _T('\0');
					SendMessage(client, EM_REPLACESEL, (WPARAM)TRUE, (LPARAM)szNew);
					bCloseAfterInsert = (BST_CHECKED == SendDlgItemMessage(hdlgFind, IDC_CLOSE_AFTER_INSERT, BM_GETCHECK, 0, 0));
					if (bCloseAfterInsert) PostMessage(hdlgFind, WM_CLOSE, 0, 0);
					FREE(szNew);
					UpdateStatus();
				default:
					return FALSE;
			}

			SSTRCPY(szFindText, szBuf);
			SSTRCPY(szReplaceText, szRepl);
			bMatchCase = (BOOL) (lpfr->Flags & FR_MATCHCASE);
			bDown = (BOOL) (lpfr->Flags & FR_DOWN);
			bWholeWord = (BOOL) (lpfr->Flags & FR_WHOLEWORD);
			if (lpfr->Flags & FR_REPLACEALL) {
				l = ReplaceAll(hdlgFind, szFindText, szReplaceText, szTmp, BST_CHECKED == SendDlgItemMessage(hdlgFind, IDC_RADIO_SELECTION, BM_GETCHECK, 0, 0), bMatchCase, bWholeWord, pbFindTextAny, NULL, NULL);
				UpdateStatus();
				return FALSE;
			}
			else if (lpfr->Flags & FR_FINDNEXT) {
				SearchFile(szBuf, bMatchCase, FALSE, bDown, bWholeWord, pbFindTextAny);
				if (frDlgId == ID_FIND) {
					bCloseAfterFind = (BST_CHECKED == SendDlgItemMessage(hdlgFind, IDC_CLOSE_AFTER_FIND, BM_GETCHECK, 0, 0));
					if (bCloseAfterFind) PostMessage(hdlgFind, WM_CLOSE, 0, 0);
				}
			}
			else if (lpfr->Flags & FR_REPLACE) {
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
				if (szBuf[lstrlen(szBuf) - 1] == _T('\r')) {
					if (szRepl[lstrlen(szRepl) - 1] != _T('\r')) {
						b = TRUE;
					}
				}
				lstrcpy(szReplaceHold, szRepl);

#endif
				if (SearchFile(szBuf, bMatchCase, FALSE, bDown, bWholeWord, pbFindTextAny)) {
#ifdef USE_RICH_EDIT // warning -- big kluge!
					TCHAR ch[2] = {_T('\0'), _T('\0')};

					if (b) {
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

					if (b) {
						SendMessage(client, EM_EXGETSEL, 0, (LPARAM)&cr);
						--cr.cpMin;
						--cr.cpMax;
						SendMessage(client, EM_EXSETSEL, 0, (LPARAM)&cr);
					}

					InvalidateRect(client, NULL, TRUE);
#else
					SendMessage(client, EM_REPLACESEL, (WPARAM)TRUE, (LPARAM)szRepl);
#endif
					SearchFile(szBuf, bMatchCase, FALSE, bDown, bWholeWord, pbFindTextAny);
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
	LoadFile(SCNUL(szFile), TRUE, TRUE);
	if (bLoading) {
		bLoading = FALSE;
		if (!SCNUL(szFile)[0]) {
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
	int nCmdLen;
	HMENU hmenu;
	MENUITEMINFO mio;
	CHARRANGE crLineCol = {-1, -1};
	LPTSTR szCmdLine;
	BOOL bSkipLanguagePlugin = FALSE;
	TCHAR* bufFn = gTmpBuf;
	TCHAR chOption;
#ifdef _DEBUG
	int _x=0;
	__stktop = (size_t)&_x;
#endif

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
	ZeroMemory(&options, sizeof(options));
	szFile = NULL;
	szCaptionFile = NULL;
	szFav = NULL;
	szDir = NULL;
	szMetapadIni = NULL;
	szFindText = NULL;
	szReplaceText = NULL;
	szCustomFilter = NULL;
	pbFindTextAny = NULL;
	szShadow = NULL;
	shadowLen = shadowAlloc = shadowRngEnd = 0;
	ZeroMemory(FindArray, sizeof(FindArray));
	ZeroMemory(ReplaceArray, sizeof(ReplaceArray));
	ZeroMemory(InsertArray, sizeof(InsertArray));

#if defined(BUILD_METAPAD_UNICODE) && ( !defined(__MINGW32__) || defined(__MINGW64_VERSION_MAJOR) )
	szCmdLine = GetCommandLine();
	szCmdLine = _tcschr(szCmdLine, _T(' ')) + 1;
#else
	szCmdLine = lpCmdLine;
#endif

	{
		TCHAR* pch;
		GetModuleFileName(hinstThis, bufFn, MAXFN);
		SSTRCPYA(szMetapadIni, bufFn, 11);

		pch = _tcsrchr(szMetapadIni, _T('\\'));
		++pch;
		*pch = _T('\0');
		lstrcat(szMetapadIni, _T("metapad.ini"));
	}
	
	if (szCmdLine[0]) {
		nCmdLen = lstrlen(szCmdLine);
		while(szCmdLine[0] == _T(' ')) { szCmdLine++; nCmdLen--; }
		while(szCmdLine[nCmdLen - 1] == _T(' ')) nCmdLen--;
		if (nCmdLen > 1 && szCmdLine[0] == _T('/')) {
			chOption = (TCHAR)CharLower((LPTSTR)szCmdLine[1]);
			szCmdLine += 2; nCmdLen -= 2;
			switch (chOption){
			case _T('s'):
				bSkipLanguagePlugin = TRUE;
				break;
			case _T('v'):
				g_bDisablePluginVersionChecking = TRUE;
				break;
			case _T('i'):
				g_bIniMode = TRUE;
				break;
			case _T('m'):
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
			default:
				szCmdLine -= 2; nCmdLen += 2;
			}
		}
	}
	
	if (!g_bIniMode) {
		WIN32_FIND_DATA FindFileData;
		HANDLE handle;

		if ((handle = FindFirstFile(SCNUL(szMetapadIni), &FindFileData)) != INVALID_HANDLE_VALUE) {
			FindClose(handle);
			g_bIniMode = TRUE;
		}
	}
	GetCurrentDirectory(MAXFN, bufFn);
	SSTRCPY(szDir, bufFn);
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
	bCloseAfterInsert = FALSE;
	bNoFindHidden = TRUE;
	bTransparent = FALSE;

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

		if (SCNUL(options.szFavDir)[0] == _T('\0') || (handle = FindFirstFile(options.szFavDir, &FindFileData)) == INVALID_HANDLE_VALUE) {
			TCHAR* pch;
			GetModuleFileName(hInstance, bufFn, MAXFN);
			SSTRCPYA(szFav, bufFn, lstrlen(STR_FAV_FILE)+4);

			pch = _tcsrchr(szFav, _T('\\'));
			++pch;
			*pch = _T('\0');
		}
		else {
			FindClose(handle);
			SSTRCPYA(szFav, options.szFavDir, lstrlen(STR_FAV_FILE)+4);
			lstrcat(szFav, _T("\\"));
		}

		lstrcat(szFav, STR_FAV_FILE);

		PopulateFavourites();
	}

	MakeNewFile();
	
	if (szCmdLine[0]) {
		if (szFile) HeapFree(globalHeap, 0, (HGLOBAL)szFile);
		szFile = (LPTSTR)HeapAlloc(globalHeap, 0, (nCmdLen+1) * sizeof(TCHAR));
		while(szCmdLine[0] == _T(' ')) { szCmdLine++; nCmdLen--; }
		while(szCmdLine[nCmdLen - 1] == _T(' ')) nCmdLen--;
		if (nCmdLen > 1 && szCmdLine[0] == _T('/')) {
			chOption = (TCHAR)CharLower((LPTSTR)szCmdLine[1]);
			szCmdLine += 2; nCmdLen -= 2;
			while(szCmdLine[0] == _T(' ')) { szCmdLine++; nCmdLen--; }
			switch (chOption){
			case _T('p'):
				if (szCmdLine[0] == _T('\"') && szCmdLine[nCmdLen - 1] == _T('\"'))
					lstrcpyn(szFile, szCmdLine + 1, nCmdLen - 1);
				else
					lstrcpyn(szFile, szCmdLine, nCmdLen + 1);
				LoadFile(szFile, FALSE, FALSE);
				SSTRCPY(szCaptionFile, szFile);
				PrintContents();
				CleanUp();
				return TRUE;
			case _T('g'): {
				TCHAR szNum[6];
				int nRlen, nClen;

				ZeroMemory(szNum, sizeof(szNum));
				for (nRlen = 0; _istdigit(szCmdLine[nRlen]); nRlen++)
					szNum[nRlen] = szCmdLine[nRlen];
				crLineCol.cpMin = _ttol(szNum);

				ZeroMemory(szNum, sizeof(szNum));
				for (nClen = 0; _istdigit(szCmdLine[nClen + nRlen + 1]); nClen++)
					szNum[nClen] = szCmdLine[nRlen + nClen + 1];
				crLineCol.cpMax = _ttol(szNum);

				if (szCmdLine[2 + nClen + nRlen] == _T('\"') && szCmdLine[nCmdLen - 1] == _T('\"'))
					lstrcpyn(szFile, szCmdLine + nClen + nRlen + 3, nCmdLen - 1);
				else
					lstrcpyn(szFile, szCmdLine + nClen + nRlen + 2, nCmdLen + 1);
				break;
			}
			case _T('e'):
				if (szCmdLine[0] == _T('\"') && szCmdLine[nCmdLen - 1] == _T('\"'))
					lstrcpyn(szFile, szCmdLine + 1, nCmdLen - 1);
				else
					lstrcpyn(szFile, szCmdLine, nCmdLen + 1);

				crLineCol.cpMax = crLineCol.cpMin = 0x7fffffff;
				break;
			default:
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

		bufFn[0] = _T('\0');
		GetFullPathName(szFile, MAXFN, bufFn, NULL);
		if (bufFn[0])
			SSTRCPY(szFile, bufFn);
		
		ExpandFilename(szFile, &szFile);
		
#ifdef USE_RICH_EDIT
		{
			DWORD dwID;
			TCHAR* szBuffer = gTmpBuf;

			wsprintf(szBuffer, STR_CAPTION_FILE, SCNUL(szCaptionFile));
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

	//SetWindowLongPtr(hwnd, GWL_EXSTYLE, GetWindowLongPtr(hwnd, GWL_EXSTYLE) | WS_CLIPCHILDREN);	//prevent flicker at expense of CPU time (no effect on Aero?)
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
