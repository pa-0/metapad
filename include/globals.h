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

#ifndef GLOBALS_H
#define GLOBALS_H

#include "consts.h"
#include "typedefs.h"

SLWA SetLWA;
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
WNDPROC wpOrigFindProc;
TCHAR szCaptionFile[MAXFN], szFile[MAXFN];
TCHAR szFav[MAXFN];
TCHAR szMetapadIni[MAXFN];
TCHAR szDir[MAXFN];
TCHAR szFindText[MAXFIND];
TCHAR szReplaceText[MAXFIND];
TCHAR szStatusMessage[MAXSTRING];
TCHAR _szString[MAXSTRING];
LPTSTR lpszShadow;
BOOL bDirtyFile, bLoading, bMatchCase, bDown, bWholeWord, bUnix, bReadOnly, bBinaryFile;
BOOL bWordWrap, bPrimaryFont, bPrint, bSmartSelect, bShowStatus /*, bWin2k*/;
BOOL bReplacingAll, bShowToolbar, bAlwaysOnTop, bCloseAfterFind, bHasFaves, bNoFindHidden;
BOOL bTransparent;
//BOOL bLinkMenu;
UINT nMRUTop;
UINT nShadowSize;
UINT uFindReplaceMsg;
int nStatusHeight, nToolbarHeight;
TCHAR szCustomFilter[2*MAXSTRING];
BOOL bInsertMode, bHideMessage;
int nReplaceMax;
HWND hwndSheet;
TCHAR FindArray[NUMFINDS][MAXFIND];
TCHAR ReplaceArray[NUMFINDS][MAXFIND];
int nEncodingType;
BOOL g_bDisablePluginVersionChecking;
BOOL g_bIniMode;

#ifdef USE_RICH_EDIT
BOOL bUpdated, bHyperlinks;
#endif

#ifndef USE_RICH_EDIT
HBRUSH BackBrush;
BOOL bQuitApp;
#endif

int _fltused; // see CMISCDAT.C for more info on this

option_struct options;

#endif
