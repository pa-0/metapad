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
 * @file settings_save.c
 * @brief Settings saving functions.
 */

#include "include/metapad.h"
#ifdef UNICODE
#include <wchar.h>
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
				writeSucceeded = WritePrivateProfileString(GetString(STR_OPTIONS), name, val, SCNUL(szMetapadIni));
				break;
			}
			case REG_SZ:
				writeSucceeded = WritePrivateProfileString(GetString(STR_OPTIONS), name, (LPTSTR)szData, SCNUL(szMetapadIni));
				break;
			case REG_BINARY: {
				TCHAR *szBuffer = kallocs(2 * (cbData + 1));
				EncodeBase(64, lpData, szBuffer, cbData, NULL );
				writeSucceeded = WritePrivateProfileString(GetString(STR_OPTIONS), name, (TCHAR*)szBuffer, SCNUL(szMetapadIni));
				kfree(&szBuffer);
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
		if (RegCreateKeyEx(HKEY_CURRENT_USER, GetString(STR_REGKEY), 0, NULL, 0, KEY_ALL_ACCESS, NULL, &key, NULL) != ERROR_SUCCESS) {
			ReportLastError();
			return;
		}
	}

	writeSucceeded &= SaveOption(key, GetString(IDSS_HIDEGOTOOFFSET), REG_DWORD, (LPBYTE)&options.bHideGotoOffset, sizeof(BOOL));
	writeSucceeded &= SaveOption(key, GetString(IDSS_SYSTEMCOLOURS), REG_DWORD, (LPBYTE)&options.bSystemColours, sizeof(BOOL));
	writeSucceeded &= SaveOption(key, GetString(IDSS_SYSTEMCOLOURS2), REG_DWORD, (LPBYTE)&options.bSystemColours2, sizeof(BOOL));
	writeSucceeded &= SaveOption(key, GetString(IDSS_NOSMARTHOME), REG_DWORD, (LPBYTE)&options.bNoSmartHome, sizeof(BOOL));
	writeSucceeded &= SaveOption(key, GetString(IDSS_NOAUTOSAVEEXT), REG_DWORD, (LPBYTE)&options.bNoAutoSaveExt, sizeof(BOOL));
	writeSucceeded &= SaveOption(key, GetString(IDSS_CONTEXTCURSOR), REG_DWORD, (LPBYTE)&options.bContextCursor, sizeof(BOOL));
	writeSucceeded &= SaveOption(key, GetString(IDSS_CURRENTFINDFONT), REG_DWORD, (LPBYTE)&options.bCurrentFindFont, sizeof(BOOL));
	writeSucceeded &= SaveOption(key, GetString(IDSS_PRINTWITHSECONDARYFONT), REG_DWORD, (LPBYTE)&options.bPrintWithSecondaryFont, sizeof(BOOL));
	writeSucceeded &= SaveOption(key, GetString(IDSS_NOSAVEHISTORY), REG_DWORD, (LPBYTE)&options.bNoSaveHistory, sizeof(BOOL));
	writeSucceeded &= SaveOption(key, GetString(IDSS_NOFINDAUTOSELECT), REG_DWORD, (LPBYTE)&options.bNoFindAutoSelect, sizeof(BOOL));
	writeSucceeded &= SaveOption(key, GetString(IDSS_RECENTONOWN), REG_DWORD, (LPBYTE)&options.bRecentOnOwn, sizeof(BOOL));
	writeSucceeded &= SaveOption(key, GetString(IDSS_DONTINSERTTIME), REG_DWORD, (LPBYTE)&options.bDontInsertTime, sizeof(BOOL));
	writeSucceeded &= SaveOption(key, GetString(IDSS_NOWARNINGPROMPT), REG_DWORD, (LPBYTE)&options.bNoWarningPrompt, sizeof(BOOL));
	writeSucceeded &= SaveOption(key, GetString(IDSS_UNFLATTOOLBAR), REG_DWORD, (LPBYTE)&options.bUnFlatToolbar, sizeof(BOOL));
	writeSucceeded &= SaveOption(key, GetString(IDSS_DIGITGRP), REG_DWORD, (LPBYTE)&options.bDigitGrp, sizeof(BOOL));
	writeSucceeded &= SaveOption(key, GetString(IDSS_STICKYWINDOW), REG_DWORD, (LPBYTE)&options.bStickyWindow, sizeof(BOOL));
	writeSucceeded &= SaveOption(key, GetString(IDSS_READONLYMENU), REG_DWORD, (LPBYTE)&options.bReadOnlyMenu, sizeof(BOOL));
	writeSucceeded &= SaveOption(key, GetString(IDSS_SELECTIONMARGINWIDTH), REG_DWORD, (LPBYTE)&options.nSelectionMarginWidth, sizeof(int));
	writeSucceeded &= SaveOption(key, GetString(IDSS_MAXMRU), REG_DWORD, (LPBYTE)&options.nMaxMRU, sizeof(int));
	writeSucceeded &= SaveOption(key, GetString(IDSS_FORMAT), REG_DWORD, (LPBYTE)&options.nFormat, sizeof(DWORD));
	writeSucceeded &= SaveOption(key, GetString(IDSS_TRANSPARENTPCT), REG_DWORD, (LPBYTE)&options.nTransparentPct, sizeof(int));
	writeSucceeded &= SaveOption(key, GetString(IDSS_NOCAPTIONDIR), REG_DWORD, (LPBYTE)&options.bNoCaptionDir, sizeof(BOOL));
	writeSucceeded &= SaveOption(key, GetString(IDSS_AUTOINDENT), REG_DWORD, (LPBYTE)&options.bAutoIndent, sizeof(BOOL));
	writeSucceeded &= SaveOption(key, GetString(IDSS_INSERTSPACES), REG_DWORD, (LPBYTE)&options.bInsertSpaces, sizeof(BOOL));
 	writeSucceeded &= SaveOption(key, GetString(IDSS_FINDAUTOWRAP), REG_DWORD, (LPBYTE)&options.bFindAutoWrap, sizeof(BOOL));
	writeSucceeded &= SaveOption(key, GetString(IDSS_QUICKEXIT), REG_DWORD, (LPBYTE)&options.bQuickExit, sizeof(BOOL));
	writeSucceeded &= SaveOption(key, GetString(IDSS_SAVEWINDOWPLACEMENT), REG_DWORD, (LPBYTE)&options.bSaveWindowPlacement, sizeof(BOOL));
	writeSucceeded &= SaveOption(key, GetString(IDSS_SAVEMENUSETTINGS), REG_DWORD, (LPBYTE)&options.bSaveMenuSettings, sizeof(BOOL));
	writeSucceeded &= SaveOption(key, GetString(IDSS_SAVEDIRECTORY), REG_DWORD, (LPBYTE)&options.bSaveDirectory, sizeof(BOOL));
	writeSucceeded &= SaveOption(key, GetString(IDSS_LAUNCHCLOSE), REG_DWORD, (LPBYTE)&options.bLaunchClose, sizeof(BOOL));
	writeSucceeded &= SaveOption(key, GetString(IDSS_NOFAVES), REG_DWORD, (LPBYTE)&options.bNoFaves, sizeof(BOOL));
#ifndef USE_RICH_EDIT
	writeSucceeded &= SaveOption(key, GetString(IDSS_DEFAULTPRINTFONT), REG_DWORD, (LPBYTE)&options.bDefaultPrintFont, sizeof(BOOL));
	writeSucceeded &= SaveOption(key, GetString(IDSS_ALWAYSLAUNCH), REG_DWORD, (LPBYTE)&options.bAlwaysLaunch, sizeof(BOOL));
#else
	writeSucceeded &= SaveOption(key, GetString(IDSS_LINKDOUBLECLICK), REG_DWORD, (LPBYTE)&options.bLinkDoubleClick, sizeof(BOOL));
	writeSucceeded &= SaveOption(key, GetString(IDSS_HIDESCROLLBARS), REG_DWORD, (LPBYTE)&options.bHideScrollbars, sizeof(BOOL));
	writeSucceeded &= SaveOption(key, GetString(IDSS_SUPPRESSUNDOBUFFERPROMPT), REG_DWORD, (LPBYTE)&options.bSuppressUndoBufferPrompt, sizeof(BOOL));
#endif
	writeSucceeded &= SaveOption(key, GetString(IDSS_LAUNCHSAVE), REG_DWORD, (LPBYTE)&options.nLaunchSave, sizeof(int));
	writeSucceeded &= SaveOption(key, GetString(IDSS_TABSTOPS), REG_DWORD, (LPBYTE)&options.nTabStops, sizeof(int));
	writeSucceeded &= SaveOption(key, GetString(IDSS_NPRIMARYFONT), REG_DWORD, (LPBYTE)&options.nPrimaryFont, sizeof(int));
	writeSucceeded &= SaveOption(key, GetString(IDSS_NSECONDARYFONT), REG_DWORD, (LPBYTE)&options.nSecondaryFont, sizeof(int));
	writeSucceeded &= SaveOption(key, GetString(IDSS_PRIMARYFONT), REG_BINARY, (LPBYTE)&options.PrimaryFont, sizeof(LOGFONT));
	writeSucceeded &= SaveOption(key, GetString(IDSS_SECONDARYFONT), REG_BINARY, (LPBYTE)&options.SecondaryFont, sizeof(LOGFONT));
	writeSucceeded &= SaveOption(key, GetString(IDSS_BROWSER), REG_SZ, (LPBYTE)options.szBrowser, MAXFN);
	writeSucceeded &= SaveOption(key, GetString(IDSS_BROWSER2), REG_SZ, (LPBYTE)options.szBrowser2, MAXFN);
	writeSucceeded &= SaveOption(key, GetString(IDSS_LANGPLUGIN), REG_SZ, (LPBYTE)options.szLangPlugin, MAXFN);
	writeSucceeded &= SaveOption(key, GetString(IDSS_FAVDIR), REG_SZ, (LPBYTE)options.szFavDir, MAXFN);
	writeSucceeded &= SaveOption(key, GetString(IDSS_ARGS), REG_SZ, (LPBYTE)options.szArgs, MAXARGS);
	writeSucceeded &= SaveOption(key, GetString(IDSS_ARGS2), REG_SZ, (LPBYTE)options.szArgs2, MAXARGS);
	writeSucceeded &= SaveOption(key, GetString(IDSS_QUOTE), REG_SZ, (LPBYTE)options.szQuote, MAXQUOTE);
	writeSucceeded &= SaveOption(key, GetString(IDSS_CUSTOMDATE), REG_SZ, (LPBYTE)options.szCustomDate, MAXDATEFORMAT);
	writeSucceeded &= SaveOption(key, GetString(IDSS_CUSTOMDATE2), REG_SZ, (LPBYTE)options.szCustomDate2, MAXDATEFORMAT);
	for (i = 0; i < 10; ++i) {
		wsprintf(keyname, GetString(IDSS_MACROARRAY), i);
		writeSucceeded &= SaveOption(key, keyname, REG_SZ, (LPBYTE)options.MacroArray[i], MAXMACRO);
	}

	writeSucceeded &= SaveOption(key, GetString(IDSS_BACKCOLOUR), REG_BINARY, (LPBYTE)&options.BackColour, sizeof(COLORREF));
	writeSucceeded &= SaveOption(key, GetString(IDSS_FONTCOLOUR), REG_BINARY, (LPBYTE)&options.FontColour, sizeof(COLORREF));
	writeSucceeded &= SaveOption(key, GetString(IDSS_BACKCOLOUR2), REG_BINARY, (LPBYTE)&options.BackColour2, sizeof(COLORREF));
	writeSucceeded &= SaveOption(key, GetString(IDSS_FONTCOLOUR2), REG_BINARY, (LPBYTE)&options.FontColour2, sizeof(COLORREF));
	writeSucceeded &= SaveOption(key, GetString(IDSS_MARGINS), REG_BINARY, (LPBYTE)&options.rMargins, sizeof(RECT));

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
		if (RegCreateKeyEx(HKEY_CURRENT_USER, GetString(STR_REGKEY), 0, NULL, 0, KEY_ALL_ACCESS, NULL, &key, NULL) != ERROR_SUCCESS) {
			ReportLastError();
			return;
		}
	}

	writeSucceeded &= SaveOption(key, GetString(IDSS_WSTATE), REG_DWORD, (LPBYTE)&max, sizeof(int));
	writeSucceeded &= SaveOption(key, GetString(IDSS_WLEFT), REG_DWORD, (LPBYTE)&left, sizeof(int));
	writeSucceeded &= SaveOption(key, GetString(IDSS_WTOP), REG_DWORD, (LPBYTE)&top, sizeof(int));
	writeSucceeded &= SaveOption(key, GetString(IDSS_WWIDTH), REG_DWORD, (LPBYTE)&width, sizeof(int));
	writeSucceeded &= SaveOption(key, GetString(IDSS_WHEIGHT), REG_DWORD, (LPBYTE)&height, sizeof(int));

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
		if (RegCreateKeyEx(HKEY_CURRENT_USER, GetString(STR_REGKEY), 0, NULL, 0, KEY_ALL_ACCESS, NULL, &key, NULL) != ERROR_SUCCESS) {
			ReportLastError();
			return;
		}
	}
	if (options.bSaveMenuSettings) {
		SaveOption(key, GetString(IDSS_WORDWRAP), REG_DWORD, (LPBYTE)&bWordWrap, sizeof(BOOL));
		SaveOption(key, GetString(IDSS_FONTIDX), REG_DWORD, (LPBYTE)&bPrimaryFont, sizeof(BOOL));
		SaveOption(key, GetString(IDSS_SMARTSELECT), REG_DWORD, (LPBYTE)&bSmartSelect, sizeof(BOOL));
#ifdef USE_RICH_EDIT
		SaveOption(key, GetString(IDSS_HYPERLINKS), REG_DWORD, (LPBYTE)&bHyperlinks, sizeof(BOOL));
#endif
		SaveOption(key, GetString(IDSS_SHOWSTATUS), REG_DWORD, (LPBYTE)&bShowStatus, sizeof(BOOL));
		SaveOption(key, GetString(IDSS_SHOWTOOLBAR), REG_DWORD, (LPBYTE)&bShowToolbar, sizeof(BOOL));
		SaveOption(key, GetString(IDSS_ALWAYSONTOP), REG_DWORD, (LPBYTE)&bAlwaysOnTop, sizeof(BOOL));
		SaveOption(key, GetString(IDSS_TRANSPARENT), REG_DWORD, (LPBYTE)&bTransparent, sizeof(BOOL));
		SaveOption(key, GetString(IDSS_CLOSEAFTERFIND), REG_DWORD, (LPBYTE)&bCloseAfterFind, sizeof(BOOL));
		SaveOption(key, GetString(IDSS_CLOSEAFTERREPLACE), REG_DWORD, (LPBYTE)&bCloseAfterReplace, sizeof(BOOL));
		SaveOption(key, GetString(IDSS_CLOSEAFTERINSERT), REG_DWORD, (LPBYTE)&bCloseAfterInsert, sizeof(BOOL));
		SaveOption(key, GetString(IDSS_NOFINDHIDDEN), REG_DWORD, (LPBYTE)&bNoFindHidden, sizeof(BOOL));
	}

	if (!options.bNoSaveHistory) {
		for (i = 0; i < NUMFINDS; ++i) {
			wsprintf(keyname, GetString(IDSS_FINDARRAY), i);
			SaveOption(key, keyname, REG_SZ, (LPBYTE)FindArray[i], MAXFIND);
			wsprintf(keyname, GetString(IDSS_REPLACEARRAY), i);
			SaveOption(key, keyname, REG_SZ, (LPBYTE)ReplaceArray[i], MAXFIND);
		}
		for (i = 0; i < NUMINSERTS; ++i) {
			wsprintf(keyname, GetString(IDSS_INSERTARRAY), i);
			SaveOption(key, keyname, REG_SZ, (LPBYTE)InsertArray[i], MAXINSERT);
		}
	}

	if (options.bSaveDirectory)
		SaveOption(key, GetString(IDSS_LASTDIRECTORY), REG_SZ, (LPBYTE)szDir, MAXFN);

	if (key != NULL)
		RegCloseKey(key);
}
