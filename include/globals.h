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

#ifndef GLOBALS_H
#define GLOBALS_H

#include "consts.h"
#include "metapad.h"



SLWA SetLWA;
HANDLE globalHeap;
HINSTANCE hinstThis;
HINSTANCE hinstLang;
HWND hwnd;
HWND client;
HWND status;
HWND toolbar;
HWND hdlgCancel;
HWND hdlgFind;
HANDLE hthread;
HMENU hrecentmenu;
HFONT hfontmain;
HFONT hfontfind;
WNDPROC wpOrigEditProc;
//WNDPROC wpOrigFindProc;
LPTSTR szCaptionFile, szFile, szDir;
LPTSTR szFav, szMetapadIni;
LPTSTR szFindText, szReplaceText, szInsert;
LPBYTE pbFindTextSpec, pbReplaceTextSpec;
TCHAR szStatusMessage[MAXSTRING];
TCHAR _szString[MAXSTRING];
LPTSTR szShadow;
TCHAR shadowHold;
DWORD shadowLen, shadowAlloc, shadowRngEnd;
BOOL bLoading, bStarted, bQuit;
BOOL bDirtyShadow, bDirtyStatus;
BOOL bDirtyFile, bMatchCase, bDown, bWholeWord, bReadOnly;
BOOL bWordWrap, bPrimaryFont, bPrint, bSmartSelect, bShowStatus;
BOOL bShowToolbar, bAlwaysOnTop, bCloseAfterFind, bHasFaves, bNoFindHidden;
BOOL bCloseAfterReplace, bCloseAfterInsert;
BOOL bTransparent, bInsertMode;
//BOOL bLinkMenu;
UINT nMRUTop;
//UINT uFindReplaceMsg;
int nStatusHeight, nToolbarHeight;
LPTSTR szCustomFilter;
HWND hwndSheet;
LPTSTR FindArray[NUMFINDS];
LPTSTR ReplaceArray[NUMFINDS];
LPTSTR InsertArray[NUMINSERTS];
int frDlgId, fontmainHt;
DWORD nFormat, randVal;
BOOL g_bDisablePluginVersionChecking;
BOOL g_bIniMode;

#ifdef USE_RICH_EDIT
BOOL bHyperlinks;
#else
HBRUSH BackBrush;
#endif

int _fltused; // see CMISCDAT.C for more info on this

option_struct options;

DWORD updateThrottle, updateTime;
DWORD savedFormat;
DWORD savedChars;
BYTE savedHash[32], savedHead[32], savedFoot[32];

#endif
