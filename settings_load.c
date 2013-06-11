/****************************************************************************/
/*                                                                          */
/*   metapad 3.6                                                            */
/*                                                                          */
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
#ifdef BUILD_METAPAD_UNICODE
#define _UNICODE
#include <wchar.h>
#else
#undef _UNICODE
#undef UNICODE
#endif

#include "include/consts.h"
#include "include/strings.h"
#include "include/typedefs.h"
#include "include/cdecode.h"
#include "include/settings_load.h"

#ifdef USE_RICH_EDIT
extern BOOL bHyperlinks;
#endif

extern int atoi(const char*);

extern TCHAR szCustomFilter[2*MAXSTRING];
extern TCHAR szDir[MAXFN];
extern TCHAR szReplaceText[MAXFIND];
extern TCHAR szMetapadIni[MAXFN];
extern TCHAR FindArray[NUMFINDS][MAXFIND];
extern TCHAR ReplaceArray[NUMFINDS][MAXFIND];
extern BOOL g_bIniMode;
extern BOOL bWordWrap;
extern BOOL bPrimaryFont;
extern BOOL bSmartSelect;
extern BOOL bShowStatus;
extern BOOL bShowToolbar;
extern BOOL bAlwaysOnTop;
extern BOOL bTransparent;
extern BOOL bCloseAfterFind;
extern BOOL bNoFindHidden;
extern option_struct options;

/**
 * Load a binary option.
 *
 * @param hKey A handle to an open registry key. If NULL, loads the option from an ini file.
 * @param name Name of the value to get.
 * @param lpData Pointer to the place to store the data.
 * @param cbData The size of lpData, in bytes.
 * @return TRUE if the value was loaded successfully, FALSE otherwise.
 */
static void LoadOptionBinary(HKEY hKey, LPCSTR name, BYTE* lpData, DWORD cbData)
{
	if (hKey) {
		RegQueryValueEx(hKey, name, NULL, NULL, lpData, &cbData);
	}
	else {
		base64_decodestate state;
		char *szBuffer = (LPTSTR)GlobalAlloc(GPTR, cbData * sizeof(TCHAR) * 2);

		if (GetPrivateProfileString("Options", name, (char*)lpData, szBuffer, cbData * 2, szMetapadIni) > 0) {
			int i;
			for (i = 0; i < lstrlen(szBuffer); ++i) {
				if (szBuffer[i] == '-') {
					szBuffer[i] = '=';
				}
			}
			base64_init_decodestate(&state);
			base64_decode_block(szBuffer, lstrlen(szBuffer), (char*)lpData, &state);
		}
	}
}

/**
 * Load a bounded option string.
 *
 * @param hKey A handle to an open registry key. If NULL, loads the option from an ini file.
 * @param name Name of the value to get.
 * @param lpData Pointer to the place to store the data.
 * @param cbData The size of lpData, in bytes.
 * @return TRUE if the value was loaded successfully, FALSE otherwise.
 */
static void LoadBoundedOptionString(HKEY hKey, LPCSTR name, BYTE* lpData, DWORD cbData)
{
	if (hKey) {
		RegQueryValueEx(hKey, name, NULL, NULL, lpData, &cbData);
	}
	else {
		char *bounded = (LPTSTR)GlobalAlloc(GPTR, cbData + 2);
		GetPrivateProfileString("Options", name, bounded, bounded, cbData + 2, szMetapadIni);
		if (lstrlen(bounded) >= 2 && bounded[0] == '[' && bounded[lstrlen(bounded)-1] == ']') {
			strncpy((char*)lpData, bounded+1, lstrlen(bounded) - 2);
		}
		else {
			strncpy((char*)lpData, bounded, lstrlen(bounded));
		}
		GlobalFree(bounded);
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

	lstrcpy(options.szQuote, "> ");
	ZeroMemory(options.szArgs, sizeof(options.szArgs));
	ZeroMemory(options.szBrowser, sizeof(options.szBrowser));
	ZeroMemory(options.szArgs2, sizeof(options.szArgs2));
	ZeroMemory(options.szBrowser2, sizeof(options.szBrowser2));
	ZeroMemory(options.szLangPlugin, sizeof(options.szLangPlugin));
	ZeroMemory(options.szFavDir, sizeof(options.szFavDir));
	ZeroMemory(&options.PrimaryFont, sizeof(LOGFONT));
	ZeroMemory(&options.SecondaryFont, sizeof(LOGFONT));
	ZeroMemory(&options.MacroArray, sizeof(options.MacroArray));
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
	options.nFormatIndex = 0;
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
		LoadOptionNumeric(key, _T("nFormatIndex"), (LPBYTE)&options.nFormatIndex, dwBufferSize);
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
#ifdef BUILD_METAPAD_UNICODE
		LoadOptionBinary(key, _T("PrimaryFont_U"), (LPBYTE)&options.PrimaryFont, dwBufferSize);
		LoadOptionBinary(key, _T("SecondaryFont_U"), (LPBYTE)&options.SecondaryFont, dwBufferSize);
#else
		LoadOptionBinary(key, _T("PrimaryFont"), (LPBYTE)&options.PrimaryFont, dwBufferSize);
		LoadOptionBinary(key, _T("SecondaryFont"), (LPBYTE)&options.SecondaryFont, dwBufferSize);
#endif
		dwBufferSize = sizeof(options.szBrowser);
		LoadOptionString(key, _T("szBrowser"), (LPBYTE)&options.szBrowser, dwBufferSize);
		dwBufferSize = sizeof(options.szBrowser2);
		LoadOptionString(key, _T("szBrowser2"), (LPBYTE)&options.szBrowser2, dwBufferSize);
		dwBufferSize = sizeof(options.szLangPlugin);
		LoadOptionString(key, _T("szLangPlugin"), (LPBYTE)&options.szLangPlugin, dwBufferSize);
		dwBufferSize = sizeof(options.szFavDir);
		LoadOptionString(key, _T("szFavDir"), (LPBYTE)&options.szFavDir, dwBufferSize);
		dwBufferSize = sizeof(options.szArgs);
		LoadOptionString(key, _T("szArgs"), (LPBYTE)&options.szArgs, dwBufferSize);
		dwBufferSize = sizeof(options.szArgs2);
		LoadOptionString(key, _T("szArgs2"), (LPBYTE)&options.szArgs2, dwBufferSize);
		dwBufferSize = sizeof(options.szQuote);
		LoadOptionBinary(key, _T("szQuote"), (LPBYTE)&options.szQuote, dwBufferSize);

		if (key != NULL) {
			dwBufferSize = sizeof(options.MacroArray);
			LoadOptionBinary(key, _T("MacroArray"), (LPBYTE)&options.MacroArray, dwBufferSize);
		}
		else {
			char keyname[14];
			int i;

			for (i = 0; i < 10; ++i) {
				wsprintf(keyname, "szMacroArray%d", i);
				LoadBoundedOptionString(key, keyname, (LPBYTE)&options.MacroArray[i], MAXMACRO);
			}
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
void LoadOptionString(HKEY hKey, LPCSTR name, BYTE* lpData, DWORD cbData)
{
	if (hKey) {
		RegQueryValueEx(hKey, name, NULL, NULL, lpData, &cbData);
	}
	else {
		GetPrivateProfileString("Options", name, (char*)lpData, (char*)lpData, cbData, szMetapadIni);
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
BOOL LoadOptionNumeric(HKEY hKey, LPCSTR name, BYTE* lpData, DWORD cbData)
{
	if (hKey) {
		return (RegQueryValueEx(hKey, name, NULL, NULL, lpData, &cbData) == ERROR_SUCCESS);
	}
	else {
		char val[10];
		if (GetPrivateProfileString("Options", name, NULL, val, 10, szMetapadIni) > 0) {
			long int longInt = atoi(val);
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
			LoadOptionNumeric(key, _T("bNoFindHidden"), (LPBYTE)&bNoFindHidden, sizeof(BOOL));
		}
		LoadOptionString(key, _T("FileFilter"), (LPBYTE)&szCustomFilter, sizeof(szCustomFilter));

		if (!options.bNoSaveHistory) {
#ifdef BUILD_METAPAD_UNICODE
			DWORD dwBufferSize = sizeof(TCHAR) * NUMFINDS * MAXFIND;
			RegQueryValueEx(key, _T("FindArray_U"), NULL, NULL, (LPBYTE)&FindArray, &dwBufferSize);
			RegQueryValueEx(key, _T("ReplaceArray_U"), NULL, NULL, (LPBYTE)&ReplaceArray, &dwBufferSize);
			ASSERT(FALSE);
#else
			if (key) {
				LoadOptionString(key, _T("FindArray"), (LPBYTE)&FindArray, sizeof(FindArray));
				LoadOptionString(key, _T("ReplaceArray"), (LPBYTE)&ReplaceArray, sizeof(ReplaceArray));
			}
			else {
				char keyname[16];
				int i;
				for (i = 0; i < 10; ++i) {
					wsprintf(keyname, "szFindArray%d", i);
					LoadBoundedOptionString(key, keyname, (LPBYTE)&FindArray[i], MAXFIND);
				}
				for (i = 0; i < 10; ++i) {
					wsprintf(keyname, "szReplaceArray%d", i);
					LoadBoundedOptionString(key, keyname, (LPBYTE)&ReplaceArray[i], MAXFIND);
				}
			}
#endif
		}

		if (options.bSaveDirectory) {
			LoadOptionString(key, _T("szLastDirectory"), (LPBYTE)&szDir, sizeof(szDir));
		}
	}

	lstrcpy(szReplaceText, ReplaceArray[0]);

	RegCloseKey(key);
}
