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
		if (GetPrivateProfileString(GetString(STR_OPTIONS), name, szBuffer, szBuffer, l, SCNUL(szMetapadIni)) > 0)
			DecodeBase(64, szBuffer, lpData, -1, 0, 1, TRUE, NULL );
		HeapFree(globalHeap, 0, szBuffer);
	}
}

/**
 * Load metapad's options.
 */
void LoadOptions(void) {
	HKEY key = NULL;
	DWORD dwBufferSize;
	TCHAR keyname[20];
	int i;

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
	options.bDigitGrp = TRUE;
	options.bStickyWindow = FALSE;
	options.bReadOnlyMenu = FALSE;
	//options.nStatusFontWidth = 16;
	options.nSelectionMarginWidth = 10;
	options.nMaxMRU = 8;
	options.nFormat = FC_ENC_ANSI | (FC_LFMT_DOS << 16);
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
		if (RegOpenKeyEx(HKEY_CURRENT_USER, GetString(STR_REGKEY), 0, KEY_ALL_ACCESS, &key) != ERROR_SUCCESS) {
			//ReportLastError();
			//return;
			key = NULL;
		}
	}

	dwBufferSize = sizeof(int);
	LoadOptionNumeric(key, GetString(IDSS_HIDEGOTOOFFSET), (LPBYTE)&options.bHideGotoOffset, dwBufferSize);
	LoadOptionNumeric(key, GetString(IDSS_SYSTEMCOLOURS), (LPBYTE)&options.bSystemColours, dwBufferSize);
	LoadOptionNumeric(key, GetString(IDSS_SYSTEMCOLOURS2), (LPBYTE)&options.bSystemColours2, dwBufferSize);
	LoadOptionNumeric(key, GetString(IDSS_NOSMARTHOME), (LPBYTE)&options.bNoSmartHome, dwBufferSize);
	LoadOptionNumeric(key, GetString(IDSS_NOAUTOSAVEEXT), (LPBYTE)&options.bNoAutoSaveExt, dwBufferSize);
	LoadOptionNumeric(key, GetString(IDSS_CONTEXTCURSOR), (LPBYTE)&options.bContextCursor, dwBufferSize);
	LoadOptionNumeric(key, GetString(IDSS_CURRENTFINDFONT), (LPBYTE)&options.bCurrentFindFont, dwBufferSize);
	LoadOptionNumeric(key, GetString(IDSS_PRINTWITHSECONDARYFONT), (LPBYTE)&options.bPrintWithSecondaryFont, dwBufferSize);
	LoadOptionNumeric(key, GetString(IDSS_NOSAVEHISTORY), (LPBYTE)&options.bNoSaveHistory, dwBufferSize);
	LoadOptionNumeric(key, GetString(IDSS_NOFINDAUTOSELECT), (LPBYTE)&options.bNoFindAutoSelect, dwBufferSize);
	LoadOptionNumeric(key, GetString(IDSS_RECENTONOWN), (LPBYTE)&options.bRecentOnOwn, dwBufferSize);
	LoadOptionNumeric(key, GetString(IDSS_DONTINSERTTIME), (LPBYTE)&options.bDontInsertTime, dwBufferSize);
	LoadOptionNumeric(key, GetString(IDSS_NOWARNINGPROMPT), (LPBYTE)&options.bNoWarningPrompt, dwBufferSize);
	LoadOptionNumeric(key, GetString(IDSS_UNFLATTOOLBAR), (LPBYTE)&options.bUnFlatToolbar, dwBufferSize);
	LoadOptionNumeric(key, GetString(IDSS_DIGITGRP), (LPBYTE)&options.bDigitGrp, dwBufferSize);
	LoadOptionNumeric(key, GetString(IDSS_STICKYWINDOW), (LPBYTE)&options.bStickyWindow, dwBufferSize);
	LoadOptionNumeric(key, GetString(IDSS_READONLYMENU), (LPBYTE)&options.bReadOnlyMenu, dwBufferSize);
	LoadOptionNumeric(key, GetString(IDSS_SELECTIONMARGINWIDTH), (LPBYTE)&options.nSelectionMarginWidth, dwBufferSize);
	LoadOptionNumeric(key, GetString(IDSS_MAXMRU), (LPBYTE)&options.nMaxMRU, dwBufferSize);
	LoadOptionNumeric(key, GetString(IDSS_FORMAT), (LPBYTE)&options.nFormat, sizeof(DWORD));
	LoadOptionNumeric(key, GetString(IDSS_TRANSPARENTPCT), (LPBYTE)&options.nTransparentPct, dwBufferSize);
	LoadOptionNumeric(key, GetString(IDSS_NOCAPTIONDIR), (LPBYTE)&options.bNoCaptionDir, dwBufferSize);
	LoadOptionNumeric(key, GetString(IDSS_AUTOINDENT), (LPBYTE)&options.bAutoIndent, dwBufferSize);
	LoadOptionNumeric(key, GetString(IDSS_INSERTSPACES), (LPBYTE)&options.bInsertSpaces, dwBufferSize);
	LoadOptionNumeric(key, GetString(IDSS_FINDAUTOWRAP), (LPBYTE)&options.bFindAutoWrap, dwBufferSize);
	LoadOptionNumeric(key, GetString(IDSS_QUICKEXIT), (LPBYTE)&options.bQuickExit, dwBufferSize);
	LoadOptionNumeric(key, GetString(IDSS_SAVEWINDOWPLACEMENT), (LPBYTE)&options.bSaveWindowPlacement, dwBufferSize);
	LoadOptionNumeric(key, GetString(IDSS_SAVEMENUSETTINGS), (LPBYTE)&options.bSaveMenuSettings, dwBufferSize);
	LoadOptionNumeric(key, GetString(IDSS_SAVEDIRECTORY), (LPBYTE)&options.bSaveDirectory, dwBufferSize);
	LoadOptionNumeric(key, GetString(IDSS_LAUNCHCLOSE), (LPBYTE)&options.bLaunchClose, dwBufferSize);
	LoadOptionNumeric(key, GetString(IDSS_NOFAVES), (LPBYTE)&options.bNoFaves, dwBufferSize);
#ifndef USE_RICH_EDIT
	LoadOptionNumeric(key, GetString(IDSS_DEFAULTPRINTFONT), (LPBYTE)&options.bDefaultPrintFont, dwBufferSize);
	LoadOptionNumeric(key, GetString(IDSS_ALWAYSLAUNCH), (LPBYTE)&options.bAlwaysLaunch, dwBufferSize);
#else
	LoadOptionNumeric(key, GetString(IDSS_LINKDOUBLECLICK), (LPBYTE)&options.bLinkDoubleClick, dwBufferSize);
	LoadOptionNumeric(key, GetString(IDSS_HIDESCROLLBARS), (LPBYTE)&options.bHideScrollbars, dwBufferSize);
	LoadOptionNumeric(key, GetString(IDSS_SUPPRESSUNDOBUFFERPROMPT), (LPBYTE)&options.bSuppressUndoBufferPrompt, dwBufferSize);
#endif
	LoadOptionNumeric(key, GetString(IDSS_LAUNCHSAVE), (LPBYTE)&options.nLaunchSave, dwBufferSize);
	LoadOptionNumeric(key, GetString(IDSS_TABSTOPS), (LPBYTE)&options.nTabStops, dwBufferSize);
	LoadOptionNumeric(key, GetString(IDSS_NPRIMARYFONT), (LPBYTE)&options.nPrimaryFont, dwBufferSize);
	LoadOptionNumeric(key, GetString(IDSS_NSECONDARYFONT), (LPBYTE)&options.nSecondaryFont, dwBufferSize);
	dwBufferSize = sizeof(LOGFONT);
#ifdef UNICODE
	LoadOptionBinary(key, GetString(IDSS_PRIMARYFONT), (LPBYTE)&options.PrimaryFont, dwBufferSize);
	LoadOptionBinary(key, GetString(IDSS_SECONDARYFONT), (LPBYTE)&options.SecondaryFont, dwBufferSize);
#else
	LoadOptionBinary(key, GetString(IDSS_PRIMARYFONT), (LPBYTE)&options.PrimaryFont, dwBufferSize);
	LoadOptionBinary(key, GetString(IDSS_SECONDARYFONT), (LPBYTE)&options.SecondaryFont, dwBufferSize);
#endif
	LoadOptionString(key, GetString(IDSS_BROWSER), &options.szBrowser, MAXFN);
	LoadOptionString(key, GetString(IDSS_BROWSER2), &options.szBrowser2, MAXFN);
	LoadOptionString(key, GetString(IDSS_LANGPLUGIN), &options.szLangPlugin, MAXFN);
	LoadOptionString(key, GetString(IDSS_FAVDIR), &options.szFavDir, MAXFN);
	LoadOptionString(key, GetString(IDSS_ARGS), &options.szArgs, MAXARGS);
	LoadOptionString(key, GetString(IDSS_ARGS2), &options.szArgs2, MAXARGS);
	LoadOptionStringDefault(key, GetString(IDSS_QUOTE), &options.szQuote, MAXQUOTE, GetString(IDSD_QUOTE));

	LoadOptionStringDefault(key, GetString(IDSS_CUSTOMDATE), &options.szCustomDate, MAXDATEFORMAT, GetString(IDSD_CUSTOMDATE));
	LoadOptionString(key, GetString(IDSS_CUSTOMDATE2), &options.szCustomDate2, MAXDATEFORMAT);

	for (i = 0; i < 10; ++i) {
		wsprintf(keyname, GetString(IDSS_MACROARRAY), i);
		LoadOptionString(key, keyname, &options.MacroArray[i], MAXMACRO);
	}

	dwBufferSize = sizeof(COLORREF);
	LoadOptionBinary(key, GetString(IDSS_BACKCOLOUR), (LPBYTE)&options.BackColour, dwBufferSize);
	LoadOptionBinary(key, GetString(IDSS_FONTCOLOUR), (LPBYTE)&options.FontColour, dwBufferSize);
	LoadOptionBinary(key, GetString(IDSS_BACKCOLOUR2), (LPBYTE)&options.BackColour2, dwBufferSize);
	LoadOptionBinary(key, GetString(IDSS_FONTCOLOUR2), (LPBYTE)&options.FontColour2, dwBufferSize);

	dwBufferSize = sizeof(RECT);
	LoadOptionBinary(key, GetString(IDSS_MARGINS), (LPBYTE)&options.rMargins, dwBufferSize);

	if (key) RegCloseKey(key);

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
		options.nFormat = FC_ENC_ANSI | (FC_LFMT_DOS << 16);
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
void LoadOptionString(HKEY hKey, LPCTSTR name, LPTSTR* lpData, DWORD cbData) {
	DWORD clen = cbData + 1, buflen = clen * sizeof(TCHAR);
	LPTSTR buf = (LPTSTR)HeapAlloc(globalHeap, 0, buflen);
	if (buf) {
		buf[0] = _T('\0');
		if (hKey) {
			if (RegQueryValueEx(hKey, name, NULL, NULL, (LPBYTE)buf, &buflen)) { FREE(buf); }
		} else GetPrivateProfileString(GetString(STR_OPTIONS), name, buf, buf, clen, SCNUL(szMetapadIni));
		SSTRCPYA(*lpData, buf, 2);
		FREE(buf);
	}
}
void LoadOptionStringDefault(HKEY hKey, LPCTSTR name, LPTSTR* lpData, DWORD cbData, LPCTSTR defVal) {
	LoadOptionString(hKey, name, lpData, cbData);
	if (defVal && defVal[0] && (!*lpData || !*lpData[0])) {
		SSTRCPYA(*lpData, defVal, 2);
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
BOOL LoadOptionNumeric(HKEY hKey, LPCTSTR name, LPBYTE lpData, DWORD cbData) {
	DWORD clen = cbData;
	TCHAR val[16];
	if (hKey)
		return (RegQueryValueEx(hKey, name, NULL, NULL, lpData, &clen) == ERROR_SUCCESS);
	else if (GetPrivateProfileString(GetString(STR_OPTIONS), name, NULL, val, 16, SCNUL(szMetapadIni)) > 0) {
		*((DWORD*)lpData) = (DWORD)_ttol(val);
		return TRUE;
	} else
		return FALSE;
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
		if (RegOpenKeyEx(HKEY_CURRENT_USER, GetString(STR_REGKEY), 0, KEY_ALL_ACCESS, &key) != ERROR_SUCCESS) {
			bSuccess = FALSE;
		}
	}

	if (bSuccess) {
		bSuccess &= LoadOptionNumeric(key, GetString(IDSS_WLEFT), (LPBYTE)left, dwBufferSize);
		bSuccess &= LoadOptionNumeric(key, GetString(IDSS_WTOP), (LPBYTE)top, dwBufferSize);
		bSuccess &= LoadOptionNumeric(key, GetString(IDSS_WWIDTH), (LPBYTE)width, dwBufferSize);
		bSuccess &= LoadOptionNumeric(key, GetString(IDSS_WHEIGHT), (LPBYTE)height, dwBufferSize);
		bSuccess &= LoadOptionNumeric(key, GetString(IDSS_WSTATE), (LPBYTE)nShow, dwBufferSize);
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
void LoadMenusAndData(void) {
	HKEY key = NULL;
	TCHAR keyname[20];
	int i;

	if (!g_bIniMode) {
		if (RegOpenKeyEx(HKEY_CURRENT_USER, GetString(STR_REGKEY), 0, KEY_ALL_ACCESS, &key) != ERROR_SUCCESS) {
			//bLoad = FALSE;
			key = NULL;
		}
	}

	if (options.bSaveMenuSettings) {
		LoadOptionNumeric(key, GetString(IDSS_WORDWRAP), (LPBYTE)&bWordWrap, sizeof(BOOL));
		LoadOptionNumeric(key, GetString(IDSS_FONTIDX), (LPBYTE)&bPrimaryFont, sizeof(BOOL));
		LoadOptionNumeric(key, GetString(IDSS_SMARTSELECT), (LPBYTE)&bSmartSelect, sizeof(BOOL));
#ifdef USE_RICH_EDIT
		LoadOptionNumeric(key, GetString(IDSS_HYPERLINKS), (LPBYTE)&bHyperlinks, sizeof(BOOL));
#endif
		LoadOptionNumeric(key, GetString(IDSS_SHOWSTATUS), (LPBYTE)&bShowStatus, sizeof(BOOL));
		LoadOptionNumeric(key, GetString(IDSS_SHOWTOOLBAR), (LPBYTE)&bShowToolbar, sizeof(BOOL));
		LoadOptionNumeric(key, GetString(IDSS_ALWAYSONTOP), (LPBYTE)&bAlwaysOnTop, sizeof(BOOL));
		LoadOptionNumeric(key, GetString(IDSS_TRANSPARENT), (LPBYTE)&bTransparent, sizeof(BOOL));
		LoadOptionNumeric(key, GetString(IDSS_CLOSEAFTERFIND), (LPBYTE)&bCloseAfterFind, sizeof(BOOL));
		LoadOptionNumeric(key, GetString(IDSS_CLOSEAFTERREPLACE), (LPBYTE)&bCloseAfterReplace, sizeof(BOOL));
		LoadOptionNumeric(key, GetString(IDSS_CLOSEAFTERINSERT), (LPBYTE)&bCloseAfterInsert, sizeof(BOOL));
		LoadOptionNumeric(key, GetString(IDSS_NOFINDHIDDEN), (LPBYTE)&bNoFindHidden, sizeof(BOOL));
	}
	LoadOptionStringDefault(key, GetString(IDSS_FILEFILTER), &szCustomFilter, MAXMACRO, GetString(IDS_DEFAULT_FILTER));

	if (!options.bNoSaveHistory) {
		for (i = 0; i < NUMFINDS; ++i) {
			wsprintf(keyname, GetString(IDSS_FINDARRAY), i);
			LoadOptionString(key, keyname, &FindArray[i], MAXFIND);
			wsprintf(keyname, GetString(IDSS_REPLACEARRAY), i);
			LoadOptionString(key, keyname, &ReplaceArray[i], MAXFIND);
		}
		for (i = 0; i < NUMINSERTS; ++i) {
			wsprintf(keyname, GetString(IDSS_INSERTARRAY), i);
			LoadOptionString(key, keyname, &InsertArray[i], MAXINSERT);
		}
	}

	if (options.bSaveDirectory)
		LoadOptionString(key, GetString(IDSS_LASTDIRECTORY), &szDir, MAXFN);

	if (key) RegCloseKey(key);
}
