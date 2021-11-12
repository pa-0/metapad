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

#include "include/consts.h"
#include "include/globals.h"
#include "include/resource.h"
#include "include/strings.h"
#include "include/macros.h"
#include "include/metapad.h"
#include "include/file_save.h"
#include "include/file_utils.h"



void LoadFile(LPTSTR szFilename, BOOL bCreate, BOOL bMRU);

/**
 * Check if a set of bytes is the byte order mark.
 *
 * @param[in] pb Pointer to the starting byte to check.
 * @param[in] bomType The type of BOM to be checked for. Valid values are TYPE_UTF_8, TYPE_UTF_16 and TYPE_UTF_16_BE.
 * @return TRUE if pb is BOM, FALSE otherwise.
 */
BOOL IsBOM(LPBYTE pb, int bomType)
{
	if (bomType == TYPE_UTF_8) {
		if ((*pb == 0xEF) & (*(pb+1) == 0xBB) & (*(pb+2) == 0xBF))
			return TRUE;
		else
			return FALSE;
	}
	else if (bomType == TYPE_UTF_16) {
		if ((*pb == 0xFF) & (*(pb+1) == 0xFE))
			return TRUE;
		else
			return FALSE;
	}
	else if (bomType == TYPE_UTF_16_BE) {
		if ((*pb == 0xFE) & (*(pb+1) == 0xFF))
			return TRUE;
		else
			return FALSE;
	}
	return FALSE;
}

int ConvertAndSetWindowText(LPTSTR szText, DWORD dwTextLen)
{
	UINT i = 0, cnt = 0, mcnt = 0, j;
	LPTSTR szBuffer = NULL;

	if (szText[0] == _T('\n')) {
		++cnt;
	}
	while (szText[i] && szText[i+1]) {
		if (szText[i] != _T('\r') && szText[i+1] == _T('\n'))
			++cnt;
		else if (szText[i] == _T('\r') && szText[i+1] != _T('\n'))
			++mcnt;
		++i;
	}
	if (i+1 < dwTextLen) return 0;	//This is a binary file (has NULLs), don't attempt to fix line endings
	/** @fixme Commented out code. */
	/*
	if (mcnt && cnt) {
		ERROROUT(_T("Malformed text file detected!"));
	}
	*/

	if (mcnt) {
		if (!options.bNoWarningPrompt)
			ERROROUT(GetString(IDS_MAC_FILE_WARNING));

		if (szText[i] == _T('\r')) ++mcnt;

		for (i = 0; szText[i] && szText[i+1]; ++i) {
			if (szText[i] == _T('\r') && szText[i+1] != _T('\n'))
				szText[i] = _T('\n');
		}

		if (szText[i] == _T('\r')) szText[i] = _T('\n');

		cnt += mcnt;
	}

	bUnix = FALSE;
	if (cnt) {
		bUnix = TRUE;
		szBuffer = (LPTSTR)HeapAlloc(globalHeap, HEAP_ZERO_MEMORY, (lstrlen(szText)+cnt+2) * sizeof(TCHAR));
		i = j = 0;
		if (szText[0] == _T('\n')) {
			szBuffer[0] = _T('\r');
			++j;
		}
		while (szText[i] != _T('\0')) {
			if (szText[i] != _T('\r') && szText[i+1] == _T('\n')) {
				szBuffer[j++] = szText[i];
				szBuffer[j] = _T('\r');
			}
			else {
				szBuffer[j] = szText[i];
			}
			j++; i++;
		}
		szBuffer[j] = _T('\0');
#ifdef STREAMING
		{
		EDITSTREAM es;
		es.dwCookie = (DWORD)szBuffer;
		es.dwError = 0;
		es.pfnCallback = EditStreamIn;
		SendMessage(client, EM_STREAMIN, (WPARAM)_SF_TEXT, (LPARAM)&es);
		}
#else
		SetWindowText(client, szBuffer);
#endif
		HeapFree(globalHeap, 0, (HGLOBAL) szBuffer);
	}
#ifdef STREAMING
	else {
		EDITSTREAM es;
		es.dwCookie = (DWORD)szText;
		es.dwError = 0;
		es.pfnCallback = EditStreamIn;
		SendMessage(client, EM_STREAMIN, (WPARAM)_SF_TEXT, (LPARAM)&es);
	}
#else
	else {
		SetWindowText(client, szText);
/** @fixme Commented out code. */
/*
typedef struct _settextex {
	DWORD flags;
	UINT codepage;
} SETTEXTEX;

SETTEXTEX ste;
CHARSETINFO csi;
ste.flags = ST_SELECTION;
TranslateCharsetInfo((DWORD FAR*) GREEK_CHARSET,&csi,TCI_SRCCHARSET );
ste.codepage = csi.ciACP;
SendMessage(client, WM_USER+97, (WPARAM) &ste, (LPARAM)szText);
*/
	}
#endif

	return cnt;
}

#ifdef USE_RICH_EDIT
int FixTextBuffer(LPTSTR szText)
{
	int cnt = 0;
	int i = 0, j = 0;
	LPTSTR szNew = (LPTSTR)HeapAlloc(globalHeap, HEAP_ZERO_MEMORY, (lstrlen(szText) + 1) * sizeof(TCHAR));

	if (lstrlen(szText) < 3) {
		if (szText[i]) szNew[j++] = szText[i++];
		if (szText[i]) szNew[j++] = szText[i++];
	}

	while (szText[i] && szText[i+1] && szText[i+2]) {
		if (szText[i] == _T('\r') && szText[i+1] == _T('\r') && szText[i+2] == _T('\n')) {
			++cnt;
		}
		else {
			szNew[j++] = szText[i];
		}
		++i;
	}

	if (szText[i]) szNew[j++] = szText[i++];
	if (szText[i]) szNew[j++] = szText[i++];

	if (cnt) lstrcpy(szText, szNew);

	HeapFree(globalHeap, 0, (HGLOBAL)szNew);

	if (cnt) {
		if (!options.bNoWarningPrompt) {
			ERROROUT(GetString(IDS_CARRIAGE_RETURN_WARNING));
		}
		SetFocus(client);
	}

	return cnt;
}
#else
void FixTextBufferLE(LPTSTR* pszBuffer)
{
	UINT i = 0, cnt = 0, j;
	LPTSTR szNewBuffer = NULL;

	if ((*pszBuffer)[0] == _T('\n')) {
		++cnt;
	}
	while ((*pszBuffer)[i] && (*pszBuffer)[i+1]) {
		if ((*pszBuffer)[i] != _T('\r') && (*pszBuffer)[i+1] == _T('\n'))
			++cnt;
		else if ((*pszBuffer)[i] == _T('\r') && (*pszBuffer)[i+1] != _T('\n'))
			++cnt;
		++i;
	}
	if (!cnt) return;

	szNewBuffer = (LPTSTR)HeapAlloc(globalHeap, HEAP_ZERO_MEMORY, (lstrlen(*pszBuffer)+cnt+2) * sizeof(TCHAR));
	i = j = 0;
	if ((*pszBuffer)[0] == _T('\n')) {
		szNewBuffer[0] = _T('\r');
		++j;
	}
	while ((*pszBuffer)[i] != _T('\0')) {
		if ((*pszBuffer)[i] != _T('\r') && (*pszBuffer)[i+1] == _T('\n')) {
			szNewBuffer[j++] = (*pszBuffer)[i];
			szNewBuffer[j] = _T('\r');
		}
		else if ((*pszBuffer)[i] == _T('\r') && (*pszBuffer)[i+1] != _T('\n')) {
			szNewBuffer[j++] = (*pszBuffer)[i];
			szNewBuffer[j] = _T('\n');
		}
		else {
			szNewBuffer[j] = (*pszBuffer)[i];
		}
		j++; i++;
	}
	szNewBuffer[j] = _T('\0');
	HeapFree(globalHeap, 0, (HGLOBAL)*pszBuffer);
	*pszBuffer = szNewBuffer;
}
#endif

void LoadFileFromMenu(WORD wMenu, BOOL bMRU)
{
	HMENU hmenu = GetMenu(hwnd);
	HMENU hsub = NULL;
	MENUITEMINFO mio;
	TCHAR szBuffer[MAXFN] = _T("\n");
	LPTSTR sztFile = NULL;

	if (!SaveIfDirty())
		return;

	if (bMRU) {
		if (options.bRecentOnOwn)
			hsub = GetSubMenu(hmenu, 1);
		else
			hsub = GetSubMenu(GetSubMenu(hmenu, 0), RECENTPOS);
	}
	else if (!options.bNoFaves) {
		hsub = GetSubMenu(hmenu, FAVEPOS);
	}
	else {
		return;
	}

	mio.cbSize = sizeof(MENUITEMINFO);
	mio.fMask = MIIM_TYPE;
	mio.fType = MFT_STRING;
	mio.cch = MAXFN;
	mio.dwTypeData = szBuffer;
	GetMenuItemInfo(hsub, wMenu, FALSE, &mio);

	FREE(szFile);
	SSTRCPY(sztFile, (szBuffer+3));
	if (sztFile[0]) {
		if (!bMRU) {
			GetPrivateProfileString(STR_FAV_APPNAME, sztFile, _T(""), szBuffer, MAXFN, SCNUL(szFav));
			if (!szBuffer[0]) {
				ERROROUT(GetString(IDS_ERROR_FAVOURITES));
				MakeNewFile();
				return;
			}
		}

		bLoading = TRUE;
		ExpandFilename(sztFile, &sztFile);
		lstrcpy(szStatusMessage, GetString(IDS_FILE_LOADING));
		UpdateStatus();
		LoadFile(sztFile, FALSE, TRUE);
		if (bLoading) {
			bLoading = FALSE;
			bDirtyFile = FALSE;
			SSTRCPY(szFile, sztFile);
			UpdateCaption();
		}
		else
			MakeNewFile();
	}
}

BOOL IsTextUTF8(LPBYTE buf){
	BOOL yes = 0;
	while (*buf) {
		if (*buf < 0x80) buf++;
		else if ((*((PWORD)buf) & 0xc0e0) == 0x80c0){ buf+=2; yes = 1; }
		else if ((*((PDWORD)buf) & 0xc0c0f0) == 0x8080e0){ buf+=3; yes = 1; }
		else if ((*((PDWORD)buf) & 0xc0c0c0f8) == 0x808080f0){ buf+=4; yes = 1; }	//valid UTF-8 outside Unicode range - Win32 cannot handle this - show truncation warning!
		else return 0;
	}
	return yes;
}

DWORD LoadFileIntoBuffer(HANDLE hFile, LPBYTE* ppBuffer, ULONG* plBufferLength, INT* pnFileEncoding)
{
	DWORD dwBytesRead = 0;
	BOOL bResult;
	INT unitest = IS_TEXT_UNICODE_UNICODE_MASK | IS_TEXT_UNICODE_REVERSE_MASK | IS_TEXT_UNICODE_NOT_UNICODE_MASK | IS_TEXT_UNICODE_NOT_ASCII_MASK;
	LPINT lpiResult = &unitest;
	long nBytesNeeded;
	BOOL bUsedDefault;
	LPBYTE pNewBuffer = NULL;
	UINT cp = CP_ACP;

	*plBufferLength = GetFileSize(hFile, NULL);
	if (!options.bNoWarningPrompt && *plBufferLength > LARGEFILESIZEWARN && MessageBox(hwnd, GetString(IDS_LARGE_FILE_WARNING), STR_METAPAD, MB_ICONQUESTION|MB_OKCANCEL) == IDCANCEL) {
		SendMessage(hwnd, WM_CLOSE, 0, 0L);
		return -1;
	}

	*ppBuffer = (LPBYTE) HeapAlloc(globalHeap, HEAP_ZERO_MEMORY, (*plBufferLength+2) * sizeof(TCHAR));
	if (*ppBuffer == NULL) {
		ReportLastError();
		return -1;
	}

	*pnFileEncoding = TYPE_UNKNOWN;

	// check for bom
	if (*plBufferLength >= SIZEOFBOM_UTF_8) {

		bResult = ReadFile(hFile, *ppBuffer, SIZEOFBOM_UTF_8, &dwBytesRead, NULL);
		if (!bResult || dwBytesRead != (DWORD)SIZEOFBOM_UTF_8)
			ReportLastError();

		if (IsBOM(*ppBuffer, TYPE_UTF_8)) {
			*pnFileEncoding = TYPE_UTF_8;
			*plBufferLength -= SIZEOFBOM_UTF_8;
		}
		else {
			SetFilePointer(hFile, -SIZEOFBOM_UTF_8, NULL, FILE_CURRENT);
		}
	}
	if (*pnFileEncoding == TYPE_UNKNOWN && *plBufferLength >= SIZEOFBOM_UTF_16) {

		bResult = ReadFile(hFile, *ppBuffer, SIZEOFBOM_UTF_16, &dwBytesRead, NULL);
		if (!bResult || dwBytesRead != (DWORD)SIZEOFBOM_UTF_16)
			ReportLastError();

		if (IsBOM(*ppBuffer, TYPE_UTF_16)) {
			*pnFileEncoding = TYPE_UTF_16;
			*plBufferLength -= SIZEOFBOM_UTF_16;
		}
		else if (IsBOM(*ppBuffer, TYPE_UTF_16_BE)) {
			*pnFileEncoding = TYPE_UTF_16_BE;
			*plBufferLength -= SIZEOFBOM_UTF_16;
		}
		else {
			SetFilePointer(hFile, -SIZEOFBOM_UTF_16, NULL, FILE_CURRENT);
		}
	}

	bResult = ReadFile(hFile, *ppBuffer, *plBufferLength, &dwBytesRead, NULL);
	if (!bResult || dwBytesRead != (DWORD)*plBufferLength)
		ReportLastError();

	// check if UTF-8 even if no bom
	if (*pnFileEncoding == TYPE_UNKNOWN && dwBytesRead > 0 && IsTextUTF8(*ppBuffer)) {
		*pnFileEncoding = TYPE_UTF_8;
	}
	// check if unicode even if no bom
	if (*pnFileEncoding == TYPE_UNKNOWN) {
		if (dwBytesRead > 2 && (IsTextUnicode(*ppBuffer, *plBufferLength, lpiResult) || 
			(!(*lpiResult & IS_TEXT_UNICODE_NOT_UNICODE_MASK) && (*lpiResult & IS_TEXT_UNICODE_NOT_ASCII_MASK)))) {
			*pnFileEncoding = ((*lpiResult & IS_TEXT_UNICODE_REVERSE_MASK) ? TYPE_UTF_16_BE : TYPE_UTF_16);

			// add unicode null - already zeroed
			/*
			(*ppBuffer)[*plBufferLength] = 0;
			(*ppBuffer)[*plBufferLength+1] = 0;
			*/
		}
	}

	nBytesNeeded = *plBufferLength;
	if ((*pnFileEncoding == TYPE_UTF_16 || *pnFileEncoding == TYPE_UTF_16_BE) && *plBufferLength) {

		if (*pnFileEncoding == TYPE_UTF_16_BE) {
			ReverseBytes(*ppBuffer, *plBufferLength);
		}
		*plBufferLength = *plBufferLength / 2;

		if (sizeof(TCHAR) < 2) {
			nBytesNeeded = WideCharToMultiByte(cp, 0, (LPCWSTR)*ppBuffer, *plBufferLength, NULL, 0, NULL, NULL);
			if (NULL == (pNewBuffer = (LPBYTE) HeapAlloc(globalHeap, HEAP_ZERO_MEMORY, nBytesNeeded+sizeof(TCHAR)))) {
				ReportLastError();
				return -1;
			} else if (!WideCharToMultiByte(cp, 0, (LPCWSTR)*ppBuffer, *plBufferLength, (LPSTR)pNewBuffer, nBytesNeeded, NULL, &bUsedDefault)) {
				ReportLastError();
				ERROROUT(GetString(IDS_UNICODE_CONVERT_ERROR));
				nBytesNeeded = 0;
			}
			if (bUsedDefault) {
				ERROROUT(GetString(IDS_UNICODE_CHARS_WARNING));
			}
			HeapFree(globalHeap, 0, (HGLOBAL)*ppBuffer);
			*ppBuffer = pNewBuffer;
		}
	} else if (sizeof(TCHAR) > 1 && *plBufferLength) {
		if (*pnFileEncoding == TYPE_UTF_8 && *plBufferLength)
			cp = CP_UTF8;
		nBytesNeeded = MultiByteToWideChar(cp, 0, *ppBuffer, *plBufferLength, NULL, 0)*sizeof(TCHAR);
		if (NULL == (pNewBuffer = (LPBYTE) HeapAlloc(globalHeap, HEAP_ZERO_MEMORY, nBytesNeeded+sizeof(TCHAR)))) {
			ReportLastError();
			return -1;
		} else if (!MultiByteToWideChar(cp, MB_ERR_INVALID_CHARS, *ppBuffer, *plBufferLength, (LPWSTR)pNewBuffer, nBytesNeeded)){
			if (!MultiByteToWideChar(cp, 0, *ppBuffer, *plBufferLength, (LPWSTR)pNewBuffer, nBytesNeeded)){
				ReportLastError();
				ERROROUT(GetString(IDS_UNICODE_LOAD_ERROR));
				nBytesNeeded = 0;
			} else ERROROUT(GetString(IDS_UNICODE_LOAD_TRUNCATION));
		}
		HeapFree(globalHeap, 0, (HGLOBAL)*ppBuffer);
		*ppBuffer = pNewBuffer;
	}
	return nBytesNeeded/sizeof(TCHAR);
}

void LoadFile(LPTSTR szFilename, BOOL bCreate, BOOL bMRU)
{
	HANDLE hFile = NULL;
	ULONG lBufferLength;
	LPBYTE pBuffer = NULL;
	LPTSTR szBuffer = NULL;
	TCHAR cPad = _T(' ');
	LONG lActualCharsRead;
	HCURSOR hcur;
	UINT i;
	TCHAR szUncFn[MAXFN+6] = _T("\\\\?\\");

	lstrcpy(szStatusMessage, GetString(IDS_FILE_LOADING));
	UpdateStatus();

	hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));
	lstrcpy(szUncFn+4, szFilename);

	for (i = 0; i < 2; ++i) {
		hFile = (HANDLE)CreateFile(szUncFn, GENERIC_READ, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile == INVALID_HANDLE_VALUE) {
			DWORD dwError = GetLastError();
			if (dwError == ERROR_FILE_NOT_FOUND && bCreate) {
				TCHAR buffer[MAXFN + 40];

				if (i == 0) {
					if (_tcschr(szFilename, _T('.')) == NULL) {
						lstrcat(szFilename, _T(".txt"));
						continue;
					}
				}

				wsprintf(buffer, GetString(IDS_CREATE_FILE_MESSAGE), szFilename);
				switch (MessageBox(hwnd, buffer, STR_METAPAD, MB_YESNOCANCEL | MB_ICONEXCLAMATION)) {
				case IDYES:
					{
						hFile = (HANDLE)CreateFile(szUncFn, GENERIC_READ, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
						if (hFile == INVALID_HANDLE_VALUE) {
							ERROROUT(GetString(IDS_FILE_CREATE_ERROR));
							*szStatusMessage = 0;
							UpdateStatus();
							bLoading = FALSE;
							SetCursor(hcur);
							return;
						}
						break;
					}
				case IDNO:
					{
						*szStatusMessage = 0;
						MakeNewFile();
						SetCursor(hcur);
						return;
					}
				case IDCANCEL:
					{
						if (bLoading)
							PostQuitMessage(0);
						*szStatusMessage = 0;
						SetCursor(hcur);
						return;
					}
				}
				break;
			}
			else {
				if (dwError == ERROR_FILE_NOT_FOUND) {
					ERROROUT(GetString(IDS_FILE_NOT_FOUND));
				}
				else if (dwError == ERROR_SHARING_VIOLATION) {
					ERROROUT(GetString(IDS_FILE_LOCKED_ERROR));
				}
				else {
					SetLastError(dwError);
					ReportLastError();
				}
				*szStatusMessage = 0;
				UpdateStatus();
				bLoading = FALSE;
				SetCursor(hcur);
				return;
			}
		}
		else {
			SwitchReadOnly(GetFileAttributes(szUncFn) & FILE_ATTRIBUTE_READONLY);
			break;
		}
	}

	if (bMRU)
		SaveMRUInfo(szFilename);


	if ((lActualCharsRead = LoadFileIntoBuffer(hFile, &pBuffer, &lBufferLength, &nEncodingType)) < 0) {
		CloseHandle(hFile);
		*szStatusMessage = 0;
		bLoading = FALSE;
		return;
	}

//	if (dwActualBytesRead < 0) goto fini;

	if (lBufferLength) {
#ifdef USE_RICH_EDIT
		lActualCharsRead -= FixTextBuffer((LPTSTR)pBuffer);
#endif

		SendMessage(client, WM_SETREDRAW, (WPARAM)FALSE, 0);
		lActualCharsRead += ConvertAndSetWindowText((LPTSTR)pBuffer, lActualCharsRead);
		SendMessage(client, WM_SETREDRAW, (WPARAM)TRUE, 0);
		switch (nEncodingType) {
			case TYPE_UTF_16:
				SetFileFormat(FILE_FORMAT_UNICODE);
				break;
			case TYPE_UTF_16_BE:
				SetFileFormat(FILE_FORMAT_UNICODE_BE);
				break;
			case TYPE_UTF_8:
				SetFileFormat(bUnix ? FILE_FORMAT_UTF8_UNIX : FILE_FORMAT_UTF8);
				break;
			default:
				SetFileFormat(bUnix ? FILE_FORMAT_UNIX : FILE_FORMAT_DOS);
		}
	}
	else {
		SetWindowText(client, _T(""));
		SetFileFormat(options.nFormatIndex);
	}
	bDirtyFile = FALSE;
	bBinaryFile = FALSE;

	szBuffer = (LPTSTR)pBuffer;
#ifdef UNICODE
	if (sizeof(TCHAR) > 1) cPad = _T('\x2400');
#endif
	if (lActualCharsRead != GetWindowTextLength(client) && bLoading) {
		i = (sizeof(TCHAR) > 1 ? IDS_BINARY_FILE_WARNING_SAFE : IDS_BINARY_FILE_WARNING);
		if (options.bNoWarningPrompt || MessageBox(hwnd, GetString(i), STR_METAPAD, MB_ICONQUESTION|MB_OKCANCEL) == IDOK) {
			for (i = 0; i < lBufferLength; i++)
				if (szBuffer[i] == _T('\0'))
					szBuffer[i] = cPad;
			SendMessage(client, WM_SETREDRAW, (WPARAM)FALSE, 0);
#ifdef STREAMING
			{
				EDITSTREAM es;
				es.dwCookie = (DWORD)pBuffer;
				es.dwError = 0;
				es.pfnCallback = EditStreamIn;
				SendMessage(client, EM_STREAMIN, (WPARAM)_SF_TEXT, (LPARAM)&es);
			}
#else
			SetWindowText(client, (LPTSTR)pBuffer);
#endif
			SendMessage(client, WM_SETREDRAW, (WPARAM)TRUE, 0);
			bBinaryFile = TRUE;
		}
		else {
			SendMessage(hwnd, WM_CLOSE, 0, 0L);
		}
	}

#ifdef BUILD_METAPAD_UNICODE
	if (bBinaryFile) SetFileFormat(FILE_FORMAT_BINARY);
#else
	if (bBinaryFile) SetFileFormat(FILE_FORMAT_DOS);
#endif
	bDirtyShadow = bDirtyStatus = TRUE;

	SetTabStops();
	SendMessage(client, EM_EMPTYUNDOBUFFER, 0, 0);

	if (szBuffer[0] == _T('.') &&
		szBuffer[1] == _T('L') &&
		szBuffer[2] == _T('O') &&
		szBuffer[3] == _T('G')) {
		CHARRANGE cr;
		cr.cpMin = cr.cpMax = lBufferLength;

#ifdef USE_RICH_EDIT
		SendMessage(client, EM_EXSETSEL, 0, (LPARAM)&cr);
#else
		SendMessage(client, EM_SETSEL, (WPARAM)cr.cpMin, (LPARAM)cr.cpMax);
#endif
		SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(ID_DATE_TIME_CUSTOM, 0), 0);
		SendMessage(client, EM_SCROLLCARET, 0, 0);
	}

//#ifndef BUILD_METAPAD_UNICODE
//fini:
//#endif
	*szStatusMessage = 0;
	UpdateStatus();
	CloseHandle(hFile);
	HeapFree(globalHeap, 0, (HGLOBAL) pBuffer);
	InvalidateRect(client, NULL, TRUE);
	SetCursor(hcur);
}
