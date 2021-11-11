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
 * @file settings_save.c
 * @brief Settings saving functions.
 */

#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0400

#include <windows.h>
#include <tchar.h>

#ifdef BUILD_METAPAD_UNICODE
#include <wchar.h>
#endif

#include "include/encoding.h"
#include "include/tmp_protos.h"
#include "include/consts.h"
#include "include/strings.h"
#include "include/typedefs.h"
#include "include/macros.h"

extern HANDLE globalHeap;
extern LPTSTR szMetapadIni;
extern option_struct options;
extern BOOL bWordWrap;
extern BOOL bPrimaryFont;
extern BOOL bSmartSelect;
extern BOOL bShowStatus;
extern BOOL bShowToolbar;
extern BOOL bAlwaysOnTop;
extern BOOL bTransparent;
extern BOOL bCloseAfterFind;
extern BOOL bCloseAfterInsert;
extern BOOL bNoFindHidden;
extern BOOL g_bIniMode;
extern LPTSTR FindArray[NUMFINDS];
extern LPTSTR ReplaceArray[NUMFINDS];
extern LPTSTR InsertArray[NUMINSERTS];
extern LPTSTR szDir;

#ifdef USE_RICH_EDIT
extern BOOL bHyperlinks;
#endif

/**
 * Save an option.
 *
 * @param[in] hKey A handle to an open registry key. If NULL, SaveOption stores the option in an ini file.
 * @param[in] name Name of the value to be set.
 * @param[in] dwType The type of lpData.
 * @param[in] lpData The data to be stored.
 * @param[in] cbData The size of lpData, in bytes.
 * @return TRUE if the value was stored successfully, FALSE otherwise.
 */
BOOL SaveOption(HKEY hKey, LPCTSTR name, DWORD dwType, CONST LPBYTE lpData, DWORD cbData)
{
	LPBYTE szData = (LPBYTE)(SCNUL((LPTSTR)lpData));
	UINT i;
	if (hKey) {
		if (dwType == REG_SZ) cbData = (lstrlen((LPTSTR)szData)+1) * sizeof(TCHAR);
		if ((i = RegSetValueEx(hKey, name, 0, dwType, szData, cbData)) != ERROR_SUCCESS){
			ReportError(i);
			return FALSE;
		} else return TRUE;
	}
	else {
		BOOLEAN writeSucceeded = TRUE;
		switch (dwType) {
			case REG_DWORD: {
				TCHAR val[10];
				int int32 = 0;
				int32 = (int32 << 8) + lpData[3];
				int32 = (int32 << 8) + lpData[2];
				int32 = (int32 << 8) + lpData[1];
				int32 = (int32 << 8) + lpData[0];
				wsprintf(val, _T("%d"), int32);
				writeSucceeded = WritePrivateProfileString(_T("Options"), name, val, SCNUL(szMetapadIni));
				break;
			}
			case REG_SZ:
				writeSucceeded = WritePrivateProfileString(_T("Options"), name, (LPTSTR)szData, SCNUL(szMetapadIni));
				break;
			case REG_BINARY: {
				TCHAR *szBuffer = (LPTSTR)HeapAlloc(globalHeap, 0, 2 * (cbData + 1) * sizeof(TCHAR));
				EncodeBase(64, lpData, szBuffer, cbData, NULL );
				writeSucceeded = WritePrivateProfileString(_T("Options"), name, (TCHAR*)szBuffer, SCNUL(szMetapadIni));
				HeapFree(globalHeap, 0, szBuffer);
				break;
			}
		}
		return writeSucceeded;
	}
}

/**
 * Save all of metapad's options.
 */
void SaveOptions(void)
{
	HKEY key = NULL;
	BOOL writeSucceeded = TRUE;
	TCHAR keyname[20];
	int i;

	if (!g_bIniMode) {
		if (RegCreateKeyEx(HKEY_CURRENT_USER, STR_REGKEY, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &key, NULL) != ERROR_SUCCESS) {
			ReportLastError();
			return;
		}
	}

	writeSucceeded &= SaveOption(key, _T("bSystemColours"), REG_DWORD, (LPBYTE)&options.bSystemColours, sizeof(BOOL));
	writeSucceeded &= SaveOption(key, _T("bSystemColours2"), REG_DWORD, (LPBYTE)&options.bSystemColours2, sizeof(BOOL));
	writeSucceeded &= SaveOption(key, _T("bNoSmartHome"), REG_DWORD, (LPBYTE)&options.bNoSmartHome, sizeof(BOOL));
	writeSucceeded &= SaveOption(key, _T("bNoAutoSaveExt"), REG_DWORD, (LPBYTE)&options.bNoAutoSaveExt, sizeof(BOOL));
	writeSucceeded &= SaveOption(key, _T("bContextCursor"), REG_DWORD, (LPBYTE)&options.bContextCursor, sizeof(BOOL));
	writeSucceeded &= SaveOption(key, _T("bCurrentFindFont"), REG_DWORD, (LPBYTE)&options.bCurrentFindFont, sizeof(BOOL));
	writeSucceeded &= SaveOption(key, _T("bPrintWithSecondaryFont"), REG_DWORD, (LPBYTE)&options.bPrintWithSecondaryFont, sizeof(BOOL));
	writeSucceeded &= SaveOption(key, _T("bNoSaveHistory"), REG_DWORD, (LPBYTE)&options.bNoSaveHistory, sizeof(BOOL));
	writeSucceeded &= SaveOption(key, _T("bNoFindAutoSelect"), REG_DWORD, (LPBYTE)&options.bNoFindAutoSelect, sizeof(BOOL));
	writeSucceeded &= SaveOption(key, _T("bHideGotoOffset"), REG_DWORD, (LPBYTE)&options.bHideGotoOffset, sizeof(BOOL));
	writeSucceeded &= SaveOption(key, _T("bRecentOnOwn"), REG_DWORD, (LPBYTE)&options.bRecentOnOwn, sizeof(BOOL));
	writeSucceeded &= SaveOption(key, _T("bDontInsertTime"), REG_DWORD, (LPBYTE)&options.bDontInsertTime, sizeof(BOOL));
	writeSucceeded &= SaveOption(key, _T("bNoWarningPrompt"), REG_DWORD, (LPBYTE)&options.bNoWarningPrompt, sizeof(BOOL));
	writeSucceeded &= SaveOption(key, _T("bUnFlatToolbar"), REG_DWORD, (LPBYTE)&options.bUnFlatToolbar, sizeof(BOOL));
	writeSucceeded &= SaveOption(key, _T("bReadOnlyMenu"), REG_DWORD, (LPBYTE)&options.bReadOnlyMenu, sizeof(BOOL));
	writeSucceeded &= SaveOption(key, _T("bStickyWindow"), REG_DWORD, (LPBYTE)&options.bStickyWindow, sizeof(BOOL));
//	writeSucceeded &= SaveOption(key, _T("nStatusFontWidth"), REG_DWORD, (LPBYTE)&options.nStatusFontWidth, sizeof(int));
	writeSucceeded &= SaveOption(key, _T("nSelectionMarginWidth"), REG_DWORD, (LPBYTE)&options.nSelectionMarginWidth, sizeof(int));
	writeSucceeded &= SaveOption(key, _T("nMaxMRU"), REG_DWORD, (LPBYTE)&options.nMaxMRU, sizeof(int));
	writeSucceeded &= SaveOption(key, _T("nFormatIndex"), REG_DWORD, (LPBYTE)&options.nFormatIndex, sizeof(int));
	writeSucceeded &= SaveOption(key, _T("nTransparentPct"), REG_DWORD, (LPBYTE)&options.nTransparentPct, sizeof(int));
	writeSucceeded &= SaveOption(key, _T("bNoCaptionDir"), REG_DWORD, (LPBYTE)&options.bNoCaptionDir, sizeof(BOOL));
	writeSucceeded &= SaveOption(key, _T("bAutoIndent"), REG_DWORD, (LPBYTE)&options.bAutoIndent, sizeof(BOOL));
	writeSucceeded &= SaveOption(key, _T("bInsertSpaces"), REG_DWORD, (LPBYTE)&options.bInsertSpaces, sizeof(BOOL));
 	writeSucceeded &= SaveOption(key, _T("bFindAutoWrap"), REG_DWORD, (LPBYTE)&options.bFindAutoWrap, sizeof(BOOL));
	writeSucceeded &= SaveOption(key, _T("bQuickExit"), REG_DWORD, (LPBYTE)&options.bQuickExit, sizeof(BOOL));
	writeSucceeded &= SaveOption(key, _T("bSaveWindowPlacement"), REG_DWORD, (LPBYTE)&options.bSaveWindowPlacement, sizeof(BOOL));
	writeSucceeded &= SaveOption(key, _T("bSaveMenuSettings"), REG_DWORD, (LPBYTE)&options.bSaveMenuSettings, sizeof(BOOL));
	writeSucceeded &= SaveOption(key, _T("bSaveDirectory"), REG_DWORD, (LPBYTE)&options.bSaveDirectory, sizeof(BOOL));
	writeSucceeded &= SaveOption(key, _T("bLaunchClose"), REG_DWORD, (LPBYTE)&options.bLaunchClose, sizeof(BOOL));
	writeSucceeded &= SaveOption(key, _T("bNoFaves"), REG_DWORD, (LPBYTE)&options.bNoFaves, sizeof(BOOL));
#ifndef USE_RICH_EDIT
	writeSucceeded &= SaveOption(key, _T("bDefaultPrintFont"), REG_DWORD, (LPBYTE)&options.bDefaultPrintFont, sizeof(BOOL));
	writeSucceeded &= SaveOption(key, _T("bAlwaysLaunch"), REG_DWORD, (LPBYTE)&options.bAlwaysLaunch, sizeof(BOOL));
#else
	writeSucceeded &= SaveOption(key, _T("bLinkDoubleClick"), REG_DWORD, (LPBYTE)&options.bLinkDoubleClick, sizeof(BOOL));
	writeSucceeded &= SaveOption(key, _T("bHideScrollbars"), REG_DWORD, (LPBYTE)&options.bHideScrollbars, sizeof(BOOL));
	writeSucceeded &= SaveOption(key, _T("bSuppressUndoBufferPrompt"), REG_DWORD, (LPBYTE)&options.bSuppressUndoBufferPrompt, sizeof(BOOL));
#endif
	writeSucceeded &= SaveOption(key, _T("nLaunchSave"), REG_DWORD, (LPBYTE)&options.nLaunchSave, sizeof(int));
	writeSucceeded &= SaveOption(key, _T("nTabStops"), REG_DWORD, (LPBYTE)&options.nTabStops, sizeof(int));
	writeSucceeded &= SaveOption(key, _T("nPrimaryFont"), REG_DWORD, (LPBYTE)&options.nPrimaryFont, sizeof(int));
	writeSucceeded &= SaveOption(key, _T("nSecondaryFont"), REG_DWORD, (LPBYTE)&options.nSecondaryFont, sizeof(int));
#ifdef BUILD_METAPAD_UNICODE
	writeSucceeded &= SaveOption(key, _T("PrimaryFontU"), REG_BINARY, (LPBYTE)&options.PrimaryFont, sizeof(LOGFONT));
	writeSucceeded &= SaveOption(key, _T("SecondaryFontU"), REG_BINARY, (LPBYTE)&options.SecondaryFont, sizeof(LOGFONT));
#else
	writeSucceeded &= SaveOption(key, _T("PrimaryFont"), REG_BINARY, (LPBYTE)&options.PrimaryFont, sizeof(LOGFONT));
	writeSucceeded &= SaveOption(key, _T("SecondaryFont"), REG_BINARY, (LPBYTE)&options.SecondaryFont, sizeof(LOGFONT));
#endif
	writeSucceeded &= SaveOption(key, _T("szBrowser"), REG_SZ, (LPBYTE)options.szBrowser, MAXFN);
	writeSucceeded &= SaveOption(key, _T("szArgs"), REG_SZ, (LPBYTE)options.szArgs, MAXARGS);
	writeSucceeded &= SaveOption(key, _T("szBrowser2"), REG_SZ, (LPBYTE)options.szBrowser2, MAXFN);
	writeSucceeded &= SaveOption(key, _T("szArgs2"), REG_SZ, (LPBYTE)options.szArgs2, MAXARGS);
	writeSucceeded &= SaveOption(key, _T("szQuote"), REG_SZ, (LPBYTE)options.szQuote, MAXQUOTE);
	writeSucceeded &= SaveOption(key, _T("szLangPlugin"), REG_SZ, (LPBYTE)options.szLangPlugin, MAXFN);
	writeSucceeded &= SaveOption(key, _T("szFavDir"), REG_SZ, (LPBYTE)options.szFavDir, MAXFN);
	writeSucceeded &= SaveOption(key, _T("szCustomDate"), REG_SZ, (LPBYTE)options.szCustomDate, MAXDATEFORMAT);
	writeSucceeded &= SaveOption(key, _T("szCustomDate2"), REG_SZ, (LPBYTE)options.szCustomDate2, MAXDATEFORMAT);
	for (i = 0; i < 10; ++i) {
		wsprintf(keyname, _T("szMacroArray%d"), i);
		writeSucceeded &= SaveOption(key, keyname, REG_SZ, (LPBYTE)options.MacroArray[i], MAXMACRO);
	}

	writeSucceeded &= SaveOption(key, _T("BackColour"), REG_BINARY, (LPBYTE)&options.BackColour, sizeof(COLORREF));
	writeSucceeded &= SaveOption(key, _T("FontColour"), REG_BINARY, (LPBYTE)&options.FontColour, sizeof(COLORREF));
	writeSucceeded &= SaveOption(key, _T("BackColour2"), REG_BINARY, (LPBYTE)&options.BackColour2, sizeof(COLORREF));
	writeSucceeded &= SaveOption(key, _T("FontColour2"), REG_BINARY, (LPBYTE)&options.FontColour2, sizeof(COLORREF));
	writeSucceeded &= SaveOption(key, _T("rMargins"), REG_BINARY, (LPBYTE)&options.rMargins, sizeof(RECT));

	if (!writeSucceeded) {
		ReportLastError();
	}

	if (key) {
		RegCloseKey(key);
	}
}

/**
 * Save a window's placement.
 *
 * @param[in] hWndSave Handle to the window which placement is to be saved.
 */
void SaveWindowPlacement(HWND hWndSave)
{
	HKEY key = NULL;
	BOOL writeSucceeded = TRUE;
	RECT rect;
	int left, top, width, height, max;
	WINDOWPLACEMENT wndpl;

	wndpl.length = sizeof(WINDOWPLACEMENT);
	GetWindowPlacement(hWndSave, &wndpl);

	if (wndpl.showCmd == SW_SHOWNORMAL) {
		GetWindowRect(hWndSave, &rect);
		max = SW_SHOWNORMAL;
	}
	else {
		if (wndpl.showCmd == SW_SHOWMAXIMIZED)
			max = SW_SHOWMAXIMIZED;
		else
			max = SW_SHOWNORMAL;
		rect = wndpl.rcNormalPosition;
	}

	left = rect.left;
	top = rect.top;
	width = rect.right - left;
	height = rect.bottom - top;

	if (!g_bIniMode) {
		if (RegCreateKeyEx(HKEY_CURRENT_USER, STR_REGKEY, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &key, NULL) != ERROR_SUCCESS) {
			ReportLastError();
			return;
		}
	}

	writeSucceeded &= SaveOption(key, _T("w_WindowState"), REG_DWORD, (LPBYTE)&max, sizeof(int));
	writeSucceeded &= SaveOption(key, _T("w_Left"), REG_DWORD, (LPBYTE)&left, sizeof(int));
	writeSucceeded &= SaveOption(key, _T("w_Top"), REG_DWORD, (LPBYTE)&top, sizeof(int));
	writeSucceeded &= SaveOption(key, _T("w_Width"), REG_DWORD, (LPBYTE)&width, sizeof(int));
	writeSucceeded &= SaveOption(key, _T("w_Height"), REG_DWORD, (LPBYTE)&height, sizeof(int));

	if (!writeSucceeded) {
		ReportLastError();
	}

	if (key != NULL) {
		RegCloseKey(key);
	}
}

/**
 * Save metapad's menus and data.
 */
void SaveMenusAndData(void)
{
	HKEY key = NULL;
	TCHAR keyname[20];
	int i;

	if (!g_bIniMode) {
		if (RegCreateKeyEx(HKEY_CURRENT_USER, STR_REGKEY, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &key, NULL) != ERROR_SUCCESS) {
			ReportLastError();
			return;
		}
	}
	if (options.bSaveMenuSettings) {
		SaveOption(key, _T("m_WordWrap"), REG_DWORD, (LPBYTE)&bWordWrap, sizeof(BOOL));
		SaveOption(key, _T("m_PrimaryFont"), REG_DWORD, (LPBYTE)&bPrimaryFont, sizeof(BOOL));
		SaveOption(key, _T("m_SmartSelect"), REG_DWORD, (LPBYTE)&bSmartSelect, sizeof(BOOL));
#ifdef USE_RICH_EDIT
		SaveOption(key, _T("m_Hyperlinks"), REG_DWORD, (LPBYTE)&bHyperlinks, sizeof(BOOL));
#endif
		SaveOption(key, _T("m_ShowStatus"), REG_DWORD, (LPBYTE)&bShowStatus, sizeof(BOOL));
		SaveOption(key, _T("m_ShowToolbar"), REG_DWORD, (LPBYTE)&bShowToolbar, sizeof(BOOL));
		SaveOption(key, _T("m_AlwaysOnTop"), REG_DWORD, (LPBYTE)&bAlwaysOnTop, sizeof(BOOL));
		SaveOption(key, _T("m_Transparent"), REG_DWORD, (LPBYTE)&bTransparent, sizeof(BOOL));
		SaveOption(key, _T("bCloseAfterFind"), REG_DWORD, (LPBYTE)&bCloseAfterFind, sizeof(BOOL));
		SaveOption(key, _T("bCloseAfterInsert"), REG_DWORD, (LPBYTE)&bCloseAfterInsert, sizeof(BOOL));
		SaveOption(key, _T("bNoFindHidden"), REG_DWORD, (LPBYTE)&bNoFindHidden, sizeof(BOOL));
	}

	if (!options.bNoSaveHistory) {
		for (i = 0; i < NUMFINDS; ++i) {
			if (!FindArray[i]) continue;
			wsprintf(keyname, _T("szFindArray%d"), i);
			SaveOption(key, keyname, REG_SZ, (LPBYTE)FindArray[i], MAXFIND);
		}
		for (i = 0; i < NUMFINDS; ++i) {
			if (!ReplaceArray[i]) continue;
			wsprintf(keyname, _T("szReplaceArray%d"), i);
			SaveOption(key, keyname, REG_SZ, (LPBYTE)ReplaceArray[i], MAXFIND);
		}
		for (i = 0; i < NUMINSERTS; ++i) {
			if (!InsertArray[i]) continue;
			wsprintf(keyname, _T("szInsertArray%d"), i);
			SaveOption(key, keyname, REG_SZ, (LPBYTE)InsertArray[i], MAXINSERT);
		}
	}

	if (options.bSaveDirectory)
		SaveOption(key, _T("szLastDirectory"), REG_SZ, (LPBYTE)szDir, MAXFN);

	if (key != NULL)
		RegCloseKey(key);
}
