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
#include "include/encoding.h"
#include "include/file_save.h"
#include "include/file_utils.h"



void LoadFile(LPTSTR szFilename, BOOL bCreate, BOOL bMRU);

#define NUMBOMS 3
static const BYTE bomLut[NUMBOMS][4] = {{0x3,0xEF,0xBB,0xBF}, {0x2,0xFF,0xFE}, {0x2,0xFE,0xFF}};

/**
 * Check if a buffer starts with a byte order mark.
 * If it does, the buffer is also advanced past the BOM and the given length value is decremented appropriately.
 *
 * @param[in,out] pb Pointer to the starting byte to check.
 * @param[in,out] pbLen Length of input.
 * @return One of ID_ENC_* enums if a BOM is found, else ID_ENC_UNKNOWN
 */
WORD CheckBOM(LPBYTE *pb, DWORD* pbLen) {
	DWORD i, j;
	for (i = 0; i < NUMBOMS; i++){
		if(pb && pbLen && *pb && *pbLen >= (j = bomLut[i][0]) && !memcmp(*pb, bomLut[i]+1, j)){
			*pb += j;
			*pbLen -= j;
			return ID_ENC_UTF8 + (WORD)i;
		}
	}
	return ID_ENC_UNKNOWN;
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

//TODO	bUnix = FALSE;
	if (cnt) {
//		bUnix = TRUE;
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
		if (options.bRecentOnOwn) 	hsub = GetSubMenu(hmenu, 1);
		else 						hsub = GetSubMenu(GetSubMenu(hmenu, 0), MPOS_FILE_RECENT);
	} else if (!options.bNoFaves) {
		hsub = GetSubMenu(hmenu, MPOS_FAVE);
	} else
		return;

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
		UpdateStatus(TRUE);
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

DWORD LoadFileIntoBuffer(HANDLE hFile, LPBYTE* ppBuffer, DWORD* plBufLen, DWORD* pnFileEncoding, WORD codepage) {
	DWORD dwBytes = 0;
	BOOL bResult;
	INT unitest = IS_TEXT_UNICODE_UNICODE_MASK | IS_TEXT_UNICODE_REVERSE_MASK | IS_TEXT_UNICODE_NOT_UNICODE_MASK | IS_TEXT_UNICODE_NOT_ASCII_MASK;
	LPINT lpiResult = &unitest;
	LPBYTE pNewBuffer = NULL;
	UINT cp = CP_ACP;
#ifndef UNICODE
	BOOL bUsedDefault;
#endif

	*plBufLen = GetFileSize(hFile, NULL);
	if (!options.bNoWarningPrompt && *plBufLen > LARGEFILESIZEWARN && MessageBox(hwnd, GetString(IDS_LARGE_FILE_WARNING), STR_METAPAD, MB_ICONQUESTION|MB_OKCANCEL) == IDCANCEL) {
		bQuitApp = TRUE;
		return -1;
	}
	*ppBuffer = (LPBYTE)HeapAlloc(globalHeap, 0, *plBufLen+2);
	if (*ppBuffer == NULL) {
		ReportLastError();
		return -1;
	}
	
	bResult = ReadFile(hFile, *ppBuffer, *plBufLen, &dwBytes, NULL);
	if (!bResult || dwBytes != *plBufLen)
		ReportLastError();
	*plBufLen = dwBytes;
	*(ppBuffer+dwBytes) = 0;
	*pnFileEncoding = CheckBOM(ppBuffer, plBufLen);

	if (!codepage) {
		// check if UTF-8 even if no bom
		if (*pnFileEncoding == ID_ENC_UNKNOWN && *plBufLen > 0 && IsTextUTF8(*ppBuffer))
			*pnFileEncoding = ID_ENC_UTF8;
		// check if unicode even if no bom
		if (*pnFileEncoding == ID_ENC_UNKNOWN && *plBufLen > 2 && (IsTextUnicode(*ppBuffer, *plBufLen, lpiResult) || 
			(!(*lpiResult & IS_TEXT_UNICODE_NOT_UNICODE_MASK) && (*lpiResult & IS_TEXT_UNICODE_NOT_ASCII_MASK))))
			*pnFileEncoding = ((*lpiResult & IS_TEXT_UNICODE_REVERSE_MASK) ? ID_ENC_UTF16BE : ID_ENC_UTF16);
	} else if (*pnFileEncoding == ID_ENC_UNKNOWN) {
		if (codepage) {
			cp = codepage;
			*pnFileEncoding = cp | (1<<31);
		} else *pnFileEncoding = ID_ENC_ANSI;
	}

	dwBytes = *plBufLen;
	if ((*pnFileEncoding == ID_ENC_UTF16 || *pnFileEncoding == ID_ENC_UTF16BE) && dwBytes) {
		*plBufLen /= 2;
		if (*pnFileEncoding == ID_ENC_UTF16BE)
			ReverseBytes(*ppBuffer, dwBytes);
#ifndef UNICODE
		if (sizeof(TCHAR) < 2) {
			dwBytes = WideCharToMultiByte(cp, 0, (LPCWSTR)*ppBuffer, *plBufLen, NULL, 0, NULL, NULL);
			if (NULL == (pNewBuffer = (LPBYTE)HeapAlloc(globalHeap, 0, dwBytes+1))) {
				ReportLastError();
				return -1;
			} else if (!WideCharToMultiByte(cp, 0, (LPCWSTR)*ppBuffer, *plBufLen, (LPSTR)pNewBuffer, dwBytes, NULL, &bUsedDefault)) {
				ReportLastError();
				ERROROUT(GetString(IDS_UNICODE_CONVERT_ERROR));
				dwBytes = 0;
			}
			if (bUsedDefault)
				ERROROUT(GetString(IDS_UNICODE_CHARS_WARNING));
			HeapFree(globalHeap, 0, (HGLOBAL)*ppBuffer);
			*ppBuffer = pNewBuffer;
		}
#endif
	} else if (sizeof(TCHAR) > 1 && *plBufLen) {
		if (*pnFileEncoding == ID_ENC_UTF8 && *plBufLen)
			cp = CP_UTF8;
		dwBytes = MultiByteToWideChar(cp, 0, *ppBuffer, *plBufLen, NULL, 0)*sizeof(TCHAR);
		if (NULL == (pNewBuffer = (LPBYTE) HeapAlloc(globalHeap, 0, dwBytes+sizeof(TCHAR)))) {
			ReportLastError();
			return -1;
		} else if (!MultiByteToWideChar(cp, MB_ERR_INVALID_CHARS, *ppBuffer, *plBufLen, (LPWSTR)pNewBuffer, dwBytes)){
			if (!MultiByteToWideChar(cp, 0, *ppBuffer, *plBufLen, (LPWSTR)pNewBuffer, dwBytes)){
				ReportLastError();
				ERROROUT(GetString(IDS_UNICODE_LOAD_ERROR));
				dwBytes = 0;
			} else ERROROUT(GetString(IDS_UNICODE_LOAD_TRUNCATION));
		}
		HeapFree(globalHeap, 0, (HGLOBAL)*ppBuffer);
		*ppBuffer = pNewBuffer;
	}
	((LPTSTR)(*ppBuffer))[dwBytes/=sizeof(TCHAR)] = _T('\0');
	return dwBytes;
}

void LoadFile(LPTSTR szFilename, BOOL bCreate, BOOL bMRU)
{
	HANDLE hFile = NULL;
	ULONG lBufferLength;
	LPBYTE pBuffer = NULL;
	LPTSTR szBuffer = NULL;
	TCHAR cPad = _T(' ');
	DWORD lChars;
	HCURSOR hcur;
	UINT i;
	TCHAR szUncFn[MAXFN+6] = _T("\\\\?\\");

	lstrcpy(szStatusMessage, GetString(IDS_FILE_LOADING));
	UpdateStatus(TRUE);

	hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));
	lstrcpy(szUncFn+4, szFilename);

	*szStatusMessage = 0;
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
					hFile = (HANDLE)CreateFile(szUncFn, GENERIC_READ, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
					if (hFile == INVALID_HANDLE_VALUE) {
						ERROROUT(GetString(IDS_FILE_CREATE_ERROR));
						UpdateStatus(TRUE);
						bLoading = FALSE;
						SetCursor(hcur);
						return;
					}
					break;
				case IDNO:
					MakeNewFile();
					SetCursor(hcur);
					return;
				case IDCANCEL:
					if (bLoading)
						PostQuitMessage(0);
					SetCursor(hcur);
					return;
				}
				break;
			} else {
				if (dwError == ERROR_FILE_NOT_FOUND) {
					ERROROUT(GetString(IDS_FILE_NOT_FOUND));
				} else if (dwError == ERROR_SHARING_VIOLATION) {
					ERROROUT(GetString(IDS_FILE_LOCKED_ERROR));
				} else {
					SetLastError(dwError);
					ReportLastError();
				}
				UpdateStatus(TRUE);
				bLoading = FALSE;
				SetCursor(hcur);
				return;
			}
		} else {
			SwitchReadOnly(GetFileAttributes(szUncFn) & FILE_ATTRIBUTE_READONLY);
			break;
		}
	}

	if (bMRU)
		SaveMRUInfo(szFilename);

	if ((lChars = LoadFileIntoBuffer(hFile, &pBuffer, &lBufferLength, &nFormat, 0)) < 0) {
		bDirtyStatus = TRUE;
		CloseHandle(hFile);
		bLoading = FALSE;
		return;
	}

	if (lBufferLength) {
#ifdef USE_RICH_EDIT
		lChars -= FixTextBuffer((LPTSTR)pBuffer);
#endif

		SendMessage(client, WM_SETREDRAW, (WPARAM)FALSE, 0);
		lChars += ConvertAndSetWindowText((LPTSTR)pBuffer, lChars);
		SendMessage(client, WM_SETREDRAW, (WPARAM)TRUE, 0);
/*		switch (nFormat) {
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
		}*/
	}
	else {
		SetWindowText(client, _T(""));
		SetFileFormat(options.nFormat);
	}
	bDirtyFile = FALSE;
//TODO	bBinaryFile = FALSE;

	szBuffer = (LPTSTR)pBuffer;
#ifdef UNICODE
	cPad = _T('\x2400');
#endif
	if (lChars != GetWindowTextLength(client) && bLoading) {
#ifdef UNICODE
		if (options.bNoWarningPrompt || MessageBox(hwnd, GetString(IDS_BINARY_FILE_WARNING_SAFE), STR_METAPAD, MB_ICONQUESTION|MB_OKCANCEL) == IDOK) {
#else
		if (options.bNoWarningPrompt || MessageBox(hwnd, GetString(IDS_BINARY_FILE_WARNING), STR_METAPAD, MB_ICONQUESTION|MB_OKCANCEL) == IDOK) {
#endif
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
//			bBinaryFile = TRUE;
		}
		else {
			bQuitApp = TRUE;
			return;
		}
	}

#ifdef UNICODE
//	if (bBinaryFile) SetFileFormat(FILE_FORMAT_BINARY);
#else
//	if (bBinaryFile) SetFileFormat(FILE_FORMAT_DOS);
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
	UpdateStatus(TRUE);
	CloseHandle(hFile);
	HeapFree(globalHeap, 0, (HGLOBAL) pBuffer);
	InvalidateRect(client, NULL, TRUE);
	SetCursor(hcur);
}
