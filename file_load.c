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

#include "include/consts.h"
#include "include/globals.h"
#include "include/resource.h"
#include "include/strings.h"
#include "include/macros.h"
#include "include/metapad.h"
#include "include/encoding.h"



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
	}
	if (*pnFileEncoding == ID_ENC_UNKNOWN) {
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
		do {
			FREE(pNewBuffer);
			dwBytes = MultiByteToWideChar(cp, 0, *ppBuffer, *plBufLen, NULL, 0)*sizeof(TCHAR);
			if (NULL == (pNewBuffer = (LPBYTE) HeapAlloc(globalHeap, 0, dwBytes+sizeof(TCHAR)))) {
				ReportLastError();
				return -1;
			} else if (!MultiByteToWideChar(cp, MB_ERR_INVALID_CHARS, *ppBuffer, *plBufLen, (LPWSTR)pNewBuffer, dwBytes)){
				if (!MultiByteToWideChar(cp, 0, *ppBuffer, *plBufLen, (LPWSTR)pNewBuffer, dwBytes)){
					if (codepage) {
						cp = CP_ACP;
						*pnFileEncoding = ID_ENC_ANSI;
						continue;
					}
					ReportLastError();
					ERROROUT(GetString(IDS_UNICODE_LOAD_ERROR));
					dwBytes = 0;
				} else ERROROUT(GetString(IDS_UNICODE_LOAD_TRUNCATION));
			}
			break;
		} while (1);
		HeapFree(globalHeap, 0, (HGLOBAL)*ppBuffer);
		*ppBuffer = pNewBuffer;
	}
	((LPTSTR)(*ppBuffer))[dwBytes/=sizeof(TCHAR)] = _T('\0');
	return dwBytes;
}

void LoadFile(LPTSTR szFilename, BOOL bCreate, BOOL bMRU) {
	HANDLE hFile = NULL;
	LPTSTR szBuffer = NULL;
	TCHAR cPad = _T(' ');
	DWORD lBytes, lChars, nCR, nLF, nStrays;
	BOOL b;
	HCURSOR hcur;
	TCHAR szUncFn[MAXFN+6] = _T("\\\\?\\");

	lstrcpy(szStatusMessage, GetString(IDS_FILE_LOADING));
	UpdateStatus(TRUE);

	hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));
	lstrcpy(szUncFn+4, szFilename);

	*szStatusMessage = 0;
	for (nCR = 0; nCR < 2; ++nCR) {
		hFile = (HANDLE)CreateFile(szUncFn, GENERIC_READ, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile == INVALID_HANDLE_VALUE) {
			DWORD dwError = GetLastError();
			if (dwError == ERROR_FILE_NOT_FOUND && bCreate) {
				TCHAR buffer[MAXFN + 40];
				if (nCR == 0) {
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

	if ((lChars = LoadFileIntoBuffer(hFile, (LPBYTE*)&szBuffer, &lBytes, &nFormat, (nFormat&(1<<31) ? (WORD)nFormat : 0))) < 0) {
		bDirtyStatus = TRUE;
		CloseHandle(hFile);
		bLoading = FALSE;
		SetCursor(hcur);
		return;
	}
	CloseHandle(hFile);

	if (lBytes) {
		nFormat |= (GetLineFmt(szBuffer, lChars, &nCR, &nLF, &nStrays, &b) << 16);
		if (b) {
			nFormat = ID_ENC_BIN | ((nFormat >> 16) & 0x7fff);
			SetWindowText(client, szBuffer);
#ifdef UNICODE
			if (options.bNoWarningPrompt || MessageBox(hwnd, GetString(IDS_BINARY_FILE_WARNING_SAFE), STR_METAPAD, MB_ICONQUESTION|MB_OKCANCEL) != IDOK) {
#else
			if (options.bNoWarningPrompt || MessageBox(hwnd, GetString(IDS_BINARY_FILE_WARNING), STR_METAPAD, MB_ICONQUESTION|MB_OKCANCEL) != IDOK) {
#endif
				bQuitApp = TRUE;
				SetCursor(hcur);
				return;
			}
			NormalizeBinary(szBuffer, lChars);
		} else if (((nFormat >> 16) & 0x7fff) == ID_LFMT_MIXED && !options.bNoWarningPrompt)
			ERROROUT(GetString(IDS_LFMT_MIXED));
		NormalizeLineFmt(&szBuffer, lChars, (WORD)((nFormat >> 16) & 0x7fff), nCR, nLF, nStrays);
		SetWindowText(client, szBuffer);
	} else {
		SetWindowText(client, _T(""));
		nFormat = options.nFormat;
	}
	SetFileFormat(nFormat);
	UpdateSavedInfo();
	SetTabStops();
	SendMessage(client, EM_EMPTYUNDOBUFFER, 0, 0);

	if (szBuffer[0] == _T('.') &&
		szBuffer[1] == _T('L') &&
		szBuffer[2] == _T('O') &&
		szBuffer[3] == _T('G')) {
		CHARRANGE cr;
		cr.cpMin = cr.cpMax = lChars;

#ifdef USE_RICH_EDIT
		SendMessage(client, EM_EXSETSEL, 0, (LPARAM)&cr);
#else
		SendMessage(client, EM_SETSEL, (WPARAM)cr.cpMin, (LPARAM)cr.cpMax);
#endif
		SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(ID_DATE_TIME_CUSTOM, 0), 0);
		SendMessage(client, EM_SCROLLCARET, 0, 0);
	}
	FREE(szBuffer);
	UpdateStatus(TRUE);
	InvalidateRect(client, NULL, TRUE);
	SetCursor(hcur);
}



void LoadFileFromMenu(WORD wMenu, BOOL bMRU) {
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