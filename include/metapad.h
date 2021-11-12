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

#ifndef METAPAD_H
#define METAPAD_H

#ifdef _DEBUG
#include <stdio.h>
#endif

#ifdef USE_RICH_EDIT
#include <richedit.h>
#include <commdlg.h>
#endif
#if !defined(UNICODE) && !defined(__MINGW64_VERSION_MAJOR)
extern long _ttol(const TCHAR*);
extern int _ttoi(const TCHAR*);
#endif
#ifdef UNICODE
#define lstrchr wcschr
#define lstrrchr wcsrchr
#else
#define lstrchr strchr
#define lstrrchr strrchr
#endif

#include "consts.h"


///// Typedefs /////

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
	LPTSTR szBrowser;
	LPTSTR szArgs;
	LPTSTR szBrowser2;
	LPTSTR szArgs2;
	LPTSTR szQuote;
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
	DWORD nFormat;
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
	LPTSTR szLangPlugin;
	LPTSTR MacroArray[10];
	LPTSTR szFavDir;
	LPTSTR szCustomDate;
	LPTSTR szCustomDate2;
} option_struct;




///// Prototypes /////

LPTSTR GetString(UINT uID);
BOOL GetCheckedState(HMENU hmenu, UINT nID, BOOL bToggle);
void CreateClient(HWND hParent, LPCTSTR szText, BOOL bWrap);
BOOL CALLBACK AbortDlgProc(HDC hdc, int nCode);
LRESULT CALLBACK AbortPrintJob(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);
void PrintContents(void);
void ReportError(UINT);
void ReportLastError(void);
void CenterWindow(HWND hwndCenter);
void SelectWord(LPTSTR* target, BOOL bSmart, BOOL bAutoSelect);
void SetFont(HFONT* phfnt, BOOL bPrimary);
void SetTabStops(void);
void UpdateStatus(BOOL refresh);
BOOL SetClientFont(BOOL bPrimary);
BOOL CALLBACK AboutDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK AdvancedPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK Advanced2PageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK GeneralPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK ViewPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI MainWndProc(HWND hwndMain, UINT Msg, WPARAM wParam, LPARAM lParam);
int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow);
LRESULT APIENTRY EditProc(HWND hwndEdit, UINT uMsg, WPARAM wParam, LPARAM lParam);
void PopulateMRUList(void);
void SaveMRUInfo(LPCTSTR szFullPath);
void SwitchReadOnly(BOOL bNewVal);
BOOL EncodeWithEscapeSeqs(TCHAR* szText);
BOOL ParseForEscapeSeqs(LPTSTR buf, LPBYTE* specials, LPCTSTR errContext);
void UpdateCaption(void);


// language_plugin.c //
HINSTANCE LoadAndVerifyLanguagePlugin(LPCTSTR szPlugin);
void FindAndLoadLanguagePlugin(void);


// external_viewers.c //
BOOL ExecuteProgram(LPCTSTR lpExecutable, LPCTSTR lpCommandLine);
void LaunchInViewer(BOOL bCustom, BOOL bSecondary);
void LaunchExternalViewer(int);


// file_load.c //
DWORD LoadFileIntoBuffer(HANDLE hFile, LPBYTE* ppBuffer, DWORD* format);
BOOL LoadFile(LPTSTR szFilename, BOOL bCreate, BOOL bMRU, BOOL insert);
BOOL LoadFileFromMenu(WORD wMenu, BOOL bMRU);
BOOL BrowseFile(BOOL load, BOOL bMRU, BOOL insert, LPTSTR* fileName)


// file_save.c //
BOOL SaveCurrentFileAs(void);
BOOL SaveCurrentFile(void);
BOOL SaveIfDirty(void);


// file_utils.c //
void SetFileFormat(DWORD format, WORD reinterp);
void MakeNewFile(void);
void FixFilterString(LPTSTR szIn);
BOOL FixShortFilename(LPCTSTR szSrc, LPTSTR* szTgt);
BOOL GetReadableFilename(LPCTSTR lfn, LPTSTR* dst);

LPCTSTR GetShadowBuffer(DWORD* len);
LPCTSTR GetShadowRange(LONG min, LONG max, LONG line, DWORD* len);
LPCTSTR GetShadowSelection(DWORD* len, CHARRANGE* pcr);
LPCTSTR GetShadowLine(LONG line, LONG cp, DWORD* len, LONG* lineout, CHARRANGE* pcr);
DWORD GetColNum(LONG cp, LONG line, DWORD* lineLen, LONG* lineout, CHARRANGE* pcr);
DWORD GetCharIndex(DWORD col, LONG line, LONG cp, DWORD* lineLen, LONG* lineout, CHARRANGE* pcr);

DWORD CalcTextSize(LPCTSTR* szText, DWORD estBytes, WORD encoding, BOOL unix, BOOL inclBOM, DWORD* numChars);
DWORD GetTextChars(LPCTSTR szText, BOOL unix);
void UpdateSavedInfo();

BOOL SearchFile(LPCTSTR szText, BOOL bCase, BOOL bDown, BOOL bWholeWord, LPBYTE pbFindSpec);
DWORD StrReplace(LPCTSTR szIn, LPTSTR* szOut, DWORD* bufLen, LPCTSTR szFind, LPCTSTR szRepl, LPBYTE pbFindSpec, LPBYTE pbReplSpec, BOOL bCase, BOOL bWholeWord, DWORD maxMatch, DWORD maxLen, BOOL matchLen);
DWORD ReplaceAll(HWND owner, DWORD nOps, DWORD recur, LPCTSTR* szFind, LPCTSTR* szRepl, LPBYTE* pbFindSpec, LPBYTE* pbReplSpec, LPTSTR szMsgBuf, BOOL selection, BOOL bCase, BOOL bWholeWord, DWORD maxMatch, DWORD maxLen, BOOL matchLen, LPCTSTR header, LPCTSTR footer);


// settings_load.h //
void LoadWindowPlacement(int* left, int* top, int* width, int* height, int* nShow);
void LoadOptionString(HKEY hKey, LPCTSTR name, LPTSTR* lpData, DWORD cbData);
void LoadOptionStringDefault(HKEY hKey, LPCTSTR name, LPTSTR* lpData, DWORD cbData, LPCTSTR);
void LoadOptions(void);
BOOL LoadOptionNumeric(HKEY hKey, LPCTSTR name, LPBYTE lpData, DWORD cbData);
void LoadMenusAndData(void);


// settings_save.h //
BOOL SaveOption(HKEY hKey, LPCTSTR name, DWORD dwType, CONST LPBYTE lpData, DWORD cbData);
void SaveWindowPlacement(HWND hWndSave);
void SaveOptions(void);
void SaveMenusAndData(void);


#endif
