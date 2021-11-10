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

#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0400

#include <windows.h>
#include <tchar.h>

#ifdef USE_RICH_EDIT
#include <richedit.h>
#endif

#include "include/globals.h"
#include "include/consts.h"
#include "include/resource.h"
#include "include/tmp_protos.h"
#include "include/typedefs.h"
#include "include/strings.h"
#include "include/macros.h"

extern HANDLE globalHeap;
extern BOOL bUnix;
extern int nEncodingType;
extern HWND client;
extern HWND hdlgFind;
extern HWND hwnd;
extern LPTSTR szCaptionFile;
extern LPTSTR szDir;
extern LPTSTR szFile;

extern option_struct options;

/**
 * Calculate the size of the current file.
 *
 * @return Current file's size.
 */
long CalculateFileSize(void)
{
	LPTSTR szBuffer;
	long nBytes;
	extern HANDLE globalHeap;
	if (nEncodingType == TYPE_UTF_16 || nEncodingType == TYPE_UTF_16_BE) {
		nBytes = GetWindowTextLength(client) * 2 + SIZEOFBOM_UTF_16;
	}
	else if (nEncodingType == TYPE_UTF_8) {
		nBytes = GetWindowTextLength(client);
		if (sizeof(TCHAR) > 1) {
			/* TODO		This can get quite expensive for very large files. Future alternatives:
				- Count chars instead
				- Ability to disable this in options
				- Do this in the background with ratelimiting
				- Keep a local copy of the text buffer
			*/
			szBuffer = (LPTSTR) HeapAlloc(globalHeap, HEAP_ZERO_MEMORY, (nBytes + 1)*sizeof(TCHAR));
			GetWindowText(client, szBuffer, nBytes+1);
			nBytes = WideCharToMultiByte(CP_UTF8, 0, szBuffer, nBytes, NULL, 0, NULL, NULL);
			HeapFree(globalHeap, 0, (HGLOBAL)szBuffer);
		}
		nBytes += SIZEOFBOM_UTF_8 - (bUnix ? (SendMessage(client, EM_GETLINECOUNT, 0, 0)) - 1 : 0);
	}
	else {
		nBytes = GetWindowTextLength(client) - (bUnix ? (SendMessage(client, EM_GETLINECOUNT, 0, 0)) - 1 : 0);
	}
	return nBytes;
}

void SetFileFormat(int nFormat)
{
	switch (nFormat) {
	case FILE_FORMAT_DOS:
		SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(ID_DOS_FILE, 0), 0);
		break;
	case FILE_FORMAT_UNIX:
		SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(ID_UNIX_FILE, 0), 0);
		break;
	case FILE_FORMAT_UTF8:
		SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(ID_UTF8_FILE, 0), 0);
		break;
	case FILE_FORMAT_UTF8_UNIX:
		SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(ID_UTF8_UNIX_FILE, 0), 0);
		break;
	case FILE_FORMAT_UNICODE:
		SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(ID_UNICODE_FILE, 0), 0);
		break;
	case FILE_FORMAT_UNICODE_BE:
		SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(ID_UNICODE_BE_FILE, 0), 0);
		break;
	case FILE_FORMAT_BINARY:
		SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(ID_BINARY_FILE, 0), 0);
		break;
	}
}

int FixShortFilename(TCHAR *szSrc, TCHAR *szDest)
{
	WIN32_FIND_DATA FindFileData;
	HANDLE hHandle;
	TCHAR sDir[MAXFN], sName[MAXFN];
	int nDestPos=0, nSrcPos=0, i;
	BOOL bOK = TRUE;

	// Copy drive letter over
	if (szSrc[1] == _T(':')) {
		szDest[nDestPos++] = szSrc[nSrcPos++];
		szDest[nDestPos++] = szSrc[nSrcPos++];
	}

	while (szSrc[nSrcPos]) {
		// If the next TCHAR is '\' we are starting from the root and want to add '\*' to sDir.
		// Otherwise we are doing relative search, so we just append '*' to sDir
		if (szSrc[nSrcPos]==_T('\\')) {
			szDest[nDestPos++] = szSrc[nSrcPos++];

			if (szSrc[nSrcPos] == _T('\\')) { // get UNC server name
				szDest[nDestPos++] = szSrc[nSrcPos++];

				while (szSrc[nSrcPos] && szSrc[nSrcPos - 1]!=_T('\\')) {
					szDest[nDestPos++] = szSrc[nSrcPos++];
				}
			}
		}

		_tcsncpy(sDir, szDest, nDestPos);
		sDir[nDestPos] = _T('*');
		sDir[nDestPos + 1] = _T('\0');

		for (i=0; szSrc[nSrcPos] && szSrc[nSrcPos]!=_T('\\'); i++)
			sName[i] = szSrc[nSrcPos++];
		sName[i] = _T('\0');

		hHandle = FindFirstFile(sDir, &FindFileData);
		bOK = (hHandle != INVALID_HANDLE_VALUE);
		while (bOK && lstrcmpi(FindFileData.cFileName, sName) != 0 && lstrcmpi(FindFileData.cAlternateFileName, sName) != 0)
			bOK = FindNextFile(hHandle, &FindFileData);

    	if (bOK)
    		_tcscpy(&szDest[nDestPos], FindFileData.cFileName);
    	else
    		_tcscpy(&szDest[nDestPos], sName);

		// Fix the length of szDest
		nDestPos = _tcslen(szDest);
		if (hHandle)
			FindClose(hHandle);
	}
	return !bOK;
}

void ExpandFilename(LPTSTR szBuffer)
{
	WIN32_FIND_DATA FileData;
	HANDLE hSearch;
	TCHAR szTmp[MAXFN] = _T("");
	LPTSTR szTmpDir;

	if (szBuffer){
		lstrcpy(szTmp, szBuffer);
		FixShortFilename(szTmp, szBuffer);
	}

	if (szDir && szDir[0] != _T('\0'))
		SetCurrentDirectory(szDir);

	hSearch = FindFirstFile(szBuffer, &FileData);
	FREE(szCaptionFile);
	if (hSearch != INVALID_HANDLE_VALUE) {
		LPCTSTR pdest;
		szTmpDir = (LPTSTR)HeapAlloc(globalHeap, 0, (MAX(lstrlen(szBuffer)+2, lstrlen(szDir)+1)) * sizeof(TCHAR));
		lstrcpy(szTmpDir, szDir);
		pdest = _tcsrchr(szBuffer, _T('\\'));
		if (pdest) {
			int result;
			result = pdest - szBuffer + 1;
			lstrcpyn(szTmpDir, szBuffer, result);
		}
		if (szTmpDir[lstrlen(szTmpDir) - 1] != _T('\\'))
			lstrcat(szTmpDir, _T("\\"));
		szCaptionFile = (LPTSTR)HeapAlloc(globalHeap, 0, (lstrlen(szTmpDir)+lstrlen(FileData.cFileName)+1) * sizeof(TCHAR));
		szCaptionFile[0] = _T('\0');
		if (!options.bNoCaptionDir)
			lstrcat(szCaptionFile, szTmpDir);
		lstrcat(szCaptionFile, FileData.cFileName);
		FindClose(hSearch);
		if (szDir) HeapFree(globalHeap, 0, (HGLOBAL)szDir);
		szDir = szTmpDir;
	} else {
		szCaptionFile = (LPTSTR)HeapAlloc(globalHeap, 0, ((szDir ? lstrlen(szDir) : 0)+(szFile ? lstrlen(szFile) : 0)+1) * sizeof(TCHAR));
		szCaptionFile[0] = _T('\0');
		if (!options.bNoCaptionDir && szDir)
			lstrcat(szCaptionFile, szDir);
		if (szFile)
			lstrcat(szCaptionFile, szFile);
	}
}

BOOL SearchFile(LPCTSTR szText, BOOL bCase, BOOL bReplaceAll, BOOL bDown, BOOL bWholeWord)
{
	BOOL bRes;
	HCURSOR hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));
	CHARRANGE cr;

#ifdef USE_RICH_EDIT
	SendMessage(client, EM_EXGETSEL, 0, (LPARAM)&cr);
#else
	SendMessage(client, EM_GETSEL, (WPARAM)&cr.cpMin, (LPARAM)&cr.cpMax);
#endif
	bRes = DoSearch(szText, cr.cpMin, cr.cpMax, bDown, bWholeWord, bCase, FALSE);

	if (bRes || bReplaceAll) {
		SetCursor(hcur);
		return bRes;
	}

	if (!options.bFindAutoWrap && MessageBox(hdlgFind ? hdlgFind : client, bDown ? GetString(IDS_QUERY_SEARCH_TOP) : GetString(IDS_QUERY_SEARCH_BOTTOM), STR_METAPAD, MB_OKCANCEL|MB_ICONQUESTION) == IDCANCEL) {
		SetCursor(hcur);
		return FALSE;
	}
	else if (options.bFindAutoWrap) MessageBeep(MB_OK);

	bRes = DoSearch(szText, cr.cpMin, cr.cpMax, bDown, bWholeWord, bCase, TRUE);

	SetCursor(hcur);
	if (!bRes)
		MessageBox(hdlgFind ? hdlgFind : client, GetString(IDS_ERROR_SEARCH), STR_METAPAD, MB_OK|MB_ICONINFORMATION);

	return bRes;
}

#ifdef USE_RICH_EDIT
BOOL DoSearch(LPCTSTR szText, LONG lStart, LONG lEnd, BOOL bDown, BOOL bWholeWord, BOOL bCase, BOOL bFromTop)
{
	LONG lPrevStart = 0;
	UINT nFlags = FR_DOWN;
	FINDTEXT ft;
	CHARRANGE cr;

	if (bWholeWord)
		nFlags |= FR_WHOLEWORD;

	if (bCase)
		nFlags |= FR_MATCHCASE;

	ft.lpstrText = (LPTSTR)szText;

	if (bDown) {
		if (bFromTop)
			ft.chrg.cpMin = 0;
		else
			ft.chrg.cpMin = lStart + (lStart == lEnd ? 0 : 1);
		ft.chrg.cpMax = nReplaceMax;

		cr.cpMin = SendMessage(client, EM_FINDTEXT, (WPARAM)nFlags, (LPARAM)&ft);

		if (_tcschr(szText, _T('\r'))) {
			LONG lLine, lLines;

			lLine = SendMessage(client, EM_EXLINEFROMCHAR, 0, (LPARAM)cr.cpMin);
			lLines = SendMessage(client, EM_GETLINECOUNT, 0, 0);

			if (lLine == lLines - 1) {
				return FALSE;
			}
		}

		if (cr.cpMin == -1)
			return FALSE;
		cr.cpMax = cr.cpMin + lstrlen(szText);

		SendMessage(client, EM_EXSETSEL, 0, (LPARAM)&cr);
	}
	else {
		ft.chrg.cpMin = 0;
		if (bFromTop)
			ft.chrg.cpMax = -1;
		else
			ft.chrg.cpMax = (lStart == lEnd ? lEnd : lEnd - 1);
		lStart = SendMessage(client, EM_FINDTEXT, (WPARAM)nFlags, (LPARAM)&ft);
		if (lStart == -1)
			return FALSE;
		else {
			while (lStart != -1) {
				lPrevStart = lStart;
				lStart = SendMessage(client, EM_FINDTEXT, (WPARAM)nFlags, (LPARAM)&ft);
				ft.chrg.cpMin = lStart + 1;
			}
		}
		cr.cpMin = lPrevStart;
		cr.cpMax = lPrevStart + lstrlen(szText);
		SendMessage(client, EM_EXSETSEL, 0, (LPARAM)&cr);
	}
	if (!bReplacingAll)	UpdateStatus();
	return TRUE;
}
#else
typedef int (WINAPI* CMPFUNC)(LPCTSTR str1, LPCTSTR str2);

BOOL DoSearch(LPCTSTR szText, LONG lStart, LONG lEnd, BOOL bDown, BOOL bWholeWord, BOOL bCase, BOOL bFromTop)
{
	LONG lSize;
	int nFindLen = lstrlen(szText);
	LPCTSTR szBuffer = GetShadowBuffer();
	LPCTSTR lpszStop, lpsz, lpszFound = NULL;
	CMPFUNC pfnCompare = bCase ? lstrcmp : lstrcmpi;

	lSize = GetWindowTextLength(client);

	if (!szBuffer) {
		ReportLastError();
		return FALSE;
	}

	if (bDown) {
		if (nReplaceMax > -1) {
			lpszStop = szBuffer + nReplaceMax - 1;
		}
		else {
			lpszStop = szBuffer + lSize - 1;
		}
		lpsz = szBuffer + (bFromTop ? 0 : lStart + (lStart == lEnd ? 0 : 1));
	}
	else {
		lpszStop = szBuffer + (bFromTop ? lSize : lStart + (nFindLen == 1 && lStart > 0 ? -1 : 0));
		lpsz = szBuffer;
	}

	while (lpszStop != szBuffer && lpsz <= lpszStop - (bDown ? 0 : nFindLen-1) && (!bDown || (bDown && lpszFound == NULL))) {
		if ((bCase && *lpsz == *szText) || (TCHAR)(DWORD_PTR)CharLower((LPTSTR)(DWORD_PTR)(BYTE)*lpsz) == (TCHAR)(DWORD_PTR)CharLower((LPTSTR)(DWORD_PTR)(BYTE)*szText)) {
			LPTSTR lpch = (LPTSTR)(lpsz + nFindLen);
			TCHAR chSave = *lpch;
			int nResult;

			*lpch = _T('\0');
			nResult = (*pfnCompare)(lpsz, szText);
			*lpch = chSave;


			if (bWholeWord && lpsz > szBuffer && (_istalnum(*(lpsz-1)) || *(lpsz-1) == _T('_') || _istalnum(*(lpsz + nFindLen)) || *(lpsz + nFindLen) == _T('_')))
			;
			else if (nResult == 0) {
				lpszFound = lpsz;
			}
		}
		lpsz++;
	}

	if (lpszFound != NULL) {
		LONG lEnd;

		lStart = lpszFound - szBuffer;
		lEnd = lStart + nFindLen;
		SendMessage(client, EM_SETSEL, (WPARAM)lStart, (LPARAM)lEnd);
		if (!bReplacingAll)	UpdateStatus();
		return TRUE;
	}
	return FALSE;
}
#endif

long ReplaceAll(LPTSTR* szBuf, long* bufLen, LPCTSTR szFind, LPCTSTR szRepl, BOOL bCase, BOOL bWholeWord){

}