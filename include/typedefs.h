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

#ifndef TYPEDEFS_H
#define TYPEDEFS_H

#include "consts.h"
#include <pshpack1.h>
typedef struct DLGTEMPLATEEX
{
	WORD dlgVer;
	WORD signature;
	DWORD helpID;
	DWORD exStyle;
	DWORD style;
	WORD cDlgItems;
	short x;
	short y;
	short cx;
	short cy;
} DLGTEMPLATEEX, *LPDLGTEMPLATEEX;
#include <poppack.h>

#ifndef USE_RICH_EDIT
typedef struct _charrange
{
	LONG	cpMin;
	LONG	cpMax;
} CHARRANGE;
#endif

typedef BOOL (WINAPI *SLWA)(HWND hwnd, COLORREF crKey, BYTE bAlpha, DWORD dwFlags);

typedef struct tag_options {
	BOOL bQuickExit;
	BOOL bSaveWindowPlacement;
	BOOL bSaveMenuSettings;
	BOOL bSaveDirectory;
	BOOL bLaunchClose;
	int nTabStops;
	int nPrimaryFont;
	int nSecondaryFont;
	int nLaunchSave;
	RECT rMargins;
	LOGFONT PrimaryFont, SecondaryFont;
	TCHAR szBrowser[MAXFN];
	TCHAR szArgs[MAXARGS];
	TCHAR szBrowser2[MAXFN];
	TCHAR szArgs2[MAXARGS];
	TCHAR szQuote[MAXQUOTE];
	BOOL bFindAutoWrap;
	BOOL bAutoIndent;
	BOOL bInsertSpaces;
	BOOL bNoCaptionDir;
	BOOL bHideGotoOffset;
	BOOL bRecentOnOwn;
	BOOL bDontInsertTime;
	BOOL bNoWarningPrompt;
	BOOL bUnFlatToolbar;
	BOOL bStickyWindow;
	BOOL bReadOnlyMenu;
	UINT nStatusFontWidth;
	UINT nSelectionMarginWidth;
	UINT nMaxMRU;
	UINT nFormatIndex;
	UINT nTransparentPct;
	BOOL bSystemColours;
	BOOL bSystemColours2;
	COLORREF BackColour, FontColour;
	COLORREF BackColour2, FontColour2;
	BOOL bNoSmartHome;
	BOOL bNoAutoSaveExt;
	BOOL bContextCursor;
	BOOL bCurrentFindFont;
#ifndef USE_RICH_EDIT
	BOOL bDefaultPrintFont;
	BOOL bAlwaysLaunch;
#else
	BOOL bSuppressUndoBufferPrompt;
	BOOL bLinkDoubleClick;
	BOOL bHideScrollbars;
#endif
	BOOL bNoFaves;
	BOOL bPrintWithSecondaryFont;
	BOOL bNoSaveHistory;
	BOOL bNoFindAutoSelect;
	TCHAR szLangPlugin[MAXFN];
	TCHAR MacroArray[10][MAXMACRO];
	TCHAR szFavDir[MAXFN];
} option_struct;

#endif // TYPEDEFS_H
