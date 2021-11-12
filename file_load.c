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
#include <commdlg.h>
#include <tchar.h>

#include "include/consts.h"
#include "include/globals.h"
#include "include/resource.h"
#include "include/strings.h"
#include "include/macros.h"
#include "include/metapad.h"
#include "include/encoding.h"



DWORD LoadFileIntoBuffer(HANDLE hFile, LPBYTE* ppBuffer, DWORD* format) {
	DWORD dwBytes = 0, buflen;
	BOOL bResult;
	INT unitest = IS_TEXT_UNICODE_UNICODE_MASK | IS_TEXT_UNICODE_REVERSE_MASK | IS_TEXT_UNICODE_NOT_UNICODE_MASK | IS_TEXT_UNICODE_NOT_ASCII_MASK;
	LPINT lpiResult = &unitest;
	WORD enc, cp;
#ifndef UNICODE
	BOOL bUsedDefault;
#endif

	if (!ppBuffer || !format) return -1;
	buflen = GetFileSize(hFile, NULL);
	if (!options.bNoWarningPrompt && buflen > LARGEFILESIZEWARN && MessageBox(hwnd, GetString(IDS_LARGE_FILE_WARNING), STR_METAPAD, MB_ICONQUESTION|MB_OKCANCEL) == IDCANCEL) {
		return -1;
	}
	*ppBuffer = (LPBYTE)HeapAlloc(globalHeap, 0, buflen+2);
	if (*ppBuffer == NULL) {
		ReportLastError();
		return -1;
	}
	
	bResult = ReadFile(hFile, *ppBuffer, buflen, &dwBytes, NULL);
	if (!bResult || dwBytes != buflen)
		ReportLastError();
	buflen = dwBytes;
	(*ppBuffer)[dwBytes] = 0;
	enc = CheckBOM(ppBuffer, &buflen);
	cp = (*format >> 31 ? (WORD)*format : 0);

	if (!cp) {
		// check if UTF-8 even if no bom
		if (enc == ID_ENC_UNKNOWN && buflen > 1 && IsTextUTF8(*ppBuffer))
			enc = ID_ENC_UTF8;
		// check if unicode even if no bom
		if (enc == ID_ENC_UNKNOWN && buflen > 2 && (IsTextUnicode(*ppBuffer, buflen, lpiResult) || 
			(!(*lpiResult & IS_TEXT_UNICODE_NOT_UNICODE_MASK) && (*lpiResult & IS_TEXT_UNICODE_NOT_ASCII_MASK))))
			enc = ((*lpiResult & IS_TEXT_UNICODE_REVERSE_MASK) ? ID_ENC_UTF16BE : ID_ENC_UTF16);
		if (enc == ID_ENC_UNKNOWN)
			enc = ID_ENC_ANSI;
		*format &= 0xfff0000;
		*format |= enc;
	}
	bResult = TRUE;
	return DecodeText(ppBuffer, buflen, format, &bResult);
}

BOOL LoadFile(LPTSTR szFilename, BOOL bCreate, BOOL bMRU, BOOL insert) {
	HANDLE hFile = NULL;
	LPTSTR szBuffer, lfn = NULL, dfn = NULL, rfn = NULL;
	LPCTSTR tbuf;
	TCHAR cPad = _T(' '), buffer[MAXFN + 40];
	DWORD lChars, nCR, nLF, nStrays, nSub, cfmt = nFormat;
	BOOL b;
	HCURSOR hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));

	bLoading = TRUE;
	lstrcpy(szStatusMessage, GetString(IDS_FILE_LOADING));
	UpdateStatus(TRUE);
	*szStatusMessage = 0;
	FixShortFilename(szFilename, &lfn);
	GetReadableFilename(lfn, &rfn);
	SSTRCPY(dfn, lfn);
	szBuffer = lstrrchr(dfn, _T('\\'));
	if (szBuffer) *szBuffer = _T('\0');
	
	while (1) {
		hFile = (HANDLE)CreateFile(lfn, GENERIC_READ, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile == INVALID_HANDLE_VALUE) {
			DWORD dwError = GetLastError();
			if (dwError == ERROR_FILE_NOT_FOUND && bCreate) {
				if (lstrchr(lfn, _T('.')) == NULL) {
					lstrcat(lfn, _T(".txt"));
					continue;
				}
				wsprintf(buffer, GetString(IDS_CREATE_FILE_MESSAGE), rfn);
				switch (MessageBox(hwnd, buffer, STR_METAPAD, MB_YESNOCANCEL | MB_ICONEXCLAMATION)) {
				case IDYES:
					hFile = (HANDLE)CreateFile(lfn, GENERIC_READ, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
					if (hFile == INVALID_HANDLE_VALUE) {
						ERROROUT(GetString(IDS_FILE_CREATE_ERROR));
						bLoading = FALSE;
						UpdateStatus(TRUE);
						SetCursor(hcur);
						FREE(lfn); FREE(dfn); FREE(rfn);
						return FALSE;
					}
					break;
				case IDNO:
					MakeNewFile();
					SetCursor(hcur);
					FREE(lfn); FREE(dfn); FREE(rfn);
					return TRUE;
				case IDCANCEL:
					bLoading = FALSE;
					UpdateStatus(TRUE);
					SetCursor(hcur);
					FREE(lfn); FREE(dfn); FREE(rfn);
					return FALSE;
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
				bLoading = FALSE;
				UpdateStatus(TRUE);
				SetCursor(hcur);
				FREE(lfn); FREE(dfn); FREE(rfn);
				return FALSE;
			}
		} else {
			SwitchReadOnly(GetFileAttributes(lfn) & FILE_ATTRIBUTE_READONLY);
			break;
		}
	}

	if ((LONG)(lChars = LoadFileIntoBuffer(hFile, (LPBYTE*)&szBuffer, &cfmt)) < 0) {
		CloseHandle(hFile);
		bLoading = FALSE;
		UpdateStatus(TRUE);
		SetCursor(hcur);
		FREE(lfn); FREE(dfn); FREE(rfn);
		return FALSE;
	}
	CloseHandle(hFile);

	if (lChars) {
		cfmt &= 0x8000ffff;
		cfmt |= (GetLineFmt(szBuffer, lChars, &nCR, &nLF, &nStrays, &nSub, &b) << 16);
		if (b) {
			cfmt = ID_ENC_BIN | ((cfmt >> 16) & 0xfff);
			tbuf = GetShadowBuffer(NULL);
			SetWindowText(client, szBuffer);
#ifdef UNICODE
			if (options.bNoWarningPrompt || MessageBox(hwnd, GetString(IDS_BINARY_FILE_WARNING_SAFE), STR_METAPAD, MB_ICONQUESTION|MB_OKCANCEL) != IDOK) {
#else
			if (options.bNoWarningPrompt || MessageBox(hwnd, GetString(IDS_BINARY_FILE_WARNING), STR_METAPAD, MB_ICONQUESTION|MB_OKCANCEL) != IDOK) {
#endif
				SetWindowText(client, tbuf);
				bLoading = FALSE;
				UpdateStatus(TRUE);
				SetCursor(hcur);
				FREE(lfn); FREE(dfn); FREE(rfn);
				return FALSE;
			}
			ImportBinary(szBuffer, lChars);
		} else if (((cfmt >> 16) & 0xfff) == ID_LFMT_MIXED && !options.bNoWarningPrompt)
			ERROROUT(GetString(IDS_LFMT_MIXED));
		b = TRUE;
		ImportLineFmt(&szBuffer, &lChars, (WORD)((cfmt >> 16) & 0xfff), nCR, nLF, nStrays, nSub, &b);
	}
	if (insert) {
		if (lChars)
			SendMessage(client, EM_REPLACESEL, (WPARAM)TRUE, (LPARAM)(LPTSTR)szBuffer);
		FREE(lfn); FREE(dfn);
	} else {
		if (lChars) {
			SetWindowText(client, szBuffer);
			SetFileFormat(cfmt, 0);
		} else {
			SetWindowText(client, _T(""));
			SetFileFormat(options.nFormat, 0);
		}
		UpdateSavedInfo();
		SetTabStops();
		SendMessage(client, EM_EMPTYUNDOBUFFER, 0, 0);
		FREE(szDir);
		FREE(szFile);
		FREE(szCaptionFile);
		szDir = dfn;
		szFile = lfn;
		if (lChars >=4 && szBuffer && szBuffer[0] == _T('.') &&
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
	}
	if (bMRU)
		SaveMRUInfo(rfn);
	FREE(rfn);
	FREE(szBuffer);
	bLoading = FALSE;
	UpdateStatus(TRUE);
	InvalidateRect(client, NULL, TRUE);
	SetCursor(hcur);
	return TRUE;
}



BOOL LoadFileFromMenu(WORD wMenu, BOOL bMRU) {
	HMENU hmenu = GetMenu(hwnd);
	HMENU hsub = NULL;
	MENUITEMINFO mio;
	TCHAR szBuffer[MAXFN+4] = _T("");
	LPTSTR pbuf = szBuffer+3;

	if (!SaveIfDirty())
		return FALSE;
	if (bMRU) {
		if (options.bRecentOnOwn) 	hsub = GetSubMenu(hmenu, 1);
		else 						hsub = GetSubMenu(GetSubMenu(hmenu, 0), MPOS_FILE_RECENT);
	} else if (!options.bNoFaves) {
		hsub = GetSubMenu(hmenu, MPOS_FAVE);
	} else
		return FALSE;

	*pbuf = _T('\0');
	mio.cbSize = sizeof(MENUITEMINFO);
	mio.fMask = MIIM_TYPE;
	mio.fType = MFT_STRING;
	mio.cch = MAXFN;
	mio.dwTypeData = szBuffer;
	GetMenuItemInfo(hsub, wMenu, FALSE, &mio);
	if (*pbuf) {
		if (!bMRU) {
			GetPrivateProfileString(STR_FAV_APPNAME, pbuf, _T(""), pbuf, MAXFN, SCNUL(szFav));
			if (!*szBuffer) {
				ERROROUT(GetString(IDS_ERROR_FAVOURITES));
				return FALSE;
			}
		}
		return LoadFile(pbuf, FALSE, TRUE, FALSE);
	}
	return FALSE;
}

BOOL BrowseFile(HWND owner, LPCTSTR defExt, LPCTSTR defDir, LPCTSTR filter, BOOL load, BOOL bMRU, BOOL insert, LPTSTR* fileName){
	OPENFILENAME ofn;
	TCHAR fn[MAXFN] = _T("");
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = owner;
	ofn.lpstrFilter = filter;
	ofn.lpstrCustomFilter = (LPTSTR)NULL;
	ofn.nMaxCustFilter = 0L;
	ofn.nFilterIndex = 1L;
	ofn.lpstrFile = fn;
	ofn.nMaxFile = MAXFN;
	ofn.lpstrFileTitle = (LPTSTR)NULL;
	ofn.nMaxFileTitle = 0L;
	ofn.lpstrInitialDir = defDir;
	ofn.lpstrTitle = (LPTSTR)NULL;
	ofn.Flags = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
	ofn.nFileOffset = 0;
	ofn.nFileExtension = 0;
	ofn.lpstrDefExt = defExt;
	if (!GetOpenFileName(&ofn)) return FALSE;
	if (fileName && *fileName) SSTRCPY(*fileName, fn);
	if (load) return LoadFile(fn, FALSE, bMRU, insert);
	return TRUE;
}