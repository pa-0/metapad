/****************************************************************************/
/*                                                                          */
/*   metapad 3.6+                                                           */
/*                                                                          */
/*   Copyright (C) 2021-2024 SoBiT Corp                                     */
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

#ifdef UNICODE
#include <wchar.h>
#define _CF_TEXT CF_UNICODETEXT
#define _SF_TEXT (SF_TEXT || SF_UNICODE)
#define TM_ENCODING TM_MULTICODEPAGE
#else
#define _CF_TEXT CF_TEXT
#define _SF_TEXT SF_TEXT
#define TM_ENCODING TM_SINGLECODEPAGE
#endif

#include "include/metapad.h"
#include <shellapi.h>
#include <commdlg.h>
#include <commctrl.h>
#include <winuser.h>


#ifndef TBSTYLE_FLAT
#define TBSTYLE_FLAT 0x0800
#endif

#if defined(__MINGW32__) && !defined(__MINGW64_VERSION_MAJOR)
 #define PROPSHEETHEADER_V1_SIZE 40
#elif !defined(__MINGW32__)
// #include "include/w32crt.h"
#endif

#ifdef _MSC_VER
#pragma intrinsic(memset)
#endif

//#pragma comment(linker, "/OPT:NOWIN98" )






SLWA SetLWA = NULL;
HANDLE globalHeap = NULL;
HINSTANCE hinstThis = NULL;
UINT_PTR tmrUpdate = 0;
WNDPROC wpOrigFindProc = NULL;
int _fltused = 0x9875; // see CMISCDAT.C for more info on this



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



/**
 * Cleanup objects.
 */
void CleanUp(void)
{
	KillTimer(hwnd, IDT_UPDATE);
	tmrUpdate = 0;
	if (szShadow) {
		szShadow -= 8;
		kfree(&szShadow);
	}
	if (hrecentmenu) DestroyMenu(hrecentmenu);
	if (hfontmain) DeleteObject(hfontmain);
	if (hfontfind) DeleteObject(hfontfind);
	if (hthread) CloseHandle(hthread);
	UnloadLanguagePlugin();

#ifdef USE_RICH_EDIT
	DestroyWindow(client);
	FreeLibrary(GetModuleHandleA(STR_RICHDLL));
#else
	if (BackBrush) DeleteObject(BackBrush);
#endif
}

/**
 * Get status bar height.
 *
 * @return Statusbar's height if bShowStatus, 0 otherwise.
 */
int GetStatusHeight(void){
	return (bShowStatus ? nStatusHeight : 0);
}

/**
 * Get toolbar height.
 *
 * @return Toolbar's height if bShowToolbar, 0 otherwise.
 */
int GetToolbarHeight(void){
	return (bShowToolbar ? nToolbarHeight : 0);
}

void SwitchReadOnly(BOOL bNewVal){
	bReadOnly = bNewVal;
	if (GetCheckedState(GetMenu(hwnd), ID_READONLY, FALSE) != bNewVal)
		GetCheckedState(GetMenu(hwnd), ID_READONLY, TRUE);
}

BOOL GetCheckedState(HMENU hmenu, UINT nID, BOOL bToggle){
	MENUITEMINFO mio;
	mio.cbSize = sizeof(MENUITEMINFO);
	mio.fMask = MIIM_STATE;
	GetMenuItemInfo(hmenu, nID, FALSE, &mio);
	if (bToggle) {
		mio.fState = mio.fState == MFS_CHECKED ? MFS_UNCHECKED : MFS_CHECKED;
		SetMenuItemInfo(hmenu, nID, FALSE, &mio);
		mio.fState = mio.fState == MFS_CHECKED ? MFS_UNCHECKED : MFS_CHECKED;
	}
	return mio.fState == MFS_CHECKED ? TRUE : FALSE;
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

void CreateStatusbar(void) {
	RECT rect;
	int nPaneSizes[NUMSTATPANES] = {0};
	status = CreateWindowEx(WS_EX_DLGMODALFRAME, STATUSCLASSNAME, kemptyStr,
		WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | SBT_NOBORDERS | SBARS_SIZEGRIP,
		0, 0, 0, 0, hwnd, (HMENU)ID_STATUSBAR, hinstThis, NULL);
	SendMessage(status, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), 0);
	GetWindowRect(status, &rect);
	nStatusHeight = rect.bottom - rect.top - 5;
	SendMessage(status, SB_SETPARTS, NUMSTATPANES, (DWORD_PTR)(LPINT)nPaneSizes);
}

void CreateClient(HWND hParent, LPCTSTR szText, BOOL bWrap)
{
#ifdef USE_RICH_EDIT
	LRESULT lres;
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
	SendMessage(client, EM_SETEVENTMASK, 0, (LPARAM)(ENM_LINK | ENM_CHANGE | ENM_SELCHANGE));
	SendMessage(client, EM_EXLIMITTEXT, 0, (LPARAM)(DWORD)0x7fffffff);

	SendMessage(client, EM_SETTEXTMODE, (WPARAM)TM_ENCODING, 0);
	//SendMessage(client, EM_SETEDITSTYLE, (WPARAM)SES_USECRLF, (LPARAM)SES_USECRLF);	//obsolete, doesn't work
	SendMessage(client, EM_SETEDITSTYLE, (WPARAM)SES_XLTCRCRLFTOCR, (LPARAM)SES_XLTCRCRLFTOCR);
	lres = SendMessage(client, EM_GETLANGOPTIONS, 0, 0);	//Fix non-TTF font rendering without quirks that TM_PLAINTEXT causes (e.g. incorrect cursor position at EOF, can't set tab size, etc)
	lres &= ~IMF_AUTOFONT;									//Credit: archives.miloush.net/michkap/archive/2006/02/13/531110.html
	SendMessage(client, EM_SETLANGOPTIONS, 0, lres);

	if (!bWordWrap && !options.bHideScrollbars) { // Hack for initially drawing hscrollbar for Richedit
		SetWindowLongPtr(client, GWL_STYLE, GetWindowLongPtr(client, GWL_STYLE) | WS_HSCROLL);
		SetWindowPos(client, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
	}

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

#ifdef USE_RICH_EDIT
void UpdateWindowText(void) {
	LPCTSTR szBuffer = GetShadowBuffer(NULL);
	bLoading = TRUE;
	RestoreClientView(0, FALSE, TRUE, TRUE);
	SetTabStops();
	SetWindowText(client, szBuffer);
	bLoading = FALSE;
	SendMessage(client, EM_EMPTYUNDOBUFFER, 0, 0);
	UpdateStatus(TRUE);
	RestoreClientView(0, TRUE, TRUE, TRUE);
	InvalidateRect(client, NULL, TRUE);
}
#endif

/**
 * Updates window title to reflect current working file's name and state.
 */
void UpdateCaption(void) {
	USHORT u;
	LPTSTR sz = NULL;
	if (!szCaptionFile) {
		if (szFile && *szFile){
			GetReadableFilename(szFile, &sz);
			kstrdupao(&szCaptionFile, sz, 32, 8);
			kfree(&sz);
			if (options.bNoCaptionDir && (sz = lstrrchr(szCaptionFile+8, _T('\\'))))
				lstrcpy(szCaptionFile+8, sz+1);
		} else
			kstrdupao(&szCaptionFile, GetString(IDS_NEW_FILE), 32, 8);
		szCaptionFile[5] = szCaptionFile[7] = _T(' ');
		szCaptionFile[6] = _T('*');
		u = (USHORT)lstrlen(szCaptionFile+8);
		*((LPWORD)szCaptionFile+1) = u;
		lstrcat(szCaptionFile+8, GetString(STR_CAPTION_FILE));
		*((LPWORD)szCaptionFile) = (USHORT)lstrlen(szCaptionFile+8) - u - 1;
		lstrcat(szCaptionFile+8, GetString(IDS_READONLY_INDICATOR));
		szCaptionFile[u + 8 + *((LPWORD)szCaptionFile)] = _T('\0');
	}
	u = *((LPWORD)szCaptionFile+1) + 8;
	szCaptionFile[u] = _T(' ');
	if (bReadOnly) 
		szCaptionFile[u + *((LPWORD)szCaptionFile)] = _T(' ');
	SetWindowText(hwnd, szCaptionFile + (bDirtyFile ? 5 : 8));
	szCaptionFile[u] = _T('\0');
	szCaptionFile[u + *((LPWORD)szCaptionFile)] = _T('\0');
}

void QueueUpdateStatus(){
	if (!tmrUpdate)
		tmrUpdate = SetTimer(hwnd, IDT_UPDATE, 16, NULL);
	else if (updateThrottle - GetTickCount() < THROTTLEMAX)
		updateThrottle += MIN(updateTime/2, 96);
}
void UpdateStatus(BOOL refresh) {
	static TCHAR szPane[48];
	static CHARRANGE oldcr = {0}, ecr = {0};
	static int nPaneSizes[NUMSTATPANES] = {0}, txtScale = 0;
	static BYTE chkhash[32];
	int tpsz[NUMSTATPANES] = {0};
	DWORD i, j, st, bytes, chars;
	LONG lLine = -1, lLines, lCol;
	BOOL full = TRUE, statup = FALSE, oldDirty = bDirtyFile;
	CHARRANGE cr;
	LPCTSTR szBuf;
	HDC dc;
	SIZE sz;

	if (!bStarted) return;
	bDirtyStatus |= refresh;
	KillTimer(hwnd, IDT_UPDATE);
	tmrUpdate = 0;
	st = GetTickCount();
	if (st < updateThrottle && (i = updateThrottle - st) < THROTTLEMAX*4) {
		//printf("\n++++%d", (updateThrottle - st));
		full = FALSE;
	}
	if (!txtScale) txtScale = (int)(LOWORD(GetDialogBaseUnits())/STATUS_FONT_CONST);
	//printf("\n....%d", updateTime);

	if (full) {
		if (bDirtyStatus){
			if (status && bShowStatus){
				//printf("F");
				if (!((nFormat >> 30) & 1)) {
					lstrcpy(szPane, _T("  "));
					if (nFormat >> 31)
						wsprintf(szPane+2, GetString(FC_ENC_CODEPAGE), (WORD)nFormat);
					else
						lstrcpy(szPane+2, GetString((WORD)nFormat));
					lstrcat(szPane+3, _T(" "));
					lstrcat(szPane+4, GetString((nFormat >> 16) & 0xfff));
					dc = GetDC(status);
					SelectObject(dc, GetStockObject(DEFAULT_GUI_FONT));
					GetTextExtentPoint(dc, _T("0"), 1, &sz);
					txtScale = sz.cx;
					GetTextExtentPoint(dc, szPane, lstrlen(szPane), &sz);
					nPaneSizes[SBPANE_TYPE] = sz.cx + txtScale * 2;
					ReleaseDC(status, dc);
					SendMessage(status, SB_SETTEXT, (WPARAM) SBPANE_TYPE, (LPARAM)(LPTSTR)szPane);
					nFormat |= (1<<30);
				}
				if (!bLoading) {
					wsprintf(szPane, GetString(IDS_STATFMT_BYTES), FormatNumber(bytes = CalcTextSize(NULL, NULL, 0, nFormat, TRUE, &chars), options.bDigitGrp, 0, 0));
					nPaneSizes[SBPANE_SIZE] = txtScale * (lstrlen(szPane) - 1);
					SendMessage(status, SB_SETTEXT, (WPARAM) SBPANE_SIZE, (LPARAM)(LPTSTR)szPane);
				}
				statup = TRUE;
			} else
				chars = GetTextChars(NULL);

			if (!bLoading) {
				bDirtyFile = TRUE;
				if (chars == savedChars && (!chars || savedFormat == (nFormat & 0x8fffffff))) {
					if (!chars) bDirtyFile = FALSE;
					else {
						j = 32 / sizeof(TCHAR);
						szBuf = GetShadowRange(0, j, -1, &i, NULL);
						if (!memcmp(szBuf, savedHead, i * sizeof(TCHAR))) {
							szBuf = GetShadowRange(chars < j ? 0 : chars-j, chars, -1, &i, NULL);
							if (!memcmp(szBuf, savedFoot, i * sizeof(TCHAR))) {
								if ((chars *= sizeof(TCHAR)) <= 64) bDirtyFile = FALSE;
								else {
									szBuf = GetShadowBuffer(&i);
									EvaHash(((LPBYTE)szBuf)+32, chars-64, chkhash);
									if (!memcmp(savedHash, chkhash, 32))
										bDirtyFile = FALSE;
								}
							}
						}
					}
				}
			}

			if (!szCaptionFile || oldDirty != bDirtyFile)
				UpdateCaption();

			if (toolbar && bShowToolbar) {
				SendMessage(toolbar, TB_CHECKBUTTON, (WPARAM)ID_EDIT_WORDWRAP, MAKELONG(bWordWrap, 0));
				SendMessage(toolbar, TB_CHECKBUTTON, (WPARAM)ID_FONT_PRIMARY, MAKELONG(bPrimaryFont, 0));
				SendMessage(toolbar, TB_CHECKBUTTON, (WPARAM)ID_ALWAYSONTOP, MAKELONG(bAlwaysOnTop, 0));
				SendMessage(toolbar, TB_ENABLEBUTTON, (WPARAM)ID_RELOAD_CURRENT, MAKELONG(SCNUL(szFile)[0] != _T('\0'), 0));
				SendMessage(toolbar, TB_ENABLEBUTTON, (WPARAM)ID_MYEDIT_UNDO, MAKELONG(SendMessage(client, EM_CANUNDO, 0, 0), 0));
#ifdef USE_RICH_EDIT
				SendMessage(toolbar, TB_ENABLEBUTTON, (WPARAM)ID_MYEDIT_REDO, MAKELONG(SendMessage(client, EM_CANREDO, 0, 0), 0));
#endif
			}
		}
	}
	if (bDirtyStatus) {
		//printf("D");
#ifdef USE_RICH_EDIT
		if (bInsertMode) 	lstrcpy(szPane, GetString(IDS_STATFMT_INS));
		else 				lstrcpy(szPane, GetString(IDS_STATFMT_OVR));
		nPaneSizes[SBPANE_INS] = 4 * txtScale + 7;
		SendMessage(status, SB_SETTEXT, (WPARAM) SBPANE_INS, (LPARAM)(LPTSTR)szPane);
#else
		nPaneSizes[SBPANE_INS] = 0;
#endif
		statup = TRUE;
	}

	if (!bLoading && ((status && bShowStatus) || (full && toolbar && bShowToolbar))){
		lCol = GetColNum(-1, -1, NULL, &lLine, &cr);
		if (full && toolbar && bShowToolbar) {
			SendMessage(toolbar, TB_ENABLEBUTTON, (WPARAM)ID_MYEDIT_CUT, MAKELONG((cr.cpMin != cr.cpMax), 0));
			SendMessage(toolbar, TB_ENABLEBUTTON, (WPARAM)ID_MYEDIT_COPY, MAKELONG((cr.cpMin != cr.cpMax), 0));
			SendMessage(toolbar, TB_ENABLEBUTTON, (WPARAM)ID_MYEDIT_PASTE, MAKELONG(IsClipboardFormatAvailable(_CF_TEXT), 0));
		}
	}
	if (status && bShowStatus && (bDirtyStatus || cr.cpMin != oldcr.cpMin || cr.cpMax != oldcr.cpMax)) {
		if (!bLoading) {
			//printf("s");
			oldcr = full ? cr : ecr;
			lLines = SendMessage(client, EM_GETLINECOUNT, 0, 0);
			wsprintf(szPane, GetString(IDS_STATFMT_LINE), FormatNumber(lLine+1, options.bDigitGrp, 0, 0), FormatNumber(lLines, options.bDigitGrp, 0, 1));
			SendMessage(status, SB_SETTEXT, (WPARAM) SBPANE_LINE, (LPARAM)(LPTSTR)szPane);
			nPaneSizes[SBPANE_LINE] = txtScale * lstrlen(szPane);
			wsprintf(szPane, GetString(IDS_STATFMT_COL), lCol);
			SendMessage(status, SB_SETTEXT, (WPARAM) SBPANE_COL, (LPARAM)(LPTSTR)szPane);
			nPaneSizes[SBPANE_COL] = txtScale * lstrlen(szPane) + 2;

			if (cr.cpMax > cr.cpMin) {
				*szPane = 0;
				if (full)
					wsprintf(szPane, GetString(IDS_STATFMT_BYTES), FormatNumber(CalcTextSize(NULL, &cr, 0, nFormat, FALSE, NULL), options.bDigitGrp, 0, 0));
				i = GetColNum(cr.cpMin, -1, NULL, &lLine, NULL);
				j = GetColNum(cr.cpMax, -1, NULL, &lLines, NULL);
				wsprintf(szPane+24, GetString(IDS_STATFMT_LINES), FormatNumber(lLines-lLine+1, options.bDigitGrp, 0, 0));
				wsprintf(szStatusMessage, GetString(IDS_STATFMT_SEL), FormatNumber(lLine+1, options.bDigitGrp, 0, 0), i, FormatNumber(lLines+1, options.bDigitGrp, 0, 1), j, szPane+24, szPane);
			} else
				*szStatusMessage = 0;
		}
		nPaneSizes[SBPANE_MESSAGE] = txtScale * (lstrlen(szStatusMessage) + 5);
		SendMessage(status, SB_SETTEXT, (WPARAM) SBPANE_MESSAGE | SBT_NOBORDERS, (LPARAM)szStatusMessage);
		statup = TRUE;
	}
	if (statup) {
		//printf("u");
		for (i = 0; i < NUMSTATPANES; i++)
			tpsz[i] = (i ? tpsz[i-1] : 0) + nPaneSizes[i];
		SendMessage(status, SB_SETPARTS, NUMSTATPANES, (DWORD_PTR)(LPINT)tpsz);
	}

	if (full){
		bDirtyStatus = FALSE;
		updateTime = (i = GetTickCount()) - st;
		updateThrottle = i + updateTime;
		//printf("\n%d", (int)(updateTime / 2));
	} else
		tmrUpdate = SetTimer(hwnd, IDT_UPDATE, 64, NULL);
}

LRESULT APIENTRY PageSetupProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_INITDIALOG:
		LocalizeDialog(IDD_PAGE_SETUP, hwndDlg);
		break;
	}
	return FALSE;
}

LRESULT APIENTRY FindProc(HWND hwndFind, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	HMENU hmenu, hsub;
	BOOL bOkEna = TRUE;
	RECT rc;
	LPTSTR sz, szNew;
	LPTSTR szTmp = gTmpBuf;
	DWORD i=0, j=0, nPos;
	//printf("%X %X %X\n", uMsg, wParam, lParam);
	switch (uMsg) {
	case WM_INITDIALOG:
		switch(frDlgId) {
			case ID_FIND: LocalizeDialog(IDD_FIND, hwndFind); break;
			case ID_REPLACE: LocalizeDialog(IDD_REPLACE, hwndFind); break;
			case ID_INSERT_TEXT:
			case ID_PASTE_MUL: LocalizeDialog(IDD_INSERT, hwndFind); break;
		}
		break;
	case WM_SHOWWINDOW:
		return TRUE;
	}
	if (!hdlgFind) return FALSE;
	switch (uMsg) {
	case WM_COMMAND:
		switch(LOWORD(wParam)){
		case IDC_ESCAPE:
		case IDC_ESCAPE2:
			if (HIWORD(wParam) != BN_CLICKED) break;
			hmenu = LocalizeMenu(IDR_ESCAPE_SEQUENCES);
			hsub = GetSubMenu(hmenu, 0);
			SendMessage(hwnd, WM_INITMENUPOPUP, (WPARAM)hsub, MAKELPARAM(1, FALSE));
			switch(LOWORD(wParam)){
				case IDC_ESCAPE:
					GetWindowRect(GetDlgItem(hwndFind, IDC_ESCAPE), &rc);
					SendDlgItemMessage(hwndFind, (frDlgId == ID_FIND || frDlgId == ID_REPLACE? IDC_DROP_FIND : IDC_DROP_INSERT), WM_GETTEXT, (WPARAM)MAXFIND, (LPARAM)szTmp);
					break;
				case IDC_ESCAPE2:
					GetWindowRect(GetDlgItem(hwndFind, IDC_ESCAPE2), &rc);
					SendDlgItemMessage(hwndFind, IDC_DROP_REPLACE, WM_GETTEXT, (WPARAM)MAXFIND, (LPARAM)szTmp);
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
				EnableMenuItem(hsub, ID_ESCAPE_WILD0, MF_BYCOMMAND | MF_GRAYED);
				EnableMenuItem(hsub, ID_ESCAPE_WILD1, MF_BYCOMMAND | MF_GRAYED);
				EnableMenuItem(hsub, ID_ESCAPE_REP0, MF_BYCOMMAND | MF_GRAYED);
				EnableMenuItem(hsub, ID_ESCAPE_REP1, MF_BYCOMMAND | MF_GRAYED);
				EnableMenuItem(hsub, ID_ESCAPE_RAND, MF_BYCOMMAND | MF_GRAYED);
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
			switch (TrackPopupMenuEx(hsub, TPM_RETURNCMD | TPM_TOPALIGN | TPM_LEFTALIGN | TPM_RIGHTBUTTON, rc.left, rc.bottom, hwnd, NULL)) {
				case ID_ESCAPE_NEWLINE:		lstrcat(szTmp, _T("\\n")); break;
				case ID_ESCAPE_TAB:			lstrcat(szTmp, _T("\\t")); break;
				case ID_ESCAPE_BACKSLASH:	lstrcat(szTmp, _T("\\\\")); break;
				case ID_ESCAPE_ANY:			lstrcat(szTmp, _T("\\_")); break;
				case ID_ESCAPE_WILD0:		lstrcat(szTmp, _T("\\?")); break;
				case ID_ESCAPE_WILD1:		lstrcat(szTmp, _T("\\*")); break;
				case ID_ESCAPE_REP0:		lstrcat(szTmp, _T("\\-")); break;
				case ID_ESCAPE_REP1:		lstrcat(szTmp, _T("\\+")); break;
				case ID_ESCAPE_RAND:		lstrcat(szTmp, _T("\\$")); break;
				case ID_ESCAPE_HEX: 		lstrcat(szTmp, _T("\\x")); break;
				case ID_ESCAPE_DEC: 		lstrcat(szTmp, _T("\\d")); break;
				case ID_ESCAPE_OCT: 		lstrcat(szTmp, _T("\\")); break;
				case ID_ESCAPE_BIN: 		lstrcat(szTmp, _T("\\b")); break;
				case ID_ESCAPE_HEXU: 		lstrcat(szTmp, _T("\\u")); break;
				case ID_ESCAPE_HEXS: 		lstrcat(szTmp, _T("\\X\\")); j = 1; break;
				case ID_ESCAPE_HEXSU: 		lstrcat(szTmp, _T("\\U\\")); j = 1; break;
				case ID_ESCAPE_B64S: 		lstrcat(szTmp, _T("\\S\\")); j = 1; break;
				case ID_ESCAPE_B64SU: 		lstrcat(szTmp, _T("\\W\\")); j = 1; break;
				case ID_ESCAPE_DISABLE:
					bNoFindHidden = !GetCheckedState(hsub, ID_ESCAPE_DISABLE, TRUE);
					break;
			}

			switch(LOWORD(wParam)){
				case IDC_ESCAPE:
					i = (frDlgId == ID_FIND || frDlgId == ID_REPLACE? IDC_DROP_FIND : IDC_DROP_INSERT); break;
				case IDC_ESCAPE2:
					i = IDC_DROP_REPLACE; break;
			}
			SendDlgItemMessage(hwndFind, i, WM_SETTEXT, (WPARAM)(BOOL)FALSE, (LPARAM)szTmp);
			SetFocus(GetDlgItem(hwndFind, i));
			if (j) {
				j = lstrlen(szTmp);
				SendDlgItemMessage(hwndFind, i, CB_SETEDITSEL, 0, MAKELPARAM(j-1, j-1));
			} else
				SendDlgItemMessage(hwndFind, i, CB_SETEDITSEL, 0, -1);

			DestroyMenu(hmenu);
			break;
		case IDC_NUM:
		case IDC_DROP_INSERT:
			if (HIWORD(wParam) != EN_CHANGE && HIWORD(wParam) != CBN_EDITCHANGE && HIWORD(wParam) != CBN_SELCHANGE && HIWORD(wParam) != CBN_SETFOCUS) break;
			bOkEna &= (0 != SendDlgItemMessage(hdlgFind, IDC_DROP_INSERT, WM_GETTEXTLENGTH, 0, 0) || SendDlgItemMessage(hdlgFind, IDC_DROP_INSERT, CB_GETCURSEL, 0, 0) >= 0);
			bOkEna &= (0 != SendDlgItemMessage(hdlgFind, IDC_NUM, WM_GETTEXTLENGTH, 0, 0));
		case IDC_DROP_FIND:
		case IDC_DROP_REPLACE:
			if (HIWORD(wParam) != EN_CHANGE && HIWORD(wParam) != CBN_EDITCHANGE && HIWORD(wParam) != CBN_SELCHANGE && HIWORD(wParam) != CBN_SETFOCUS) break;
			bOkEna &= (0 != SendDlgItemMessage(hdlgFind, IDC_DROP_FIND, WM_GETTEXTLENGTH, 0, 0) || SendDlgItemMessage(hdlgFind, IDC_DROP_FIND, CB_GETCURSEL, 0, 0) >= 0);
			EnableWindow(GetDlgItem(hdlgFind, IDOK), bOkEna);
			if (frDlgId == ID_REPLACE){
				EnableWindow(GetDlgItem(hdlgFind, 1024), bOkEna);
				EnableWindow(GetDlgItem(hdlgFind, 1025), bOkEna);
				SendDlgItemMessage(hdlgFind, IDC_DROP_FIND, WM_GETTEXT, MAXFIND, (WPARAM)szTmp);
				if (!bNoFindHidden) bOkEna &= ParseForEscapeSeqs(szTmp, NULL, NULL);
				i = lstrlen(szTmp);
				SendDlgItemMessage(hdlgFind, IDC_DROP_REPLACE, WM_GETTEXT, MAXFIND, (WPARAM)szTmp);
				if (!bNoFindHidden) bOkEna &= ParseForEscapeSeqs(szTmp, NULL, NULL);
				j = lstrlen(szTmp);
				bOkEna &= i > j;
				EnableWindow(GetDlgItem(hdlgFind, 1026), bOkEna);
			}
			break;
		case IDOK:
		case 0x400:
		case 0x401:
		case 0x402:
			if (HIWORD(wParam) != BN_CLICKED) break;
			switch (frDlgId) {
				case ID_REPLACE:
					SendDlgItemMessage(hdlgFind, IDC_DROP_REPLACE, WM_GETTEXT, MAXFIND, (WPARAM)szTmp);
					nPos = SendDlgItemMessage(hdlgFind, IDC_DROP_REPLACE, CB_FINDSTRINGEXACT, 0, (WPARAM)szTmp);
					if (nPos == CB_ERR && !szTmp[0]) {
						for (i = 0, j = MIN(NUMFINDS, SendDlgItemMessage(hdlgFind, IDC_DROP_REPLACE, CB_GETCOUNT, 0, 0)); i < j; i++) {
							if (!SendDlgItemMessage(hdlgFind, IDC_DROP_REPLACE, CB_GETLBTEXTLEN, i, 0)) {
								nPos = i;
								break;
							}
						}
					}
					if (nPos == CB_ERR) {
						if (SendDlgItemMessage(hdlgFind, IDC_DROP_REPLACE, CB_GETCOUNT, 0, 0) >= NUMFINDS)
							SendDlgItemMessage(hdlgFind, IDC_DROP_REPLACE, CB_DELETESTRING, (LPARAM)NUMFINDS-1, 0);
					} else {
						SendDlgItemMessage(hdlgFind, IDC_DROP_REPLACE, CB_DELETESTRING, (LPARAM)nPos, 0);
					}
					SendDlgItemMessage(hdlgFind, IDC_DROP_REPLACE, CB_INSERTSTRING, 0, (WPARAM)szTmp);
					SendDlgItemMessage(hdlgFind, IDC_DROP_REPLACE, CB_SETCURSEL, (LPARAM)0, 0);
					if (!bNoFindHidden && !ParseForEscapeSeqs(szTmp, &pbReplaceTextSpec, GetString(IDS_ESCAPE_CTX_REPLACE))) return FALSE;
					kstrdup(&szReplaceText, szTmp);
				case ID_FIND:
					SendDlgItemMessage(hdlgFind, IDC_DROP_FIND, WM_GETTEXT, MAXFIND, (WPARAM)szTmp);
					nPos = SendDlgItemMessage(hdlgFind, IDC_DROP_FIND, CB_FINDSTRINGEXACT, 0, (WPARAM)szTmp);
					if (nPos == CB_ERR) {
						if (SendDlgItemMessage(hdlgFind, IDC_DROP_FIND, CB_GETCOUNT, 0, 0) >= NUMFINDS)
							SendDlgItemMessage(hdlgFind, IDC_DROP_FIND, CB_DELETESTRING, (LPARAM)NUMFINDS-1, 0);
					} else {
						SendDlgItemMessage(hdlgFind, IDC_DROP_FIND, CB_DELETESTRING, (LPARAM)nPos, 0);
					}
					SendDlgItemMessage(hdlgFind, IDC_DROP_FIND, CB_INSERTSTRING, 0, (WPARAM)szTmp);
					SendDlgItemMessage(hdlgFind, IDC_DROP_FIND, CB_SETCURSEL, (LPARAM)0, 0);
					bWholeWord = SendDlgItemMessage(hdlgFind, 0x410, BM_GETCHECK, 0, 0);
					bMatchCase = SendDlgItemMessage(hdlgFind, 0x411, BM_GETCHECK, 0, 0);
					bDown = (frDlgId != ID_FIND || SendDlgItemMessage(hdlgFind, 0x421, BM_GETCHECK, 0, 0));
					if (!bNoFindHidden && !ParseForEscapeSeqs(szTmp, &pbFindTextSpec, GetString(IDS_ESCAPE_CTX_FIND))) return FALSE;
					kstrdup(&szFindText, szTmp);
					break;
				case ID_INSERT_TEXT:
				case ID_PASTE_MUL:
					SendDlgItemMessage(hdlgFind, IDC_NUM, WM_GETTEXT, 16, (WPARAM)szTmp);
					j = _ttol(szTmp);
					if (!j) return FALSE;
					SendDlgItemMessage(hdlgFind, IDC_DROP_INSERT, WM_GETTEXT, MAXFIND, (WPARAM)szTmp);
					if (frDlgId == ID_PASTE_MUL && lstrcmp(szTmp, SCNUL(szInsert)) != 0)
						frDlgId = ID_INSERT_TEXT;
					if (frDlgId == ID_INSERT_TEXT) {
						nPos = SendDlgItemMessage(hdlgFind, IDC_DROP_INSERT, CB_FINDSTRINGEXACT, 0, (WPARAM)szTmp);
						if (nPos == CB_ERR) {
							if (SendDlgItemMessage(hdlgFind, IDC_DROP_INSERT, CB_GETCOUNT, 0, 0) >= NUMINSERTS)
								SendDlgItemMessage(hdlgFind, IDC_DROP_INSERT, CB_DELETESTRING, (LPARAM)NUMINSERTS-1, 0);
						} else {
							SendDlgItemMessage(hdlgFind, IDC_DROP_INSERT, CB_DELETESTRING, (LPARAM)nPos, 0);
						}
						SendDlgItemMessage(hdlgFind, IDC_DROP_INSERT, CB_INSERTSTRING, 0, (WPARAM)szTmp);
						SendDlgItemMessage(hdlgFind, IDC_DROP_INSERT, CB_SETCURSEL, (LPARAM)0, 0);
					}
					if (!bNoFindHidden && !ParseForEscapeSeqs(szTmp, NULL, GetString(IDS_ESCAPE_CTX_INSERT))) return FALSE;
					if (!(i = lstrlen(szTmp)))
						return FALSE;
					kstrdup(&szInsert, szTmp);
					if (!options.bNoWarningPrompt && (LONGLONG)i * j > LARGEPASTEWARN) {
						wsprintf(szTmp, GetString(IDS_LARGE_PASTE_WARNING), FormatNumber((LONGLONG)i * j, options.bDigitGrp, 0, 0));
						nPos = MessageBox(hdlgFind, szTmp, GetString(STR_METAPAD), MB_ICONQUESTION|MB_OKCANCEL);
						if (nPos == IDCANCEL)
							return FALSE;
					}
					
					//GetLineFmt(szInsert, l1, 0, &i, &j, &k, &nPos, NULL);
					//ImportLineFmt(&szInsert, &l1, (nFormat >> 16) & 0xfff, i, j, k, nPos, &bOkEna);
					if (!(szNew = kallocs((LONGLONG)i * j + 1)))
						return FALSE;
					for (sz = szNew, i = lstrlen(szInsert); j; j--, sz += i)
						lstrcpy(sz, szInsert);
					*sz = _T('\0');
					SendMessage(client, EM_REPLACESEL, (WPARAM)TRUE, (LPARAM)szNew);
					bCloseAfterInsert = (BST_CHECKED == SendDlgItemMessage(hdlgFind, IDC_CLOSE_AFTER_INSERT, BM_GETCHECK, 0, 0));
					if (bCloseAfterInsert) PostMessage(hdlgFind, WM_CLOSE, 0, 0);
					kfree(&szNew);
					QueueUpdateStatus();
				default:
					return FALSE;
			}

			i = 0;
			switch (LOWORD(wParam)){
				case 0x400:
					if (!ReplaceAll(NULL, 1, 0, &szFindText, &szReplaceText, &pbFindTextSpec, &pbReplaceTextSpec, NULL, TRUE, bMatchCase, bWholeWord, 1, 0, FALSE, NULL, NULL)) {
						if (!SearchFile(szFindText, bMatchCase, TRUE, bWholeWord, pbFindTextSpec)) break;
						if (!ReplaceAll(NULL, 1, 0, &szFindText, &szReplaceText, &pbFindTextSpec, &pbReplaceTextSpec, NULL, TRUE, bMatchCase, bWholeWord, 1, 0, FALSE, NULL, NULL)) break;
					}				
				case IDOK:
					i = SearchFile(szFindText, bMatchCase, bDown || frDlgId != ID_FIND, bWholeWord, pbFindTextSpec);
					if (frDlgId == ID_FIND) {
						bCloseAfterFind = (BST_CHECKED == SendDlgItemMessage(hdlgFind, IDC_CLOSE_AFTER_FIND, BM_GETCHECK, 0, 0));
						if (bCloseAfterFind) PostMessage(hdlgFind, WM_CLOSE, 0, 0);
					}
					break;
				case 0x402:
				case 0x401:
					i = ReplaceAll(hdlgFind, 1, (LOWORD(wParam) == 0x402 ? 1 : 0), &szFindText, &szReplaceText, &pbFindTextSpec, &pbReplaceTextSpec, szTmp, bOkEna = (BST_CHECKED == SendDlgItemMessage(hdlgFind, IDC_RADIO_SELECTION, BM_GETCHECK, 0, 0)), bMatchCase, bWholeWord, 0, 0, FALSE, NULL, NULL);
					QueueUpdateStatus();
					bCloseAfterReplace = (BST_CHECKED == SendDlgItemMessage(hdlgFind, IDC_CLOSE_AFTER_REPLACE, BM_GETCHECK, 0, 0));
					if (bCloseAfterReplace) PostMessage(hdlgFind, WM_CLOSE, 0, 0);
					if (!bOkEna) return FALSE;
					break;
			}
			if (i && !IsSelectionVisible()) SendMessage(hwnd, WM_COMMAND, (WPARAM)ID_SCROLLTO_SELA, 0);
			break;
		case IDCANCEL:
			if (HIWORD(wParam) != BN_CLICKED) break;
			switch (frDlgId){
				case ID_REPLACE:
					for (i = 0, j = MIN(NUMFINDS, SendDlgItemMessage(hdlgFind, IDC_DROP_REPLACE, CB_GETCOUNT, 0, 0)); i < j; i++) {
						SendDlgItemMessage(hdlgFind, IDC_DROP_REPLACE, CB_GETLBTEXT, i, (WPARAM)szTmp);
						kstrdup(&ReplaceArray[i], szTmp);
					}
				case ID_FIND:
					for (i = 0, j = MIN(NUMFINDS, SendDlgItemMessage(hdlgFind, IDC_DROP_FIND, CB_GETCOUNT, 0, 0)); i < j; i++) {
						SendDlgItemMessage(hdlgFind, IDC_DROP_FIND, CB_GETLBTEXT, i, (WPARAM)szTmp);
						kstrdup(&FindArray[i], szTmp);
					}
					bWholeWord = SendDlgItemMessage(hdlgFind, 0x410, BM_GETCHECK, 0, 0);
					bMatchCase = SendDlgItemMessage(hdlgFind, 0x411, BM_GETCHECK, 0, 0);
					bDown = (frDlgId != ID_FIND || SendDlgItemMessage(hdlgFind, 0x421, BM_GETCHECK, 0, 0));
					break;
				case ID_INSERT_TEXT:
					for (i = 0, j = MIN(NUMINSERTS, SendDlgItemMessage(hdlgFind, IDC_DROP_INSERT, CB_GETCOUNT, 0, 0)); i < j; i++) {
						SendDlgItemMessage(hdlgFind, IDC_DROP_INSERT, CB_GETLBTEXT, i, (WPARAM)szTmp);
						kstrdup(&InsertArray[i], szTmp);
					}
					break;
			}
			gfr.Flags = FR_ENABLETEMPLATE | FR_ENABLEHOOK;
			if (wpOrigFindProc) SetWindowLongPtr(hdlgFind, GWLP_WNDPROC, (LONG_PTR)wpOrigFindProc);
			hdlgFind = NULL;
			frDlgId = -1;
			kfree(&szInsert);
			break;
		}
		break;
	case WM_KEYDOWN:
		if ((lParam & 0xffff) > 1) break;
	case WM_SYSCOMMAND:
		switch(wParam) {
			case 'V':
			case 'Z':
			case 'Y':
				if (!(GetKeyState(VK_SHIFT) & 0x8000)) return FALSE;
			case 'F':
			case 'H':
			case 'S':
			case VK_TAB:
				if (!(GetKeyState(VK_CONTROL) & 0x8000)) return FALSE;
		}
		switch(wParam) {
			case 'V': SendMessage(hwnd, WM_COMMAND, (WPARAM)ID_PASTE_MUL, 0); return TRUE;
			case 'F': SendMessage(hwnd, WM_COMMAND, (WPARAM)ID_FIND, 0); return TRUE;
			case 'H': SendMessage(hwnd, WM_COMMAND, (WPARAM)ID_REPLACE, 0); return TRUE;
			case 'S': SendMessage(hwnd, WM_COMMAND, (WPARAM)ID_MYFILE_SAVE, 0); return TRUE;
			case 'Z': SendMessage(hwnd, WM_COMMAND, (WPARAM)ID_MYEDIT_UNDO, 0); return TRUE;
#ifdef USE_RICH_EDIT
			case 'Y': SendMessage(hwnd, WM_COMMAND, (WPARAM)ID_MYEDIT_REDO, 0); return TRUE;
#endif
			case VK_F3: SendMessage(hwnd, WM_COMMAND, (WPARAM)(GetKeyState(VK_SHIFT) & 0x8000 ? ID_FIND_PREV : ID_FIND_NEXT), 0); return TRUE;
			case VK_F2: SendMessage(hwnd, WM_COMMAND, (WPARAM)ID_INSERT_TEXT, 0); return TRUE;
			case VK_F12: SendMessage(hdlgFind, WM_COMMAND, MAKEWPARAM(IDC_ESCAPE, BN_CLICKED), 0); return TRUE;
			case VK_F1:
			case VK_TAB: SendMessage(hwnd, WM_ACTIVATE, 0, 0); return TRUE;
		}
		break;
	}
	if (wpOrigFindProc) return CallWindowProc(wpOrigFindProc, hwndFind, uMsg, wParam, lParam);
	return FALSE;
}

LRESULT APIENTRY EditProc(HWND hwndEdit, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	CHARRANGE cr, cr2;
	LRESULT lRes;
	LPTSTR sz, szBuf;

	//printf("\n%X %X %X  ", uMsg, wParam, lParam);
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
			lRes = CallWindowProc(wpOrigEditProc, hwndEdit, uMsg, wParam, lParam);
			QueueUpdateStatus();
			return lRes;
	case WM_LBUTTONDBLCLK:
#ifndef USE_RICH_EDIT
		if (bSmartSelect) {
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
			HMENU hmenu = LocalizeMenu(IDR_POPUP);
			HMENU hsub = GetSubMenu(hmenu, 0);
			POINT pt;
			UINT id;

/** dropped feature */
/*			if (bLinkMenu) {
				bLinkMenu = FALSE;
			}
*/
			cr = GetSelection();
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
			lRes = CallWindowProc(wpOrigEditProc, hwndEdit, uMsg, wParam, lParam);
			QueueUpdateStatus();
			return lRes;
	case WM_CHAR:
		if (options.bAutoIndent && (TCHAR)wParam == _T('\r')) {
			cr = GetSelection();
#ifdef USE_RICH_EDIT
			cr.cpMin--; cr.cpMax--;
#endif
			sz = (szBuf = ((LPTSTR)GetShadowLine(-1, cr.cpMin, NULL, NULL, &cr2))) - 1;
			if (!*szBuf) break;
			while (*++sz == _T('\t') || *sz == _T(' ')) ;
			*sz = _T('\0');
			szBuf[MAX(0,cr.cpMin - cr2.cpMin)] = _T('\0');
#ifndef USE_RICH_EDIT
			*--szBuf = _T('\n');
			*--szBuf = _T('\r');
#endif
			CallWindowProc(wpOrigEditProc, client, EM_REPLACESEL, (WPARAM)TRUE, (LPARAM)szBuf);
			return 0;
		}
		break;
/*	case EM_REPLACESEL:
// This seems to cause more refresh blinking rather than preventing it... 
		{
			LONG lStartLine, lEndLine;
			break;

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

			SendMessage(hwndEdit, WM_SETREDRAW, (WPARAM)FALSE, 0);
			lRes = CallWindowProc(wpOrigEditProc, hwndEdit, uMsg, wParam, lParam);
			SendMessage(hwndEdit, WM_SETREDRAW, (WPARAM)TRUE, 0);
			SetWindowPos(client, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

			return lRes;
		}*/
	case WM_IME_NOTIFY:
		//Selection was changed (LE)
		if (!bLoading)
			QueueUpdateStatus();
		break;
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

LRESULT CALLBACK AbortPrintJob(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
		case WM_INITDIALOG:
			CenterWindow(hwndDlg);
			SetDlgItemText(hwndDlg, IDD_ABORT_PRINT+2, SCNUL8(szCaptionFile)+8);
			LocalizeDialog(IDD_ABORT_PRINT, hwndDlg);
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

	if (pd.Flags & PD_SELECTION)
		szBuffer = GetShadowSelection(&l, NULL);
	else
		szBuffer = GetShadowBuffer(&l);
	lStringLen = (LONG)l;
	bPrint = TRUE;

	if (SetAbortProc(pd.hDC, AbortDlgProc) == SP_ERROR) {
		ERROROUT(GetString(IDS_PRINT_ABORT_ERROR));
		return;
	}

	hdlgCancel = CreateDialog(hinstThis, MAKEINTRESOURCE(IDD_ABORT_PRINT), hwnd, (DLGPROC) AbortPrintJob);
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
	di.lpszDocName = SCNUL8(szCaptionFile)+8;

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
	GETTEXTLENGTHEX gtl = {0};

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

	hdlgCancel = CreateDialog(hinstThis, MAKEINTRESOURCE(IDD_ABORT_PRINT), hwnd, (DLGPROC) AbortPrintJob);
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
	di.lpszDocName = SCNUL8(szCaptionFile)+8;
	di.lpszOutput = NULL;

	lTextLength = SendMessage(client, EM_GETTEXTLENGTHEX, (WPARAM)&gtl, 0);
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
	MessageBox(NULL, msgBuf, GetString(STR_METAPAD), MB_OK | MB_ICONSTOP);
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
	UINT i = 1;

	if (options.nMaxMRU == 0)
		return;
	if (!g_bIniMode && RegCreateKeyEx(HKEY_CURRENT_USER, GetString(STR_REGKEY), 0, NULL, 0, KEY_ALL_ACCESS, NULL, &key, NULL) != ERROR_SUCCESS) {
		ReportLastError();
		return;
	}
	wsprintf(szKey, GetString(IDSS_MRU), nMRUTop);
	LoadOptionString(key, szKey, &szBuffer, MAXFN);

	if (lstrcmp(SCNUL(szFullPath), SCNUL(szBuffer)) != 0) {
		if (++nMRUTop > options.nMaxMRU) {
			nMRUTop = 1;
		}
		SaveOption(key, GetString(IDSS_MRUTOP), REG_DWORD, (LPBYTE)&nMRUTop, sizeof(int));
		wsprintf(szKey, GetString(IDSS_MRU), nMRUTop);
		LoadOptionString(key, szKey, &szTopVal, MAXFN);
		SaveOption(key, szKey, REG_SZ, (LPBYTE)szFullPath, MAXFN);
		for (i = 1; i <= options.nMaxMRU; ++i) {
			if (i == nMRUTop) continue;
			wsprintf(szKey,GetString(IDSS_MRU), i);
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
	kfree(&szBuffer);
	kfree(&szTopVal);
}

void PopulateFavourites(void) {
	LPTSTR szBuffer = NULL;
	TCHAR *szName = gTmpBuf, *szMenu = gTmpBuf+MAXFN;
	INT i, j, cnt, accel;
	MENUITEMINFO mio;
	HMENU hmenu = GetMenu(hwnd);
	HMENU hsub = GetSubMenu(hmenu, MPOS_FAVE);

	bHasFaves = FALSE;
	while (GetMenuItemCount(hsub) > 4)
		DeleteMenu(hsub, 4, MF_BYPOSITION);
	mio.cbSize = sizeof(MENUITEMINFO);
	mio.fMask = MIIM_TYPE | MIIM_ID;
	szBuffer = kallocs(0xffff);

	if (GetPrivateProfileString(GetString(STR_FAV_APPNAME), NULL, NULL, szBuffer, 0xffff, SCNUL(szFav))) {
		bHasFaves = TRUE;
		for (i = 0, j = 0, cnt = accel = 1; /*cnt <= MAXFAVES*/; ++j, ++i) {
			szName[j] = szBuffer[i];
			if (szBuffer[i] == _T('\0')) {
				if (lstrcmp(szName, _T("-")) == 0) {
					mio.fType = MFT_SEPARATOR;
					InsertMenuItem(hsub, cnt + 3, TRUE, &mio);
					++cnt;
					j = -1;
				} else {
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
	kfree(&szBuffer);
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
		hsub = GetSubMenu(GetSubMenu(hmenu, 0), MPOS_FILE_RECENT);

	if (options.nMaxMRU == 0) {
		if (options.bRecentOnOwn)
			EnableMenuItem(hmenu, 1, MF_BYPOSITION | MF_GRAYED);
		else
			EnableMenuItem(GetSubMenu(hmenu, 0), MPOS_FILE_RECENT, MF_BYPOSITION | MF_GRAYED);
		return;
	}
	else {
		if (options.bRecentOnOwn)
			EnableMenuItem(hmenu, 1, MF_BYPOSITION | MF_ENABLED);
		else
			EnableMenuItem(GetSubMenu(hmenu, 0), MPOS_FILE_RECENT, MF_BYPOSITION | MF_ENABLED);
	}

	if (!g_bIniMode && RegCreateKeyEx(HKEY_CURRENT_USER, GetString(STR_REGKEY), 0, NULL, 0, KEY_ALL_ACCESS, NULL, &key, &nPrevSave) != ERROR_SUCCESS)
		ReportLastError();

	if (g_bIniMode || nPrevSave == REG_OPENED_EXISTING_KEY) {
		UINT i, num = 1, cnt = 0;
		while (hsub && GetMenuItemCount(hsub))
			DeleteMenu(hsub, 0, MF_BYPOSITION);

		LoadOptionNumeric(key, GetString(IDSS_MRUTOP), (LPBYTE)&nMRUTop, sizeof(int));
		mio.cbSize = sizeof(MENUITEMINFO);
		mio.fMask = MIIM_TYPE | MIIM_ID;
		mio.fType = MFT_STRING;

		i = nMRUTop;
		while (cnt < options.nMaxMRU) {
			wsprintf(szKey, GetString(IDSS_MRU), i);
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
	kfree(&szBuff2);
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
	LONG lLine, lLineLen;
	LPCTSTR szBuf;
	CHARRANGE cr, cr2;

#ifdef USE_RICH_EDIT
	SendMessage(client, EM_EXGETSEL, 0, (LPARAM)&cr);
	lLine = SendMessage(client, EM_EXLINEFROMCHAR, 0, (LPARAM)cr.cpMin);
	if (lLine == SendMessage(client, EM_EXLINEFROMCHAR, 0, (LPARAM)cr.cpMax)) {
#else
	SendMessage(client, EM_GETSEL, (WPARAM)&cr.cpMin, (LPARAM)&cr.cpMax);
	lLine = SendMessage(client, EM_LINEFROMCHAR, (WPARAM)cr.cpMin, 0);
	if (lLine == SendMessage(client, EM_LINEFROMCHAR, (WPARAM)cr.cpMax, 0)) {
#endif
		if (!*(szBuf = GetShadowLine(lLine, -1, &lLineLen, NULL, &cr2))) return;
		cr.cpMin -= cr2.cpMin;
		cr.cpMax -= cr2.cpMin;
		if (cr.cpMin == cr.cpMax && bAutoSelect) {
			if (bSmart) {
				if (_istprint(szBuf[cr.cpMax]) && !_istspace(szBuf[cr.cpMax])) {
					cr.cpMax++;
				} else {
					while (cr.cpMin && (_istalnum(szBuf[cr.cpMin-1]) || szBuf[cr.cpMin-1] == _T('_')))
						cr.cpMin--;
				}
				if (_istalnum(szBuf[cr.cpMin]) || szBuf[cr.cpMin] == _T('_')) {
					while (cr.cpMin && (_istalnum(szBuf[cr.cpMin-1]) || szBuf[cr.cpMin-1] == _T('_')))
						cr.cpMin--;
					while (cr.cpMax < lLineLen && (_istalnum(szBuf[cr.cpMax]) || szBuf[cr.cpMax] == _T('_')))
						cr.cpMax++;
				}
			} else {
				if (_istprint(szBuf[cr.cpMax])) {
					while (cr.cpMin && (!_istspace(szBuf[cr.cpMin-1])))
						cr.cpMin--;
					while (cr.cpMax < lLineLen && (!_istspace(szBuf[cr.cpMax])))
						cr.cpMax++;
				} else {
					while (cr.cpMin && (_istspace(szBuf[cr.cpMin-1])))
						cr.cpMin--;
					while (cr.cpMin && (!_istspace(szBuf[cr.cpMin-1])))
						cr.cpMin--;
				}
				while (cr.cpMax < lLineLen && _istspace(szBuf[cr.cpMax]) && szBuf[cr.cpMax])
					cr.cpMax++;
			}
		}
		if (bAutoSelect || cr.cpMin != cr.cpMax) {
			if (target) {
				lLineLen = MIN(cr.cpMax - cr.cpMin, MAXFIND-1);
				kfree(target);
				*target = kallocs(lLineLen+1);
				lstrcpyn(*target, szBuf + cr.cpMin, lLineLen+1);
				(*target)[lLineLen] = _T('\0');
			}
			cr.cpMin += cr2.cpMin;
			cr.cpMax += cr2.cpMin;
			SetSelection(cr);
		}
		UpdateStatus(FALSE);
	}
}

#ifdef STREAMING
DWORD CALLBACK EditStreamIn(DWORD dwCookie, LPBYTE pbBuff, LONG cb, LONG *pcb)
{
	LPTSTR szBuffer = (LPTSTR)dwCookie;
	LONG lBufferLength = lstrlen(szBuffer);
	static LONG nBytesDone = 0;

	lstrcpy(szStatusMessage, _T("Loading file... %d            "));
	wsprintf(szStatusMessage, szStatusMessage, nBytesDone);
	UpdateStatus(TRUE);

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

void SetFont(HFONT* phfnt, BOOL bPrimary) {
	LOGFONT f, logfind;
	HDC clientdc;
	if (*phfnt) DeleteObject(*phfnt);
	if (hfontfind) DeleteObject(hfontfind);
	if ((bPrimary ? options.nPrimaryFont : options.nSecondaryFont) == 0) {
		*phfnt = GetStockObject(bPrimary ? SYSTEM_FIXED_FONT : ANSI_FIXED_FONT);
		hfontfind = *phfnt;
		fontHt = -3;
	} else {
		f = (bPrimary ? options.PrimaryFont : options.SecondaryFont);
		*phfnt = CreateFontIndirect(&f);
		CopyMemory((PVOID)&logfind, (CONST VOID*)(&f), sizeof(LOGFONT));
		clientdc = GetDC(client);
		fontHt = -f.lfHeight;
		if (fontHt < 0) fontHt = MulDiv(-fontHt, GetDeviceCaps(clientdc, LOGPIXELSY), 72);
		logfind.lfHeight = -MulDiv(LOWORD(GetDialogBaseUnits())+2, GetDeviceCaps(clientdc, LOGPIXELSY), 72);
		hfontfind = CreateFontIndirect(&logfind);
		ReleaseDC(client, clientdc);
	}
}

BOOL SetClientFont(BOOL bPrimary) {
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
		if (!options.bSuppressUndoBufferPrompt && MessageBox(hwnd, GetString(IDS_FONT_UNDO_WARNING), GetString(STR_METAPAD), MB_ICONEXCLAMATION | MB_OKCANCEL) == IDCANCEL)
			return FALSE;
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

void SetTabStops(void) {
#ifdef USE_RICH_EDIT
	INT nWidth, nTmp, ct;
	PARAFORMAT pf;
	HDC clientDC;
	BOOL bOldDirty = bDirtyFile;
	CHARRANGE cr, cr2;
	LPCTSTR ts;

	bDirtyFile = TRUE;
	SendMessage(client, EM_EXGETSEL, 0, (LPARAM)&cr);
	SendMessage(client, WM_SETREDRAW, (WPARAM)FALSE, 0);
	cr2.cpMin = 0;
	cr2.cpMax = -1;
	SendMessage(client, EM_EXSETSEL, 0, (LPARAM)&cr2);

	clientDC = GetDC(client);
	SelectObject (clientDC, hfontmain);
	if (!GetCharWidth32(clientDC, (UINT)VK_SPACE, (UINT)VK_SPACE, &nWidth) && !GetCharWidth(clientDC, (UINT)VK_SPACE, (UINT)VK_SPACE, &nWidth))
		ERROROUT(GetString(IDS_CHAR_WIDTH_ERROR));
	ReleaseDC(client, clientDC);

	ZeroMemory(&pf, sizeof(pf));
	pf.cbSize = sizeof(PARAFORMAT);
	SendMessage(client, EM_GETPARAFORMAT, 0, (LPARAM)&pf);
	pf.dwMask = PFM_TABSTOPS;
	pf.cTabCount = MAX_TAB_STOPS;
	nTmp = nWidth * 15 * options.nTabStops;
	for (ct = 0; ct < pf.cTabCount; ct++)
		pf.rgxTabs[ct] = (ct+1) * nTmp;

	ts = GetShadowBuffer(NULL);
	SendMessage(client, WM_SETTEXT, 0, (LPARAM)kemptyStr);
	if (!SendMessage(client, EM_SETPARAFORMAT, 0, (LPARAM)(PARAFORMAT FAR *)&pf))
		ERROROUT(GetString(IDS_PARA_FORMAT_ERROR));
	SendMessage(client, WM_SETTEXT, 0, (LPARAM)kemptyStr);
	SendMessage(client, WM_SETTEXT, 0, (LPARAM)ts);

	SendMessage(client, EM_EXSETSEL, 0, (LPARAM)&cr);
	SendMessage(client, WM_SETREDRAW, (WPARAM)TRUE, 0);
	InvalidateRect(hwnd, NULL, TRUE);
	bDirtyFile = bOldDirty;
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
		mio.dwTypeData = (LPTSTR)GetString(ID_READONLY);
		mio.wID = ID_READONLY;
		if (bReadOnly)
			mio.fState = MFS_CHECKED;
		else
			mio.fState = 0;

		InsertMenuItem(GetSubMenu(hmenu, 0), MPOS_FILE_READONLY, TRUE, &mio);
	}
	else
		DeleteMenu(GetSubMenu(hmenu, 0), MPOS_FILE_READONLY, MF_BYPOSITION);
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
		mio.dwTypeData = (LPTSTR)GetString(IDS_RECENT_MENU);
		InsertMenuItem(hmenu, 1, TRUE, &mio);
		if (hrecentmenu)
			DestroyMenu(hrecentmenu);
		hrecentmenu = mio.hSubMenu;

		DeleteMenu(GetSubMenu(hmenu, 0), MPOS_FILE_RECENT, MF_BYPOSITION);
		DeleteMenu(GetSubMenu(hmenu, 0), MPOS_FILE_RECENT, MF_BYPOSITION);
	}
	else {
		mio.dwTypeData = (LPTSTR)GetString(IDM_MENU_BASE+13);
		InsertMenuItem(GetSubMenu(hmenu, 0), MPOS_FILE_RECENT, TRUE, &mio);
		if (hrecentmenu)
			DestroyMenu(hrecentmenu);
		hrecentmenu = mio.hSubMenu;
		mio.hSubMenu = 0;
		mio.fType = MFT_SEPARATOR;
		mio.fMask = MIIM_TYPE;
		InsertMenuItem(GetSubMenu(hmenu, 0), MPOS_FILE_RECENT + 1, TRUE, &mio);
		DeleteMenu(hmenu, 1, MF_BYPOSITION);
	}
	DrawMenuBar(hwnd);
}

BOOL CreateMainMenu(HWND hwnd) {
	MENUITEMINFO mio;
	HMENU hmenu, hsub, hmold = GetMenu(hwnd);
	TCHAR* bufFn = gTmpBuf;
	if (!(hmenu = LocalizeMenu(IDR_MENU))){
		ReportLastError();
		return FALSE;
	}
	SetMenu(hwnd, hmenu);
	mio.cbSize = sizeof(MENUITEMINFO);
	if (HaveLanguagePlugin()) {
		hsub = GetSubMenu(hmenu, 4);
		mio.fMask = MIIM_TYPE | MIIM_ID;
		mio.fType = MFT_STRING;
		mio.wID = ID_ABOUT_PLUGIN;
		mio.dwTypeData = (LPTSTR)GetString(IDS_MENU_LANGUAGE_PLUGIN);
		InsertMenuItem(hsub, 1, TRUE, &mio);
	}
	mio.fMask = MIIM_STATE;
	mio.fState = bWordWrap ? MFS_CHECKED : MFS_UNCHECKED;
	SetMenuItemInfo(hmenu, ID_EDIT_WORDWRAP, 0, &mio);
	mio.fState = bSmartSelect ? MFS_CHECKED : MFS_UNCHECKED;
	SetMenuItemInfo(hmenu, ID_SMARTSELECT, 0, &mio);
#ifdef USE_RICH_EDIT
	mio.fState = bHyperlinks ? MFS_CHECKED : MFS_UNCHECKED;
	SetMenuItemInfo(hmenu, ID_SHOWHYPERLINKS, 0, &mio);
#endif
	if (SetLWA) {
		if (bTransparent) SendMessage(hwnd, WM_COMMAND, (WPARAM)ID_TRANSPARENT, 0);
	} else {
		hsub = GetSubMenu(GetMenu(hwnd), 3);
		DeleteMenu(hsub, 4, MF_BYPOSITION);
	}
	mio.fState = bShowStatus ? MFS_CHECKED : MFS_UNCHECKED;
	SetMenuItemInfo(hmenu, ID_SHOWSTATUS, 0, &mio);
	mio.fState = bShowToolbar ? MFS_CHECKED : MFS_UNCHECKED;
	SetMenuItemInfo(hmenu, ID_SHOWTOOLBAR, 0, &mio);
	mio.fState = bPrimaryFont ? MFS_CHECKED : MFS_UNCHECKED;
	SetMenuItemInfo(hmenu, ID_FONT_PRIMARY, 0, &mio);
	if (bAlwaysOnTop) SendMessage(hwnd, WM_COMMAND, (WPARAM)ID_ALWAYSONTOP, 0);
	if (options.bRecentOnOwn)
		FixMRUMenus();
	if (!options.bReadOnlyMenu)
		FixReadOnlyMenu();
	PopulateMRUList();
	if (options.bNoFaves) {
		DeleteMenu(hmenu, MPOS_FAVE, MF_BYPOSITION);
		SetWindowPos(hwnd, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
	} else {
		WIN32_FIND_DATA FindFileData;
		HANDLE handle;
		if (SCNUL(options.szFavDir)[0] == _T('\0') || (handle = FindFirstFile(options.szFavDir, &FindFileData)) == INVALID_HANDLE_VALUE) {
			TCHAR* pch;
			GetModuleFileName(hinstThis, bufFn, MAXFN);
			kstrdupa(&szFav, bufFn, lstrlen(GetString(STR_FAV_FILE))+4);
			pch = lstrrchr(szFav, _T('\\'));
			++pch;
			*pch = _T('\0');
		} else {
			FindClose(handle);
			kstrdupa(&szFav, options.szFavDir, lstrlen(GetString(STR_FAV_FILE))+4);
			lstrcat(szFav, _T("\\"));
		}
		lstrcat(szFav, GetString(STR_FAV_FILE));
		PopulateFavourites();
	}
	if (hmold) DestroyMenu(hmold);
	SetFileFormat(-1, 0);
	return TRUE;
}

void GotoLine(LONG lLine, LONG lOffset) {
	DWORD l;
	CHARRANGE cr;
	if (lLine == -1 || lOffset == -1) return;
	l = SendMessage(client, EM_GETLINECOUNT, 0, 0);
	if (lLine > (LONG)l) lLine = l;
	else if (lLine < 1) lLine = 1;
	l = GetCharIndex(options.bHideGotoOffset ? 1 : lOffset, lLine-1, -1, NULL, NULL, &cr);
	if (!options.bHideGotoOffset)
		cr.cpMax = (cr.cpMin += l);
	SetSelection(cr);
	SendMessage(client, EM_SCROLLCARET, 0, 0);
	UpdateStatus(FALSE);
}

BOOL CALLBACK AboutDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	static BOOL bIcon = FALSE;
	switch (uMsg) {
		case WM_INITDIALOG:
			if (bIcon)
				SendDlgItemMessage(hwndDlg, IDC_DLGICON, STM_SETICON, (WPARAM)LoadIcon(hinstThis, MAKEINTRESOURCE(IDI_EYE)), 0);
			CenterWindow(hwndDlg);
			SetWindowPos(client, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED); // hack for icon jitter
			SetDlgItemText(hwndDlg, IDC_STATICX, GetString(STR_ABOUT));
			SetDlgItemText(hwndDlg, IDC_EDIT_URL, GetString(STR_URL));
			SetDlgItemText(hwndDlg, IDOK, GetString(IDC_OK));
			SetDlgItemText(hwndDlg, IDC_STATIC_COPYRIGHT, GetString(STR_COPYRIGHT));
			SetDlgItemText(hwndDlg, IDC_STATIC_COPYRIGHT2, GetString(IDS_ALLRIGHTS));
			SetWindowText(hwndDlg, GetString(STR_METAPAD));
			LocalizeDialog(0, hwndDlg);
			break;
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
						INT8 rands[EGGNUM] = {0, -3, 6, -8, 0, -6, 10, -3, -8, 5, 9, 2, -3, -4, 0};
						GetWindowRect(hwndDlg, &r);
						for (i = 0; i < 300; ++i)
							SetWindowPos(hwndDlg, HWND_TOP, r.left+rands[i % EGGNUM], r.top+rands[(i+1) % EGGNUM], 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
						hicon = LoadIcon(hinstThis, MAKEINTRESOURCE(bIcon ? IDI_PAD : IDI_EYE));
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

BOOL CALLBACK AboutPluginDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
		case WM_INITDIALOG:
			SetDlgItemText(hwndDlg, IDC_EDIT_PLUGIN_LANG, GetString(IDS_PLUGIN_LANGUAGE));
			SetDlgItemText(hwndDlg, IDC_EDIT_PLUGIN_RELEASE, GetString(IDS_PLUGIN_RELEASE));
			SetDlgItemText(hwndDlg, IDC_EDIT_PLUGIN_TRANSLATOR, GetString(IDS_PLUGIN_TRANSLATOR));
			SetDlgItemText(hwndDlg, IDC_EDIT_PLUGIN_EMAIL, GetString(IDS_PLUGIN_EMAIL));
			LocalizeDialog(IDD_ABOUT_PLUGIN, hwndDlg);
			CenterWindow(hwndDlg);
			return TRUE;
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


BOOL CALLBACK GotoDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	TCHAR szLine[12];
	LONG lLine, lCol = 0;
	switch (uMsg) {
		case WM_INITDIALOG:
			CenterWindow(hwndDlg);
			lCol = GetColNum(-1, -1, NULL, &lLine, NULL);
			wsprintf(szLine, _T("%d"), lLine+1);
			SetDlgItemText(hwndDlg, IDC_LINE, szLine);
			if (options.bHideGotoOffset) {
				HWND hwndItem = GetDlgItem(hwndDlg, IDC_OFFSET);
				ShowWindow(hwndItem, SW_HIDE);
				hwndItem = GetDlgItem(hwndDlg, IDC_OFFSET_TEXT);
				ShowWindow(hwndItem, SW_HIDE);
			} else {
				wsprintf(szLine, _T("%d"), lCol);
				SetDlgItemText(hwndDlg, IDC_OFFSET, szLine);
				SendDlgItemMessage(hwndDlg, IDC_LINE, EM_SETSEL, 0, (LPARAM)-1);
			}
			LocalizeDialog(IDD_GOTO, hwndDlg);
			return TRUE;
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDOK:
					GetDlgItemText(hwndDlg, IDC_LINE, szLine, 12);
					lLine = _ttol(szLine);
					if (!options.bHideGotoOffset) {
						GetDlgItemText(hwndDlg, IDC_OFFSET, szLine, 12);
						lCol = _ttol(szLine);
					}
					GotoLine(lLine, lCol);
				case IDCANCEL:
					EndDialog(hwndDlg, wParam);
			}
			return TRUE;
		default:
			return FALSE;
	}
}

BOOL CALLBACK AddFavDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	TCHAR* szName = gTmpBuf;
	switch (uMsg) {
		case WM_INITDIALOG: {
			SetDlgItemText(hwndDlg, IDC_DATA, SCNUL8(szCaptionFile)+8);
			CenterWindow(hwndDlg);
			LocalizeDialog(IDD_FAV_NAME, hwndDlg);
			return TRUE;
		}
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDOK:
					GetDlgItemText(hwndDlg, IDC_DATA, szName, MAXFN);
					WritePrivateProfileString(GetString(STR_FAV_APPNAME), szName, SCNUL(szFile), SCNUL(szFav));
					PopulateFavourites();
				case IDCANCEL:
					EndDialog(hwndDlg, wParam);
			}
			return TRUE;
		default:
			return FALSE;
	}
}

BOOL CALLBACK CPDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	LONG i, j;
	TCHAR buf[64], buf2[128];
	switch (uMsg) {
		case WM_INITDIALOG:
			CenterWindow(hwndDlg);
			for (i=0, j=GetNumKnownCPs(); i < j; i++) {
				PrintCPName(GetKnownCP(i), buf, _T("%d"));
				SendDlgItemMessage(hwndDlg, IDC_DATA, CB_ADDSTRING, 0, (LPARAM)buf);
			}
			LocalizeDialog(IDD_CP, hwndDlg);
			return TRUE;
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDOK:
					GetDlgItemText(hwndDlg, IDC_DATA, buf, 16);
					i = _ttol(buf);
					if ((!i && (*buf < _T('0') || *buf > _T('9'))) || i < 0 || i > 0xffff || !MultiByteToWideChar(i, 0, (LPCSTR)kemptyStr, 1, NULL, 0)) {
						ERROROUT(GetString(IDS_ENC_BAD));
						SetFocus(hwndDlg);
						break;
					}
					if (newFormat) {
						newFormat = (1<<31) | (newFormat & 0xfff0000) | (WORD)i;
						PrintCPName((WORD)i, buf, GetString(ID_ENC_CODEPAGE));
						wsprintf(buf2, GetString(IDS_SETDEF_FORMAT_WARN), buf);
						MSGOUT(buf2);
					} else SetFileFormat((WORD)i | (1<<31), 1);
				case IDCANCEL:
					EndDialog(hwndDlg, wParam);
			}
			return TRUE;
		case WM_DESTROY:
			if (!newFormat) SetFocus(client);
			return FALSE;
		default:
			return FALSE;
	}
}

BOOL CALLBACK AdvancedPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	HKEY key;
	HMENU hmenu, hsub;
	MENUITEMINFO mio;
	RECT rc;
	TCHAR szInt[64];
	INT i, nTmp;
	DWORD u, v;
	WORD enc, lfmt, cp;
	switch (uMsg) {
	case WM_INITDIALOG:
		SendDlgItemMessage(hwndDlg, IDC_GOTO_HIDE_OFFSET, BM_SETCHECK, (WPARAM) options.bHideGotoOffset, 0);
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

		wsprintf(szInt, _T("%d"), options.nMaxMRU);
		SetDlgItemText(hwndDlg, IDC_EDIT_MAX_MRU, szInt);

		if (SetWindowTheme) SetWindowTheme(GetDlgItem(hwndDlg, IDC_BUTTON_FORMAT),(LPTSTR)kemptyStr,(LPTSTR)kemptyStr);
		SendDlgItemMessage(hwndDlg, IDC_BUTTON_FORMAT, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)(HANDLE)CreateMappedBitmap(hinstThis, IDB_DROP_ARROW, 0, NULL, 0));
		newFormat = options.nFormat;
		LocalizeDialog(IDD_PROPPAGE_A1, hwndDlg);
		return TRUE;
	case WM_NOTIFY:
		switch (((NMHDR FAR *)lParam)->code) {
		case PSN_KILLACTIVE:
			GetDlgItemText(hwndDlg, IDC_EDIT_MAX_MRU, szInt, 10);
			nTmp = _ttoi(szInt);
			if (nTmp < 0 || nTmp > 16) {
				ERROROUT(GetString(IDS_MAX_RECENT_WARNING));
				SetFocus(GetDlgItem(hwndDlg, IDC_EDIT_MAX_MRU));
				SetWindowLong (hwndDlg, DWLP_MSGRESULT, TRUE);
			}
			return TRUE;
		case PSN_APPLY:
			GetDlgItemText(hwndDlg, IDC_EDIT_MAX_MRU, szInt, 3);
			nTmp = _ttoi(szInt);
			options.nMaxMRU = nTmp;

			options.bHideGotoOffset = (BST_CHECKED == SendDlgItemMessage(hwndDlg, IDC_GOTO_HIDE_OFFSET, BM_GETCHECK, 0, 0));
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
			options.nFormat = newFormat;
			return TRUE;
		}
		return FALSE;
	case WM_DESTROY:
		newFormat = 0;
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
			MessageBox(hwndDlg, GetString(IDS_STICKY_MESSAGE), GetString(STR_METAPAD), MB_ICONINFORMATION);
			break;
		case IDC_BUTTON_CLEAR_FIND:
			if (MessageBox(hwndDlg, GetString(IDS_CLEAR_FIND_WARNING), GetString(STR_METAPAD), MB_ICONEXCLAMATION | MB_OKCANCEL) == IDOK) {
				for (i = 0; i < NUMFINDS; i++)
					kfree(&FindArray[i]);
				for (i = 0; i < NUMFINDS; i++)
					kfree(&ReplaceArray[i]);
				for (i = 0; i < NUMINSERTS; i++)
					kfree(&InsertArray[i]);
				ZeroMemory(FindArray, sizeof(FindArray));
				ZeroMemory(ReplaceArray, sizeof(ReplaceArray));
				ZeroMemory(InsertArray, sizeof(InsertArray));
				kfree(&szFindText);
				kfree(&szReplaceText);
			}
			break;
		case IDC_BUTTON_CLEAR_RECENT:
			if (MessageBox(hwndDlg, GetString(IDS_CLEAR_RECENT_WARNING), GetString(STR_METAPAD), MB_ICONEXCLAMATION | MB_OKCANCEL) == IDOK) {
				key = NULL;
				if (!g_bIniMode && RegCreateKeyEx(HKEY_CURRENT_USER, GetString(STR_REGKEY), 0, NULL, 0, KEY_ALL_ACCESS, NULL, &key, NULL) != ERROR_SUCCESS) {
					ReportLastError();
					break;
				}
				for (v = 1; v <= options.nMaxMRU; ++v) {
					wsprintf(szInt, GetString(IDSS_MRU), v);
					SaveOption(key, szInt, REG_SZ, NULL, 1);
				}
				if (key != NULL)
					RegCloseKey(key);
				PopulateMRUList();
			}
			break;
		case IDC_BUTTON_FORMAT:
			if (HIWORD(wParam) != BN_CLICKED) break;
			hmenu = LocalizeMenu(IDR_MENU);
			hsub = GetSubMenu(hmenu, 0);
			hsub = GetSubMenu(hsub, MPOS_FILE_FORMAT_STATIC);
			SendMessage(hwnd, WM_INITMENUPOPUP, (WPARAM)hsub, MAKELPARAM(1, FALSE));
			GetWindowRect(GetDlgItem(hwndDlg, IDC_BUTTON_FORMAT), &rc);
			enc = cp = (WORD)newFormat;
			lfmt = (newFormat >> 16) & 0xfff;
			if (newFormat >> 31) enc = FC_ENC_CODEPAGE;

			mio.cbSize = sizeof(MENUITEMINFO);
			mio.fMask = MIIM_TYPE | MIIM_ID;
			for (u = 0, v = GetMenuItemCount(hsub); u < v; u++){
				mio.fType = MFT_STRING;
				mio.cch = sizeof(szInt)/sizeof(TCHAR);
				mio.dwTypeData = szInt;
				GetMenuItemInfo(hsub, u, TRUE, &mio);
				AddMenuAccelStr(0, NULL, szInt, NULL, TRUE);
				SetMenuItemInfo(hsub, u, TRUE, &mio);
			}
			if (enc == FC_ENC_CODEPAGE){
				PrintCPName(cp, szInt, GetString(ID_ENC_CODEPAGE));
				mio.fType = MFT_STRING;
				mio.dwTypeData = szInt;
				mio.wID = ID_ENC_CODEPAGE;
				InsertMenuItem(hsub, v, TRUE, &mio);
			}
			CheckMenuRadioItem(hsub, ID_ENC_BASE, ID_ENC_END, enc % 1000 + ID_MENUCMD_BASE, MF_BYCOMMAND);
			CheckMenuRadioItem(hsub, ID_LFMT_BASE, ID_LFMT_END, lfmt % 1000 + ID_MENUCMD_BASE, MF_BYCOMMAND);
			switch (v = TrackPopupMenuEx(hsub, TPM_RETURNCMD | TPM_TOPALIGN | TPM_LEFTALIGN | TPM_RIGHTBUTTON, rc.left, rc.bottom, hwnd, NULL)) {
				case ID_ENC_CODEPAGE: break;
				case ID_ENC_CUSTOM:
					DialogBox(hinstThis, MAKEINTRESOURCE(IDD_CP), hwndDlg, (DLGPROC)CPDialogProc);
					break;
				case 0: break;
				default:
					if (!v) break;
					v = v % 1000 + FC_BASE;
					if (v >= FC_LFMT_BASE && v < FC_LFMT_END) v <<= 16;
					if (!(v & 0x8000ffff)) v |= newFormat & 0x8000ffff;
					if (!(v & 0xfff0000)) v |= newFormat & 0xfff0000;
					newFormat = v;
			}
			DestroyMenu(hmenu);
			break;
		}
		return FALSE;
	default:
		return FALSE;
	}
}

BOOL CALLBACK Advanced2PageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	INT i;
	LPTSTR sz = NULL, buf = gTmpBuf;
	HINSTANCE hinstTemp;
	switch (uMsg) {
	case WM_INITDIALOG:
		SendDlgItemMessage(hwndDlg, IDC_EDIT_LANG_PLUGIN, EM_LIMITTEXT, (WPARAM)MAXFN-1, 0);
		SendDlgItemMessage(hwndDlg, IDC_CUSTOMDATE, EM_LIMITTEXT, (WPARAM)MAXDATEFORMAT-1, 0);
		SendDlgItemMessage(hwndDlg, IDC_CUSTOMDATE2, EM_LIMITTEXT, (WPARAM)MAXDATEFORMAT-1, 0);
		SetDlgItemText(hwndDlg, IDC_EDIT_LANG_PLUGIN, SCNUL(options.szLangPlugin));
		SetDlgItemText(hwndDlg, IDC_CUSTOMDATE, SCNUL(options.szCustomDate));
		SetDlgItemText(hwndDlg, IDC_CUSTOMDATE2, SCNUL(options.szCustomDate2));
		for (i = 0; i < 10; i++) {
			SendDlgItemMessage(hwndDlg, IDC_MACRO_1+i, EM_LIMITTEXT, (WPARAM)(MAXMACRO-1), 0);
			SetDlgItemText(hwndDlg, IDC_MACRO_1+i, SCNUL(options.MacroArray[i]));
			wsprintf(buf, GetString(IDC_TEXT_MACRO), (i+1)%10);
			SetDlgItemText(hwndDlg, IDC_TEXT_MACRO+i, buf);
		}
		if (!SCNUL(options.szLangPlugin)[0]) {
			SendDlgItemMessage(hwndDlg, IDC_RADIO_LANG_DEFAULT, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
			SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_RADIO_LANG_DEFAULT, 0), 0);
		}
		else {
			SendDlgItemMessage(hwndDlg, IDC_RADIO_LANG_PLUGIN, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
			SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_RADIO_LANG_PLUGIN, 0), 0);
		}
		LocalizeDialog(IDD_PROPPAGE_A2, hwndDlg);
		return TRUE;
	case WM_NOTIFY:
		switch (((NMHDR FAR *)lParam)->code) {
		case PSN_KILLACTIVE:
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
		case PSN_APPLY:
			for (i = 0; i < 10; i++) {
				GetDlgItemText(hwndDlg, IDC_MACRO_1+i, buf, MAXMACRO);	kstrdupnul(&options.MacroArray[i], buf);
			}
			GetDlgItemText(hwndDlg, IDC_CUSTOMDATE, buf, MAXDATEFORMAT);	kstrdupnul(&options.szCustomDate, buf);
			GetDlgItemText(hwndDlg, IDC_CUSTOMDATE2, buf, MAXDATEFORMAT);	kstrdupnul(&options.szCustomDate2, buf);
			GetDlgItemText(hwndDlg, IDC_EDIT_LANG_PLUGIN, buf, MAXFN);
			if (BST_CHECKED != SendDlgItemMessage(hwndDlg, IDC_RADIO_LANG_PLUGIN, BM_GETCHECK, 0, 0)) *buf = 0;
			kstrdupnul(&options.szLangPlugin, buf);
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
			if (BrowseFile(hwndDlg, _T("dll"), NULL, FixFilterString((LPTSTR)GetString(IDS_FILTER_PLUGIN)), FALSE, FALSE, FALSE, &sz)){
				if (g_bDisablePluginVersionChecking) {
					SetDlgItemText(hwndDlg, IDC_EDIT_LANG_PLUGIN, sz);
				}
				else {
					if (LoadAndVerifyLanguagePlugin(sz, TRUE, &hinstTemp))
						SetDlgItemText(hwndDlg, IDC_EDIT_LANG_PLUGIN, sz);
					else {
						SendDlgItemMessage(hwndDlg, IDC_RADIO_LANG_DEFAULT, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
						SendDlgItemMessage(hwndDlg, IDC_RADIO_LANG_PLUGIN, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
						SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_RADIO_LANG_DEFAULT, 0), 0);
					}
				}
			}
			kfree(&sz);
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
			SendDlgItemMessage(hwndDlg, IDC_DIGITGRP, BM_SETCHECK, (WPARAM) options.bDigitGrp, 0);
			SendDlgItemMessage(hwndDlg, IDC_SYSTEM_COLOURS, BM_SETCHECK, (WPARAM) options.bSystemColours, 0);
			SendDlgItemMessage(hwndDlg, IDC_SYSTEM_COLOURS2, BM_SETCHECK, (WPARAM) options.bSystemColours2, 0);
			SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_SYSTEM_COLOURS, 0), 0);
			SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_SYSTEM_COLOURS2, 0), 0);

			LocalizeDialog(IDD_PROPPAGE_VIEW, hwndDlg);
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
				options.bDigitGrp = (BST_CHECKED == SendDlgItemMessage(hwndDlg, IDC_DIGITGRP, BM_GETCHECK, 0, 0));
				options.PrimaryFont = TmpPrimaryFont;
				options.SecondaryFont = TmpSecondaryFont;
				options.BackColour = TmpBackColour;
				options.FontColour = TmpFontColour;
				options.BackColour2 = TmpBackColour2;
				options.FontColour2 = TmpFontColour2;
			}
		case PSN_RESET:
			if (hfont) DeleteObject(hfont);
			if (hfont2) DeleteObject(hfont2);
			if (hbackbrush) DeleteObject(hbackbrush);
			if (hfontbrush) DeleteObject(hfontbrush);
			if (hbackbrush2) DeleteObject(hbackbrush2);
			if (hfontbrush2) DeleteObject(hfontbrush2);
			break;
		}
		break;
	case WM_CTLCOLORBTN:
		if (GetDlgItem(hwndDlg, IDC_COLOUR_BACK) == (HWND)lParam) return (BOOL)hbackbrush;
		if (GetDlgItem(hwndDlg, IDC_COLOUR_FONT) == (HWND)lParam) return (BOOL)hfontbrush;
		if (GetDlgItem(hwndDlg, IDC_COLOUR_BACK2) == (HWND)lParam) return (BOOL)hbackbrush2;
		if (GetDlgItem(hwndDlg, IDC_COLOUR_FONT2) == (HWND)lParam) return (BOOL)hfontbrush2;
		return FALSE;
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

BOOL CALLBACK GeneralPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	LPTSTR sz = NULL, buf = gTmpBuf;
	TCHAR szInt[5];
	int nTmp;
	switch (uMsg) {
	case WM_INITDIALOG:
		CenterWindow(GetParent(hwndDlg));
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
		LocalizeDialog(IDD_PROPPAGE_GENERAL, hwndDlg);
		return TRUE;
	case WM_NOTIFY:
		switch (((NMHDR FAR *)lParam)->code) {
		case PSN_KILLACTIVE:
			GetDlgItemText(hwndDlg, IDC_TAB_STOP, szInt, 5);
			nTmp = _ttoi(szInt);
			if (nTmp < 1 || nTmp > 100) {
				ERROROUT(GetString(IDS_TAB_SIZE_WARNING));
				SetFocus(GetDlgItem(hwndDlg, IDC_TAB_STOP));
				SetWindowLong (hwndDlg, DWLP_MSGRESULT, TRUE);
			}
			return TRUE;
		case PSN_APPLY:
			GetDlgItemText(hwndDlg, IDC_TAB_STOP, szInt, 5);
			nTmp = _ttoi(szInt);
			options.nTabStops = nTmp;

			GetDlgItemText(hwndDlg, IDC_EDIT_BROWSER, buf, MAXFN);		kstrdupnul(&options.szBrowser, buf);
			GetDlgItemText(hwndDlg, IDC_EDIT_ARGS, buf, MAXARGS);		kstrdupnul(&options.szArgs, buf);
			GetDlgItemText(hwndDlg, IDC_EDIT_BROWSER2, buf, MAXFN);		kstrdupnul(&options.szBrowser2, buf);
			GetDlgItemText(hwndDlg, IDC_EDIT_ARGS2, buf, MAXARGS);		kstrdupnul(&options.szArgs2, buf);
			GetDlgItemText(hwndDlg, IDC_EDIT_QUOTE, buf, MAXQUOTE);		kstrdupnul(&options.szQuote, buf);

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
			break;
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_BUTTON_BROWSE:
		case IDC_BUTTON_BROWSE2:
			if (BrowseFile(hwndDlg, _T("exe"), NULL, FixFilterString((LPTSTR)GetString(IDS_FILTER_EXEC)), FALSE, FALSE, FALSE, &sz))
				SetDlgItemText(hwndDlg, LOWORD(wParam) == IDC_BUTTON_BROWSE ? IDC_EDIT_BROWSER : IDC_EDIT_BROWSER2, sz);
			kfree(&sz);
		}
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

int CALLBACK SheetInitProc(HWND hwndDlg, UINT uMsg, LPARAM lParam) {
	if (uMsg == PSCB_PRECREATE) {
		if (((LPDLGTEMPLATEEX)lParam)->signature == 0xFFFF)
			((LPDLGTEMPLATEEX)lParam)->style &= ~DS_CONTEXTHELP;
		else
			((LPDLGTEMPLATE)lParam)->style &= ~DS_CONTEXTHELP;
	}
	return TRUE;
}

LRESULT WINAPI MainWndProc(HWND hwndMain, UINT Msg, WPARAM wParam, LPARAM lParam)
{
#ifdef USE_RICH_EDIT
	static TEXTRANGE tr;
	static BYTE keys[256];
#endif
	static CHARRANGE cr, cr2;
	static PAGESETUPDLG psd;
	static PROPSHEETHEADER psh;
	static PROPSHEETPAGE pages[4];
	static RECT rect;
	static WINDOWPLACEMENT wp, wp2;
	static option_struct tmpOptions;
	static TCHAR gDummyBuf[1], gDummyBuf2[1];

#ifdef USE_RICH_EDIT
	ENLINK* pLink;
	POINT pt;
#endif
	HBITMAP hb;
	HCURSOR hCur;
	HDROP hDrop;
	HGLOBAL hMem, hMem2;
	HKEY key;
	HMENU hMenu;
	LPTOOLTIPTEXT lpttt;
	LPCTSTR szOld;
	LPTSTR sz = NULL, szNew;
	LPTSTR szTmp = gTmpBuf;
	BOOL b = FALSE, abort, uni;
	BYTE base;
	DWORD l, sl, len;
	LONG i, j, k, nPos, ena, enc;

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
		if (bLoading) break;
		UpdateStatus(FALSE);
		if (status && bShowStatus)
			InvalidateRect(status, NULL, TRUE);
		SetFocus(client);
		break;
	case WM_DROPFILES:
		hDrop = (HDROP) wParam;
		if (!SaveIfDirty())
			break;
		DragQueryFile(hDrop, 0, szTmp, MAXFN);
		DragFinish(hDrop);
		LoadFile(szTmp, FALSE, TRUE, FALSE, 0, NULL);
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
			QueueUpdateStatus();
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

		if (nPos == MPOS_EDIT) {
			ena = IsClipboardFormatAvailable(_CF_TEXT) ? MF_ENABLED : MF_GRAYED;
			EnableMenuItem(hMenu, ID_MYEDIT_PASTE, MF_BYCOMMAND | ena);
			EnableMenuItem(hMenu, ID_PASTE_B64, MF_BYCOMMAND | ena);
			EnableMenuItem(hMenu, ID_PASTE_HEX, MF_BYCOMMAND | ena);
			EnableMenuItem(hMenu, ID_PASTE_MUL, MF_BYCOMMAND | ena);
			EnableMenuItem(hMenu, ID_COMMIT_WORDWRAP, MF_BYCOMMAND | (bWordWrap ? MF_ENABLED : MF_GRAYED));
			cr = GetSelection();
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
		} else if (!options.bNoFaves && nPos == MPOS_FAVE) {
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
					case ID_MYFILE_NEW: lpttt->lpszText = (LPTSTR)GetString(IDS_TB_NEWFILE); break;
					case ID_MYFILE_OPEN: lpttt->lpszText = (LPTSTR)GetString(IDS_TB_OPENFILE); break;
					case ID_MYFILE_SAVE: lpttt->lpszText = (LPTSTR)GetString(IDS_TB_SAVEFILE); break;
					case ID_PRINT: lpttt->lpszText = (LPTSTR)GetString(IDS_TB_PRINT); break;
					case ID_FIND: lpttt->lpszText = (LPTSTR)GetString(IDS_TB_FIND); break;
					case ID_REPLACE: lpttt->lpszText = (LPTSTR)GetString(IDS_TB_REPLACE); break;
					case ID_MYEDIT_CUT: lpttt->lpszText = (LPTSTR)GetString(IDS_TB_CUT); break;
					case ID_MYEDIT_COPY: lpttt->lpszText = (LPTSTR)GetString(IDS_TB_COPY); break;
					case ID_MYEDIT_PASTE: lpttt->lpszText = (LPTSTR)GetString(IDS_TB_PASTE); break;
					case ID_MYEDIT_UNDO: lpttt->lpszText = (LPTSTR)GetString(IDS_TB_UNDO); break;
#ifdef USE_RICH_EDIT
					case ID_MYEDIT_REDO: lpttt->lpszText = (LPTSTR)GetString(IDS_TB_REDO); break;
#endif
					case ID_VIEW_OPTIONS: lpttt->lpszText = (LPTSTR)GetString(IDS_TB_SETTINGS); break;
					case ID_RELOAD_CURRENT: lpttt->lpszText = (LPTSTR)GetString(IDS_TB_REFRESH); break;
					case ID_EDIT_WORDWRAP: lpttt->lpszText = (LPTSTR)GetString(IDS_TB_WORDWRAP); break;
					case ID_FONT_PRIMARY: lpttt->lpszText = (LPTSTR)GetString(IDS_TB_PRIMARYFONT); break;
					case ID_ALWAYSONTOP: lpttt->lpszText = (LPTSTR)GetString(IDS_TB_ONTOP); break;
					case ID_FILE_LAUNCHVIEWER: lpttt->lpszText = (LPTSTR)GetString(IDS_TB_PRIMARYVIEWER); break;
					case ID_LAUNCH_SECONDARY_VIEWER: lpttt->lpszText = (LPTSTR)GetString(IDS_TB_SECONDARYVIEWER); break;
				}
				break;
#ifdef USE_RICH_EDIT
			case EN_LINK:
				pLink = (ENLINK *) lParam;
				if (pLink->msg != WM_SETCURSOR && pLink->msg != WM_LBUTTONUP && pLink->msg != WM_LBUTTONDBLCLK) break;
				if (!GetCursorPos(&pt) || !ScreenToClient(client, &pt)) break;
				i = pt.y;
				k = SendMessage(client, EM_CHARFROMPOS, 0, (LPARAM)&pt);
				//printf("%d,%d  %08X %d ----- ",pt.x,pt.y,k,k);
				SendMessage(client, EM_POSFROMCHAR, (WPARAM)&pt, k);
				abort = (k >= (LONG)GetTextChars(NULL) || i-(fontHt < 0 ? 15 : fontHt)-2 > pt.y);
				//printf("%d, %d,  %08X %d   -> %d,%d\n",i, fontHt, abort,(LONG)GetTextChars(NULL),pt.x,pt.y);
				switch (pLink->msg) {
					case WM_SETCURSOR:
						if (abort || options.bLinkDoubleClick)
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
						if (abort) break;
						tr.chrg.cpMin = pLink->chrg.cpMin;
						tr.chrg.cpMax = pLink->chrg.cpMax;
						tr.lpstrText = kallocsz(pLink->chrg.cpMax - pLink->chrg.cpMin + 1);

						SendMessage(client, EM_GETTEXTRANGE, 0, (LPARAM)&tr);
						ShellExecute(NULL, NULL, tr.lpstrText, NULL, NULL, SW_SHOWNORMAL);
						kfree(&tr.lpstrText);
						break;
				}
			case EN_SELCHANGE:
				//Selection was changed (RICH)
				if (!bLoading)
					QueueUpdateStatus();
				break;
#endif
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
			case ID_CLIENT:
				switch (HIWORD(wParam)) {
					case EN_CHANGE:
						*szStatusMessage = 0;
						bDirtyShadow = bDirtyStatus = TRUE;
						QueueUpdateStatus();
						break;
#ifdef USE_RICH_EDIT
					case EN_STOPNOUNDO:
						ERROROUT(GetString(IDS_CANT_UNDO_WARNING));
						break;
#endif
					case EN_ERRSPACE:
					case EN_MAXTEXT:
#ifdef USE_RICH_EDIT
						wsprintf(szTmp, GetString(IDS_MEMORY_LIMIT), FormatNumber(GetTextChars(NULL), options.bDigitGrp, 0, 0));
						ERROROUT(szTmp);
#else
						if (bLoading) {
							bLoading = FALSE;
							if (options.bAlwaysLaunch || MessageBox(hwnd, GetString(IDS_QUERY_LAUNCH_VIEWER), GetString(STR_METAPAD), MB_ICONQUESTION | MB_YESNO) == IDYES) {
								if (!SCNUL(options.szBrowser)[0]) {
									MessageBox(hwnd, GetString(IDS_VIEWER_MISSING), GetString(STR_METAPAD), MB_OK|MB_ICONEXCLAMATION);
									SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(ID_VIEW_OPTIONS, 0), 0);
									kfree(&szFile);
									break;
								}
								LaunchExternalViewer(0, NULL);
							}
							if (!IsWindowVisible(hwnd))
								bQuit = TRUE;
						} else {
							MessageBox(hwnd, GetString(IDS_LE_MEMORY_LIMIT), GetString(STR_METAPAD), MB_ICONEXCLAMATION | MB_OK);
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
					SearchFile(szFindText, bMatchCase, TRUE, bWholeWord, pbFindTextSpec);
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
					SearchFile(szFindText, bMatchCase, (LOWORD(wParam) == ID_FIND_NEXT_WORD ? TRUE : FALSE), bWholeWord, pbFindTextSpec);
					SendMessage(client, EM_SCROLLCARET, 0, 0);
				}
				break;
			case ID_FIND_PREV:
				if (SCNUL(szFindText)[0]) {
					SearchFile(szFindText, bMatchCase, FALSE, bWholeWord, pbFindTextSpec);
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
				l = 0;
				abort = FALSE;
				*szTmp = 0;
				ZeroMemory(&wp, sizeof(WINDOWPLACEMENT));
				if (hdlgFind) {
					l = 1;
					if (frDlgId == LOWORD(wParam) && frDlgId != ID_PASTE_MUL) {
						SetFocus(hdlgFind);
						break;
					} else {
						switch (frDlgId){
							case ID_FIND:
							case ID_REPLACE:
								SendDlgItemMessage(hdlgFind, IDC_DROP_FIND, WM_GETTEXT, MAXFIND, (LPARAM)szTmp);
								break;
							case ID_INSERT_TEXT:
								SendDlgItemMessage(hdlgFind, IDC_DROP_INSERT, WM_GETTEXT, MAXFIND, (LPARAM)szTmp);
								break;
						}
						kstrdupnul(&szFindText, szTmp);
						wp.length = sizeof(WINDOWPLACEMENT);
						GetWindowPlacement(hdlgFind, &wp);
						SendMessage(hdlgFind, WM_CLOSE, 0, 0);
						kstrdupnul(&szInsert, szFindText);
					}
				}

				wpOrigFindProc = NULL;
				frDlgId = LOWORD(wParam);
				ZeroMemory(&gfr, sizeof(FINDREPLACE));
				gfr.lStructSize = sizeof(FINDREPLACE);
				gfr.hwndOwner = hwndMain;
				gfr.lpstrFindWhat = gDummyBuf;
				gfr.wFindWhatLen = sizeof(TCHAR);
				gfr.lpstrReplaceWith = gDummyBuf2;
				gfr.wReplaceWithLen = sizeof(TCHAR);
				gfr.hInstance = hinstThis;
				gfr.Flags = FR_ENABLETEMPLATE | FR_ENABLEHOOK;
				gfr.lpfnHook = (LPFRHOOKPROC)FindProc;
				hb = CreateMappedBitmap(hinstThis, IDB_DROP_ARROW, 0, NULL, 0);
				switch (LOWORD(wParam)) {
					case ID_REPLACE:
						kstrdup(&szReplaceText, ReplaceArray[0]);
					case ID_FIND:
						if (!l) SelectWord(&szFindText, TRUE, !options.bNoFindAutoSelect);
						if (!SCNUL(szFindText)[0]) kstrdupnul(&szFindText, FindArray[0]);
						if (bWholeWord) gfr.Flags |= FR_WHOLEWORD;
						if (bDown) gfr.Flags |= FR_DOWN;
						if (bMatchCase) gfr.Flags |= FR_MATCHCASE;
						cr = GetSelection();
						break;
					case ID_PASTE_MUL:
						SendMessage(hwnd, WM_COMMAND, ID_INT_GETCLIPBOARD, 0);
						kstrdupnul(&szInsert, szTmp);
					case ID_INSERT_TEXT:
						if (!szInsert){
							if (!l) SelectWord(&szInsert, TRUE, !options.bNoFindAutoSelect);
							if (!SCNUL(szInsert)[0]) kstrdupnul(&szInsert, InsertArray[0]);
						}
						break;
				}
				if (abort) break;

				switch (LOWORD(wParam)) {
					case ID_INSERT_TEXT:
					case ID_PASTE_MUL:
						gfr.lpTemplateName = MAKEINTRESOURCE(IDD_INSERT);
						hdlgFind = FindText(&gfr);
						SendDlgItemMessage(hdlgFind, IDC_DROP_INSERT, CB_LIMITTEXT, (WPARAM)(MAXFIND-1), 0);
						SendDlgItemMessage(hdlgFind, IDC_NUM, EM_LIMITTEXT, (WPARAM)9, 0);
						SendDlgItemMessage(hdlgFind, IDC_CLOSE_AFTER_INSERT, BM_SETCHECK, (WPARAM) bCloseAfterInsert, 0);
						for (i = 0; i < NUMINSERTS; ++i)
							if (SCNUL(InsertArray[i])[0])
								SendDlgItemMessage(hdlgFind, IDC_DROP_INSERT, CB_ADDSTRING, 0, (LPARAM)InsertArray[i]);
						if (options.bCurrentFindFont) SendDlgItemMessage(hdlgFind, IDC_DROP_INSERT, WM_SETFONT, (WPARAM)hfontfind, 0);
						if (SCNUL(szInsert)[0]) {
							SendDlgItemMessage(hdlgFind, IDC_DROP_INSERT, WM_SETTEXT, 0, (LPARAM)szInsert);
							SendDlgItemMessage(hdlgFind, IDC_DROP_INSERT, CB_SETEDITSEL, 0, MAKELPARAM(0, -1));
						}
						SendDlgItemMessage(hdlgFind, IDC_NUM, WM_SETTEXT, 0, (LPARAM)_T("1"));
						break;
					case ID_REPLACE:
						gfr.lpTemplateName = MAKEINTRESOURCE(IDD_REPLACE);
						hdlgFind = ReplaceText(&gfr);
						if (SetWindowTheme) SetWindowTheme(GetDlgItem(hdlgFind, IDC_ESCAPE2),(LPTSTR)kemptyStr,(LPTSTR)kemptyStr);
						SendDlgItemMessage(hdlgFind, IDC_ESCAPE2, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)(HANDLE)hb);
						SendDlgItemMessage(hdlgFind, IDC_DROP_REPLACE, CB_LIMITTEXT, (WPARAM)(MAXFIND-1), 0);
						for (i = 0; i < NUMFINDS; ++i)
							if (ReplaceArray[i])
								SendDlgItemMessage(hdlgFind, IDC_DROP_REPLACE, CB_ADDSTRING, 0, (LPARAM)ReplaceArray[i]);
						if (options.bCurrentFindFont) SendDlgItemMessage(hdlgFind, IDC_DROP_REPLACE, WM_SETFONT, (WPARAM)hfontfind, 0);
#ifdef USE_RICH_EDIT
						if (SendMessage(client, EM_EXLINEFROMCHAR, 0, (LPARAM)cr.cpMin) == SendMessage(client, EM_EXLINEFROMCHAR, 0, (LPARAM)cr.cpMax))
#else
						if (SendMessage(client, EM_LINEFROMCHAR, (WPARAM)cr.cpMin, 0) == SendMessage(client, EM_LINEFROMCHAR, (WPARAM)cr.cpMax, 0))
#endif
							SendDlgItemMessage(hdlgFind, IDC_RADIO_WHOLE, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
						else
							SendDlgItemMessage(hdlgFind, IDC_RADIO_SELECTION, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
						if (SCNUL(szReplaceText)[0]) {
							SendDlgItemMessage(hdlgFind, IDC_DROP_REPLACE, WM_SETTEXT, 0, (LPARAM)szReplaceText);
							SendDlgItemMessage(hdlgFind, IDC_DROP_REPLACE, CB_SETEDITSEL, 0, MAKELPARAM(0, -1));
						}
						SendDlgItemMessage(hdlgFind, IDC_CLOSE_AFTER_REPLACE, BM_SETCHECK, (WPARAM) bCloseAfterReplace, 0);
						break;
					case ID_FIND:
						gfr.lpTemplateName = MAKEINTRESOURCE(IDD_FIND);
						hdlgFind = FindText(&gfr);
						SendDlgItemMessage(hdlgFind, IDC_CLOSE_AFTER_FIND, BM_SETCHECK, (WPARAM) bCloseAfterFind, 0);
						break;
				}
				
				if (SetWindowTheme) SetWindowTheme(GetDlgItem(hdlgFind, IDC_ESCAPE),(LPTSTR)kemptyStr,(LPTSTR)kemptyStr);
				SendDlgItemMessage(hdlgFind, IDC_ESCAPE, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)(HANDLE)hb);
				switch (LOWORD(wParam)) {
					case ID_REPLACE:
					case ID_FIND:
						SendDlgItemMessage(hdlgFind, IDC_DROP_FIND, CB_LIMITTEXT, (WPARAM)(MAXFIND-1), 0);
						for (i = 0; i < NUMFINDS; ++i)
							if (SCNUL(FindArray[i])[0])
								SendDlgItemMessage(hdlgFind, IDC_DROP_FIND, CB_ADDSTRING, 0, (WPARAM)FindArray[i]);
						if (options.bCurrentFindFont) SendDlgItemMessage(hdlgFind, IDC_DROP_FIND, WM_SETFONT, (WPARAM)hfontfind, 0);
						if (SCNUL(szFindText)[0]) {
							SendDlgItemMessage(hdlgFind, IDC_DROP_FIND, WM_SETTEXT, 0, (LPARAM)szFindText);
							SendDlgItemMessage(hdlgFind, IDC_DROP_FIND, CB_SETEDITSEL, 0, MAKELPARAM(0, -1));
						}
						break;
				}

				wp2.length = sizeof(WINDOWPLACEMENT);
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
					wp.length = sizeof(WINDOWPLACEMENT);
					GetWindowPlacement(hwnd, &wp);
					i = WIDTH(wp2.rcNormalPosition);
					wp2.rcNormalPosition.right = wp.rcNormalPosition.right - 32;
					wp2.rcNormalPosition.left = wp2.rcNormalPosition.right - i;
					i = HEIGHT(wp2.rcNormalPosition);
					wp2.rcNormalPosition.top = wp.rcNormalPosition.top + 85;
					wp2.rcNormalPosition.bottom = i + wp2.rcNormalPosition.top;
				}
				SetWindowPlacement(hdlgFind, &wp2);
				gfr.Flags = FR_ENABLETEMPLATE;
				wpOrigFindProc = (WNDPROC)SetWindowLongPtr(hdlgFind, GWLP_WNDPROC, (LONG_PTR)FindProc);
				break;
			case ID_MYEDIT_DELETE:
				SendMessage(client, WM_CLEAR, 0, 0);
				QueueUpdateStatus();
				break;
#ifdef USE_RICH_EDIT
			case ID_MYEDIT_REDO:
				RestoreClientView(0, FALSE, FALSE, TRUE);
				SendMessage(client, EM_REDO, 0, 0);
				RestoreClientView(1, FALSE, FALSE, TRUE);
				RestoreClientView(0, TRUE, FALSE, TRUE);
				if (!IsSelectionVisible())
					RestoreClientView(1, TRUE, FALSE, TRUE);
				QueueUpdateStatus();
				break;
#endif
			case ID_MYEDIT_UNDO:
				RestoreClientView(0, FALSE, FALSE, TRUE);
				SendMessage(client, EM_UNDO, 0, 0);
				RestoreClientView(1, FALSE, FALSE, TRUE);
				RestoreClientView(0, TRUE, FALSE, TRUE);
				if (!IsSelectionVisible())
					RestoreClientView(1, TRUE, FALSE, TRUE);
				QueueUpdateStatus();
				break;
			case ID_CLEAR_CLIPBRD:
				if (!OpenClipboard(NULL)) {
					ERROROUT(GetString(IDS_CLIPBOARD_OPEN_ERROR));
					break;
				}
				if (!EmptyClipboard())
					ReportLastError();
				CloseClipboard();
				UpdateStatus(TRUE);
				break;
			case ID_MYEDIT_CUT:
			case ID_MYEDIT_COPY:
			case ID_COPY_B64:
			case ID_COPY_HEX:
				l = base = 0;
				enc = (nFormat >> 31 ? FC_ENC_CODEPAGE : (WORD)nFormat);
				switch(LOWORD(wParam)){
					case ID_MYEDIT_CUT:
						SendMessage(client, WM_CUT, 0, 0); break;
					case ID_COPY_B64: if (!base) base = 64;
					case ID_COPY_HEX: if (!base) base = 16;
					default:
						SendMessage(client, WM_COPY, 0, 0); break;
				}
				//OpenClipboard() fails if another program has the clipboard open. [https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-openclipboard]
				//Since we just copied stuff above, a program hooked into clipboard updates could be reading the clipboard right now, so just keep trying to get our turn
				while (!OpenClipboard(NULL));
				if ( hMem = GetClipboardData(_CF_TEXT) ) {
					if (!(szOld = (LPCTSTR)GlobalLock(hMem)))
						ERROROUT(GetString(IDS_GLOBALLOCK_ERROR));
					else {
						sl = 0;
						sz = (LPTSTR)szOld;
						ExportLineFmtLE(&sz, &sl, (nFormat >> 16) & 0xfff, (DWORD)-1, &b);
						if (base && enc == FC_ENC_BIN) ExportBinary(sz, sl);
						if (base && sl) sl = EncodeText((LPBYTE*)&sz, sl, nFormat, &b, NULL);
						switch (base){
							case 16: if (!l) l = sl * 2;
							case 64: if (!l) l = ((sl + 2) / 3) * 4;
								if (enc == FC_ENC_UTF16 || enc == FC_ENC_UTF16BE)
									ReverseBytes((LPBYTE)sz, sl);
							default: if (!l) l = sl; break;
						}
						if (!(hMem2 = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, sizeof(TCHAR) * (l+1))) || !(szNew = (LPTSTR)GlobalLock(hMem2)))
							ERROROUT(GetString(IDS_GLOBALLOCK_ERROR));
						else {
							if (base) EncodeBase(base, (LPBYTE)sz, szNew, sl, NULL);
							else {
								lstrcpy(szNew, sz);
								if (enc == FC_ENC_BIN) ExportBinary(szNew, sl);
							}
#ifdef UNICODE
							sl = EncodeText((LPBYTE*)&sz, sl, FC_ENC_ANSI, &b, NULL);
#endif
							SetLastError(NO_ERROR);
							if ((!GlobalUnlock(hMem) && (l = GetLastError())) || (!GlobalUnlock(hMem2) && (l = GetLastError()))) {
								ERROROUT(GetString(IDS_CLIPBOARD_UNLOCK_ERROR));
								ReportError(l);
							}
							if (!EmptyClipboard() || !SetClipboardData(_CF_TEXT, hMem2))
								ReportLastError();
#ifdef UNICODE
							if ((hMem2 = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, sl+1)) && (szNew = (LPTSTR)GlobalLock(hMem2))) {
								memcpy(szNew, sz, sl+1);
								SetLastError(NO_ERROR);
								if ((!GlobalUnlock(hMem2) && (l = GetLastError()))) {
									ERROROUT(GetString(IDS_CLIPBOARD_UNLOCK_ERROR));
									ReportError(l);
								}
								SetClipboardData(CF_TEXT, hMem2);
							}
#endif
						}
						if (b) kfree(&sz);
					}
				}
				CloseClipboard();
				QueueUpdateStatus();
				break;
			case ID_MYEDIT_PASTE: 
			case ID_PASTE_B64:
			case ID_PASTE_HEX:
			case ID_INT_GETCLIPBOARD:
				base = 0;
				enc = ((sl = nFormat) >> 31 ? FC_ENC_CODEPAGE : (WORD)nFormat);
				if (!OpenClipboard(NULL)) {
					ERROROUT(GetString(IDS_CLIPBOARD_OPEN_ERROR));
					break;
				}
				if ( hMem = GetClipboardData(_CF_TEXT) ) {
					if (!(sz = (LPTSTR)(szOld = (LPCTSTR)GlobalLock(hMem)))) {
						ERROROUT(GetString(IDS_GLOBALLOCK_ERROR));
					} else {
						uni = (enc == FC_ENC_UTF16 || enc == FC_ENC_UTF16BE);
						l = lstrlen(szOld);
						switch(LOWORD(wParam)){
							case ID_PASTE_B64: if (!base) { base = 64; szNew = uni ? _T("\\W") : _T("\\S"); }
							case ID_PASTE_HEX: if (!base) { base = 16; szNew = uni ? _T("\\U") : _T("\\X"); }
								sz = kallocs(l+3);
								lstrcpy(sz, szNew);
								lstrcat(sz, szOld);
								if (!ParseForEscapeSeqs(sz, NULL, GetString(IDS_ESCAPE_CTX_CLIPBRD))) { kfree(&sz); }
								else {
									b = TRUE;
									ExportBinary(sz, l=lstrlen(sz));
									if (!uni) l = EncodeText((LPBYTE*)&sz, l, FC_ENC_ANSI, &b, NULL);
								}
								break;
						}
						if (sz) {
							if (base && l && !uni) l = DecodeText((LPBYTE*)&sz, l, &sl, &b);
							GetLineFmt(sz, l, 0, &i, &j, &k, &nPos, &uni);
							if (uni) ImportBinary(sz, l);
							ImportLineFmt(&sz, &l, (nFormat >> 16) & 0xfff, i, j, k, nPos, &b);
							if (LOWORD(wParam) == ID_INT_GETCLIPBOARD)
								lstrcpyn(szTmp, sz, MAXFIND);
							else
								SendMessage(client, EM_REPLACESEL, (WPARAM)TRUE, (LPARAM)sz);
						}
						GlobalUnlock(hMem);
						if (b) kfree(&sz);
					}
				}
				CloseClipboard();
				QueueUpdateStatus();
				break;
			case ID_HOME:
				cr = GetSelection();
				szOld = GetShadowLine(-1, -1, &len, NULL, &cr2);
				for (l = 0; l < len; l++) {
					if (szOld[l] != _T('\t') && szOld[l] != _T(' '))
						break;
				}
				if (options.bNoSmartHome || (LONG)l == cr.cpMin - cr2.cpMin) {
					SendMessage(client, WM_KEYDOWN, (WPARAM)VK_HOME, 0);
					SendMessage(client, WM_KEYUP, (WPARAM)VK_HOME, 0);
					break;
				}
				cr2.cpMax = (cr2.cpMin += l);
				SetSelection(cr2);
				SendMessage(client, EM_SCROLLCARET, 0, 0);
				break;
			case ID_MYEDIT_SELECTALL:
				cr.cpMin = 0;
				cr.cpMax = -1;
				SetSelection(cr);
				UpdateStatus(FALSE);
				break;
			case ID_SCROLLTO_SELA:
				cr = cr2 = GetSelection();
				cr2.cpMax = cr2.cpMin;
				SetSelection(cr2);
				SendMessage(client, EM_SCROLLCARET, 0, 0);
				SendMessage(client, EM_SCROLL, SB_PAGEDOWN, 0);
				SendMessage(client, EM_SCROLLCARET, 0, 0);
				SendMessage(client, EM_SCROLL, SB_LINEUP, 0);
				SetSelection(cr);
				break;
			case ID_SCROLLTO_SELE:
				SendMessage(client, EM_SCROLLCARET, 0, 0);
				SendMessage(client, EM_SCROLL, SB_PAGEUP, 0);
				SendMessage(client, EM_SCROLLCARET, 0, 0);
				SendMessage(client, EM_SCROLL, SB_LINEDOWN, 0);
				break;
#ifdef USE_RICH_EDIT
			case ID_SHOWHYPERLINKS:
				if (SendMessage(client, EM_CANUNDO, 0, 0)) {
					if (!options.bSuppressUndoBufferPrompt && MessageBox(hwnd, GetString(IDS_UNDO_HYPERLINKS_WARNING), GetString(STR_METAPAD), MB_ICONEXCLAMATION | MB_OKCANCEL) == IDCANCEL)
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
				UpdateStatus(TRUE);
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
					UpdateStatus(TRUE);
					ShowWindow(toolbar, SW_SHOW);
				} else
					ShowWindow(toolbar, SW_HIDE);

				SendMessage(toolbar, WM_SIZE, 0, 0);
				GetClientRect(hwndMain, &rect);
				SetWindowPos(client, 0, 0, bShowToolbar ? GetToolbarHeight() : rect.top - GetToolbarHeight(), rect.right - rect.left, rect.bottom - rect.top - GetStatusHeight() - GetToolbarHeight(), SWP_NOZORDER | SWP_SHOWWINDOW);
				break;
			case ID_SHOWSTATUS:
				if (!IsWindow(status))
					CreateStatusbar();

				bShowStatus = !GetCheckedState(GetMenu(hwndMain), ID_SHOWSTATUS, TRUE);
				if (bShowStatus) {
					UpdateStatus(TRUE);
					ShowWindow(status, SW_SHOW);
				} else
					ShowWindow(status, SW_HIDE);

				GetClientRect(hwndMain, &rect);
				SetWindowPos(client, 0, 0, GetToolbarHeight(), rect.right - rect.left, rect.bottom - rect.top - GetStatusHeight() - GetToolbarHeight(), SWP_NOMOVE | SWP_NOZORDER | SWP_SHOWWINDOW);
				SendMessage(status, WM_SIZE, 0, 0);
				break;
			case ID_READONLY:
				if (!options.bReadOnlyMenu || !SCNUL(szFile)[0]) break;
				bReadOnly = !GetCheckedState(GetMenu(hwndMain), ID_READONLY, FALSE);
				if (bReadOnly)
					ena = SetFileAttributes(szFile, GetFileAttributes(szFile) | FILE_ATTRIBUTE_READONLY);
				else
					ena = SetFileAttributes(szFile, ((GetFileAttributes(szFile) & ~FILE_ATTRIBUTE_READONLY) == 0 ? FILE_ATTRIBUTE_NORMAL : GetFileAttributes(szFile) & ~FILE_ATTRIBUTE_READONLY));
				if (!ena) {
					if (GetLastError() == ERROR_ACCESS_DENIED)
						ERROROUT(GetString(IDS_CHANGE_READONLY_ERROR));
					else ReportLastError();
					bReadOnly = !bReadOnly;
					break;
				}
				SwitchReadOnly(bReadOnly);
				UpdateCaption();
				break;
			case ID_FONT_PRIMARY:
				bPrimaryFont = !GetCheckedState(GetMenu(hwndMain), ID_FONT_PRIMARY, TRUE);
				if (!SetClientFont(bPrimaryFont)) {
					GetCheckedState(GetMenu(hwndMain), ID_FONT_PRIMARY, TRUE);
					bPrimaryFont = !bPrimaryFont;
				}
#ifndef USE_RICH_EDIT
				UpdateStatus(TRUE);
#endif
				break;
			case ID_VIEW_OPTIONS:
				ZeroMemory(&pages[0], sizeof(pages[0]));
				pages[0].dwSize = sizeof(PROPSHEETPAGE);
				pages[0].dwFlags = PSP_DEFAULT | PSP_USETITLE;
				pages[0].hInstance = hinstThis;
				pages[0].pszTemplate = MAKEINTRESOURCE(IDD_PROPPAGE_GENERAL);
				pages[0].pszTitle = GetString(IDD_PROPPAGE_GENERAL);
				pages[0].pfnDlgProc = (DLGPROC)GeneralPageProc;
				ZeroMemory(&pages[1], sizeof(pages[1]));
				pages[1].dwSize = sizeof(PROPSHEETPAGE);
				pages[1].hInstance = hinstThis;
				pages[1].dwFlags = PSP_DEFAULT | PSP_USETITLE;
				pages[1].pszTemplate = MAKEINTRESOURCE(IDD_PROPPAGE_VIEW);
				pages[1].pszTitle = GetString(IDD_PROPPAGE_VIEW);
				pages[1].pfnDlgProc = (DLGPROC)ViewPageProc;
				ZeroMemory(&pages[2], sizeof(pages[2]));
				pages[2].dwSize = sizeof(PROPSHEETPAGE);
				pages[2].hInstance = hinstThis;
				pages[2].dwFlags = PSP_DEFAULT | PSP_USETITLE;
				pages[2].pszTemplate = MAKEINTRESOURCE(IDD_PROPPAGE_A2);
				pages[2].pszTitle = GetString(IDD_PROPPAGE_A2);
				pages[2].pfnDlgProc = (DLGPROC)Advanced2PageProc;
				ZeroMemory(&pages[3], sizeof(pages[3]));
				pages[3].dwSize = sizeof(PROPSHEETPAGE);
				pages[3].hInstance = hinstThis;
				pages[3].dwFlags = PSP_DEFAULT | PSP_USETITLE;
				pages[3].pszTemplate = MAKEINTRESOURCE(IDD_PROPPAGE_A1);
				pages[3].pszTitle = GetString(IDD_PROPPAGE_A1);
				pages[3].pfnDlgProc = (DLGPROC)AdvancedPageProc;

				ZeroMemory(&psh, sizeof(psh));
				psh.dwSize = sizeof(PROPSHEETHEADER);
				psh.dwFlags = PSH_USECALLBACK | PSH_NOAPPLYNOW | PSH_PROPSHEETPAGE;
				psh.hwndParent = hwndMain;
				psh.nPages = 4;
				psh.pszCaption = GetString(IDS_SETTINGS_TITLE);
				psh.ppsp = (LPCPROPSHEETPAGE) pages;
				psh.pfnCallback = SheetInitProc;
				memcpy(&tmpOptions, &options, sizeof(option_struct));
				LoadOptions();
				i = PropertySheet(&psh);
#ifdef UNICODE
				if (i < 0 && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED) {	//Win95 compat
					EncodeText((LPBYTE*)&psh.pszCaption, -1, FC_ENC_ANSI, NULL, NULL);
					for (l = 0; l < psh.nPages; l++)
						EncodeText((LPBYTE*)&pages[l].pszTitle, -1, FC_ENC_ANSI, NULL, NULL);
					i = PropertySheetA((PROPSHEETHEADERA*)&psh);
				}
#endif
				if (i > 0) {
					SaveOptions();
					if (options.bLaunchClose && options.nLaunchSave == 2)
						MessageBox(hwndMain, GetString(IDS_LAUNCH_WARNING), GetString(STR_METAPAD), MB_ICONEXCLAMATION);
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
							} else {
								options.nSecondaryFont = tmpOptions.nSecondaryFont;
								memcpy((LPVOID)&options.SecondaryFont, (LPVOID)&tmpOptions.SecondaryFont, sizeof(LOGFONT));
							}
						}

					if (SCNUL(szFile)[0]) {
						kfree(&szCaptionFile);
						UpdateCaption();
					}
					SendMessage(client, EM_SETMARGINS, (WPARAM)EC_LEFTMARGIN, MAKELPARAM(options.nSelectionMarginWidth, 0));
					if (bTransparent) {
						SetLWA(hwnd, 0, (BYTE)((255 * (100 - options.nTransparentPct)) / 100), LWA_ALPHA);
					}
#ifdef USE_RICH_EDIT
					if (tmpOptions.bHideScrollbars != options.bHideScrollbars)
						ERROROUT(GetString(IDS_RESTART_HIDE_SB));
#endif
					if (tmpOptions.bNoFaves != options.bNoFaves || lstrcmp(SCNUL(tmpOptions.szLangPlugin), SCNUL(options.szLangPlugin))) {
						FindAndLoadLanguagePlugin();
						CreateMainMenu(hwnd);
						kfree(&szCaptionFile);
					}
					PopulateMRUList();
					UpdateStatus(TRUE);
				} else if (i < 0)
					ReportLastError();
				break;
			case ID_HELP_ABOUT:
				DialogBox(hinstThis, MAKEINTRESOURCE(IDD_ABOUT), hwndMain, (DLGPROC)AboutDialogProc);
				break;
			case ID_ABOUT_PLUGIN:
				DialogBox(hinstThis, MAKEINTRESOURCE(IDD_ABOUT_PLUGIN), hwndMain, (DLGPROC)AboutPluginDialogProc);
				break;
			case ID_MYFILE_OPEN:
				//SetCurrentDirectory(SCNUL(szDir));
				if (!SaveIfDirty())
					break;
				LoadOptions();
				b = TRUE;
			case ID_INSERT_FILE:
				BrowseFile(client, _T("txt"), SCNUL(szDir), SCNUL(szCustomFilter), TRUE, b, !b, NULL);
				break;
			case ID_MYFILE_NEW:
				if (!SaveIfDirty())
					break;
				MakeNewFile();
				break;
			case ID_EDIT_SELECTWORD:
				cr = GetSelection();
				cr.cpMin = cr.cpMax;
				SetSelection(cr);
				SelectWord(NULL, bSmartSelect, TRUE);
				break;
			case ID_GOTOLINE:
				DialogBox(hinstThis, MAKEINTRESOURCE(IDD_GOTO), hwndMain, (DLGPROC)GotoDialogProc);
				break;
			case ID_EDIT_WORDWRAP:
				hCur = SetCursor(LoadCursor(NULL, IDC_WAIT));
				RestoreClientView(0, FALSE, TRUE, TRUE);
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
				if (!DestroyWindow(client))
					ReportLastError();
				CreateClient(hwnd, NULL, bWordWrap);
				SetWindowText(client, szOld);
				SetClientFont(bPrimaryFont);
				GetClientRect(hwnd, &rect);
				SetWindowPos(client, 0, 0, bShowToolbar ? GetToolbarHeight() : rect.top - GetToolbarHeight(), rect.right - rect.left, rect.bottom - rect.top - GetStatusHeight() - GetToolbarHeight(), SWP_NOZORDER | SWP_SHOWWINDOW);
				SetFocus(client);
#endif
				RestoreClientView(0, TRUE, TRUE, TRUE);
				SetCursor(hCur);
				UpdateStatus(TRUE);
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
			case ID_INDENT:
			case ID_UNINDENT:
			case ID_STRIPCHAR:
			case ID_STRIP_TRAILING_WS:
			case ID_QUOTE:
#ifdef USE_RICH_EDIT
				SendMessage(client, EM_EXGETSEL, 0, (LPARAM)&cr);
				i = SendMessage(client, EM_EXLINEFROMCHAR, 0, (LPARAM)cr.cpMin);
				j = SendMessage(client, EM_EXLINEFROMCHAR, 0, (LPARAM)cr.cpMax);
#else
				SendMessage(client, EM_GETSEL, (WPARAM)&cr.cpMin, (LPARAM)&cr.cpMax);
				i = SendMessage(client, EM_LINEFROMCHAR, (WPARAM)cr.cpMin, 0);
				j = SendMessage(client, EM_LINEFROMCHAR, (WPARAM)cr.cpMax, 0);
#endif
				if (i < j || LOWORD(wParam) != ID_INDENT) {
					szOld = GetShadowBuffer(&l);
					if (cr.cpMin == cr.cpMax){ cr.cpMin--; cr.cpMax++; }
					for (sl = cr.cpMin;	sl < l	 && (szOld[sl] == '\r' || szOld[sl] == '\n'); sl++) ;
					for (			  ;	sl--	 &&  szOld[sl] != '\r' && szOld[sl] != '\n' ; ) ;
					cr.cpMin = sl+1;
					for (sl = cr.cpMax;	sl--	 && (szOld[sl] == '\r' || szOld[sl] == '\n'); ) ;
					for (			  ;	sl++ < l && szOld[sl] != '\r' && szOld[sl] != '\n'; ) ;
					cr.cpMax = sl;
					if (cr.cpMax == cr.cpMin)
						i=j;
					else if(cr.cpMax > cr.cpMin)
						SetSelection(cr);
				}
#ifdef USE_RICH_EDIT
				sz = _T("\r");
#else
				sz = _T("\r\n");
#endif
			case ID_UNTABIFY:
			case ID_TABIFY:
				switch (LOWORD(wParam)){
					case ID_QUOTE:
					case ID_INDENT:
					case ID_UNTABIFY:
					case ID_TABIFY:
						lstrcpy(szTmp, _T("\t \r\n"));
						szTmp[1] = _T('\0');
						if (options.bInsertSpaces || LOWORD(wParam) == ID_UNTABIFY || LOWORD(wParam) == ID_TABIFY) {
#if UNICODE
							wmemset(szTmp+4, _T(' '), options.nTabStops);
#else
							memset(szTmp+4, _T(' '), options.nTabStops);
#endif
							if (i >= j && LOWORD(wParam) == ID_INDENT)
								szTmp[4 + options.nTabStops - (GetColNum(cr.cpMin, i, NULL, NULL, NULL)-1) % options.nTabStops] = _T('\0');
							else
								szTmp[4 + options.nTabStops] = _T('\0');
						}
						else
							lstrcpy(szTmp+4, _T("\t"));
						break;
				}
				switch (LOWORD(wParam)){
					case ID_INDENT:
						if (i >= j) {
							SendMessage(client, EM_REPLACESEL, (WPARAM)TRUE, (LPARAM)(szTmp+4));
							return FALSE;
						}
						break;
					case ID_QUOTE:
						lstrcpy(szTmp+4, SCNUL(options.szQuote));
						break;
				}
				switch (LOWORD(wParam)){
					case ID_INDENT:
					case ID_QUOTE:
						szTmp += 2;
#ifdef USE_RICH_EDIT
						*(++szTmp) = _T('\r');
#endif
						break;
					case ID_UNTABIFY:
					case ID_TABIFY:
						sz = szTmp+4;
						break;
				}
				switch (LOWORD(wParam)){
					case ID_INDENT:
						ReplaceAll(hwnd, 1, 0, &sz, &szTmp, NULL, NULL, NULL, TRUE, TRUE, FALSE, 0, 0, FALSE, sz, NULL);
						break;
					case ID_QUOTE:
						if (!ParseForEscapeSeqs(szTmp, &pbReplaceTextSpec, GetString(IDS_ESCAPE_CTX_QUOTE))) break;
						ReplaceAll(hwnd, 1, 0, &sz, &szTmp, NULL, &pbReplaceTextSpec, NULL, TRUE, TRUE, FALSE, 0, 0, FALSE, sz, NULL);
						break;
				/* TODO: (Un)Tabify impl. is very naive and breaks alignment! */
					case ID_UNTABIFY:
						ReplaceAll(hwnd, 1, 0, &szTmp, &sz, NULL, NULL, NULL, TRUE, TRUE, FALSE, 0, 0, FALSE, NULL, NULL);
						break;
					case ID_TABIFY:
						ReplaceAll(hwnd, 1, 0, &sz, &szTmp, NULL, NULL, NULL, TRUE, TRUE, FALSE, 0, 0, FALSE, NULL, NULL);
						break;
					case ID_UNINDENT:
						if (options.nTabStops < 2) {
#ifdef USE_RICH_EDIT
							static LPCTSTR usf[] = {_T("\r\t"), _T("\r ")};
							static LPCTSTR usr[] = {_T("\r "), _T("\r")};
							ReplaceAll(hwnd, 2, 0, usf, usr, NULL, NULL, NULL, TRUE, TRUE, FALSE, 0, 0, FALSE, sz, NULL);
#else
							static LPCTSTR usf[] = {_T("\r\n\t"), _T("\r\n ")};
							static LPCTSTR usr[] = {_T("\r\n "), _T("\r\n")};
							ReplaceAll(hwnd, 2, 0, usf, usr, NULL, NULL, NULL, TRUE, TRUE, FALSE, 0, 0, FALSE, sz, NULL);
#endif
						} else {
#ifdef USE_RICH_EDIT
							static LPCTSTR usf[] = {_T("\r  \t"), _T("\r   "), _T("\r\t")};
							static LPCTSTR usr[] = {_T("\r   \t"), _T("\r "), _T("\r")};
							static LPBYTE usfs[] = {(LPBYTE)"\0\0\4\0", (LPBYTE)"\0\0\4\1", NULL};
							static LPBYTE usrs[] = {(LPBYTE)"\0\0\0\4\0", (LPBYTE)"\0\1", NULL};
							ReplaceAll(hwnd, 3, 0, usf, usr, usfs, usrs, NULL, TRUE, TRUE, FALSE, 0, options.nTabStops+1, TRUE, sz, NULL);
#else
							static LPCTSTR usf[] = {_T("\r\n  \t"), _T("\r\n   "), _T("\r\n\t")};
							static LPCTSTR usr[] = {_T("\r\n   \t"), _T("\r\n "), _T("\r\n")};
							static LPBYTE usfs[] = {(LPBYTE)"\0\0\0\4\0", (LPBYTE)"\0\0\0\4\1", NULL};
							static LPBYTE usrs[] = {(LPBYTE)"\0\0\0\0\4\0", (LPBYTE)"\0\0\1", NULL};
							ReplaceAll(hwnd, 3, 0, usf, usr, usfs, usrs, NULL, TRUE, TRUE, FALSE, 0, options.nTabStops+2, TRUE, sz, NULL);
#endif
						}
						break;
					case ID_STRIPCHAR: {
#ifdef USE_RICH_EDIT
						static LPCTSTR usf[] = {_T("\r\r"), _T("\r\r"), _T("\r ")};
						static LPCTSTR usr[] = {_T("\r \r"), _T("\r \r"), _T("\r")};
						static LPBYTE usfs[] = {NULL, NULL, (LPBYTE)"\0\1"};
						ReplaceAll(hwnd, 3, 0, usf, usr, usfs, NULL, NULL, TRUE, TRUE, FALSE, 0, 0, FALSE, sz, NULL);
#else
						static LPCTSTR usf[] = {_T("\r\n\r"), _T("\r\n\r"), _T("\r\n ")};
						static LPCTSTR usr[] = {_T("\r\n \r"), _T("\r\n \r"), _T("\r\n")};
						static LPBYTE usfs[] = {NULL, NULL, (LPBYTE)"\0\0\1"};
						ReplaceAll(hwnd, 3, 0, usf, usr, usfs, NULL, NULL, TRUE, TRUE, FALSE, 0, 0, FALSE, sz, NULL);
#endif
						}
						break;
					case ID_STRIP_TRAILING_WS: {
						static LPCTSTR usf[] = {_T(" \r"), _T("\t\r")};
						static LPCTSTR usr[] = {_T("\r"), _T("\r")};
						ReplaceAll(hwnd, 2, 2, usf, usr, NULL, NULL, NULL, TRUE, TRUE, FALSE, 0, 0, FALSE, NULL, sz);
						}
						break;
				}
#ifdef USE_RICH_EDIT
				InvalidateRect(client, NULL, TRUE);
#endif
				QueueUpdateStatus();
				break;
#ifdef USE_RICH_EDIT
			case ID_INSERT_MODE:
				bInsertMode = !bInsertMode;
				SendMessage(client, WM_KEYDOWN, (WPARAM)VK_INSERT, (LPARAM)0x510001);
				SendMessage(client, WM_KEYUP, (WPARAM)VK_INSERT, (LPARAM)0xC0510001);
				UpdateStatus(TRUE);
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
					GetDateFormat(LOCALE_USER_DEFAULT, (LOWORD(wParam) == ID_DATE_TIME_LONG ? DATE_LONGDATE : 0), NULL, NULL, szTmp+0x100, 100);
					lstrcat(szTmp, szTmp+0x100);
				}
				if (bLoading)
					lstrcat(szTmp, _T("\r\n"));
				SendMessage(client, EM_REPLACESEL, (WPARAM)TRUE, (LPARAM)szTmp);
#ifdef USE_RICH_EDIT
				InvalidateRect(client, NULL, TRUE);
#endif

				if (bLoading)
					UpdateSavedInfo();
				QueueUpdateStatus();
				break;
			case ID_PAGESETUP:
				ZeroMemory(&psd, sizeof(PAGESETUPDLG));
				psd.lStructSize = sizeof(PAGESETUPDLG);
				psd.Flags |= /*PSD_INTHOUSANDTHSOFINCHES | */PSD_DISABLEORIENTATION | PSD_DISABLEPAPER | PSD_DISABLEPRINTER | PSD_ENABLEPAGESETUPTEMPLATE | PSD_MARGINS | PSD_ENABLEPAGESETUPHOOK;
				psd.hwndOwner = hwndMain;
				psd.hInstance = hinstThis;
				psd.rtMargin = options.rMargins;
				psd.lpfnPageSetupHook = (LPPAGESETUPHOOK)PageSetupProc;
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
			case ID_STRIP_CR:
			case ID_STRIP_CR_SPACE:
			case ID_MAKE_LOWER:
			case ID_MAKE_UPPER:
			case ID_MAKE_INVERSE:
			case ID_MAKE_SENTENCE:
			case ID_MAKE_TITLE:
			case ID_MAKE_OEM:
			case ID_MAKE_ANSI:
				hCur = SetCursor(LoadCursor(NULL, IDC_WAIT));
				szOld = GetShadowSelection(&l, NULL);
				if (!l) {
					ERROROUT(GetString(IDS_NO_SELECTED_TEXT));
					break;
				}
				k = (LONG)l + 1;
				szNew = kallocsz(k);
				switch (LOWORD(wParam)) {
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
					lstrcpy((LPTSTR)szOld, szNew);
					MultiByteToWideChar(CP_ACP, 0, (LPSTR)szOld, k, szNew, k);
#endif
					break;
				case ID_MAKE_ANSI:
#ifdef UNICODE
					lstrcpy(szNew, szOld);
					WideCharToMultiByte(CP_ACP, 0, szNew, k, (LPSTR)szOld, k, NULL, NULL);
#endif
					OemToCharBuffA((LPSTR)szOld, (LPSTR)szNew, k);
#ifdef UNICODE
					//OemToCharBuffW corrupts newlines, use original ANSI function instead and re-convert to unicode here
					lstrcpy((LPTSTR)szOld, szNew);
					MultiByteToWideChar(CP_ACP, 0, (LPSTR)szOld, k, szNew, k);
#endif
					break;
				}
				if (lstrcmp(szOld, szNew) != 0)
					SendMessage(client, EM_REPLACESEL, (WPARAM)TRUE, (LPARAM)szNew);
				SetSelection(cr);
#ifdef USE_RICH_EDIT
				InvalidateRect(client, NULL, TRUE);
#endif
				kfree(&szNew);
				SetCursor(hCur);
				break;
			case ID_NEW_INSTANCE:
				GetModuleFileName(hinstThis, szTmp, MAXFN);
				//ShellExecute(NULL, NULL, szTmp, NULL, SCNUL(szDir), SW_SHOWNORMAL);
				ExecuteProgram(szTmp, kemptyStr);
				break;
			case ID_LAUNCH_ASSOCIATED_VIEWER:
				LaunchInViewer(FALSE, FALSE);
				break;
			case ID_ENC_CUSTOM:
				DialogBox(hinstThis, MAKEINTRESOURCE(IDD_CP), hwndMain, (DLGPROC)CPDialogProc);
				break;
			case ID_LFMT_DOS:
			case ID_LFMT_UNIX:
			case ID_LFMT_MAC:
			case ID_LFMT_MIXED:
			case ID_ENC_ANSI:
			case ID_ENC_UTF8:
			case ID_ENC_UTF16:
			case ID_ENC_UTF16BE:
			case ID_ENC_BIN:
				SetFileFormat(LOWORD(wParam) % 1000 + FC_BASE, 1);
				break;
			case ID_RELOAD_CURRENT:
				if (!*SCNUL(szFile) || !SaveIfDirty()) break;
				RestoreClientView(0, FALSE, TRUE, TRUE);
				LoadFile(szFile, FALSE, FALSE, FALSE, 0, NULL);
				RestoreClientView(0, TRUE, TRUE, TRUE);
				break;
/*#ifdef USE_RICH_EDIT
//This is now done automatically in UpdateStatus(). (v3.7+)
			case ID_SHOWFILESIZE:
				hCur = SetCursor(LoadCursor(NULL, IDC_WAIT));

				wsprintf(szStatusMessage, GetString(IDS_BYTE_LENGTH), CalculateFileSize());
				UpdateStatus(TRUE);
				if (!bShowStatus)
					SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(ID_SHOWSTATUS, 0), 0);
				SetCursor(hCur);
				break;
#endif*/
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
				sz = (LPTSTR)GetShadowSelection(&l, NULL);
				lstrcpyn(szTmp, sz, MAXMACRO);
				if (l > MAXMACRO - 1) {
					ERROROUT(GetString(IDS_MACRO_LENGTH_ERROR));
					break;
				}
				if (!EncodeWithEscapeSeqs(szTmp)) {
					ERROROUT(GetString(IDS_MACRO_LENGTH_ERROR));
					break;
				}
				kstrdupnul(&options.MacroArray[i], szTmp);
				wsprintf(szTmp, GetString(IDSS_MACROARRAY), i);
				if (!g_bIniMode)
					RegCreateKeyEx(HKEY_CURRENT_USER, GetString(STR_REGKEY), 0, NULL, 0, KEY_ALL_ACCESS, NULL, &key, NULL);
				if (!SaveOption(key, szTmp, REG_SZ, (LPBYTE)options.MacroArray[i], MAXMACRO))
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
				sz = options.MacroArray[LOWORD(wParam) - ID_MACRO_1];
				if (!*SCNUL(sz)) break;
				lstrcpy(szTmp, sz);
				if (!ParseForEscapeSeqs(szTmp, NULL, GetString(IDS_ESCAPE_CTX_MACRO))) break;
				SendMessage(client, EM_REPLACESEL, (WPARAM)TRUE, (LPARAM)szTmp);
#ifdef USE_RICH_EDIT
				InvalidateRect(client, NULL, TRUE);
#endif
				break;
			case ID_COMMIT_WORDWRAP:
				if (bWordWrap) {
					hCur = SetCursor(LoadCursor(NULL, IDC_WAIT));
					l = GetWindowTextLength(client);
					nPos = SendMessage(client, EM_GETLINECOUNT, 0, 0);
					sz = szNew = kallocs(l + nPos + 1);
					for (i = 0; i < nPos; i++) {
						szOld = GetShadowLine(i, -1, &sl, NULL, NULL);
						lstrcpyn(sz, szOld, sl+1);
						sz += sl;
						if (i < nPos - 1) {
							*sz++ = _T('\r');
#ifndef USE_RICH_EDIT
							*sz++ = _T('\n');
#endif
						}
					}
					*sz = _T('\0');
					RestoreClientView(0, FALSE, FALSE, TRUE);
					cr.cpMin = 0;
					cr.cpMax = -1;
					SendMessage(client, WM_SETREDRAW, (WPARAM)FALSE, 0);
					SetSelection(cr);
					SendMessage(client, EM_REPLACESEL, (WPARAM)TRUE, (LPARAM)szNew);
					kfree(&szNew);
					cr.cpMax = 0;
					SetSelection(cr);
					SendMessage(client, WM_SETREDRAW, (WPARAM)TRUE, 0);
					RestoreClientView(0, TRUE, FALSE, TRUE);
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
				DialogBox(hinstThis, MAKEINTRESOURCE(IDD_FAV_NAME), hwndMain, (DLGPROC)AddFavDialogProc);
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
		if (ID_FAV_RANGE_BASE < LOWORD(wParam) && LOWORD(wParam) <= ID_FAV_RANGE_MAX) {
			LoadFileFromMenu(LOWORD(wParam), FALSE);
			return FALSE;
		}
		break;
	case WM_TIMER:
		switch (LOWORD(wParam)) {
			case IDT_UPDATE:
				if (bLoading) break;
				UpdateStatus(FALSE);
				break;
		}
		break;
	default:
		return DefWindowProc(hwndMain, Msg, wParam, lParam);
	}
	return FALSE;
}

DWORD WINAPI LoadThread(LPVOID lpParameter) {
	if (LoadFile(SCNUL(szFile), TRUE, TRUE, FALSE, 0, NULL)) {
#ifdef USE_RICH_EDIT
		if (lpParameter != NULL) {
			GotoLine(((CHARRANGE*)lpParameter)->cpMin, ((CHARRANGE*)lpParameter)->cpMax);
		} else {
			SendMessage(client, EM_SCROLLCARET, 0, 0);
		}
#endif
	} else {
		bQuit = TRUE;
		return 1;
	}
	SendMessage(client, EM_SETREADONLY, (WPARAM)FALSE, 0);
	EnableWindow(hwnd, TRUE);
	SendMessage(hwnd, WM_ACTIVATE, 0, 0);
	return 0;
}

#if defined(__MINGW32__) && !defined(__MINGW64_VERSION_MAJOR)
#include "mingw-unicode-gui.c"
#endif
int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR szCmdLine, int nCmdShow)
{
	WNDCLASS wc;
	MSG msg;
	HACCEL accel = NULL;
	int left, top, width, height;
	int nCmdLen;
	CHARRANGE crLineCol = {-1, -1};
	BOOL bSkipLanguagePlugin = FALSE;
	TCHAR *bufFn = gTmpBuf, *pch;
	TCHAR chOption;
#ifdef _DEBUG
	int _x=0;
	__stktop = (size_t)&_x;
#endif

	globalHeap = GetProcessHeap();
	hinstThis = hInstance;
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
		wc.lpszClassName = GetString(STR_METAPAD);
		if (!RegisterClass(&wc)) {
			ReportLastError();
			return FALSE;
		}
	}

	bLoading = bStarted = bQuit = FALSE;
	ZeroMemory(&options, sizeof(options));
	hwnd = client = status = toolbar = hdlgCancel = hdlgFind = NULL;
	hrecentmenu = NULL;
	hfontmain = hfontfind = NULL;
	szFile = szCaptionFile = szFav = szDir = szMetapadIni = szFindText = szReplaceText = szInsert = szCustomFilter = NULL;
	pbFindTextSpec = NULL;
	pbReplaceTextSpec = NULL;
	frDlgId = -1;
	szShadow = NULL;
	shadowLen = shadowAlloc = shadowRngEnd = 0;
	shadowLine = -1;
	bDirtyShadow = bDirtyStatus = TRUE;
	nFormat = 0;
	g_bIniMode = FALSE;
	gbNT = (LOBYTE(LOWORD(GetVersion())) >= 5);
	updateThrottle = updateTime = 0;
	savedFormat = 0;
	savedChars = 0;
	fontHt = -3;
	ZeroMemory(FindArray, sizeof(FindArray));
	ZeroMemory(ReplaceArray, sizeof(ReplaceArray));
	ZeroMemory(InsertArray, sizeof(InsertArray));
	*szStatusMessage = 0;
	randVal = GetTickCount();

	GetModuleFileName(hinstThis, bufFn, MAXFN);
	kstrdupa(&szMetapadIni, bufFn, 12);

	pch = lstrrchr(szMetapadIni, _T('\\'));
	++pch;
	*pch = _T('\0');
	lstrcat(szMetapadIni, GetString(STR_INI_FILE));
	
	if (szCmdLine && *szCmdLine) {
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
					SaveOption(NULL, GetString(IDSS_WSTATE), REG_DWORD, (LPBYTE)&nShow, sizeof(int));
					SaveOption(NULL, GetString(IDSS_WLEFT), REG_DWORD, (LPBYTE)&left, sizeof(int));
					SaveOption(NULL, GetString(IDSS_WTOP), REG_DWORD, (LPBYTE)&top, sizeof(int));
					SaveOption(NULL, GetString(IDSS_WWIDTH), REG_DWORD, (LPBYTE)&width, sizeof(int));
					SaveOption(NULL, GetString(IDSS_WHEIGHT), REG_DWORD, (LPBYTE)&height, sizeof(int));
				}
				g_bIniMode = TRUE;
				SaveOptions();
				MSGOUT(GetString(IDS_MIGRATED));
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
	kstrdup(&szDir, bufFn);
	LoadOptions();
	
//	options.nStatusFontWidth = LOWORD(GetDialogBaseUnits());
	bWordWrap = FALSE;
	bPrimaryFont = TRUE;
	bSmartSelect = TRUE;
#ifdef USE_RICH_EDIT
	bHyperlinks = TRUE;
#endif
	bShowStatus = TRUE;
	bShowToolbar = TRUE;
	bAlwaysOnTop = FALSE;
	bCloseAfterFind = bCloseAfterReplace = bCloseAfterInsert = FALSE;
	bNoFindHidden = TRUE;
	bTransparent = FALSE;
	bDirtyFile = FALSE;
	bDown = TRUE;
	bWholeWord = FALSE;
	bInsertMode = TRUE;

	LoadMenusAndData();
	FixFilterString(szCustomFilter);
	if (options.bSaveWindowPlacement || options.bStickyWindow)
		LoadWindowPlacement(&left, &top, &width, &height, &nCmdShow);
	else {
		left = top = width = height = CW_USEDEFAULT;
		nCmdShow = SW_SHOWNORMAL;
	}
	
	if (!(hwnd = CreateWindowEx( WS_EX_ACCEPTFILES, GetString(STR_METAPAD), GetString(STR_CAPTION_FILE), WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
		left, top, width, height, NULL, NULL, hInstance, NULL))) {
		ReportLastError();
		return FALSE;
	}
	if (SetLWA = (SLWA)(GetProcAddress(GetModuleHandleA("user32.dll"), "SetLayeredWindowAttributes")))
		SetLWA(hwnd, 0, 255, LWA_ALPHA);
	SetWindowTheme = (SWT)(GetProcAddress(GetModuleHandleA("uxtheme.dll"), "SetWindowTheme"));
	if (!(accel = LoadAccelerators(hinstThis, MAKEINTRESOURCE(IDR_ACCELERATOR)))) {
		ReportLastError();
		return FALSE;
	}
	if (!ParseAccels(accel)) ReportLastError();
	if (!bSkipLanguagePlugin) FindAndLoadLanguagePlugin();
	if (!CreateMainMenu(hwnd)) return FALSE;

#ifdef USE_RICH_EDIT
	if (LoadLibraryA(STR_RICHDLL) == NULL) {
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
	//uFindReplaceMsg = RegisterWindowMessage(FINDMSGSTRING);
	if (bShowStatus) CreateStatusbar();
	if (bShowToolbar) CreateToolbar();
	InitCommonControls();
	MakeNewFile();
	if (szCmdLine && *szCmdLine) {
		kfree(&szFile);
		szFile = kallocs(nCmdLen+1);
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
				if (!LoadFile(szFile, FALSE, FALSE, FALSE, 0, NULL))
					return FALSE;
				UpdateCaption();
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
			if (szCmdLine[0] == _T('\"')) lstrcpyn(szFile, szCmdLine + 1, lstrchr(szCmdLine+1, _T('\"')) - szCmdLine);
			else lstrcpyn(szFile, szCmdLine, nCmdLen + 1);
		}

		bufFn[0] = _T('\0');
		GetFullPathName(szFile, MAXFN, bufFn, NULL);
		if (bufFn[0])
			kstrdup(&szFile, bufFn);
		
#ifdef USE_RICH_EDIT
		{
			DWORD dwID;
			UpdateCaption();
			SendMessage(client, EM_SETREADONLY, (WPARAM)TRUE, 0);
			EnableWindow(hwnd, FALSE);
			hthread = CreateThread(NULL, 0, LoadThread, (LPVOID)&(crLineCol), 0, &dwID);
		}
#else
		LoadThread(NULL);
		SendMessage(client, EM_SCROLLCARET, 0, 0);
		GotoLine(crLineCol.cpMin, crLineCol.cpMax);
#endif
	} 
	
	if (bQuit)
		PostQuitMessage(0);
	else {
		ShowWindow(hwnd, nCmdShow);
		bStarted = TRUE;
	}

	//SetWindowLongPtr(hwnd, GWL_EXSTYLE, GetWindowLongPtr(hwnd, GWL_EXSTYLE) | WS_CLIPCHILDREN);	//prevent flicker at expense of CPU time (no effect on Aero?)
	UpdateStatus(TRUE);

	SendMessage(client, EM_SETMARGINS, (WPARAM)EC_LEFTMARGIN, MAKELPARAM(options.nSelectionMarginWidth, 0));
	SetFocus(client);

	while (GetMessage(&msg, NULL, 0,0)) {
		if (hdlgFind && (msg.message == WM_KEYDOWN || msg.message == WM_SYSCOMMAND) && GetActiveWindow() == hdlgFind && FindProc(hdlgFind, msg.message, msg.wParam, msg.lParam)) continue;
		if (!(hdlgFind && IsDialogMessage(hdlgFind, &msg)) && !TranslateAccelerator(hwnd, accel, &msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return msg.wParam;
}
