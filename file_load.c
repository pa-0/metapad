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
#include "include/macros.h"
#include "include/metapad.h"
#include "include/encoding.h"



DWORD LoadFileIntoBuffer(HANDLE hFile, LPBYTE* ppBuffer, DWORD* format) {
	DWORD dwBytes = 0, buflen, l;
	BOOL bResult;
	INT unitest = IS_TEXT_UNICODE_UNICODE_MASK | IS_TEXT_UNICODE_REVERSE_MASK | IS_TEXT_UNICODE_NOT_UNICODE_MASK | IS_TEXT_UNICODE_NOT_ASCII_MASK;
	LPINT lpiResult = &unitest;
	WORD enc, cp;

	if (!ppBuffer || !format) return -1;
	buflen = GetFileSize(hFile, NULL);
	if (!options.bNoWarningPrompt && buflen > LARGEFILESIZEWARN && MessageBox(hwnd, GetString(IDS_LARGE_FILE_WARNING), GetString(STR_METAPAD), MB_ICONQUESTION|MB_OKCANCEL) == IDCANCEL) {
		return -1;
	}
	*ppBuffer = (LPBYTE)HeapAlloc(globalHeap, 0, buflen+sizeof(TCHAR));
	if (*ppBuffer == NULL) {
		ReportLastError();
		return -1;
	}
	
	bResult = ReadFile(hFile, *ppBuffer, l = MIN(buflen, 4), &dwBytes, NULL);
	if (!bResult || dwBytes != l)
		ReportLastError();
	enc = CheckBOM(*ppBuffer, &dwBytes);
	SetFilePointer(hFile, dwBytes, NULL, FILE_BEGIN);
	buflen -= dwBytes;
	bResult = ReadFile(hFile, *ppBuffer, buflen, &dwBytes, NULL);
	if (!bResult || dwBytes != buflen)
		ReportLastError();
	buflen = dwBytes;
	(*ppBuffer)[dwBytes] = 0;
	
	cp = (*format >> 31 ? (WORD)*format : 0);

	if (!cp) {
		// check if UTF-8 even if no bom
		if (enc == FC_ENC_UNKNOWN && buflen > 1 && IsTextUTF8(*ppBuffer))
			enc = FC_ENC_UTF8;
		// check if unicode even if no bom
		if (enc == FC_ENC_UNKNOWN && buflen > 2 && (IsTextUnicode(*ppBuffer, buflen, lpiResult) || 
			(!(*lpiResult & IS_TEXT_UNICODE_NOT_UNICODE_MASK) && (*lpiResult & IS_TEXT_UNICODE_NOT_ASCII_MASK))))
			enc = ((*lpiResult & IS_TEXT_UNICODE_REVERSE_MASK) ? FC_ENC_UTF16BE : FC_ENC_UTF16);
		if (enc == FC_ENC_UNKNOWN)
			enc = FC_ENC_ANSI;
	}
	if (enc != FC_ENC_UNKNOWN) {
		*format &= 0xfff0000;
		*format |= enc;
	}
	bResult = TRUE;
	return DecodeText(ppBuffer, buflen, format, &bResult);
}

BOOL LoadFile(LPTSTR szFilename, BOOL bCreate, BOOL bMRU, BOOL insert, LPTSTR* textOut) {
	HANDLE hFile = NULL;
	LPTSTR szBuffer, lfn = NULL, dfn = NULL, rfn = NULL;
	LPCTSTR tbuf;
	TCHAR cPad = _T(' '), msgbuf[MAXFN + 40];
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
		if (hFile == INVALID_HANDLE_VALUE || GetFileSize(hFile, NULL) == (DWORD)-1) {
			DWORD dwError = GetLastError();
			if (dwError == ERROR_INVALID_HANDLE){
				gbLFN = FALSE;
				return LoadFile(szFilename, bCreate, bMRU, insert, textOut);
			} else if (dwError == ERROR_FILE_NOT_FOUND && bCreate) {
				if (lstrchr(lfn, _T('.')) == NULL) {
					lstrcat(lfn, _T(".txt"));
					continue;
				}
				wsprintf(msgbuf, GetString(IDS_CREATE_FILE_MESSAGE), rfn);
				switch (MessageBox(hwnd, msgbuf, GetString(STR_METAPAD), MB_YESNOCANCEL | MB_ICONEXCLAMATION)) {
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
				if (dwError == ERROR_FILE_NOT_FOUND || dwError == ERROR_BAD_NETPATH) {
					ERROROUT(GetString(IDS_FILE_NOT_FOUND));
				} else if (dwError == ERROR_SHARING_VIOLATION) {
					ERROROUT(GetString(IDS_FILE_LOCKED_ERROR));
				} else
					ReportError(dwError);
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
		cfmt |= (GetLineFmt(szBuffer, lChars, (options.nFormat>>16)&0xfff, &nCR, &nLF, &nStrays, &nSub, &b) << 16);
		if (b && !textOut) {
			cfmt = FC_ENC_BIN | (cfmt & 0xfff0000);
			if (!insert) {
				bLoading = FALSE;
				tbuf = GetShadowBuffer(NULL);
				bLoading = TRUE;
				RestoreClientView(0, FALSE, TRUE, TRUE);
				SetWindowText(client, szBuffer);
			}
#ifdef UNICODE
			if (!options.bNoWarningPrompt && MessageBox(hwnd, GetString(IDS_BINARY_FILE_WARNING_SAFE), GetString(STR_METAPAD), MB_ICONQUESTION|MB_OKCANCEL) != IDOK) {
#else
			if (!options.bNoWarningPrompt && MessageBox(hwnd, GetString(IDS_BINARY_FILE_WARNING), GetString(STR_METAPAD), MB_ICONQUESTION|MB_OKCANCEL) != IDOK) {
#endif
				if (!insert) {
					SetWindowText(client, tbuf);
					RestoreClientView(0, TRUE, TRUE, TRUE);
				}
				bLoading = FALSE;
				UpdateStatus(TRUE);
				SetCursor(hcur);
				FREE(lfn); FREE(dfn); FREE(rfn);
				return FALSE;
			}
			ImportBinary(szBuffer, lChars);
		} else if (((cfmt >> 16) & 0xfff) == FC_LFMT_MIXED) {
			if (insert && ((nFormat >> 16) & 0xfff) != FC_LFMT_MIXED) {
				cfmt &= 0x8000ffff;
				cfmt |= nFormat & 0xfff0000;
				ERROROUT(GetString(IDS_LFMT_FIXED));
			} else if (!options.bNoWarningPrompt && ((nFormat >> 16) & 0xfff) != FC_LFMT_MIXED) {
				ERROROUT(GetString(IDS_LFMT_MIXED));
			}
		}
		b = TRUE;
		ImportLineFmt(&szBuffer, &lChars, (WORD)((cfmt >> 16) & 0xfff), nCR, nLF, nStrays, nSub, &b);
	}
	if (!textOut) {
		if (insert) {
			if (lChars)
				SendMessage(client, EM_REPLACESEL, (WPARAM)TRUE, (LPARAM)(LPTSTR)szBuffer);
		} else {
			SetTabStops();
			SetWindowText(client, lChars ? szBuffer : _T(""));
			if (bLoading) {
				SetFileFormat(cfmt, 0);
				SendMessage(client, EM_EMPTYUNDOBUFFER, 0, 0);
				FREE(szDir);
				FREE(szFile);
				FREE(szCaptionFile);
				szDir = dfn;
				szFile = lfn;
				lfn = NULL; dfn = NULL;
				if (lChars >=4 && szBuffer && szBuffer[0] == _T('.') &&
					szBuffer[1] == _T('L') &&
					szBuffer[2] == _T('O') &&
					szBuffer[3] == _T('G')) {
					CHARRANGE cr;
					cr.cpMin = cr.cpMax = lChars;
					SetSelection(cr);
					SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(ID_DATE_TIME_CUSTOM, 0), 0);
					SendMessage(client, EM_SCROLLCARET, 0, 0);
				}
				bLoading = FALSE;
				UpdateSavedInfo();
			} else
				MakeNewFile();
		}
		if (bMRU)
			SaveMRUInfo(rfn);
		FREE(szBuffer);
	} else
		*textOut = szBuffer;
	FREE(lfn); FREE(dfn); FREE(rfn);
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
			GetPrivateProfileString(GetString(STR_FAV_APPNAME), pbuf, _T(""), pbuf, MAXFN, SCNUL(szFav));
			if (!*szBuffer) {
				ERROROUT(GetString(IDS_ERROR_FAVOURITES));
				return FALSE;
			}
		}
		return LoadFile(pbuf, FALSE, TRUE, FALSE, NULL);
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
	if (fileName) SSTRCPY(*fileName, fn);
	if (load) return LoadFile(fn, FALSE, bMRU, insert, NULL);
	return TRUE;
}