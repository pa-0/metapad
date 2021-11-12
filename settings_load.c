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
 * @file settings_load.c
 * @brief Settings loading functions.
 */

#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0400

#include <windows.h>
#include <tchar.h>
#ifdef UNICODE
#include <wchar.h>
#endif

#include "include/consts.h"
#include "include/globals.h"
#include "include/resource.h"
#include "include/strings.h"
#include "include/macros.h"
#include "include/metapad.h"
#include "include/encoding.h"

/**
 * Load a binary option.
 *
 * @param hKey A handle to an open registry key. If NULL, loads the option from an ini file.
 * @param name Name of the value to get.
 * @param lpData Pointer to the place to store the data.
 * @param cbData The size of lpData, in bytes.
 * @return TRUE if the value was loaded successfully, FALSE otherwise.
 */
static void LoadOptionBinary(HKEY hKey, LPCTSTR name, LPBYTE lpData, DWORD cbData)
{
	if (hKey) {
		RegQueryValueEx(hKey, name, NULL, NULL, lpData, &cbData);
	}
	else {
		UINT l = (cbData + 1) * sizeof(TCHAR) * 2;
		TCHAR *szBuffer = (LPTSTR)HeapAlloc(globalHeap, HEAP_ZERO_MEMORY, l);
		EncodeBase(64, lpData, szBuffer, cbData, NULL );
		if (GetPrivateProfileString(_T("Options"), name, szBuffer, szBuffer, l, SCNUL(szMetapadIni)) > 0)
			DecodeBase(64, szBuffer, lpData, -1, 0, 1, TRUE, NULL );
		HeapFree(globalHeap, 0, szBuffer);
	}
}

/**
 * Load metapad's options.
 */
void LoadOptions(void)
{
	HKEY key = NULL;

	options.nTabStops = 4;
	options.rMargins.top = options.rMargins.bottom = options.rMargins.left = options.rMargins.right = 500;
	options.nLaunchSave = options.nPrimaryFont = options.nSecondaryFont = 0;
	options.bNoCaptionDir = options.bFindAutoWrap = options.bSaveWindowPlacement = options.bLaunchClose = options.bQuickExit = TRUE;
	options.bAutoIndent = TRUE;
	options.bInsertSpaces = FALSE;
	options.bSystemColours = TRUE;
	options.bSystemColours2 = TRUE;
	options.bSaveMenuSettings = TRUE;
	options.bSaveDirectory = TRUE;
	options.bNoSmartHome = FALSE;
	options.bNoAutoSaveExt = FALSE;
	options.bContextCursor = FALSE;
	options.bCurrentFindFont = FALSE;
	options.bPrintWithSecondaryFont = FALSE;
	options.bNoSaveHistory = FALSE;
	options.bNoFindAutoSelect = FALSE;
	options.bNoFaves = FALSE;

	ZeroMemory(&options.PrimaryFont, sizeof(LOGFONT));
	ZeroMemory(&options.SecondaryFont, sizeof(LOGFONT));
	ZeroMemory(&options.MacroArray, sizeof(options.MacroArray));
	options.szFavDir = NULL;
	options.szQuote = NULL;
	options.szCustomDate = NULL;
	options.szCustomDate2 = NULL;
	options.szArgs = NULL;
	options.szArgs2 = NULL;
	options.szBrowser = NULL;
	options.szBrowser2 = NULL;
	options.szLangPlugin = NULL;
	options.bHideGotoOffset = FALSE;
	options.bRecentOnOwn = FALSE;
	options.bDontInsertTime = FALSE;
	options.bNoWarningPrompt = FALSE;
	options.bUnFlatToolbar = TRUE;
	options.bStickyWindow = FALSE;
	options.bReadOnlyMenu = FALSE;
	//options.nStatusFontWidth = 16;
	options.nSelectionMarginWidth = 10;
	options.nMaxMRU = 8;
	options.nFormat = 0;
	options.nTransparentPct = 25;
	options.BackColour = GetSysColor(COLOR_WINDOW);
	options.FontColour = GetSysColor(COLOR_WINDOWTEXT);
	options.BackColour2 = GetSysColor(COLOR_WINDOW);
	options.FontColour2 = GetSysColor(COLOR_WINDOWTEXT);

#ifndef USE_RICH_EDIT
	options.bDefaultPrintFont = FALSE;
	options.bAlwaysLaunch = FALSE;
#else
	options.bHideScrollbars = FALSE;
	options.bLinkDoubleClick = FALSE;
	options.bSuppressUndoBufferPrompt = FALSE;
#endif

	if (!g_bIniMode) {
		if (RegOpenKeyEx(HKEY_CURRENT_USER, STR_REGKEY, 0, KEY_ALL_ACCESS, &key) != ERROR_SUCCESS) {
			ReportLastError();
			return;
		}
	}

	{
		DWORD dwBufferSize;
		TCHAR keyname[20];
		int i;

		dwBufferSize = sizeof(int);
		LoadOptionNumeric(key, _T("bHideGotoOffset"), (LPBYTE)&options.bHideGotoOffset, dwBufferSize);
		LoadOptionNumeric(key, _T("bSystemColours"), (LPBYTE)&options.bSystemColours, dwBufferSize);
		LoadOptionNumeric(key, _T("bSystemColours2"), (LPBYTE)&options.bSystemColours2, dwBufferSize);
		LoadOptionNumeric(key, _T("bNoSmartHome"), (LPBYTE)&options.bNoSmartHome, dwBufferSize);
		LoadOptionNumeric(key, _T("bNoAutoSaveExt"), (LPBYTE)&options.bNoAutoSaveExt, dwBufferSize);
		LoadOptionNumeric(key, _T("bContextCursor"), (LPBYTE)&options.bContextCursor, dwBufferSize);
		LoadOptionNumeric(key, _T("bCurrentFindFont"), (LPBYTE)&options.bCurrentFindFont, dwBufferSize);
		LoadOptionNumeric(key, _T("bPrintWithSecondaryFont"), (LPBYTE)&options.bPrintWithSecondaryFont, dwBufferSize);
		LoadOptionNumeric(key, _T("bNoSaveHistory"), (LPBYTE)&options.bNoSaveHistory, dwBufferSize);
		LoadOptionNumeric(key, _T("bNoFindAutoSelect"), (LPBYTE)&options.bNoFindAutoSelect, dwBufferSize);
		LoadOptionNumeric(key, _T("bRecentOnOwn"), (LPBYTE)&options.bRecentOnOwn, dwBufferSize);
		LoadOptionNumeric(key, _T("bDontInsertTime"), (LPBYTE)&options.bDontInsertTime, dwBufferSize);
		LoadOptionNumeric(key, _T("bNoWarningPrompt"), (LPBYTE)&options.bNoWarningPrompt, dwBufferSize);
		LoadOptionNumeric(key, _T("bUnFlatToolbar"), (LPBYTE)&options.bUnFlatToolbar, dwBufferSize);
		LoadOptionNumeric(key, _T("bStickyWindow"), (LPBYTE)&options.bStickyWindow, dwBufferSize);
		LoadOptionNumeric(key, _T("bReadOnlyMenu"), (LPBYTE)&options.bReadOnlyMenu, dwBufferSize);
//		LoadOptionNumeric(key, _T("nStatusFontWidth"), (LPBYTE)&options.nStatusFontWidth, dwBufferSize);
		LoadOptionNumeric(key, _T("nSelectionMarginWidth"), (LPBYTE)&options.nSelectionMarginWidth, dwBufferSize);
		LoadOptionNumeric(key, _T("nMaxMRU"), (LPBYTE)&options.nMaxMRU, dwBufferSize);
		LoadOptionNumeric(key, _T("nFormat"), (LPBYTE)&options.nFormat, sizeof(DWORD));
		LoadOptionNumeric(key, _T("nTransparentPct"), (LPBYTE)&options.nTransparentPct, dwBufferSize);
		LoadOptionNumeric(key, _T("bNoCaptionDir"), (LPBYTE)&options.bNoCaptionDir, dwBufferSize);
		LoadOptionNumeric(key, _T("bAutoIndent"), (LPBYTE)&options.bAutoIndent, dwBufferSize);
		LoadOptionNumeric(key, _T("bInsertSpaces"), (LPBYTE)&options.bInsertSpaces, dwBufferSize);
		LoadOptionNumeric(key, _T("bFindAutoWrap"), (LPBYTE)&options.bFindAutoWrap, dwBufferSize);
		LoadOptionNumeric(key, _T("bQuickExit"), (LPBYTE)&options.bQuickExit, dwBufferSize);
		LoadOptionNumeric(key, _T("bSaveWindowPlacement"), (LPBYTE)&options.bSaveWindowPlacement, dwBufferSize);
		LoadOptionNumeric(key, _T("bSaveMenuSettings"), (LPBYTE)&options.bSaveMenuSettings, dwBufferSize);
		LoadOptionNumeric(key, _T("bSaveDirectory"), (LPBYTE)&options.bSaveDirectory, dwBufferSize);
		LoadOptionNumeric(key, _T("bLaunchClose"), (LPBYTE)&options.bLaunchClose, dwBufferSize);
		LoadOptionNumeric(key, _T("bNoFaves"), (LPBYTE)&options.bNoFaves, dwBufferSize);
#ifndef USE_RICH_EDIT
		LoadOptionNumeric(key, _T("bDefaultPrintFont"), (LPBYTE)&options.bDefaultPrintFont, dwBufferSize);
		LoadOptionNumeric(key, _T("bAlwaysLaunch"), (LPBYTE)&options.bAlwaysLaunch, dwBufferSize);
#else
		LoadOptionNumeric(key, _T("bLinkDoubleClick"), (LPBYTE)&options.bLinkDoubleClick, dwBufferSize);
		LoadOptionNumeric(key, _T("bHideScrollbars"), (LPBYTE)&options.bHideScrollbars, dwBufferSize);
		LoadOptionNumeric(key, _T("bSuppressUndoBufferPrompt"), (LPBYTE)&options.bSuppressUndoBufferPrompt, dwBufferSize);
#endif
		LoadOptionNumeric(key, _T("nLaunchSave"), (LPBYTE)&options.nLaunchSave, dwBufferSize);
		LoadOptionNumeric(key, _T("nTabStops"), (LPBYTE)&options.nTabStops, dwBufferSize);
		LoadOptionNumeric(key, _T("nPrimaryFont"), (LPBYTE)&options.nPrimaryFont, dwBufferSize);
		LoadOptionNumeric(key, _T("nSecondaryFont"), (LPBYTE)&options.nSecondaryFont, dwBufferSize);
		dwBufferSize = sizeof(LOGFONT);
#ifdef UNICODE
		LoadOptionBinary(key, _T("PrimaryFontU"), (LPBYTE)&options.PrimaryFont, dwBufferSize);
		LoadOptionBinary(key, _T("SecondaryFontU"), (LPBYTE)&options.SecondaryFont, dwBufferSize);
#else
		LoadOptionBinary(key, _T("PrimaryFont"), (LPBYTE)&options.PrimaryFont, dwBufferSize);
		LoadOptionBinary(key, _T("SecondaryFont"), (LPBYTE)&options.SecondaryFont, dwBufferSize);
#endif
		LoadOptionString(key, _T("szBrowser"), &options.szBrowser, MAXFN);
		LoadOptionString(key, _T("szBrowser2"), &options.szBrowser2, MAXFN);
		LoadOptionString(key, _T("szLangPlugin"), &options.szLangPlugin, MAXFN);
		LoadOptionString(key, _T("szFavDir"), &options.szFavDir, MAXFN);
		LoadOptionString(key, _T("szArgs"), &options.szArgs, MAXARGS);
		LoadOptionString(key, _T("szArgs2"), &options.szArgs2, MAXARGS);
		LoadOptionStringDefault(key, _T("szQuote"), &options.szQuote, MAXQUOTE, _T("> "));
			
		LoadOptionStringDefault(key, _T("szCustomDate"), &options.szCustomDate, MAXDATEFORMAT, _T("yyyyMMdd-HHmmss "));
		LoadOptionString(key, _T("szCustomDate2"), &options.szCustomDate2, MAXDATEFORMAT);

		for (i = 0; i < 10; ++i) {
			wsprintf(keyname, _T("szMacroArray%d"), i);
			LoadOptionString(key, keyname, &options.MacroArray[i], MAXMACRO);
		}

		dwBufferSize = sizeof(COLORREF);
		LoadOptionBinary(key, _T("BackColour"), (LPBYTE)&options.BackColour, dwBufferSize);
		LoadOptionBinary(key, _T("FontColour"), (LPBYTE)&options.FontColour, dwBufferSize);
		LoadOptionBinary(key, _T("BackColour2"), (LPBYTE)&options.BackColour2, dwBufferSize);
		LoadOptionBinary(key, _T("FontColour2"), (LPBYTE)&options.FontColour2, dwBufferSize);

		dwBufferSize = sizeof(RECT);
		LoadOptionBinary(key, _T("rMargins"), (LPBYTE)&options.rMargins, dwBufferSize);

		if (key != NULL) {
			RegCloseKey(key);
		}
	}

#ifndef USE_RICH_EDIT
	if (options.bSystemColours) {
		options.BackColour = GetSysColor(COLOR_WINDOW);
		options.FontColour = GetSysColor(COLOR_WINDOWTEXT);
	}
	if (options.bSystemColours2) {
		options.BackColour2 = GetSysColor(COLOR_WINDOW);
		options.FontColour2 = GetSysColor(COLOR_WINDOWTEXT);
	}
#endif
	if (options.nFormat < (1<<16))
		options.nFormat = ID_ENC_ANSI | (ID_LFMT_DOS << 16);
}

/**
 * Load a string option.
 *
 * @param hKey A handle to an open registry key. If NULL, loads the option from an ini file.
 * @param name Name of the value to get.
 * @param lpData Pointer to the place to store the data.
 * @param cbData The size of lpData, in bytes.
 * @return TRUE if the value was loaded successfully, FALSE otherwise.
 */
void LoadOptionString(HKEY hKey, LPCTSTR name, LPTSTR* lpData, DWORD cbData)
{
	DWORD clen = cbData + 1, buflen = clen * sizeof(TCHAR);
	LPTSTR buf = (LPTSTR)HeapAlloc(globalHeap, 0, buflen);
	if (buf) {
		buf[0] = _T('\0');
		if (hKey) RegQueryValueEx(hKey, name, NULL, NULL, (LPBYTE)buf, &buflen);
		else GetPrivateProfileString(_T("Options"), name, buf, buf, clen, SCNUL(szMetapadIni));
		SSTRCPY(*lpData, buf);
		HeapFree(globalHeap, 0, (HGLOBAL)buf);
	}
}
void LoadOptionStringDefault(HKEY hKey, LPCTSTR name, LPTSTR* lpData, DWORD cbData, LPCTSTR defVal)
{
	LoadOptionString(hKey, name, lpData, cbData);
	if (defVal && defVal[0] && (!*lpData || !*lpData[0])) {
		SSTRCPY(*lpData, defVal);
	}
}

/**
 * Load a numeric option.
 *
 * @param hKey A handle to an open registry key. If NULL, loads the option from an ini file.
 * @param name Name of the value to get.
 * @param lpData Pointer to the place to store the data.
 * @param cbData The size of lpData, in bytes.
 * @return TRUE if the value was loaded successfully, FALSE otherwise.
 */
BOOL LoadOptionNumeric(HKEY hKey, LPCTSTR name, LPBYTE lpData, DWORD cbData)
{
	DWORD clen = cbData;
	if (hKey) {
		return (RegQueryValueEx(hKey, name, NULL, NULL, lpData, &clen) == ERROR_SUCCESS);
	}
	else {
		TCHAR val[10];
		if (GetPrivateProfileString(_T("Options"), name, NULL, val, 10, SCNUL(szMetapadIni)) > 0) {
			long int longInt = _ttoi(val);
			lpData[3] = (int)((longInt >> 24) & 0xFF) ;
			lpData[2] = (int)((longInt >> 16) & 0xFF) ;
			lpData[1] = (int)((longInt >> 8) & 0XFF);
			lpData[0] = (int)((longInt & 0XFF));
			return TRUE;
		}
		else {
			return FALSE;
		}
	}
}

/**
 * Load main window's placement.
 *
 * @param[in] left Pointer to window's left border position.
 * @param[in] top Pointer to window's top border position.
 * @param[in] width Pointer to window's width.
 * @param[in] height Pointer to window's height.
 * @param[in] nShow Pointer to window's showing controls.
 */
void LoadWindowPlacement(int* left, int* top, int* width, int* height, int* nShow)
{
	HKEY key = NULL;
	DWORD dwBufferSize = sizeof(int);
	BOOL bSuccess = TRUE;

	if (!g_bIniMode) {
		if (RegOpenKeyEx(HKEY_CURRENT_USER, STR_REGKEY, 0, KEY_ALL_ACCESS, &key) != ERROR_SUCCESS) {
			bSuccess = FALSE;
		}
	}

	if (bSuccess) {
		bSuccess &= LoadOptionNumeric(key, _T("w_Left"), (LPBYTE)left, dwBufferSize);
		bSuccess &= LoadOptionNumeric(key, _T("w_Top"), (LPBYTE)top, dwBufferSize);
		bSuccess &= LoadOptionNumeric(key, _T("w_Width"), (LPBYTE)width, dwBufferSize);
		bSuccess &= LoadOptionNumeric(key, _T("w_Height"), (LPBYTE)height, dwBufferSize);
		bSuccess &= LoadOptionNumeric(key, _T("w_WindowState"), (LPBYTE)nShow, dwBufferSize);
	}

	if (key != NULL) {
		RegCloseKey(key);
	}

	if (!bSuccess) {
		*left = *top = *width = *height = CW_USEDEFAULT;
		*nShow = SW_SHOWNORMAL;
		options.bStickyWindow = 0;
	}
}

/**
 * Load metapad's menus and data.
 */
void LoadMenusAndData(void)
{
	HKEY key = NULL;
	BOOL bLoad = TRUE;
	TCHAR keyname[20];
	int i;

	if (!g_bIniMode) {
		if (RegOpenKeyEx(HKEY_CURRENT_USER, STR_REGKEY, 0, KEY_ALL_ACCESS, &key) != ERROR_SUCCESS) {
			bLoad = FALSE;
		}
	}

	if (bLoad) {
		if (options.bSaveMenuSettings) {
			LoadOptionNumeric(key, _T("m_WordWrap"), (LPBYTE)&bWordWrap, sizeof(BOOL));
			LoadOptionNumeric(key, _T("m_PrimaryFont"), (LPBYTE)&bPrimaryFont, sizeof(BOOL));
			LoadOptionNumeric(key, _T("m_SmartSelect"), (LPBYTE)&bSmartSelect, sizeof(BOOL));
#ifdef USE_RICH_EDIT
			LoadOptionNumeric(key, _T("m_Hyperlinks"), (LPBYTE)&bHyperlinks, sizeof(BOOL));
#endif
			LoadOptionNumeric(key, _T("m_ShowStatus"), (LPBYTE)&bShowStatus, sizeof(BOOL));
			LoadOptionNumeric(key, _T("m_ShowToolbar"), (LPBYTE)&bShowToolbar, sizeof(BOOL));
			LoadOptionNumeric(key, _T("m_AlwaysOnTop"), (LPBYTE)&bAlwaysOnTop, sizeof(BOOL));
			LoadOptionNumeric(key, _T("m_Transparent"), (LPBYTE)&bTransparent, sizeof(BOOL));
			LoadOptionNumeric(key, _T("bCloseAfterFind"), (LPBYTE)&bCloseAfterFind, sizeof(BOOL));
			LoadOptionNumeric(key, _T("bCloseAfterReplace"), (LPBYTE)&bCloseAfterReplace, sizeof(BOOL));
			LoadOptionNumeric(key, _T("bCloseAfterInsert"), (LPBYTE)&bCloseAfterInsert, sizeof(BOOL));
			LoadOptionNumeric(key, _T("bNoFindHidden"), (LPBYTE)&bNoFindHidden, sizeof(BOOL));
		}
		LoadOptionStringDefault(key, _T("FileFilter"), &szCustomFilter, MAXFN, GetString(IDS_DEFAULT_FILTER));

		if (!options.bNoSaveHistory) {
			for (i = 0; i < NUMFINDS; ++i) {
				wsprintf(keyname, _T("szFindArray%d"), i);
				LoadOptionString(key, keyname, &FindArray[i], MAXFIND);
			}
			for (i = 0; i < NUMFINDS; ++i) {
				wsprintf(keyname, _T("szReplaceArray%d"), i);
				LoadOptionString(key, keyname, &ReplaceArray[i], MAXFIND);
				if (i == 0) LoadOptionString(key, keyname, &szReplaceText, MAXFIND);
			}
			for (i = 0; i < NUMINSERTS; ++i) {
				wsprintf(keyname, _T("szInsertArray%d"), i);
				LoadOptionString(key, keyname, &InsertArray[i], MAXINSERT);
			}
		}

		if (options.bSaveDirectory)
			LoadOptionString(key, _T("szLastDirectory"), &szDir, MAXFN);
	}

	RegCloseKey(key);
}
